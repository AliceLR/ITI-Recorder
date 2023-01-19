/* ITI Recorder
 *
 * Copyright (C) 2023 Alice Rowan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "AudioBuffer.hpp"
#include "Config.hpp"
#include "Soundcard.hpp"

#include <alsa/asoundlib.h>

static void async_callback(snd_async_handler_t *a);

static class Soundcard_ALSA final: public Soundcard
{
  static constexpr unsigned DEFAULT_LATENCY_US = 100000; /* 100ms */

  snd_pcm_t *audio_in = nullptr;
  snd_async_handler_t *async_in = nullptr;
  AudioInput *in_target = nullptr;
  bool in_fail = false;

  snd_rawmidi_t *midi_out[GlobalConfig::max_inputs]{};
  unsigned midi_max = 0;

public:
  Soundcard_ALSA(const char *name): Soundcard(name) {}

  virtual ~Soundcard_ALSA()
  {
    deinit();
  }

  virtual void deinit()
  {
    int err;
    if(audio_in)
    {
      if(async_in)
      {
        err = snd_async_del_handler(async_in);
        if(err)
        {
          fprintf(stderr, "ALSA PCM: error deleting async handler: %s\n",
           snd_strerror(err));
        }
        async_in = nullptr;
      }

      err = snd_pcm_close(audio_in);
      if(err)
      {
        fprintf(stderr, "ALSA PCM: error closing stream: %s\n",
         snd_strerror(err));
      }

      audio_in = nullptr;
      in_target = nullptr;
    }

    for(snd_rawmidi_t *out : midi_out)
    {
      if(out)
      {
        err = snd_rawmidi_close(out);
        if(err)
        {
          fprintf(stderr, "ALSA RawMidi: error closing stream: %s\n",
           snd_strerror(err));
        }
      }
    }
    memset(midi_out, 0, sizeof(midi_out));
    midi_max = 0;
  }

  virtual bool init_audio_in(const char *interface)
  {
    int err = snd_pcm_open(&audio_in, interface,
     SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC);
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error opening stream '%s': %s\n",
       interface, snd_strerror(err));
      return false;
    }
    return true;
  }

  virtual bool audio_capture_start(AudioInput &dest)
  {
    int state = audio_in ? snd_pcm_state(audio_in) : -1;
    if(!audio_in || (state != SND_PCM_STATE_OPEN && state != SND_PCM_STATE_SETUP))
    {
      if(audio_in)
        fprintf(stderr, "ALSA PCM: invalid start state %d\n", state);
      else
        fprintf(stderr, "ALSA PCM: null audio_in\n");
      return false;
    }

    int err = snd_pcm_set_params(audio_in,
      SND_PCM_FORMAT_S16,
      SND_PCM_ACCESS_MMAP_INTERLEAVED,
      dest.channels,
      dest.rate,
      0,
      DEFAULT_LATENCY_US
    );
    if(err)
    {
      fprintf(stderr, "ALSA PCM: WARNING: allowing resampling\n");
      err = snd_pcm_set_params(audio_in,
        SND_PCM_FORMAT_S16,
        SND_PCM_ACCESS_MMAP_INTERLEAVED,
        dest.channels,
        dest.rate,
        1,
        DEFAULT_LATENCY_US
      );
    }
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error initializing stream: %s\n",
       snd_strerror(err));
      return false;
    }

    err = snd_pcm_prepare(audio_in);
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error preparing stream: %s\n",
       snd_strerror(err));
      snd_pcm_drop(audio_in);
      return false;
    }

    err = snd_async_add_pcm_handler(&async_in, audio_in, async_callback, this);
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error initializing async handler: %s\n",
       snd_strerror(err));
      snd_pcm_drop(audio_in);
      return false;
    }

    err = snd_pcm_start(audio_in);
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error starting PCM stream: %s\n",
       snd_strerror(err));
      snd_pcm_drop(audio_in);
      return false;
    }

    in_target = &dest;
    in_fail = false;
    in_channels = dest.channels;
    in_rate = dest.rate;
    return true;
  }

  virtual bool audio_capture_stop()
  {
    if(!audio_in)
      return false;

    //int err = snd_pcm_drain(audio_in);
    int err = snd_pcm_drop(audio_in);
    if(err)
    {
      fprintf(stderr, "ALSA PCM: error stopping PCM stream: %s\n",
       snd_strerror(err));
    }

    if(async_in)
    {
      err = snd_async_del_handler(async_in);
      if(err)
      {
        fprintf(stderr, "ALSA PCM: error deleting async handler: %s\n",
         snd_strerror(err));
      }
      async_in = nullptr;
    }
    return true;
  }

  bool audio_capture_error(snd_pcm_sframes_t _err)
  {
    fprintf(stderr, "ALSA PCM: stream error: %s\n",
     (_err == -EPIPE) ? "xrun" : snd_strerror(_err));

    int err = snd_pcm_prepare(audio_in);
    if(err < 0)
    {
      fprintf(stderr, "ALSA PCM: failed to recover: %s\n",
       snd_strerror(err));
      in_fail = true;
      return false;
    }
    err = snd_pcm_start(audio_in);
    if(err < 0)
    {
      fprintf(stderr, "ALSA_PCM: failed to restart: %s\n",
       snd_strerror(err));
      in_fail = true;
      return false;
    }

    fprintf(stderr, "ALCA PCM: recovered\n");
    return true;
  }

  void audio_capture_callback()
  {
    const snd_pcm_channel_area_t *areas;
    snd_pcm_uframes_t offset;
    snd_pcm_uframes_t frames;
    int err;

    int state = snd_pcm_state(audio_in);
    if(state != SND_PCM_STATE_RUNNING)
    {
      if(state != SND_PCM_STATE_XRUN)
        return;
      if(!audio_capture_error(-EPIPE))
        return;
    }

    snd_pcm_sframes_t avail_in = snd_pcm_avail_update(audio_in);
    if(avail_in < 0)
      if(!audio_capture_error(avail_in))
        return;

    while(avail_in > 0)
    {
      frames = avail_in;
      err = snd_pcm_mmap_begin(audio_in, &areas, &offset, &frames);
      if(err)
      {
        fprintf(stderr, "ALSA PCM: mmap open failed: %s\n",
         snd_strerror(err));
        return;
      }
      avail_in -= frames;

      const uint8_t *src = reinterpret_cast<const uint8_t *>(areas[0].addr);
      size_t step = areas[0].step >> 3;
      size_t start = (areas[0].first >> 3) + step * offset;

      in_target->write(src + start, frames);

      snd_pcm_mmap_commit(audio_in, offset, frames);
    }
  }


  virtual bool init_midi_out(const char *interface, unsigned num)
  {
    if(num >= GlobalConfig::max_inputs)
    {
      fprintf(stderr, "ALSA RawMidi: invalid input number %u!\n", num);
      return false;
    }

    if(midi_out[num])
      return true;

    /* FIXME: mode? */
    int err = snd_rawmidi_open(nullptr, &midi_out[num], interface, 0);
    if(err)
    {
      fprintf(stderr, "ALSA RawMidi: error opening stream: %s\n", snd_strerror(err));
      return false;
    }

    err = snd_rawmidi_nonblock(midi_out[num], 1);
    if(err)
    {
      fprintf(stderr, "ALSA RawMidi: failed to set nonblock: %s\n", snd_strerror(err));
      return false;
    }

    midi_max = num + 1;
    return true;
  }

  virtual void midi_write(const std::vector<uint8_t> &data, int num)
  {
    if(num >= 0 && (unsigned)num < GlobalConfig::max_inputs)
    {
      if(midi_out[num])
        snd_rawmidi_write(midi_out[num], data.data(), data.size());
    }
    else

    for(unsigned i = 0; i < midi_max; i++)
    {
      if(midi_out[i])
        snd_rawmidi_write(midi_out[i], data.data(), data.size());
    }
  }
} soundcard_alsa("ALSA");


static void async_callback(snd_async_handler_t *a)
{
  Soundcard_ALSA *card = reinterpret_cast<Soundcard_ALSA *>(snd_async_handler_get_callback_private(a));

  if(card)
    card->audio_capture_callback();
}

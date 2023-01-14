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

#include "Config.hpp"
#include "Soundcard.hpp"

#include <alsa/asoundlib.h>
#include <time.h>

static class Soundcard_ALSA final: public Soundcard
{
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
    // FIXME:

    for(snd_rawmidi_t *out : midi_out)
    {
      if(out)
      {
        err = snd_rawmidi_close(out);
        if(err)
          fprintf(stderr, "ALSA RawMidi: error closing stream: %s\n", snd_strerror(err));
      }
    }
    midi_max = 0;
  }

  virtual bool init_audio_in(const char *interface)
  {
    // FIXME:
    return false;
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

  virtual void delay(unsigned ms)
  {
    struct timespec req;
    struct timespec rem;
    int ret = EINTR;

    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000;

    while(ret == EINTR)
    {
      ret = nanosleep(&req, &rem);
      if(ret == EINTR)
        req = rem;
    }
  }
} soundcard_alsa("ALSA");

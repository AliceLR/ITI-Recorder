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
#include "AudioOutput.hpp"
#include "Event.hpp"
#include "Config.hpp"
#include "Midi.hpp"
#include "Platform.hpp"
#include "Soundcard.hpp"

#include <inttypes.h>
#include <typeinfo>

#define OUTPUT_DIR "output"

static size_t schedule_events(EventSchedule &ev,
 const std::shared_ptr<GlobalConfig> &cfg,
 const std::shared_ptr<PlaybackConfig> &play,
 const std::vector<const MIDIInterface *> &midi_interfaces,
 AudioBuffer<int16_t> &buffer)
{
  bool add_cues = cfg->output_on;
  unsigned cues = 0;
  size_t time_ms = 0;

  if(cfg->program_on)
  {
    for(const MIDIInterface *mi : midi_interfaces)
      mi->program(ev);
  }
  else
    fprintf(stderr, "not programming interface(s)\n");

  if(cfg->output_noise_removal)
  {
    AudioCueEvent::schedule(ev, buffer, AudioCue::NoiseStart, time_ms);
    time_ms += cfg->output_noise_ms;
    AudioCueEvent::schedule(ev, buffer, AudioCue::NoiseEnd, time_ms - 1);
  }

  if(play->PlaybackOn)
  {
    std::vector<uint8_t> out;
    for(unsigned i = play->MinNote; i <= play->MaxNote; i++)
    {
      /* On cue */
      if(add_cues)
      {
        AudioCueEvent::schedule(ev, buffer, AudioCue::On, time_ms);
        cues++;
      }

      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->note_on(out, i, play->OnVelocity);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += play->On_ms;

      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->note_off(out, i, play->OffVelocity);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += play->Off_ms;

      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->all_off(out);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += play->Quiet_ms;

      /* Off cue */
      if(add_cues)
      {
        AudioCueEvent::schedule(ev, buffer, AudioCue::Off, time_ms - 10);
        cues++;
      }
    }
    buffer.reserve_cues(cues);
  }
  else
    fprintf(stderr, "not performing playback\n");

  return time_ms;
}

static bool try_init(Soundcard &card,
 const std::shared_ptr<GlobalConfig> &cfg,
 const std::shared_ptr<PlaybackConfig> &play,
 const std::vector<const MIDIInterface *> &midi_interfaces)
{
  if(cfg->output_on)
  {
    if(!card.init_audio_in(cfg->audio_device))
    {
      fprintf(stderr, "couldn't initialize '%s': audio in\n", card.name);
      card.deinit();
      return false;
    }
  }

  if(cfg->program_on || play->PlaybackOn)
  {
    for(const MIDIInterface *mi : midi_interfaces)
    {
      const InputConfig *ic = mi->get_input_config();
      if(!ic || !card.init_midi_out(ic->midi_device, mi->device))
      {
        fprintf(stderr, "couldn't initialize '%s': MIDI out\n", card.name);
        card.deinit();
        return false;
      }
    }
  }
  card.select();
  return true;
}

static Soundcard &initialize_soundcard(
 const std::shared_ptr<GlobalConfig> &cfg,
 const std::shared_ptr<PlaybackConfig> &play,
 const std::vector<const MIDIInterface *> &midi_interfaces)
{
  bool has_card = false;
  if(strcasecmp("default", cfg->audio_driver))
  {
    if(try_init(Soundcard::get(cfg->audio_driver), cfg, play, midi_interfaces))
      has_card = true;
  }
  else

  for(Soundcard &card : Soundcard::list())
  {
    if(try_init(card, cfg, play, midi_interfaces))
    {
      has_card = true;
      break;
    }
  }

  if(!has_card)
  {
    fprintf(stderr, "failed to initialize any device.\n");
    exit(1);
  }

  Soundcard &card = Soundcard::get();
  fprintf(stderr, "using audio interface '%s'\n", card.name);
  return card;
}


int main(int argc, char **argv)
{
  ConfigContext ctx{};

  if(!ctx.init(argc, argv))
    return 1;

  const auto cfg = ctx.get_interface_as<GlobalConfig>("global");
  const auto play = ctx.get_interface_as<PlaybackConfig>("Playback");

  if(cfg == nullptr || play == nullptr)
    return 1;

  std::vector<const MIDIInterface *> midi_interfaces;

  /* Get all MIDI synths. */
  for(auto &p : ctx.get_interfaces())
  {
    /* pointer - failure returns nullptr */
    MIDIInterface *mi = dynamic_cast<MIDIInterface *>(p.get());
    if(mi)
    {
      /* Load external SysEx if applicable. */
      mi->load();

      midi_interfaces.push_back(mi);
    }
  }
  if(midi_interfaces.size() < 1)
  {
    fprintf(stderr, "nothing to do\n");
    return 0;
  }

  /* Schedule MIDI events and user program prompts. */
  EventSchedule ev;
  AudioBuffer<int16_t> buffer(2, cfg->audio_rate);

  size_t time_ms = schedule_events(ev, cfg, play, midi_interfaces, buffer);
  uint64_t buffer_frames =
   cast_multiply<uint64_t>(cfg->audio_rate, ev.total_duration() + 30000) / 1000;

  if(buffer_frames * buffer.frame_size() > SIZE_MAX)
  {
    fprintf(stderr, "can't fit sample buffer of %" PRIu64 "u frames in RAM!\n",
     buffer_frames);
    return 0;
  }

  /* Confirm MIDI devices and manual synthesizer configuration. */
  fprintf(stderr, "Start note:   %s\n", MIDIInterface::get_note(play->MinNote));
  fprintf(stderr, "End note:     %s\n", MIDIInterface::get_note(play->MaxNote));
  fprintf(stderr, "Duration:     %.2fs\n", time_ms / 1000.0);
  fprintf(stderr, "Buffer frames:%zu\n", buffer_frames);
  fprintf(stderr, "\n");

  for(unsigned i = 0; i < midi_interfaces.size(); i++)
  {
    const MIDIInterface *mi = midi_interfaces[i];
    const InputConfig *ic = mi->get_input_config();
    const char *device = ic ? ic->midi_device : "?";
    unsigned channel = ic ? ic->midi_channel : 0;

    fprintf(stderr, "Interface %2u: '%s' on port '%s' channel %u\n",
     i, mi->tag, device, channel);
  }
  fprintf(stderr, "\n");

  // FIXME: these should print literally anything about their interface
  while(ev.has_next())
  {
    if(ev.next_time() != EventSchedule::NOTICE_TIME)
      break;

    std::shared_ptr<Event> notice = ev.pop();
    notice->task();
  }
  fprintf(stderr, "Press 'enter' to continue.\n");
  Platform::wait_input();

  /* Preallocate recording buffer. */
  if(cfg->output_on)
    buffer.resize(buffer_frames);

  /* Initialize sound device. */
  Soundcard &card = initialize_soundcard(cfg, play, midi_interfaces);

  // FIXME: signal handler to send all off event on abort.

  if(cfg->output_on)
  {
    if(!card.audio_capture_start(buffer))
    {
      fprintf(stderr, "Failed to start audio capture\n");
      return 0;
    }
  }

  /* Run remaining scheduled events. */
  while(ev.has_next())
  {
    int ms = ev.next_time() - ev.previous_time();
    if(ms > 0)
      Platform::delay(ms);

    std::shared_ptr<Event> event = ev.pop();
    event->task();
  }

  if(cfg->output_on)
  {
    card.audio_capture_stop();
    fprintf(stderr, "total frames read: %zu\n", buffer.total_frames());

    for(const AudioCue &c : buffer.get_cues())
      fprintf(stderr, "%10" PRIu64 " : cue %s\n", c.frame,
       AudioCue::type_str(c.type));

    if(!Platform::mkdir_recursive("output"))
    {
      fprintf(stderr, "failed to create output directory\n");
      return 0;
    }

    /* Output audio (debug, no processing) */
    if(cfg->output_debug)
      AudioOutput<Audio::RAW>::write(buffer, OUTPUT_DIR "/pre.raw");

    // FIXME: amplify and noise removal

    /* Remove silence from individual samples. */
    buffer.shrink_cues(cfg->output_noise_threshold);
    fprintf(stderr, "\ncues after processing:\n");
    for(const AudioCue &c : buffer.get_cues())
      fprintf(stderr, "%10" PRIu64 " : cue %s\n", c.frame,
       AudioCue::type_str(c.type));

    /* Output audio */
    if(cfg->output_debug)
      AudioOutput<Audio::RAW>::write(buffer, OUTPUT_DIR "/post.raw");

    if(cfg->output_wav)
      AudioOutput<Audio::WAV>::write_all(buffer, play->MinNote,
       OUTPUT_DIR "/%.wav");

    // FIXME: output audio
  }

  return 0;
}

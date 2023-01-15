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

#include "Event.hpp"
#include "Config.hpp"
#include "Midi.hpp"
#include "Platform.hpp"
#include "Soundcard.hpp"

#include <typeinfo>

/* FIXME: schedule audio recording, noise profile events. */
static size_t schedule_events(EventSchedule &ev, const GlobalConfig *cfg,
 const std::vector<const MIDIInterface *> &midi_interfaces)
{
  size_t time_ms = 0;

  if(cfg->program_on)
  {
    for(const MIDIInterface *mi : midi_interfaces)
      mi->program(ev);
  }
  else
    fprintf(stderr, "not programming interface(s)\n");

  //time_ms += cfg->output_noise_ms; // Noise profile.

  if(cfg->playback_on)
  {
    std::vector<uint8_t> out;
    for(unsigned i = cfg->min_note; i <= cfg->max_note; i++)
    {
      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->note_on(out, i, cfg->on_velocity);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += cfg->on_ms;

      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->note_off(out, i, cfg->off_velocity);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += cfg->off_ms;

      for(const MIDIInterface *mi : midi_interfaces)
      {
        out.resize(0);
        mi->all_off(out);
        MIDIEvent::schedule(ev, *mi, out, time_ms);
      }
      time_ms += cfg->quiet_ms;
    }
  }
  else
    fprintf(stderr, "not performing playback\n");

  return time_ms;
}

/* FIXME: initialize audio input */
static Soundcard &initialize_soundcard(const GlobalConfig *cfg,
 const std::vector<const MIDIInterface *> &midi_interfaces)
{
  bool has_card = false;
  for(Soundcard &sc : Soundcard::list())
  {
    bool ok = true;
    for(const MIDIInterface *mi : midi_interfaces)
    {
      const InputConfig *ic = mi->get_input_config();
      if(!ic || !sc.init_midi_out(ic->midi_device, mi->device))
      {
        ok = false;
        break;
      }
    }

    if(ok)
    {
      sc.select();
      has_card = true;
      break;
    }
    else
      sc.deinit();
  }
  if(!has_card)
  {
    fprintf(stderr, "failed to initialize MIDI.\n");
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

  std::shared_ptr<ConfigInterface> _cfg = ctx.get_interface("global");
  if(_cfg == nullptr)
    return 1;

  std::vector<const MIDIInterface *> midi_interfaces;

  const GlobalConfig *cfg = reinterpret_cast<GlobalConfig *>(_cfg.get());

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
  size_t time_ms = schedule_events(ev, cfg, midi_interfaces);

  /* Confirm MIDI devices and manual synthesizer configuration. */
  fprintf(stderr, "Start note:   %s\n", MIDIInterface::get_note(cfg->min_note));
  fprintf(stderr, "End note:     %s\n", MIDIInterface::get_note(cfg->max_note));
  fprintf(stderr, "Duration:     %.2fs\n", time_ms / 1000.0);
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

  if(cfg->output_on)
  {
    // FIXME: allocate audio input buffer
  }

  /* Initialize sound device. */
  Soundcard &card = initialize_soundcard(cfg, midi_interfaces);

  // FIXME: signal handler to send all off event on abort.

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
    // FIXME: clean, slice, and output audio
    // FIXME: might be useful with playback off just to capture a noise sample
  }

  return 0;
}

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

#ifndef MIDI_HPP
#define MIDI_HPP

#include <limits.h>
#include <stdint.h>
#include <vector>

#include "Config.hpp"
#include "Event.hpp"

class MIDIInterface;

class MIDIEvent : public Event
{
  const MIDIInterface &interface;
  std::vector<uint8_t> data;

public:
  MIDIEvent(const MIDIInterface &_i,
   const std::vector<uint8_t> &_data, int _time_ms):
   Event(_time_ms), interface(_i), data(_data) {}

  MIDIEvent(const MIDIInterface &_i,
   std::vector<uint8_t> &&_data, int _time_ms):
   Event(_time_ms), interface(_i), data(std::move(_data)) {}

  virtual void task() const;

  static void schedule(EventSchedule &ev, const MIDIInterface &_i,
   const std::vector<uint8_t> &_data, int _time_ms)
  {
    ev.push(std::shared_ptr<Event>(new MIDIEvent(_i, _data, _time_ms)));
  }

  static void schedule(EventSchedule &ev, const MIDIInterface &_i,
   std::vector<uint8_t> &&_data, int _time_ms)
  {
    ev.push(std::shared_ptr<Event>(new MIDIEvent(_i, std::move(_data), _time_ms)));
  }
};

class MIDIInterface : public ConfigInterface
{
  mutable InputConfig *config = nullptr;

public:
  Option<unsigned>  device;

  MIDIInterface(ConfigContext &_ctx, const char *_tag, int _id):
   ConfigInterface(_ctx, _tag, _id),
   /* Options */
   device(options, 1, 1, GlobalConfig::max_inputs, "MIDI")
  {}

  virtual ~MIDIInterface() {}

  virtual void program(EventSchedule &ev) const = 0;
  virtual bool load()
  {
    return false;
  }

  void cc(std::vector<uint8_t> &out, unsigned param, unsigned value) const;
  void program_change(std::vector<uint8_t> &out, unsigned program) const;
  void note_on(std::vector<uint8_t> &out, unsigned note, unsigned velocity) const;
  void note_off(std::vector<uint8_t> &out, unsigned note, unsigned velocity = 0x40) const;
  void all_off(std::vector<uint8_t> &out) const;

  const InputConfig *get_input_config() const
  {
    if(!config)
    {
      auto cfg = ctx.get_interface("MIDI", device);
      config = dynamic_cast<InputConfig *>(cfg.get()); /* pointer - failure returns nullptr */
    }
    return config;
  }

  static const char *get_note(unsigned note);
  static uint8_t roland_checksum(const uint8_t *d, size_t sz);
  static bool load_file(std::vector<uint8_t> &out, const char *path);
};

#endif /* MIDI_HPP */

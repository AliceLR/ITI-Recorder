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

#ifndef SOUNDCARD_HPP
#define SOUNDCARD_HPP

#include <stdint.h>
#include <string.h>
#include <functional>
#include <vector>

class AudioInput; /* AudioBuffer.hpp */

class Soundcard
{
  static std::vector<std::reference_wrapper<Soundcard>> soundcards;
  static std::reference_wrapper<Soundcard> active;

protected:
  unsigned in_channels = 0;
  unsigned in_rate = 0;

public:
  const char * const name;

  Soundcard(const char *_name, bool searchable = true): name(_name)
  {
    if(searchable)
      soundcards.push_back(*this);
  }

  virtual void deinit() = 0;

  virtual bool init_audio_in(const char *interface) = 0;
  virtual bool audio_capture_start(AudioInput &dest) = 0;
  virtual bool audio_capture_stop() = 0;

  virtual bool init_midi_out(const char *interface, unsigned num) = 0;
  virtual void midi_write(const std::vector<uint8_t> &data, int num) = 0;

  void select()
  {
    active = *this;
  }

  unsigned channels() const
  {
    return in_channels;
  }

  unsigned rate() const
  {
    return in_rate;
  }

  static const std::vector<std::reference_wrapper<Soundcard>> &list()
  {
    return soundcards;
  }

  static Soundcard &get(const char *name)
  {
    for(Soundcard &sc : soundcards)
      if(!strcmp(sc.name, name))
        return sc;

    return active;
  }

  static Soundcard &get()
  {
    return active;
  }
};

#endif /* SOUNDCARD_HPP */

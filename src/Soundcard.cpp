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

#include "Soundcard.hpp"

std::vector<std::reference_wrapper<Soundcard>> Soundcard::soundcards;

static class DummySoundcard : public Soundcard
{
public:
  DummySoundcard(const char *name): Soundcard(name) {}

  virtual void deinit()
  {
    return;
  }

  virtual bool init_audio_in(const char *interface)
  {
    return false;
  }

  virtual bool init_midi_out(const char *interface, unsigned num)
  {
    return false;
  }

  virtual void midi_write(const std::vector<uint8_t> &data, int num)
  {
    return;
  }

  virtual void delay(unsigned ms)
  {
    return;
  }
} dummy("dummy");

std::reference_wrapper<Soundcard> Soundcard::active(dummy);

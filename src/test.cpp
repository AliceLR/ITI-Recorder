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
#include "Midi.hpp"
#include "Soundcard.hpp"

int main(int argc, char **argv)
{
  ConfigContext ctx{};

  if(!ctx.init(argc, argv))
    return 1;

  for(auto &interface : ctx.get_interfaces())
  {
    /* Load external SysEx if applicable. */
    MIDIInterface *midi = dynamic_cast<MIDIInterface *>(interface.get());
    if(midi)
      midi->load();

    interface->print();
  }

  return 0;
}

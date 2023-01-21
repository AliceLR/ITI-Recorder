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

#include "AudioOutput.hpp"

template<> template<>
bool AudioOutput<Audio::RAW>::write<int16_t>(const AudioBuffer<int16_t> &buffer,
 const char *filename)
{
  size_t end = buffer.total_frames() * buffer.frame_size();
  return write_file(buffer.get_samples().data(), end, filename);
}

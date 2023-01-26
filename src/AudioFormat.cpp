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

#include "AudioFormat.hpp"

#include <stdio.h>

bool AudioFormat::write_file(const std::vector<uint8_t> &out,
 const char *filename)
{
  FILE *fp = fopen(filename, "wb");
  if(fp)
  {
    if(fwrite(out.data(), 1, out.size(), fp) < out.size())
      fprintf(stderr, "error writing file '%s'\n", filename);

    fclose(fp);
    return true;
  }
  return false;
}

bool AudioFormat::write_file(const void *out, size_t out_len,
 const char *filename)
{
  FILE *fp = fopen(filename, "wb");
  if(fp)
  {
    if(fwrite(out, 1, out_len, fp) < out_len)
      fprintf(stderr, "error writing file '%s'\n", filename);

    fclose(fp);
    return true;
  }
  return false;
}

bool AudioFormat::convert(ConfigContext &ctx,
 std::vector<uint8_t> &out, const AudioBuffer<uint8_t> &buffer,
 const AudioCue &start, const AudioCue &end) const
{
  return false;
}

bool AudioFormat::convert(ConfigContext &ctx,
 std::vector<uint8_t> &out, const AudioBuffer<int16_t> &buffer,
 const AudioCue &start, const AudioCue &end) const
{
  return false;
}

bool AudioFormat::convert(ConfigContext &ctx,
 std::vector<uint8_t> &out, const AudioBuffer<int32_t> &buffer,
 const AudioCue &start, const AudioCue &end) const
{
  return false;
}

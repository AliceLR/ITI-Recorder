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

#ifndef AUDIOOUTPUT_HPP
#define AUDIOOUTPUT_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "AudioBuffer.hpp"
#include "Midi.hpp"

class AudioFormat
{
  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<uint8_t> &buffer,
   const AudioCue &start, const AudioCue &end) const;
  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<int16_t> &buffer,
   const AudioCue &start, const AudioCue &end) const;
  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end) const;

protected:
  static bool write_file(const std::vector<uint8_t> &out, const char *filename);
  static bool write_file(const void *out, size_t out_len, const char *filename);

public:

  virtual bool save(ConfigContext &ctx,
   const AudioBuffer<int16_t> &buffer, const AudioCue &start, const AudioCue &end,
   const char *filename) const
  {
    std::vector<uint8_t> out;
    if(!convert(ctx, out, buffer, start, end))
      return false;

    return write_file(out, filename);
  }

  virtual bool save(ConfigContext &ctx, const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end, const char *filename) const
  {
    std::vector<uint8_t> out;
    if(!convert(ctx, out, buffer, start, end))
      return false;

    return write_file(out, filename);
  }

  template<class T>
  bool save(ConfigContext &ctx, const AudioBuffer<T> &buffer,
   const char *filename) const
  {
    AudioCue start{ 0, AudioCue::NoteOn, -1 };
    AudioCue end{ buffer.total_frames(), AudioCue::NoteOff, -1 };
    return save(ctx, buffer, start, end, filename);
  }

  template<class T>
  bool save_all(ConfigContext &ctx, const AudioBuffer<T> &buffer,
   const char *filename) const
  {
    char name[512];

    const std::vector<AudioCue> &cues = buffer.get_cues();
    const char *pos = strrchr(filename, '%');
    int offset = pos ? pos - filename : strlen(filename);
    int offset2 = pos ? offset + 1 : offset;

    for(size_t i = 1; i < cues.size(); i++)
    {
      const AudioCue &on = cues[i - 1];
      const AudioCue &off = cues[i];
      if(on.type == AudioCue::NoteOn && off.type == AudioCue::NoteOff &&
       on.value == off.value)
      {
        const char *note = MIDIInterface::get_note(on.value);
        snprintf(name, sizeof(name), "%*.*s%s%s",
         offset, offset, filename, note, filename + offset2);

        if(!save(ctx, buffer, on, off, name))
          return false;

        i++;
      }
    }
    return true;
  }
};


/* Format specializations--see individual format compilation units. */
extern const AudioFormat &AudioFormatITI;
extern const AudioFormat &AudioFormatRaw;
extern const AudioFormat &AudioFormatWAVE;

#endif /* AUDIOOUTPUT_HPP */

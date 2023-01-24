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
  virtual bool convert(std::vector<uint8_t> &out, const AudioBuffer<int16_t> &buffer,
   const AudioCue &start, const AudioCue &end) const;
  virtual bool convert(std::vector<uint8_t> &out, const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end) const;

protected:
  static bool write_file(const std::vector<uint8_t> &out, const char *filename);
  static bool write_file(const void *out, size_t out_len, const char *filename);

public:

  virtual bool save(const AudioBuffer<int16_t> &buffer,
   const AudioCue &start, const AudioCue &end, const char *filename) const
  {
    std::vector<uint8_t> out;
    if(!convert(out, buffer, start, end))
      return false;

    return write_file(out, filename);
  }

  virtual bool save(const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end, const char *filename) const
  {
    std::vector<uint8_t> out;
    if(!convert(out, buffer, start, end))
      return false;

    return write_file(out, filename);
  }

  template<class T>
  bool save(const AudioBuffer<T> &buffer, const char *filename) const
  {
    AudioCue start{ 0, AudioCue::NoteOn };
    AudioCue end{ buffer.total_frames(), AudioCue::NoteOff };
    return save(buffer, start, end, filename);
  }

  template<class T>
  bool save_all(const AudioBuffer<T> &buffer, const OptionNote &first,
   const char *filename) const
  {
    char name[512];

    const std::vector<AudioCue> &cues = buffer.get_cues();
    const char *pos = strrchr(filename, '%');
    int offset = pos ? pos - filename : strlen(filename);
    int offset2 = pos ? offset + 1 : offset;
    unsigned next = first;

    for(size_t i = 1; i < cues.size(); i++)
    {
      if(cues[i - 1].type == AudioCue::NoteOn && cues[i].type == AudioCue::NoteOff)
      {
        const char *note = MIDIInterface::get_note(next++);
        snprintf(name, sizeof(name), "%*.*s%s%s",
         offset, offset, filename, note, filename + offset2);

        if(!save(buffer, cues[i - 1], cues[i], name))
          return false;

        i++;
      }
    }
    return true;
  }
};


/* Format specializations--see individual format compilation units. */
extern const AudioFormat &AudioFormatRaw;
extern const AudioFormat &AudioFormatWAVE;

#endif /* AUDIOOUTPUT_HPP */

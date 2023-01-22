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

enum class Audio
{
  RAW,
  SAM,
  WAV,
  FLAC,
  ITI
};

class AudioOutputCommon
{
protected:
  static bool write_file(const std::vector<uint8_t> &out, const char *filename);
  static bool write_file(const void *out, size_t out_len, const char *filename);
};

template<Audio FORMAT>
class AudioOutput : protected AudioOutputCommon
{
  template<class T>
  static bool convert(std::vector<uint8_t> &out, const AudioBuffer<T> &buffer,
   const AudioCue &start, const AudioCue &end);

public:

  template<class T>
  static bool write(const AudioBuffer<T> &buffer,
   const AudioCue &start, const AudioCue &end, const char *filename)
  {
    std::vector<uint8_t> out;
    if(!convert(out, buffer, start, end))
      return false;

    return write_file(out, filename);
  }

  template<class T>
  static bool write(const AudioBuffer<T> &buffer, const char *filename)
  {
    AudioCue start{ 0, AudioCue::On };
    AudioCue end{ buffer.frames(), AudioCue::Off };
    return write(buffer, start, end, filename);
  }

  template<class T>
  static bool write_all(const AudioBuffer<T> &buffer, const OptionNote &first,
   const char *filename)
  {
    char name[512];

    const std::vector<AudioCue> &cues = buffer.get_cues();
    const char *pos = strrchr(filename, '%');
    int offset = pos ? pos - filename : strlen(filename);
    int offset2 = pos ? offset + 1 : offset;
    unsigned next = first;

    for(size_t i = 1; i < cues.size(); i++)
    {
      if(cues[i - 1].type == AudioCue::On && cues[i].type == AudioCue::Off)
      {
        const char *note = MIDIInterface::get_note(next++);
        snprintf(name, sizeof(name), "%*.*s%s%s",
         offset, offset, filename, note, filename + offset2);

        if(!write(buffer, cues[i - 1], cues[i], name))
          return false;

        i++;
      }
    }
    return true;
  }
};


/* Format specializations--see individual format compilation units. */

template<> template<>
bool AudioOutput<Audio::RAW>::write<int16_t>(const AudioBuffer<int16_t> &buffer,
 const char *filename);

template<> template<>
bool AudioOutput<Audio::WAV>::convert<int16_t>(std::vector<uint8_t> &out,
 const AudioBuffer<int16_t> &buffer, const AudioCue &start, const AudioCue &end);

#endif /* AUDIOOUTPUT_HPP */

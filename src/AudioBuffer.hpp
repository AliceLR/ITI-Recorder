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

#ifndef AUDIOBUFFER_HPP
#define AUDIOBUFFER_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>
#include <vector>

class AudioInput
{
public:
  unsigned channels;
  unsigned rate;

  AudioInput(unsigned c, unsigned r): channels(c), rate(r) {}

  /* Read S16 stereo interleaved audio frames from a buffer. */
  virtual bool write(const void *frames_i, size_t num_frames_i) = 0;
};

template<class T, class U=typename std::enable_if<std::is_integral<T>::value>::type>
class AudioBuffer : public AudioInput
{
  std::vector<T> samples;
  size_t frames_left = 0;
  size_t idx = 0;

public:
  AudioBuffer(unsigned c, unsigned r): AudioInput(c, r)
  {
    if(c < 1)
      throw "invalid channels count";
  }

  void resize(size_t new_frames)
  {
    size_t new_size = new_frames * channels;
    if(new_size == samples.size())
      return;

    if(new_size < samples.size())
    {
      frames_left = 0;
    }
    else
      frames_left = new_size - samples.size();

    samples.resize(new_size);
  }

  virtual bool write(const void *frames_i, size_t num_frames_i)
  {
    num_frames_i = std::min(num_frames_i, frames_left);
    if(num_frames_i)
    {
      memcpy(&(samples[idx]), frames_i, num_frames_i * frame_size());
      idx += num_frames_i * channels;
      frames_left -= num_frames_i;
      return true;
    }
    return false;
  }

  size_t frame_size() const
  {
    return channels * sizeof(T);
  }

  size_t total_in() const
  {
    return idx / channels;
  }

  const std::vector<T> &get_samples() const
  {
    return samples;
  }
};

#endif /* AUDIOBUFFER_HPP */

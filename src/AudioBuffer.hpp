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

#include "Event.hpp"

struct AudioCue
{
  size_t frame;
  bool on;
};

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
  std::vector<AudioCue> cues;
  size_t frames_left = 0;
  size_t frame = 0;
  size_t idx = 0;

  static constexpr size_t abs(ssize_t v)
  {
    return (v > 0) ? v : -v;
  }

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
      frame = new_frames;
      idx = new_size;
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
      frame += num_frames_i;
      frames_left -= num_frames_i;
      return true;
    }
    return false;
  }

  void shrink_cues(size_t threshold)
  {
    for(size_t i = 0; i < cues.size(); i++)
    {
      size_t pos = cues[i].frame * channels;
      if(pos > samples.size())
        continue;

      if(cues[i].on)
      {
        size_t bound = frame;
        if(i + 1 < cues.size())
          bound = std::min(bound, cues[i + 1].frame);

        bound *= channels;
        for(; pos < bound; pos += channels)
          for(size_t smp = 0; smp < channels; smp++)
            if(abs(samples[pos + smp]) >= threshold)
              goto stop_on;
      stop_on:
        cues[i].frame = pos / channels;
      }
      else
      {
        size_t bound = 0;
        if(i > 0)
          bound = std::max(bound, cues[i - 1].frame);

        bound *= channels;
        for(; pos > bound; pos -= channels)
          for(ssize_t smp = -(ssize_t)channels; smp < 0; smp++)
            if(abs(samples[pos + smp]) >= threshold)
              goto stop_off;
      stop_off:
        cues[i].frame = pos / channels;
      }
    }
  }

  void reserve_cues(unsigned n)
  {
    cues.reserve(n);
  }

  void cue(bool on)
  {
    cues.push_back({ frame, on });
  }

  size_t frame_size() const
  {
    return channels * sizeof(T);
  }

  size_t total_frames() const
  {
    return frame;
  }

  const std::vector<T> &get_samples() const
  {
    return samples;
  }

  const std::vector<AudioCue> &get_cues() const
  {
    return cues;
  }
};


class AudioCueEvent
{
  template<class T>
  class _AudioCueEvent : public Event
  {
    AudioBuffer<T> &buffer;
    bool on;

  public:
    _AudioCueEvent(AudioBuffer<T> &_buffer, bool _on, int _time_ms):
    Event(_time_ms), buffer(_buffer), on(_on) {}

    virtual void task() const
    {
      fprintf(stderr, "cue: %s\n", on?"on":"off");
      buffer.cue(on);
    }
  };

public:
  template<class T>
  static void schedule(EventSchedule &ev, AudioBuffer<T> &_buffer, bool _on, int _time_ms)
  {
    ev.push(std::make_shared<_AudioCueEvent<T>>(_buffer, _on, _time_ms));
  }
};

#endif /* AUDIOBUFFER_HPP */

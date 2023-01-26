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
  enum Type
  {
    NoteOn,
    NoteOff,
    NoiseStart,
    NoiseEnd
  };

  size_t frame;
  Type type;
  int value;

  static constexpr const char *type_str(Type t)
  {
    switch(t)
    {
    case NoteOn: return "NoteOn";
    case NoteOff: return "NoteOff";
    case NoiseStart: return "NoiseStart";
    case NoiseEnd: return "NoiseEnd";
    }
    return "?";
  }
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

      if(cues[i].type == AudioCue::NoteOn)
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

      if(cues[i].type == AudioCue::NoteOff)
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

  void cue(AudioCue::Type type, int value)
  {
    cues.push_back({ frame, type, value });
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

  const T &operator[](size_t idx) const
  {
    return samples[idx];
  }
};


class AudioCueEvent
{
  template<class T>
  class _AudioCueEvent : public Event
  {
    AudioBuffer<T> &buffer;
    AudioCue::Type type;
    int value;
    bool has_value;

  public:
    _AudioCueEvent(AudioBuffer<T> &_buffer, AudioCue::Type _type, int _time_ms):
    Event(_time_ms), buffer(_buffer), type(_type), value(0), has_value(false) {}
    _AudioCueEvent(AudioBuffer<T> &_buffer, AudioCue::Type _type, int _value, int _time_ms):
    Event(_time_ms), buffer(_buffer), type(_type), value(_value), has_value(true) {}

    virtual void task() const
    {
      if(has_value)
        fprintf(stderr, "cue: %s = %d\n", AudioCue::type_str(type), value);
      else
        fprintf(stderr, "cue: %s\n", AudioCue::type_str(type));
      buffer.cue(type, value);
    }
  };

public:
  template<class T>
  static void schedule(EventSchedule &ev, AudioBuffer<T> &_buffer,
   AudioCue::Type _type, int _time_ms)
  {
    ev.push(std::make_shared<_AudioCueEvent<T>>(_buffer, _type, _time_ms));
  }

  template<class T>
  static void schedule(EventSchedule &ev, AudioBuffer<T> &_buffer,
   AudioCue::Type _type, int _val, int _time_ms)
  {
    ev.push(std::make_shared<_AudioCueEvent<T>>(_buffer, _type, _val, _time_ms));
  }
};

#endif /* AUDIOBUFFER_HPP */

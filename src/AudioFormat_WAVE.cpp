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

class Chunk
{
  std::vector<std::reference_wrapper<Chunk>> subchunks;
  std::vector<uint8_t> data;
  char magic[4];

public:
  Chunk(char a, char b, char c, char d)
  {
    magic[0] = a;
    magic[1] = b;
    magic[2] = c;
    magic[3] = d;
  }

  size_t length() const
  {
    size_t len = data.size();

    for(const Chunk &c : subchunks)
      len += c.length() + 8;

    return len;
  }

  template<class V>
  void insert(V v);

  template<class V, class... REST>
  void insert(V v, REST... rest)
  {
    insert<V>(v);
    insert<REST...>(rest...);
  }

  void insert(Chunk &c)
  {
    subchunks.push_back(c);
  }

  void reserve(size_t sz)
  {
    data.reserve(sz);
  }

  void flush(std::vector<uint8_t> &out)
  {
    for(char c : magic)
      out.push_back(c);

    size_t len = length();
    out.push_back(len & 0xff);
    out.push_back((len >> 8) & 0xff);
    out.push_back((len >> 16) & 0xff);
    out.push_back((len >> 24) & 0xff);

    out.insert(out.end(), data.begin(), data.end());
    data = std::vector<uint8_t>(); /* Delete contents. */

    for(Chunk &c : subchunks)
      c.flush(out);
  }
};

template<>
void Chunk::insert(uint8_t v)
{
  data.push_back(v);
}
template<>
void Chunk::insert(char v)
{
  data.push_back(v);
}

template<>
void Chunk::insert(uint16_t v)
{
  insert(
    static_cast<uint8_t>(v & 0xff),
    static_cast<uint8_t>((v >> 8) & 0xff)
  );
}
template<>
void Chunk::insert(int16_t v)
{
  insert(static_cast<uint16_t>(v));
}

template<>
void Chunk::insert(uint32_t v)
{
  insert(
    static_cast<uint8_t>(v & 0xff),
    static_cast<uint8_t>((v >> 8) & 0xff),
    static_cast<uint8_t>((v >> 16) & 0xff),
    static_cast<uint8_t>((v >> 24) & 0xff)
  );
}
template<>
void Chunk::insert(int32_t v)
{
  insert(static_cast<uint32_t>(v));
}


static class _AudioFormatWAVE : public AudioFormat
{
  template<class T>
  bool _convert(std::vector<uint8_t> &out, const AudioBuffer<T> &buffer,
   const AudioCue &start, const AudioCue &end) const
  {
    Chunk riff('R','I','F','F');
    riff.insert('W','A','V','E');

    Chunk fmt_('f','m','t',' ');
    riff.insert(fmt_);
    fmt_.insert<int16_t>(1);
    fmt_.insert<uint16_t>(buffer.channels);
    fmt_.insert<uint32_t>(buffer.rate);
    fmt_.insert<uint32_t>(buffer.rate * buffer.channels * sizeof(T));
    fmt_.insert<uint16_t>(buffer.channels * sizeof(T));
    fmt_.insert<uint16_t>(8 * sizeof(T));

    Chunk data('d','a','t','a');
    riff.insert(data);

    const std::vector<T> &smp = buffer.get_samples();
    size_t pos = start.frame * buffer.channels;
    size_t stop = end.frame * buffer.channels;

    data.reserve((stop - pos) * sizeof(T));
    for(; pos < stop; pos++)
      data.insert(smp[pos]);

    out.reserve(riff.length() + 8);
    riff.flush(out);
    return true;
  }

  virtual bool convert(std::vector<uint8_t> &out, const AudioBuffer<int16_t> &buffer,
   const AudioCue &start, const AudioCue &end) const
  {
    return _convert(out, buffer, start, end);
  }

  virtual bool convert(std::vector<uint8_t> &out, const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end) const
  {
    return _convert(out, buffer, start, end);
  }
} wave;

const AudioFormat &AudioFormatWAVE = wave;

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

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <type_traits>

enum class Endian
{
  LITTLE,
  BIG,
};


/**
 * File IO-style buffer writer with compile-time verifiable bounds checking.
 * This version of the class uses a reference, and is less likely to cause a
 * compiler to output slow code. However, it can not be used constexpr.
 */
template<size_t N, class ELEMENT = uint8_t, size_t I = 0>
class Buffer
{
  static_assert(I <= N, "oopsie woopsie that's a buffew ovewfwowwy OwO");

  ELEMENT (&buf)[N];

  template<int J, Endian ENDIAN>
  constexpr void put(uint8_t i_8)
  {
    buf[J] = i_8;
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int8_t i_8)
  {
    put<J, ENDIAN>(static_cast<uint8_t>(i_8));
  }
  template<int J, Endian ENDIAN>
  constexpr void put(char i_8)
  {
    put<J, ENDIAN>(static_cast<uint8_t>(i_8));
  }

  template<int J, Endian ENDIAN>
  constexpr void put(uint16_t i16)
  {
    if(ENDIAN == Endian::LITTLE)
    {
      buf[J + 0] = (i16 >> 0) & 0xff;
      buf[J + 1] = (i16 >> 8) & 0xff;
    }
    else
    {
      buf[J + 0] = (i16 >> 8) & 0xff;
      buf[J + 1] = (i16 >> 0) & 0xff;
    }
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int16_t i16)
  {
    put<J, ENDIAN>(static_cast<uint16_t>(i16));
  }

  template<int J, Endian ENDIAN>
  constexpr void put(uint32_t i32)
  {
    if(ENDIAN == Endian::LITTLE)
    {
      buf[J + 0] = (i32 >>  0) & 0xff;
      buf[J + 1] = (i32 >>  8) & 0xff;
      buf[J + 2] = (i32 >> 16) & 0xff;
      buf[J + 3] = (i32 >> 24) & 0xff;
    }
    else
    {
      buf[J + 0] = (i32 >> 24) & 0xff;
      buf[J + 1] = (i32 >> 16) & 0xff;
      buf[J + 2] = (i32 >>  8) & 0xff;
      buf[J + 3] = (i32 >>  0) & 0xff;
    }
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int32_t i32)
  {
    put<J, ENDIAN>(static_cast<uint32_t>(i32));
  }

  template<size_t J, class T, size_t LEN>
  constexpr void put(T (&arr)[LEN])
  {
    for(size_t i = 0; i < LEN; i++)
      buf[J + i] = arr[i];
  }
  template<size_t J, Endian ENDIAN, class T, size_t LEN>
  constexpr void put(T (&arr)[LEN])
  {
    put<J>(arr);
  }

  template<size_t J, Endian ENDIAN>
  constexpr void put()
  {
    return;
  }
  template<size_t J, Endian ENDIAN, class T, class... REST>
  constexpr void put(T value, REST... rest)
  {
    put<J, ENDIAN>(value);
    put<J + sizeof(T), ENDIAN, REST...>(rest...);
  }

public:
  constexpr Buffer(ELEMENT (&b)[N]): buf(b) {}

  template<class T, class U =
   typename std::enable_if<sizeof(T) == 1 && std::is_integral<T>::value>::type>
  constexpr Buffer<N, ELEMENT, I + 1> append(T i_8)
  {
    buf[I] = i_8;
    return Buffer<N, ELEMENT, I + 1>(buf);
  }

  template<class T, Endian ENDIAN = Endian::LITTLE, class U =
   typename std::enable_if<sizeof(T) == 2 && std::is_integral<T>::value>::type>
  constexpr Buffer<N, ELEMENT, I + 2> append(T i16)
  {
    put<I, ENDIAN>(i16);
    return Buffer<N, ELEMENT, I + 2>(buf);
  }

  template<class T, Endian ENDIAN = Endian::LITTLE, class U =
   typename std::enable_if<sizeof(T) == 4 && std::is_integral<T>::value>::type>
  constexpr Buffer<N, ELEMENT, I + 4> append(T i32)
  {
    put<I, ENDIAN>(i32);
    return Buffer<N, ELEMENT, I + 4>(buf);
  }

  template<int LEN, class T, class U =
   typename std::enable_if<sizeof(T) == 1 && std::is_integral<T>::value>::type>
  constexpr Buffer<N, ELEMENT, I + LEN> append(T (&arr)[LEN])
  {
    put<I>(arr);
    return Buffer<N, ELEMENT, I + LEN>(buf);
  }

  template<int LEN>
  constexpr Buffer<N, ELEMENT, I + LEN> skip() const
  {
    return Buffer<N, ELEMENT, I + LEN>(buf);
  }

  /* Parameter pack insert */
  template<Endian ENDIAN = Endian::LITTLE, class... REST>
  constexpr Buffer<N, ELEMENT, I + sizeof...(REST)> append(REST... rest)
  {
    put<I, ENDIAN, REST...>(rest...);
    return Buffer<N, ELEMENT, I + sizeof...(REST)>(buf);
  }

  /* Reset iterator, required for copy */
  template<size_t J = 0>
  constexpr operator Buffer<N, ELEMENT, J>()
  {
    return Buffer<N, ELEMENT,  J>(buf);
  }

  /* Assert this is the end of the buffer. */
  constexpr Buffer<N, ELEMENT, I> check() const
  {
    static_assert(I >= N, "did not fill the buffer");
    return *this;
  }

  /* Oh no! Unsafe!
   * (also allows subscripting) */
  using buftype = ELEMENT(&)[N];
  constexpr operator buftype()
  {
    return buf;
  }
  using const_buftype = const ELEMENT(&)[N];
  constexpr operator const_buftype() const
  {
    return buf;
  }
  constexpr const_buftype get() const
  {
    return buf;
  }

  constexpr auto begin() const
  {
    return std::begin(buf);
  }
  constexpr auto end() const
  {
    return std::end(buf);
  }
};


/**
 * File IO-style buffer writer with compile-time verifiable bounds checking.
 * This version of the class uses an internal byte array, which means it can
 * be used to create an array compile time. However, the potential large
 * number of array copies required may cause slow code otherwise.
 */
template<size_t N, class ELEMENT = uint8_t, size_t I = 0>
class StaticBuffer
{
  static_assert(I <= N, "oopsie woopsie that's a buffew ovewfwowwy OwO");

  ELEMENT buf[N];

  template<int J, Endian ENDIAN>
  constexpr void put(uint8_t i_8)
  {
    buf[J] = i_8;
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int8_t i_8)
  {
    put<J, ENDIAN>(static_cast<uint8_t>(i_8));
  }
  template<int J, Endian ENDIAN>
  constexpr void put(char i_8)
  {
    put<J, ENDIAN>(static_cast<uint8_t>(i_8));
  }

  template<int J, Endian ENDIAN>
  constexpr void put(uint16_t i16)
  {
    if(ENDIAN == Endian::LITTLE)
    {
      buf[J + 0] = (i16 >> 0) & 0xff;
      buf[J + 1] = (i16 >> 8) & 0xff;
    }
    else
    {
      buf[J + 0] = (i16 >> 8) & 0xff;
      buf[J + 1] = (i16 >> 0) & 0xff;
    }
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int16_t i16)
  {
    put<J, ENDIAN>(static_cast<uint16_t>(i16));
  }

  template<int J, Endian ENDIAN>
  constexpr void put(uint32_t i32)
  {
    if(ENDIAN == Endian::LITTLE)
    {
      buf[J + 0] = (i32 >>  0) & 0xff;
      buf[J + 1] = (i32 >>  8) & 0xff;
      buf[J + 2] = (i32 >> 16) & 0xff;
      buf[J + 3] = (i32 >> 24) & 0xff;
    }
    else
    {
      buf[J + 0] = (i32 >> 24) & 0xff;
      buf[J + 1] = (i32 >> 16) & 0xff;
      buf[J + 2] = (i32 >>  8) & 0xff;
      buf[J + 3] = (i32 >>  0) & 0xff;
    }
  }
  template<int J, Endian ENDIAN>
  constexpr void put(int32_t i32)
  {
    put<J, ENDIAN>(static_cast<uint32_t>(i32));
  }

  template<size_t J, class T, size_t LEN>
  constexpr void put(T (&arr)[LEN])
  {
    for(size_t i = 0; i < LEN; i++)
      buf[J + i] = arr[i];
  }
  template<size_t J, Endian ENDIAN, class T, size_t LEN>
  constexpr void put(T (&arr)[LEN])
  {
    put<J>(arr);
  }

  template<size_t J, Endian ENDIAN>
  constexpr void put()
  {
    return;
  }
  template<size_t J, Endian ENDIAN, class T, class... REST>
  constexpr void put(T value, REST... rest)
  {
    put<J, ENDIAN>(value);
    put<J + sizeof(T), ENDIAN, REST...>(rest...);
  }

public:
  constexpr StaticBuffer(): buf{} {}
  constexpr StaticBuffer(ELEMENT (&b)[N]): buf{}
  {
    put<0>(b);
  }

  template<class T, class U =
   typename std::enable_if<sizeof(T) == 1 && std::is_integral<T>::value>::type>
  constexpr StaticBuffer<N, ELEMENT, I + 1> append(T i_8)
  {
    buf[I] = i_8;
    return StaticBuffer<N, ELEMENT, I + 1>(buf);
  }

  template<class T, Endian ENDIAN = Endian::LITTLE, class U =
   typename std::enable_if<sizeof(T) == 2 && std::is_integral<T>::value>::type>
  constexpr StaticBuffer<N, ELEMENT, I + 2> append(T i16)
  {
    put<I, ENDIAN>(i16);
    return StaticBuffer<N, ELEMENT, I + 2>(buf);
  }

  template<class T, Endian ENDIAN = Endian::LITTLE, class U =
   typename std::enable_if<sizeof(T) == 4 && std::is_integral<T>::value>::type>
  constexpr StaticBuffer<N, ELEMENT, I + 4> append(T i32)
  {
    put<I, ENDIAN>(i32);
    return StaticBuffer<N, ELEMENT, I + 4>(buf);
  }

  template<int LEN, class T, class U =
   typename std::enable_if<sizeof(T) == 1 && std::is_integral<T>::value>::type>
  constexpr StaticBuffer<N, ELEMENT, I + LEN> append(T (&arr)[LEN])
  {
    put<I>(arr);
    return StaticBuffer<N, ELEMENT, I + LEN>(buf);
  }

  template<int LEN>
  constexpr StaticBuffer<N, ELEMENT, I + LEN> skip() const
  {
    return StaticBuffer<N, ELEMENT, I + LEN>(buf);
  }

  /* Parameter pack insert */
  template<Endian ENDIAN = Endian::LITTLE, class... REST>
  constexpr StaticBuffer<N, ELEMENT, I + sizeof...(REST)> append(REST... rest)
  {
    put<I, ENDIAN, REST...>(rest...);
    return StaticBuffer<N, ELEMENT, I + sizeof...(REST)>(buf);
  }

  /* Reset iterator, required for copy */
  template<size_t J = 0>
  constexpr operator StaticBuffer<N, ELEMENT, J>()
  {
    return StaticBuffer<N, ELEMENT,  J>(buf);
  }

  /* Oh no! Unsafe!
   * (also allows subscripting) */
  using buftype = ELEMENT(&)[N];
  constexpr operator buftype()
  {
    return buf;
  }
  using const_buftype = const ELEMENT(&)[N];
  constexpr operator const_buftype() const
  {
    return buf;
  }
  constexpr const_buftype get() const
  {
    return buf;
  }

  constexpr auto begin() const
  {
    return std::begin(buf);
  }
  constexpr auto end() const
  {
    return std::end(buf);
  }
};


#ifdef TESTING
int main()
{
  uint8_t buf[7];

  /* Compiles */
  Buffer<7>(buf)
    .append('I','M','P','I')
    .append<uint8_t>(1)
    .append<int16_t>(2);

  /* Compiles */
  constexpr StaticBuffer<7> st = StaticBuffer<7>()
    .append<uint8_t>(0xcd)
    .append<uint16_t>(0x89ab)
    .append<int32_t>(0x01234567);

  /* Compiles */
  StaticBuffer<7> cpy = StaticBuffer<7>()
    .append(st.get());

  /* Does not compile */
  //Buffer<7>(buf).append(1).append(2);

  /* Don't optimize out */
  printf("%d %d %d %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
  printf("%d %d %d %d %d %d %d\n", st[0], st[1], st[2], st[3], st[4], st[5], st[6]);
  printf("%d %d %d %d %d %d %d\n", cpy[0], cpy[1], cpy[2], cpy[3], cpy[4], cpy[5], cpy[6]);
  return 0;
}
#endif

#endif /* BUFFER_HPP */

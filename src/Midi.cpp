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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Config.hpp"
#include "Midi.hpp"
#include "Soundcard.hpp"

void MIDIEvent::task() const
{
  //*
  for(uint8_t d : data)
  {
    printf("%02x ", d);
    if(d == 0xf7)
      printf("\n");
  }
  printf("\n");
  //*/
  Soundcard::get().midi_write(data, interface.device);
}

void MIDIInterface::cc(std::vector<uint8_t> &out, unsigned param, unsigned value) const
{
  const InputConfig *cfg = get_input_config();
  uint8_t midi[3];

  midi[0] = 0xb0 | (cfg->midi_channel.value() - 1);
  midi[1] = param;
  midi[2] = value;

  out.insert(out.end(), std::begin(midi), std::end(midi));
}

void MIDIInterface::program_change(std::vector<uint8_t> &out, unsigned program) const
{
  const InputConfig *cfg = get_input_config();
  uint8_t midi[2];

  midi[0] = 0xc0 | (cfg->midi_channel.value() - 1);
  midi[1] = program;

  out.insert(out.end(), std::begin(midi), std::end(midi));
}

void MIDIInterface::note_on(std::vector<uint8_t> &out, unsigned note, unsigned velocity) const
{
  const InputConfig *cfg = get_input_config();
  uint8_t midi[3];

  midi[0] = 0x90 | (cfg->midi_channel.value() - 1);
  midi[1] = note;
  midi[2] = velocity;

  out.insert(out.end(), std::begin(midi), std::end(midi));
}

void MIDIInterface::note_off(std::vector<uint8_t> &out, unsigned note, unsigned velocity) const
{
  const InputConfig *cfg = get_input_config();
  uint8_t midi[3];

  midi[0] = 0x80 | (cfg->midi_channel.value() - 1);
  midi[1] = note;
  midi[2] = velocity;

  out.insert(out.end(), std::begin(midi), std::end(midi));
}

void MIDIInterface::all_off(std::vector<uint8_t> &out) const
{
  const InputConfig *cfg = get_input_config();
  uint8_t midi[3] = { 0xb0, 0xf8, 0x00 };

  midi[0] |= cfg->midi_channel.value() - 1;

  out.insert(out.end(), std::begin(midi), std::end(midi));
}

const char *MIDIInterface::get_note(unsigned note)
{
  static constexpr const char *notes[128] =
  {
    "C-n", "C#n", "D-n", "D#n", "E-n", "F-n",
    "F#n", "G-n", "G#n", "A-n", "A#n", "B-n",

    "C-0", "C#0", "D-0", "D#0", "E-0", "F-0",
    "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",

    "C-1", "C#1", "D-1", "D#1", "E-1", "F-1",
    "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",

    "C-2", "C#2", "D-2", "D#2", "E-2", "F-2",
    "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",

    "C-3", "C#3", "D-3", "D#3", "E-3", "F-3",
    "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",

    "C-4", "C#4", "D-4", "D#4", "E-4", "F-4",
    "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",

    "C-5", "C#5", "D-5", "D#5", "E-5", "F-5",
    "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",

    "C-6", "C#6", "D-6", "D#6", "E-6", "F-6",
    "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",

    "C-7", "C#7", "D-7", "D#7", "E-7", "F-7",
    "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",

    "C-8", "C#8", "D-8", "D#8", "E-8", "F-8",
    "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",

    "C-9", "C#9", "D-9", "D#9", "E-9", "F-9",
    "F#9", "G-9",
  };

  if(note < 128)
    return notes[note];

  return "n/a";
}

uint8_t MIDIInterface::roland_checksum(const uint8_t *d, size_t sz)
{
  int sum = 0;
  for(size_t i = 0; i < sz; i++)
    sum += d[i];

  return (-sum) & 0x7f;
}

bool MIDIInterface::load_file(std::vector<uint8_t> &out, const char *path)
{
  struct stat st;

  if(stat(path, &st) == 0 && S_ISREG(st.st_mode))
  {
    FILE *fp = fopen(path, "rb");
    if(fp)
    {
      out.resize(st.st_size);

      off_t sz = fread(out.data(), 1, st.st_size, fp);
      if(sz < st.st_size)
        out.resize(sz);

      fclose(fp);
      return true;
    }
  }
  else
    fprintf(stderr, "couldn't find file '%s', ignoring\n", path);

  return false;
}

/*
class MIDIRegister : public ConfigRegister
{
public:
  MIDIRegister(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new MIDIInterface(ctx, tag, id));
  }
} midi("MIDIInterface");
*/

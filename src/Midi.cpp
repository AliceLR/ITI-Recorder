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

#include <ctype.h>
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
    fprintf(stderr, "%02x ", d);
    if(d == 0xf7)
      fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
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
    "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1",
    "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",

    "C0", "C#0", "D0", "D#0", "E0", "F0",
    "F#0", "G0", "G#0", "A0", "A#0", "B0",

    "C1", "C#1", "D1", "D#1", "E1", "F1",
    "F#1", "G1", "G#1", "A1", "A#1", "B1",

    "C2", "C#2", "D2", "D#2", "E2", "F2",
    "F#2", "G2", "G#2", "A2", "A#2", "B2",

    "C3", "C#3", "D3", "D#3", "E3", "F3",
    "F#3", "G3", "G#3", "A3", "A#3", "B3",

    "C4", "C#4", "D4", "D#4", "E4", "F4",
    "F#4", "G4", "G#4", "A4", "A#4", "B4",

    "C5", "C#5", "D5", "D#5", "E5", "F5",
    "F#5", "G5", "G#5", "A5", "A#5", "B5",

    "C6", "C#6", "D6", "D#6", "E6", "F6",
    "F#6", "G6", "G#6", "A6", "A#6", "B6",

    "C7", "C#7", "D7", "D#7", "E7", "F7",
    "F#7", "G7", "G#7", "A7", "A#7", "B7",

    "C8", "C#8", "D8", "D#8", "E8", "F8",
    "F#8", "G8", "G#8", "A8", "A#8", "B8",

    "C9", "C#9", "D9", "D#9", "E9", "F9",
    "F#9", "G9",
  };

  if(note < 128)
    return notes[note];

  return "n/a";
}

/**
 * Get the MIDI note for a given note string.
 * @param note  a nul-terminated string representing a note.
 * @return      a value between 0 and 127 (inclusive) representing a note in
 *              the range of C-1 to G9, otherwise -1.
 */
int MIDIInterface::get_note_value(const char *note)
{
  if(!note[0] || !note[1])
    return -1;

  int val = 0;
  int octave = 1;
  switch(tolower((uint8_t)*note))
  {
  case 'c':
    val = 0;
    break;
  case 'd':
    val = 2;
    break;
  case 'e':
    val = 4;
    break;
  case 'f':
    val = 5;
    break;
  case 'g':
    val = 7;
    break;
  case 'a':
    val = 9;
    break;
  case 'b':
    val = 11;
    break;
  default:
    return -1;
  }
  note++;

  if(*note == '#' || *note == 's')
  {
    val++;
    note++;
  }
  if(*note == 'b')
  {
    val--;
    note++;
  }

  if(*note == '-')
  {
    octave = -1;
    note++;
  }
  if(*note < '0' || *note > '9')
    return -1;

  octave *= (*note - '0');
  if(octave < -1 || note[1] != '\0')
    return -1;

  int midi_note = (octave + 1) * 12 + val;
  if(midi_note < 0 || midi_note > 127)
    return -1;
  return midi_note;
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

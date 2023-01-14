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

#include "Config.hpp"
#include "Event.hpp"
#include "Midi.hpp"

#include <ctype.h>

/* Convert Roland MIDI RAM addresses to continuous 21-bit values. */
static constexpr unsigned address(uint8_t a, uint8_t b, uint8_t c)
{
  return ((a & 0x80) || (b & 0x80) || (c & 0x80)) ? 0 : ((a << 14) | (b << 7) | c);
}
static constexpr unsigned address(unsigned m)
{
  return (m & 0x808080) ? 0 : ((m & 0x7f0000) >> 2) | ((m & 0x7f00) >> 1) | (m & 0x7f);
}
static constexpr unsigned address_add(unsigned m, unsigned a)
{
  return address(m) + address(a);
}

static constexpr unsigned MemoryToneGroup = 2;

class OptionTone : public ConfigOption
{
  unsigned gr = 0; // 0=a 1=b 2=c/i 3=r
  unsigned to = 0; // 1-64

public:
  OptionTone(std::vector<ConfigOption *> &v,
   const char *def, const char *_key):
   ConfigOption(v, _key)
  {
    OptionTone::handle(def);
  }

  virtual ~OptionTone() {}
  virtual bool handle(const char *value)
  {
    unsigned g;
    unsigned t;

    switch(tolower(value[0]))
    {
      case 'a':
        g = 0;
        break;
      case 'b':
        g = 1;
        break;
      case 'c':
      case 'i':
        g = 2;
        break;
      case 'r':
        g = 3;
        break;
      default:
        return false;
    }
    if(value[1] < '0' || value[1] > '9' ||
       value[2] < '0' || value[2] > '9' || value[3])
      return false;

    t = (value[1] - '0') * 10 + (value[2] - '0');
    if(t < 1 || t > 64)
      return false;

    to = t;
    gr = g;
    return true;
  }

  virtual void print()
  {
    static constexpr char groups[4] = { 'a', 'b', 'i', 'r' };
    printf("%s=%c%02u\n", key, groups[gr], to);
  }

  unsigned group() const { return gr; }
  unsigned tone() const { return to; }

  unsigned group(unsigned g)
  {
    if(g <= 3)
      gr = g;
    return gr;
  }
  unsigned tone(unsigned t)
  {
    if(t >= 1 && t <= 64)
      to = t;
    return to;
  }
};

class OptionBiasPoint : public ConfigOption
{
  unsigned val;

public:
  OptionBiasPoint(std::vector<ConfigOption *> &v, const char *def, const char *_key):
   ConfigOption(v, _key)
  {
    if(!handle(def))
      throw "bad default string";
  }

  virtual bool handle(const char *value)
  {
    if(value[0] != '<' && value[0] != '>')
      return false;

    int a = MIDIInterface::get_note_value(value + 1);
    if((unsigned)a < MIDIInterface::A1 || (unsigned)a > MIDIInterface::C7)
      return false;

    a -= MIDIInterface::A1;
    val = (value[0] == '<') ? a : a + 64;
    return true;
  }

  virtual void print()
  {
    printf("%s=%c%s\n", key, (val < 64 ? '<' : '>'),
     MIDIInterface::get_note((val & 63) + MIDIInterface::A1));
  }

  operator unsigned() const { return val; }
  OptionBiasPoint &operator=(unsigned v)
  {
    if(v < 128)
      val = v;
    return *this;
  }
};


class VAPartial : public ConfigSubinterface
{
public:
  static constexpr EnumValue Keyfollow[] =
  {
    { "-1", 0 },
    { "-1/2", 1 },
    { "-1/4", 2 },
    { "0", 3 },
    { "1/8", 4 },
    { "1/4", 5 },
    { "3/8", 6 },
    { "1/2", 7 },
    { "5/8", 8 },
    { "3/4", 9 },
    { "7/8", 10 },
    { "1", 11 },
    { "5/4", 12 },
    { "3/2", 13 },
    { "2", 14 },
    { "s1", 15 },
    { "s2", 16 },
    { }
  };
  static constexpr EnumValue Waveform[] =
  {
    { "square", 0 },
    { "squ", 0 },
    { "saw", 1 },
    { }
  };
  OptionBool        Mute; // Actually stored in the tone.

  OptionNote        WGPitchCoarse;      // 0-96, 0=C1, 96=C9
  Option<int>       WGPitchFine;        // -50 to 50 (SysEx: 0-100)
  Enum<Keyfollow>   WGPitchKeyfollow;
  OptionBool        WGPitchBender;
  Enum<Waveform>    WGWaveform;         // squ saw (SysEx: bit 0 of Waveform/PCM Bank)
  Option<unsigned>  WGPCMBank;          // 1-2 (SysEx: bit 1 of Waveform/PCM Bank)
  Option<unsigned>  WGPCMWave;          // 1-128 (SysEx: 0-127)
  Option<unsigned>  WGPulseWidth;       // 0-100
  Option<int>       WGPulseWidthVelocity; // -7 to 7 (SysEx: 0-14)

  Option<unsigned>  PEnvDepth;          // 0-10
  Option<unsigned>  PEnvVelocity;       // 0-3
  Option<unsigned>  PEnvTimeKeyfollow;  // 0-4
  Option<unsigned>  PEnvTime1;          // 0-100
  Option<unsigned>  PEnvTime2;
  Option<unsigned>  PEnvTime3;
  Option<unsigned>  PEnvTime4;
  Option<int>       PEnvLevel0;         // -50 to 50 (SysEx: 0-100)
  Option<int>       PEnvLevel1;
  Option<int>       PEnvLevel2;
  Option<int>       PEnvSustainLevel;   // D-110 and MT-32
  Option<int>       PEnvEndLevel;

  Option<unsigned>  LFORate;            // 0-100
  Option<unsigned>  LFODepth;
  Option<unsigned>  LFOModulation;

  Option<unsigned>  TVFCutoff;          // 0-100
  Option<unsigned>  TVFResonance;       // 0-30
  Enum<Keyfollow>   TVFKeyfollow;       // like WGPitchKeyfollow but no s1/s2
  OptionBiasPoint   TVFBiasPoint;       // 0-127 (<A1 to <C7 >A1 to >C7)
  Option<int>       TVFBiasLevel;       // -7 to 7 (SysEx: 0-14)

  Option<unsigned>  TVFEnvDepth;        // 0-100
  Option<unsigned>  TVFEnvVelocity;     // 0-100
  Option<unsigned>  TVFEnvDepthKeyfollow; // 0-4
  Option<unsigned>  TVFEnvTimeKeyfollow;  // 0-4
  Option<unsigned>  TVFEnvTime1;        // 0-100
  Option<unsigned>  TVFEnvTime2;
  Option<unsigned>  TVFEnvTime3;
  Option<unsigned>  TVFEnvTime4;
  Option<unsigned>  TVFEnvTime5;        // D-110 and MT-32. Keyboard time 4 saved here.
  Option<unsigned>  TVFEnvLevel1;       // 0-100
  Option<unsigned>  TVFEnvLevel2;
  Option<unsigned>  TVFEnvLevel3;       // D-110 and MT-32
  Option<unsigned>  TVFEnvSustainLevel;

  Option<unsigned>  TVALevel;           // 0-100
  Option<int>       TVAVelocity;        // -50 to 50 (SysEx: 0-100)
  OptionBiasPoint   TVABiasPoint1;      // see TVFBiasPoint
  Option<int>       TVABiasLevel1;      // -12 to 0 (SysEx: 0-12)
  OptionBiasPoint   TVABiasPoint2;
  Option<int>       TVABiasLevel2;

  Option<unsigned>  TVAEnvTimeKeyfollow;  // 0-4
  Option<unsigned>  TVAEnvVelocity;     // 0-4
  Option<unsigned>  TVAEnvTime1;        // 0-100
  Option<unsigned>  TVAEnvTime2;
  Option<unsigned>  TVAEnvTime3;
  Option<unsigned>  TVAEnvTime4;
  Option<unsigned>  TVAEnvTime5;        // D-110 and MT-32. Keyboard time 4 saved here.
  Option<unsigned>  TVAEnvLevel1;       // 0-100
  Option<unsigned>  TVAEnvLevel2;
  Option<unsigned>  TVAEnvLevel3;       // D-110 and MT-32
  Option<unsigned>  TVAEnvSustainLevel;

  VAPartial(std::vector<ConfigSubinterface *> &v, const char *_name):
   ConfigSubinterface(v, _name),
   /* Options */
   Mute(options, "off", "Mute"),
   WGPitchCoarse(options, "C4", "C1", "C9", 0, "WGPitchCoarse"),
   WGPitchFine(options, 0, -50, 50, "WGPitchFine"),
   WGPitchKeyfollow(options, "s1", "WGPitchKeyfollow"),
   WGPitchBender(options, true, "WGPitchBender"),
   WGWaveform(options, "square", "WGWaveform"),
   WGPCMBank(options, 1, 1, 2, "WGPCMBank"),
   WGPCMWave(options, 1, 1, 128, "WGPCMWave"),
   WGPulseWidth(options, 0, 0, 100, "WGPulseWidth"),
   WGPulseWidthVelocity(options, 0, -7, 7, "WGPulseWidthVelocity"),

   PEnvDepth(options, 0, 0, 4, "PEnvDepth"),
   PEnvVelocity(options, 0, 0, 3, "PEnvVelocity"),
   PEnvTimeKeyfollow(options, 0, 0, 4, "PEnvTimeKeyfollow"),
   PEnvTime1(options, 0, 0, 100, "PEnvTime1"),
   PEnvTime2(options, 0, 0, 100, "PEnvTime2"),
   PEnvTime3(options, 0, 0, 100, "PEnvTime3"),
   PEnvTime4(options, 0, 0, 100, "PEnvTime4"),
   PEnvLevel0(options, 0, -50, 50, "PEnvLevel0"),
   PEnvLevel1(options, 0, -50, 50, "PEnvLevel1"),
   PEnvLevel2(options, 0, -50, 50, "PEnvLevel2"),
   PEnvSustainLevel(options, 0, -50, 50, "PEnvSustainLevel"),
   PEnvEndLevel(options, 0, -50, 50, "PEnvEndLevel"),

   LFORate(options, 0, 0, 100, "LFORate"),
   LFODepth(options, 0, 0, 100, "LFODepth"),
   LFOModulation(options, 0, 0, 100, "LFOModulation"),

   TVFCutoff(options, 0, 0, 100, "TVFCutoff"),
   TVFResonance(options, 0, 0, 30, "TVFResonance"),
   TVFKeyfollow(options, "0", "TVFKeyfollow"),
   TVFBiasPoint(options, "<A1", "TVFBiasPoint"),
   TVFBiasLevel(options, 0, -7, 7, "TVFBiasLevel"),

   TVFEnvDepth(options, 0, 0, 100, "TVFEnvDepth"),
   TVFEnvVelocity(options, 0, 0, 100, "TVFEnvVelocity"),
   TVFEnvDepthKeyfollow(options, 0, 0, 4, "TVFEnvDepthKeyfollow"),
   TVFEnvTimeKeyfollow(options, 0, 0, 4, "TVFEnvTimeKeyfollow"),
   TVFEnvTime1(options, 0, 0, 100, "TVFEnvTime1"),
   TVFEnvTime2(options, 0, 0, 100, "TVFEnvTime2"),
   TVFEnvTime3(options, 0, 0, 100, "TVFEnvTime3"),
   TVFEnvTime4(options, 0, 0, 100, "TVFEnvTime4"),
   TVFEnvTime5(options, 0, 0, 100, "TVFEnvTime5"),
   TVFEnvLevel1(options, 0, 0, 100, "TVFEnvLevel1"),
   TVFEnvLevel2(options, 0, 0, 100, "TVFEnvLevel2"),
   TVFEnvLevel3(options, 0, 0, 100, "TVFEnvLevel3"),
   TVFEnvSustainLevel(options, 0, 0, 100, "TVFEnvSustainLevel"),

   TVALevel(options, 100, 0, 100, "TVALevel"),
   TVAVelocity(options, 50, -50, 50, "TVAVelocity"),
   TVABiasPoint1(options, ">C4", "TVABiasPoint1"),
   TVABiasLevel1(options, 0, -12, 0, "TVABiasLevel1"),
   TVABiasPoint2(options, "<C4", "TVABiasPoint2"),
   TVABiasLevel2(options, 0, -12, 0, "TVABiasLevel2"),

   TVAEnvTimeKeyfollow(options, 0, 0, 4, "TVAEnvTimeKeyfollow"),
   TVAEnvVelocity(options, 0, 0, 4, "TVAEnvVelocity"),
   TVAEnvTime1(options, 0, 0, 100, "TVAEnvTime1"),
   TVAEnvTime2(options, 0, 0, 100, "TVAEnvTime2"),
   TVAEnvTime3(options, 0, 0, 100, "TVAEnvTime3"),
   TVAEnvTime4(options, 0, 0, 100, "TVAEnvTime4"),
   TVAEnvTime5(options, 0, 0, 100, "TVAEnvTime5"),
   TVAEnvLevel1(options, 100, 0, 100, "TVAEnvLevel1"),
   TVAEnvLevel2(options, 100, 0, 100, "TVAEnvLevel2"),
   TVAEnvLevel3(options, 100, 0, 100, "TVAEnvLevel3"),
   TVAEnvSustainLevel(options, 100, 0, 100, "TVAEnvSustainLevel")
  {}

  /* Not an independent SysEx, but repeated 4 times in the tone SysEx. */
  void program(uint8_t *buf, bool is_mt32) const;
  void load(const uint8_t *buf, bool is_mt32);
};

class VATone : public ConfigSubinterface
{
public:
  VAPartial &p1;
  VAPartial &p2;
  VAPartial &p3;
  VAPartial &p4;

  OptionString<10>  Name;
  Option<unsigned>  Structure12;  // 1-13 (SysEx: 0-12)
  Option<unsigned>  Structure34;
  OptionBool        Sustain;      // inverted in SysEx: 0=sustain, 1=no sustain

  VATone(std::vector<ConfigSubinterface *> &v, const char *_name,
   VAPartial &_p1, VAPartial &_p2, VAPartial &_p3, VAPartial &_p4):
   ConfigSubinterface(v, _name),
   p1(_p1), p2(_p2), p3(_p3), p4(_p4),
   /* Options */
   Name(options, "<default>", "Name"),
   Structure12(options, 1, 1, 13, "Structure12"),
   Structure34(options, 1, 1, 13, "Structure34"),
   Sustain(options, true, "Sustain")
  {}

  void program(std::vector<uint8_t> &out, unsigned UnitID, unsigned part, bool is_mt32) const;
  void load(const uint8_t *buf, bool is_mt32);
};

class VAPatch : public ConfigSubinterface
{
public:
  static constexpr EnumValue KeyModes[] =
  {
    { "whole", 0 },
    { "dual", 1 },
    { "split", 2 },
    { }
  };
  static constexpr EnumValue AssignModes[] =
  {
    { "POLY1", 0 },
    { "POLY2", 1 },
    { "POLY3", 2 },
    { "POLY4", 3 },
    { }
  };
  static constexpr EnumValue ReverbModes[] =
  {
    { "Room1", 0 },
    { "Room2", 1 },
    { "Hall1", 2 },
    { "Hall2", 3 },
    { "Plate", 4 },
    { "Tap1",  5 },
    { "Tap2",  6 },
    { "Tap3",  7 },
    { "off",   8 },
    { }
  };

  OptionString<16>  Name;
  Option<unsigned>  Level;            // 0-100
  Enum<KeyModes>    KeyMode;          // 0=whole 1=dual 2=split
  OptionNote        SplitPoint;       // 0-61 = C2-C#7
  Option<int>       Balance;          // -50 to 50 (SysEx: 0-100) (0=Lower max, 100=Upper max)
  OptionTone        LowerTone;
  OptionTone        UpperTone;
  Option<int>       LowerKeyShift;    // -24 to 24 (SysEx: 0-48)
  Option<int>       UpperKeyShift;    // -24 to 24 (SysEx: 0-48)
  Option<int>       LowerFinetune;    // -50 to 50 (SysEx: 0-100)
  Option<int>       UpperFinetune;    // -50 to 50 (SysEx: 0-100)
  Option<unsigned>  LowerBenderRange; // 0 to 24
  Option<unsigned>  UpperBenderRange; // 0 to 24
  Enum<AssignModes> LowerAssignMode;  // 0=POLY1 1=POLY2 2=POLY3 3=POLY4
  Enum<AssignModes> UpperAssignMode;
  OptionBool        LowerReverb;
  OptionBool        UpperReverb;
  Enum<ReverbModes> ReverbMode;
  Option<unsigned>  ReverbTime;       // 1-8 (SysEx: 0-7)
  Option<unsigned>  ReverbLevel;      // 0-7

  VAPatch(std::vector<ConfigSubinterface *> &v, const char *_name):
   ConfigSubinterface(v, _name),
   /* Options */
   Name(options, "<default>", "Name"),
   Level(options, 100, 0, 100, "Level"),
   KeyMode(options, "whole", "KeyMode"),
   SplitPoint(options, "C4", "C2", "C#7", 0, "SplitPoint"),
   Balance(options, 0, -50, 50, "Balance"),
   LowerTone(options, "i01", "LowerTone"),
   UpperTone(options, "i01", "UpperTone"),
   LowerKeyShift(options, 0, -24, 24, "LowerKeyShift"),
   UpperKeyShift(options, 0, -24, 24, "UpperKeyShift"),
   LowerFinetune(options, 0, -50, 50, "LowerFinetune"),
   UpperFinetune(options, 0, -50, 50, "UpperFinetune"),
   LowerBenderRange(options, 2, 0, 24, "LowerBenderRange"),
   UpperBenderRange(options, 2, 0, 24, "UpperBenderRange"),
   LowerAssignMode(options, "POLY1", "LowerAssignMode"),
   UpperAssignMode(options, "POLY1", "UpperAssignMode"),
   LowerReverb(options, false, "LowerReverb"),
   UpperReverb(options, false, "UpperReverb"),
   ReverbMode(options, "Off", "ReverbMode"),
   ReverbTime(options, 1, 1, 8, "ReverbTime"),
   ReverbLevel(options, 0, 0, 7, "ReverbLevel")
  {}

  void program_patch(std::vector<uint8_t> &out, unsigned UnitID) const;
  void program_timbre(std::vector<uint8_t> &out, unsigned UnitID, unsigned part) const;

  void load_patch(const uint8_t *buf);
  void load_timbre(const uint8_t *buf);
};

class VAPatchFX : public ConfigSubinterface
{
public:
  static constexpr EnumValue Modes[] =
  {
    { "off", 0 },
    { "chord", 1},
    { "chordplay", 1 },
    { "harmony", 2 },
    { "chase", 3 },
    { "arpeggio", 4 },
    { }
  };
  static constexpr EnumValue ArpModes[] =
  {
    { "up", 0 },
    { "down", 1 },
    { "ud", 2 },
    { "u&d", 2 },
    { "updown", 2 },
    { "up&down", 2 },
    { "rnd", 3 },
    { "random", 3 },
    { }
  };

  Enum<Modes>       Mode;           // 0=off 1=chord 2=harmony 3=chase 4=arpeggio
  Option<unsigned>  Rate;           // 0-100
  Option<int>       HarmonyBalance; // -12 to 0 (SysEx: 0-12)
  Option<int>       ChaseShift;     // -12 to 12 (SysEx: 0-24)
  Option<unsigned>  ChaseLevel;     // 0-100
  Enum<ArpModes>    ArpeggioMode;   // 0=up 1=down 2=up&down 3=random

  VAPatchFX(std::vector<ConfigSubinterface *> &v, const char *_name):
   ConfigSubinterface(v, _name),
   /* Options*/
   Mode(options, "off", "Mode"),
   Rate(options, 50, 0, 100, "Rate"),
   HarmonyBalance(options, 0, -12, 0, "HarmonyBalance"),
   ChaseShift(options, 0, -12, 12, "ChaseShift"),
   ChaseLevel(options, 50, 0, 100, "ChaseLevel"),
   ArpeggioMode(options, "up", "ArpeggioMode")
  {}

  void program(std::vector<uint8_t> &out, unsigned UnitID) const;
  void load(const uint8_t *buf);
};

class D5Interface final : public MIDIInterface
{
public:
  VAPatch   patch;
  VAPatchFX patchfx;
  VATone    upper;
  VATone    lower;
  VAPartial parts[8];
  bool      is_mt32;
  bool      is_d110;
  bool      is_d5;

  /* Setup */
  OptionString<512> SysExPath;
  Option<unsigned>  SysExPatch;
  Option<unsigned>  SysExMode;
  Option<unsigned>  UnitID;
  Option<unsigned>  Part;

  D5Interface(ConfigContext &_ctx, const char *_tag, int _id):
   MIDIInterface(_ctx, _tag, _id),
   patch(subs, "patch"),
   patchfx(subs, "patchfx"),
   upper(subs, "upper", parts[0], parts[1], parts[2], parts[3]),
   lower(subs, "lower", parts[4], parts[5], parts[6], parts[7]),
   parts{ { subs, "upper.partial1" },
          { subs, "upper.partial2" },
          { subs, "upper.partial3" },
          { subs, "upper.partial4" },
          { subs, "lower.partial1" },
          { subs, "lower.partial2" },
          { subs, "lower.partial3" },
          { subs, "lower.partial4" }},
   /* Options */
   SysExPath(options, "", "SysExPath"),
   SysExPatch(options, 1, 1, 128, "SysExPatch"),
   SysExMode(options, 0, 0, 1, "SysExMode"),
   UnitID(options, 17, 17, 32, "UnitID"),
   Part(options, 0, 0, 8, "Part")
  {
    is_mt32 = !strcasecmp(_tag, "MT-32");
    is_d110 = !strcasecmp(_tag, "D-110");
    is_d5 = !strcasecmp(_tag, "D-5");
  }

  virtual void program(EventSchedule &ev) const
  {
    std::vector<uint8_t> out;
    unsigned part = Part;

    if(part == 0 && (is_mt32 || is_d110))
    {
      /* MT-32 is always the same as Multi Timbral mode.
       * TODO: D-110 has an additional "Patch" layer unrelated to the keyboards. */
      part = 1;
    }

    if(part)
    {
      /* Multi Timbral mode */
      patch.program_timbre(out, UnitID, part);
      upper.program(out, UnitID, part, is_mt32 || is_d110);
      // TODO: how is reverb configured here?
    }
    else
    {
      /* Performance mode */
      patch.program_patch(out, UnitID);
      if(is_d5)
        patchfx.program(out, UnitID);

      upper.program(out, UnitID, 1, is_mt32 || is_d110);
      lower.program(out, UnitID, 2, is_mt32 || is_d110);
    }

    MIDIEvent::schedule(ev, *this, std::move(out), EventSchedule::PROGRAM_TIME);
  }

  virtual bool load();
};


/*************************
 * Programming functions *
 *************************/

void VAPartial::program(uint8_t *buf, bool is_mt32) const
{
  buf[0]  = WGPitchCoarse;
  buf[1]  = WGPitchFine + 50;
  buf[2]  = WGPitchKeyfollow;
  buf[3]  = WGPitchBender;
  buf[4]  = ((WGPCMBank - 1) << 1) | WGWaveform;
  buf[5]  = WGPCMWave - 1;
  buf[6]  = WGPulseWidth;
  buf[7]  = WGPulseWidthVelocity + 7;
  buf[8]  = PEnvDepth;
  buf[9]  = PEnvVelocity;
  buf[10] = PEnvTimeKeyfollow;
  buf[11] = PEnvTime1;
  buf[12] = PEnvTime2;
  buf[13] = PEnvTime3;
  buf[14] = PEnvTime4;
  buf[15] = PEnvLevel0 + 50;
  buf[16] = PEnvLevel1 + 50;
  buf[17] = PEnvLevel2 + 50;
  buf[18] = PEnvSustainLevel + 50;
  buf[19] = PEnvEndLevel + 50;
  buf[20] = LFORate;
  buf[21] = LFODepth;
  buf[22] = LFOModulation;
  buf[23] = TVFCutoff;
  buf[24] = TVFResonance;
  buf[25] = std::min(TVFKeyfollow.value(), 14U);
  buf[26] = TVFBiasPoint;
  buf[27] = TVFBiasLevel + 7;
  buf[28] = TVFEnvDepth;
  buf[29] = TVFEnvVelocity;
  buf[30] = TVFEnvDepthKeyfollow;
  buf[31] = TVFEnvTimeKeyfollow;
  buf[32] = TVFEnvTime1;
  buf[33] = TVFEnvTime2;
  buf[34] = TVFEnvTime3;
  buf[35] = TVFEnvTime4;
  buf[36] = is_mt32 ? TVFEnvTime5 : TVFEnvTime4;
  buf[37] = TVFEnvLevel1;
  buf[38] = TVFEnvLevel2;
  buf[39] = TVFEnvLevel3;
  buf[40] = TVFEnvSustainLevel;
  buf[41] = TVALevel;
  buf[42] = TVAVelocity + 50;
  buf[43] = TVABiasPoint1;
  buf[44] = TVABiasLevel1 + 12;
  buf[45] = TVABiasPoint2;
  buf[46] = TVABiasLevel2 + 12;
  buf[47] = TVAEnvTimeKeyfollow;
  buf[48] = TVAEnvVelocity;
  buf[49] = TVAEnvTime1;
  buf[50] = TVAEnvTime2;
  buf[51] = TVAEnvTime3;
  buf[52] = TVAEnvTime4;
  buf[53] = is_mt32 ? TVAEnvTime5 : TVFEnvTime4;
  buf[54] = TVAEnvLevel1;
  buf[55] = TVAEnvLevel2;
  buf[56] = TVAEnvLevel3;
  buf[57] = TVAEnvSustainLevel;
}

void VATone::program(std::vector<uint8_t> &out, unsigned UnitID, unsigned part, bool is_mt32) const
{
  uint8_t buf[256];
  unsigned offset = 0xf6 * (part - 1);
  buf[0]  = 0xf0; /* SysEx */
  buf[1]  = 0x41; /* Roland */
  buf[2]  = UnitID - 1;
  buf[3]  = 0x16; /* Model ID */
  buf[4]  = 0x12; /* Command: Data set 1 */
  buf[5]  = 0x04; /* Address 040-00-00h + f6h * (part - 1) */
  buf[6]  = offset >> 7;
  buf[7]  = offset & 0x7f;

  for(int i = 0; i < 10; i++)
    buf[8 + i] = Name[i];

  buf[18] = Structure12;
  buf[19] = Structure34;
  buf[20] = (p1.Mute) | (p2.Mute << 1) | (p3.Mute << 2) | (p4.Mute << 3);
  buf[21] = Sustain ? 0 : 1;

  p1.program(buf + 22, is_mt32);
  p2.program(buf + 80, is_mt32);
  p3.program(buf + 138, is_mt32);
  p4.program(buf + 196, is_mt32);

  buf[254] = MIDIInterface::roland_checksum(buf + 5, 249);
  buf[255] = 0xf7; /* End SysEx */

  out.insert(out.end(), std::begin(buf), std::end(buf));
}

void VAPatch::program_patch(std::vector<uint8_t> &out, unsigned UnitID) const
{
  uint8_t buf[48];
  buf[0]  = 0xf0; /* SysEx */
  buf[1]  = 0x41; /* Roland */
  buf[2]  = UnitID - 1;
  buf[3]  = 0x16; /* Model ID */
  buf[4]  = 0x12; /* Command: Data set 1 */
  buf[5]  = 0x03; /* Address 030400h */
  buf[6]  = 0x04;
  buf[7]  = 0x00;
  buf[8]  = KeyMode;
  buf[9]  = SplitPoint;
  buf[10] = LowerTone.group();
  buf[11] = LowerTone.tone() - 1;
  buf[12] = UpperTone.group();
  buf[13] = UpperTone.tone() - 1;
  buf[14] = LowerKeyShift + 24;
  buf[15] = UpperKeyShift + 24;
  buf[16] = LowerFinetune + 50;
  buf[17] = UpperFinetune + 50;
  buf[18] = LowerBenderRange;
  buf[19] = UpperBenderRange;
  buf[20] = LowerAssignMode;
  buf[21] = UpperAssignMode;
  buf[22] = LowerReverb;
  buf[23] = UpperReverb;
  buf[24] = ReverbMode;
  buf[25] = ReverbTime;
  buf[26] = ReverbLevel;
  buf[27] = Balance;
  buf[28] = Level;
  for(int i = 0; i < 16; i++)
    buf[29 + i] = Name[i] & 0x7f;
  buf[45] = 0x00;
  buf[46] = MIDIInterface::roland_checksum(buf + 5, 41);
  buf[47] = 0xf7; /* End SysEx */

  out.insert(out.end(), std::begin(buf), std::end(buf));
}

void VAPatch::program_timbre(std::vector<uint8_t> &out, unsigned UnitID, unsigned part) const
{
  uint8_t buf[18];
  buf[0]  = 0xf0; /* SysEx */
  buf[1]  = 0x41; /* Roland */
  buf[2]  = UnitID - 1;
  buf[3]  = 0x16; /* Model ID */
  buf[4]  = 0x12; /* Command: Data set 1 */
  buf[5]  = 0x03; /* Address 03-00-00h + 10h * (part - 1) */
  buf[6]  = 0x00;
  buf[7]  = 0x10 * (part - 1);
  buf[8]  = UpperTone.group();
  buf[9]  = UpperTone.tone() - 1;
  buf[10] = UpperKeyShift + 24;
  buf[11] = UpperFinetune + 50;
  buf[12] = UpperBenderRange;
  buf[13] = UpperAssignMode;
  buf[14] = UpperReverb; // TODO: D-110 has "Output Assign" here instead.
  buf[15] = 0x00;
  buf[16] = MIDIInterface::roland_checksum(buf + 5, 11);
  buf[17] = 0xf7; /* End SysEx */

  out.insert(out.end(), std::begin(buf), std::end(buf));
}

void VAPatchFX::program(std::vector<uint8_t> &out, unsigned UnitID) const
{
  uint8_t buf[16];
  buf[0]  = 0xf0; /* SysEx */
  buf[1]  = 0x41; /* Roland */
  buf[2]  = UnitID - 1;
  buf[3]  = 0x16; /* Model ID */
  buf[4]  = 0x12; /* Command: Data set 1 */
  buf[5]  = 0x03; /* Address 03-04-40h */
  buf[6]  = 0x04;
  buf[7]  = 0x40;
  buf[8]  = Mode;
  buf[9]  = Rate;
  buf[10] = HarmonyBalance + 12;
  buf[11] = ChaseShift + 12;
  buf[12] = ChaseLevel;
  buf[13] = ArpeggioMode;
  buf[14] = MIDIInterface::roland_checksum(buf + 5, 9);
  buf[15] = 0xf7; /* End SysEx */

  out.insert(out.end(), std::begin(buf), std::end(buf));
}


/*********************
 * Loading functions *
 *********************/

void VAPartial::load(const uint8_t *buf, bool is_mt32)
{
  WGPitchCoarse         = buf[0];
  WGPitchFine           = (int)buf[1] - 50;
  WGPitchKeyfollow      = buf[2];
  WGPitchBender         = buf[3];
  WGPCMBank             = buf[4] >> 1;
  WGWaveform            = buf[4] & 1;
  WGPCMWave             = buf[5];
  WGPulseWidth          = buf[6];
  WGPulseWidthVelocity  = (int)buf[7] - 7;
  PEnvDepth             = buf[8];
  PEnvVelocity          = buf[9];
  PEnvTimeKeyfollow     = buf[10];
  PEnvTime1             = buf[11];
  PEnvTime1             = buf[12];
  PEnvTime1             = buf[13];
  PEnvTime1             = buf[14];
  PEnvLevel0            = (int)buf[15] - 50;
  PEnvLevel1            = (int)buf[16] - 50;
  PEnvLevel2            = (int)buf[17] - 50;
  PEnvSustainLevel      = (int)buf[18] - 50; // D-110 and MT-32
  PEnvEndLevel          = (int)buf[19] - 50;
  LFORate               = buf[20];
  LFODepth              = buf[21];
  LFOModulation         = buf[22];
  TVFCutoff             = buf[23];
  TVFResonance          = buf[24];
  TVFKeyfollow          = std::min((unsigned)buf[25], 14U);
  TVFBiasPoint          = buf[26];
  TVFBiasLevel          = (int)buf[27] - 7;
  TVFEnvDepth           = buf[28];
  TVFEnvVelocity        = buf[29];
  TVFEnvDepthKeyfollow  = buf[30];
  TVFEnvTimeKeyfollow   = buf[31];
  TVFEnvTime1           = buf[32];
  TVFEnvTime2           = buf[33];
  TVFEnvTime3           = buf[34];
  TVFEnvTime4           = is_mt32 ? buf[35] : buf[36];
  TVFEnvTime5           = buf[36];
  TVFEnvLevel1          = buf[37];
  TVFEnvLevel2          = buf[38];
  TVFEnvLevel3          = buf[39]; // D-110 and MT-32
  TVFEnvSustainLevel    = buf[40];
  TVALevel              = buf[41];
  TVAVelocity           = (int)buf[42] - 50;
  TVABiasPoint1         = buf[43];
  TVABiasLevel1         = (int)buf[44] - 12;
  TVABiasPoint2         = buf[45];
  TVABiasLevel2         = (int)buf[46] - 12;
  TVAEnvTimeKeyfollow   = buf[47];
  TVAEnvVelocity        = buf[48];
  TVAEnvTime1           = buf[49];
  TVAEnvTime2           = buf[50];
  TVAEnvTime3           = buf[51];
  TVAEnvTime4           = is_mt32 ? buf[52] : buf[53];
  TVAEnvTime5           = buf[53];
  TVAEnvLevel1          = buf[54];
  TVAEnvLevel2          = buf[55];
  TVAEnvLevel3          = buf[56];
  TVAEnvSustainLevel    = buf[57];
}

void VATone::load(const uint8_t *buf, bool is_mt32)
{
  char name[11];
  memcpy(name, buf + 0, 10);
  name[10] = '\0';
  Name = name;

  Structure12       = buf[10];
  Structure34       = buf[11];
  p1.Mute           = (buf[12] >> 0) & 1;
  p2.Mute           = (buf[12] >> 1) & 1;
  p3.Mute           = (buf[12] >> 2) & 1;
  p4.Mute           = (buf[12] >> 3) & 1;
  Sustain           = 1 - buf[13];

  p1.load(buf + 0x0e, is_mt32);
  p2.load(buf + 0x48, is_mt32);
  p3.load(buf + 0x82, is_mt32);
  p4.load(buf + 0xbc, is_mt32);
}

void VAPatch::load_patch(const uint8_t *buf)
{
  char name[17];

  LowerTone.group(buf[2]);
  LowerTone.tone(buf[3] + 1);
  UpperTone.group(buf[4]);
  UpperTone.tone(buf[5] + 1);

  KeyMode           = buf[0];
  SplitPoint        = buf[1];
  LowerKeyShift     = (int)buf[6] - 24;
  UpperKeyShift     = (int)buf[7] - 24;
  LowerFinetune     = (int)buf[8] - 50;
  UpperFinetune     = (int)buf[9] - 50;
  LowerBenderRange  = buf[10];
  UpperBenderRange  = buf[11];
  LowerAssignMode   = buf[12];
  UpperAssignMode   = buf[13];
  LowerReverb       = buf[14];
  UpperReverb       = buf[15];
  ReverbMode        = buf[16];
  ReverbTime        = buf[17];
  ReverbLevel       = buf[18];
  Balance           = buf[19];
  Level             = buf[20];

  memcpy(name, buf + 21, 16);
  name[16] = '\0';
  Name = name;
}

void VAPatch::load_timbre(const uint8_t *buf)
{
  UpperTone.group(buf[0]);
  UpperTone.tone(buf[1] + 1);

  UpperKeyShift     = (int)buf[2] - 24;
  UpperFinetune     = (int)buf[3] - 50;
  UpperBenderRange  = buf[4];
  UpperAssignMode   = buf[5];
  UpperReverb       = buf[6];
}

void VAPatchFX::load(const uint8_t *buf)
{
  Mode              = buf[0];
  Rate              = buf[1];
  HarmonyBalance    = (int)buf[2] - 12;
  ChaseShift        = (int)buf[3] - 12;
  ChaseLevel        = buf[4];
  ArpeggioMode      = buf[5];
}

/* std::vector and cohorts feel the need to individually initialize each byte... */
template<size_t START, size_t END, size_t SECTIONS=1>
class D5Memory
{
  static_assert(END > START, "wtf");
  static constexpr bool use_buf() { return END - START <= 256; }
  static constexpr size_t buf_size() { return use_buf() ? END - START : 1; }
  static constexpr size_t section_size() { return (END - START) / SECTIONS; }

  uint8_t *ptr;
  uint8_t buf[buf_size()]{};
  bool has_section[SECTIONS]{};

public:
  D5Memory()
  {
    if(!use_buf())
      ptr = reinterpret_cast<uint8_t *>(calloc(1, END - START));
    else
      ptr = buf;
  }
  ~D5Memory()
  {
    if(!use_buf())
      free(ptr);
  }

  uint8_t *get_ptr(size_t addr) { return ptr + addr - START; }
  uint8_t &operator[](size_t addr) { return ptr[addr - START]; }
  operator bool() const { return !!ptr; }

  void load(std::vector<uint8_t> &in, unsigned &inpos, unsigned &sum,
   unsigned &pos, unsigned offset = 0)
  {
    unsigned start = START + offset;
    unsigned end = END + offset;
    if(pos > end)
      return;

    /* Skip any bytes prior to start. */
    while(pos < start && inpos < in.size() && in[inpos] < 0x80)
    {
      sum += in[inpos++];
      pos++;
    }
    /* Read bytes up until end. */
    unsigned i = pos - start;
    while(pos < end && inpos < in.size() && in[inpos] < 0x80)
    {
      if(!(pos & 0xf) && SECTIONS > 1)
        has_section[(i / section_size()) % SECTIONS] = true;
      else
        has_section[0] = true;

      sum += in[inpos];
      ptr[i++] = in[inpos++];
      pos++;
    }
  }

  bool has(unsigned section = 0) const
  {
    return section < SECTIONS ? has_section[section] : false;
  }
};

bool D5Interface::load()
{
  std::vector<uint8_t> in;

  if(!SysExPath[0] || !MIDIInterface::load_file(in, SysExPath))
    return false;

  D5Memory<address(0x30000), address(0x30100),   8> timbre_temp;
  D5Memory<address(0x30400), address(0x30426),   1> patch_temp;
  D5Memory<address(0x30440), address(0x30446),   1> patchfx_temp;
  D5Memory<address(0x40000), address(0x40f30),   8> tone_temp;

  D5Memory<address(0x50000), address(0x50008),   1> timbre_mem; // Only load 1
  D5Memory<address(0x70000), address(0x70026),   1> patch_mem; // Only load 1
  D5Memory<address(0x80000), address(0x90000),  64> tone_mem;
  D5Memory<address(0xd0000), address(0xd0600), 128> patchfx_mem;

  if(!timbre_temp || !patch_temp || !patchfx_temp || !tone_temp ||
     !timbre_mem  || !patch_mem  || !patchfx_mem  || !tone_mem)
    return false;

  unsigned timbre_start = (SysExPatch - 1) * address(0x8);
  unsigned patch_start  = (SysExPatch - 1) * address(0x26);

  unsigned inpos = 0;

  /* Load SysEx to memory buffer. */
  while(inpos < in.size())
  {
    while(inpos < in.size() && in[inpos] != 0xf0)
      inpos++;

    if(inpos + 10 >= in.size())
      break;

    if(in[inpos + 1] != 0x41 || /* Roland*/
       in[inpos + 3] != 0x16 || /* Model ID */
       in[inpos + 4] != 0x12)   /* Command: Set data 1 */
      continue;

    unsigned pos = address(in[inpos + 5], in[inpos + 6], in[inpos + 7]);
    unsigned sum = in[inpos + 5] + in[inpos + 6] + in[inpos + 7];
    inpos += 8;

    /* Load memory ranges of interest to their respective buffers.
     * These need to be loaded in order. Unrelated bytes are ignored.
     */
    timbre_temp.load(in, inpos, sum, pos);  /* 0x30000 */
    patch_temp.load(in, inpos, sum, pos);   /* 0x30400 */
    patchfx_temp.load(in, inpos, sum, pos); /* 0x30440 */
    tone_temp.load(in, inpos, sum, pos);    /* 0x40000 */
    timbre_mem.load(in, inpos, sum, pos, timbre_start);
    patch_mem.load(in, inpos, sum, pos, patch_start);
    tone_mem.load(in, inpos, sum, pos);     /* 0x80000 */
    patchfx_mem.load(in, inpos, sum, pos);  /* 0xd0000 */

    /* Skip any remaining bytes and check sum. */
    while(inpos < in.size() && in[inpos] < 0x80)
      sum += in[inpos++];

    if((sum & 0x7f) || inpos >= in.size() || in[inpos] != 0xf7)
      return false;
  }

  /* Load patch information from memory. */
  // FIXME: check temp timbres
  // FIXME: check temp patch
  if(SysExMode) // Load Timbre from bulk dump
  {
    //
  }
  else // Load Patch from bulk dump
  {
    //
  }

  return true;
}

static class D5Register : public ConfigRegister
{
public:
  D5Register(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new D5Interface(ctx, tag, id));
  }
} reg5("D-5"),
  reg10("D-10"),
  reg20("D-20"),
  reg110("D-110"),
  regmt32("MT-32");

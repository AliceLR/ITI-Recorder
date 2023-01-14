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

class DX7Operator : public ConfigSubinterface
{
public:
  static constexpr EnumValue Modes[] =
  {
    { "ratio", 0 },
    { "fixed", 1 },
    { }
  };
  static constexpr EnumValue KSLCurve[] =
  {
    { "-LIN", 0 },
    { "-LINEAR", 0 },
    { "-LN", 0 },
    { "-EXP", 1 },
    { "-EX", 1 },
    { "+EXP", 2 },
    { "+EX", 2 },
    { "+LIN", 3 },
    { "+LINEAR", 3 },
    { "+LN", 3 },
    { }
  };
  OptionBool        Enable;
  Option<unsigned>  Level;
  Enum<Modes>       Mode;             // 0=ratio 1=fixed
  Option<unsigned>  Coarse;           // 0-31
  Option<unsigned>  Fine;
  Option<int>       Detune;           // -7 to 7 (SysEx: 0-14)
  Option<unsigned>  EGRate1;
  Option<unsigned>  EGRate2;
  Option<unsigned>  EGRate3;
  Option<unsigned>  EGRate4;
  Option<unsigned>  EGLevel1;
  Option<unsigned>  EGLevel2;
  Option<unsigned>  EGLevel3;
  Option<unsigned>  EGLevel4;
  OptionNote        KSLBreakPoint;    // A-1 = 0h, C3 = 27h, C8 = 63h (99)
  Option<unsigned>  KSLLeftDepth;
  Option<unsigned>  KSLRightDepth;
  Enum<KSLCurve>    KSLLeftCurve;     // 0=-linear 1=-exp 2=+exp 3=+linear
  Enum<KSLCurve>    KSLRightCurve;
  Option<unsigned>  RateScaling;      // 0-7
  Option<unsigned>  ModulationLevel;  // 0-3
  Option<unsigned>  KeyVelocityLevel; // 0-7

  DX7Operator(std::vector<ConfigSubinterface *> &v, const char *_name):
   ConfigSubinterface(v, _name),
   Enable(options, true, "Enable"),
   Level(options, 99, 0, 99, "Level"),
   Mode(options, "ratio", "Mode"),
   Coarse(options, 1, 0, 31, "Coarse"),
   Fine(options, 0, 0, 99, "Fine"),
   Detune(options, 0, -7, 7, "Detune"),
   EGRate1(options, 99, 0, 99, "EGRate1"),
   EGRate2(options, 99, 0, 99, "EGRate2"),
   EGRate3(options, 99, 0, 99, "EGRate3"),
   EGRate4(options, 99, 0, 99, "EGRate4"),
   EGLevel1(options, 99, 0, 99, "EGLevel1"),
   EGLevel2(options, 99, 0, 99, "EGLevel2"),
   EGLevel3(options, 99, 0, 99, "EGLevel3"),
   EGLevel4(options, 0, 0, 99, "EGLevel4"),
   KSLBreakPoint(options, "C3", "A-1", "C8", 0, "KSLBreakPoint"),
   KSLLeftDepth(options, 0, 0, 99, "KSLLeftDepth"),
   KSLRightDepth(options, 0, 0, 99, "KSLRightDepth"),
   KSLLeftCurve(options, "-LIN", "KSLLeftCurve"),
   KSLRightCurve(options, "-LIN", "KSLRightCurve"),
   RateScaling(options, 0, 0, 7, "RateScaling"),
   ModulationLevel(options, 0, 0, 3, "ModulationLevel"),
   KeyVelocityLevel(options, 0, 0, 7, "KeyVelocityLevel")
  {}

  void program(std::vector<uint8_t> &out) const;
  void load(const uint8_t *in);
  void load_packed(const uint8_t *in);
};

class DX7PitchEG : public ConfigSubinterface
{
public:
  Option<unsigned>  EGRate1;
  Option<unsigned>  EGRate2;
  Option<unsigned>  EGRate3;
  Option<unsigned>  EGRate4;
  Option<unsigned>  EGLevel1;
  Option<unsigned>  EGLevel2;
  Option<unsigned>  EGLevel3;
  Option<unsigned>  EGLevel4;

 DX7PitchEG(std::vector<ConfigSubinterface *> &v, const char *_name):
   ConfigSubinterface(v, _name),
   EGRate1(options, 99, 0, 99, "EGRate1"),
   EGRate2(options, 99, 0, 99, "EGRate2"),
   EGRate3(options, 99, 0, 99, "EGRate3"),
   EGRate4(options, 99, 0, 99, "EGRate4"),
   EGLevel1(options, 50, 0, 99, "EGLevel1"),
   EGLevel2(options, 50, 0, 99, "EGLevel2"),
   EGLevel3(options, 50, 0, 99, "EGLevel3"),
   EGLevel4(options, 50, 0, 99, "EGLevel4")
  {}

  void program(std::vector<uint8_t> &out) const;
  void load(const uint8_t *in);
};

class DX7Interface final : public MIDIInterface
{
public:
  static constexpr EnumValue Waveforms[] =
  {
    { "TR", 0 },
    { "tri", 0 },
    { "triangle", 0 },
    { "SD", 1 },
    { "sawdown", 1 },
    { "rampdown", 1 },
    { "SU", 2 },
    { "saw", 2 },
    { "sawup", 2 },
    { "rampup", 2 },
    { "SQ", 3 },
    { "square", 3 },
    { "SI", 4 },
    { "sin", 4 },
    { "sine", 4 },
    { "SH", 5 },
    { "s&hold", 5 },
    { }
  };

  DX7Operator       ops[6];
  DX7PitchEG        pitcheg;

  OptionString<512> SysExPath;        // Optional
  Option<unsigned>  SysExPatch;       // Optional
  OptionString<10>  Name;
  Option<unsigned>  Algorithm;        // 1-32 (SysEx: 0-31)
  Option<unsigned>  Feedback;         // 0-7
  OptionBool        OscillatorSync;
  Option<unsigned>  LFOSpeed;
  Option<unsigned>  LFODelay;
  Option<unsigned>  LFOPitchModDepth;
  Option<unsigned>  LFOAmpModDepth;
  OptionBool        LFOSync;
  Enum<Waveforms>   LFOWaveform;      // 0=tri 1=rampdown 2=rampup 3=sqr 4=sin 5="sh"?
  Option<unsigned>  PitchModLevel;    // 0-7
  Option<int>       Transpose;        // -24 to 24 (SysEx: 0-48)

  // FIXME: function parameters

  DX7Interface(ConfigContext &_ctx, const char *_tag, int _id):
   MIDIInterface(_ctx, _tag, _id),
   ops{ { subs, "operator1" },
        { subs, "operator2" },
        { subs, "operator3" },
        { subs, "operator4" },
        { subs, "operator5" },
        { subs, "operator6" }},

   pitcheg(subs, "pitch"),

   /* options */
   SysExPath(options, "", "SysExPath"),
   SysExPatch(options, 1, 1, 32, "SysExPatch"),
   Name(options, "<default>", "Name"),
   Algorithm(options, 1, 1, 32, "Algorithm"),
   Feedback(options, 0, 0, 7, "Feedback"),
   OscillatorSync(options, true, "OscillatorSync"),
   LFOSpeed(options, 35, 0, 99, "LFOSpeed"),
   LFODelay(options, 0, 0, 99, "LFODelay"),
   LFOPitchModDepth(options, 0, 0, 99, "LFOPitchModDepth"),
   LFOAmpModDepth(options, 0, 0, 99, "LFOAmpModDepth"),
   LFOSync(options, true, "LFOSync"),
   LFOWaveform(options, "TR", "LFOWaveform"),
   PitchModLevel(options, 3, 0, 7, "PitchModLevel"),
   Transpose(options, 0, -24, 24, "Transpose")
  {}

  virtual ~DX7Interface() {}

  virtual bool load();
  void load_voice(const uint8_t *buf);
  void load_voice_packed(const uint8_t *buf);
  void load_param(unsigned param, unsigned value);

  void param(std::vector<uint8_t> &out, unsigned num, unsigned v) const
  {
    const InputConfig *in = get_input_config();
    uint8_t buf[7];

    buf[0] = 0xf0;
    buf[1] = 0x43; // Yamaha
    buf[2] = 0x10 | (in ? in->midi_channel - 1 : 0);
    buf[3] = (num >> 7);   // Voice
    buf[4] = (num & 0x7f); // Voice
    buf[5] = v & 0x7f;
    buf[6] = 0xf7;

    out.insert(out.end(), std::begin(buf), std::end(buf));
  }

  void program_op(std::vector<uint8_t> &out, const DX7Operator &op) const
  {
    uint8_t buf[21];

    buf[0]  = op.EGRate1;
    buf[1]  = op.EGRate2;
    buf[2]  = op.EGRate3;
    buf[3]  = op.EGRate4;
    buf[4]  = op.EGLevel1;
    buf[5]  = op.EGLevel2;
    buf[6]  = op.EGLevel3;
    buf[7]  = op.EGLevel4;
    buf[8]  = op.KSLBreakPoint;
    buf[9]  = op.KSLLeftDepth;
    buf[10] = op.KSLRightDepth;
    buf[11] = op.KSLLeftCurve;
    buf[12] = op.KSLRightCurve;
    buf[13] = op.RateScaling;
    buf[14] = op.ModulationLevel;
    buf[15] = op.KeyVelocityLevel;
    buf[16] = op.Level;
    buf[17] = op.Mode ? 1 : 0;
    buf[18] = op.Coarse;
    buf[19] = op.Fine;
    buf[20] = op.Detune + 7;

    out.insert(out.end(), std::begin(buf), std::end(buf));
  }

  virtual void program(EventSchedule &ev) const
  {
    const InputConfig *in = get_input_config();
    std::vector<uint8_t> out;

    /* Programmable parameters. */
    uint8_t buf[29];

    out.resize(6);
    out[0] = 0xf0;
    out[1] = 0x43;
    out[2] = 0x00 | (in ? in->midi_channel - 1 : 0);
    out[3] = 0x00;
    out[4] = (155 >> 7);
    out[5] = (155 & 0x7f);

    for(int i = 5; i >= 0; i--)
      program_op(out, ops[i]);

    buf[0]  = pitcheg.EGRate1;
    buf[1]  = pitcheg.EGRate2;
    buf[2]  = pitcheg.EGRate3;
    buf[3]  = pitcheg.EGRate4;
    buf[4]  = pitcheg.EGLevel1;
    buf[5]  = pitcheg.EGLevel2;
    buf[6]  = pitcheg.EGLevel3;
    buf[7]  = pitcheg.EGLevel4;
    buf[8]  = Algorithm - 1;
    buf[9]  = Feedback;
    buf[10] = OscillatorSync;
    buf[11] = LFOSpeed;
    buf[12] = LFODelay;
    buf[13] = LFOPitchModDepth;
    buf[14] = LFOAmpModDepth;
    buf[15] = LFOSync;
    buf[16] = LFOWaveform;
    buf[17] = PitchModLevel;
    buf[18] = Transpose + 24;

    for(int i = 0; i < 10; i++)
      buf[19 + i] = Name[i] & 0x7f;

    out.insert(out.end(), std::begin(buf), std::end(buf));

    unsigned sum = 0;
    for(size_t i = 0; i < 155; i++)
      sum += out[i + 6];

    out.push_back((-sum) & 0x7f);
    out.push_back(0xf7);

    // Set operator enable flags.
    unsigned enable = 0;
    for(int i = 0; i < 6; i++)
      if(ops[i].Enable)
        enable |= 0x20 >> i;

    param(out, 155, enable);

    /* FIXME: function parameters */

    MIDIEvent::schedule(ev, *this, std::move(out), EventSchedule::PROGRAM_TIME);
  }
};


/*********************
 * Loading functions *
 *********************/

void DX7Operator::load(const uint8_t *buf)
{
  EGRate1           = buf[0];
  EGRate2           = buf[1];
  EGRate3           = buf[2];
  EGRate4           = buf[3];
  EGLevel1          = buf[4];
  EGLevel2          = buf[5];
  EGLevel3          = buf[6];
  EGLevel4          = buf[7];
  KSLBreakPoint     = buf[8];
  KSLLeftDepth      = buf[9];
  KSLRightDepth     = buf[10];
  KSLLeftCurve      = buf[11];
  KSLRightCurve     = buf[12];
  RateScaling       = buf[13];
  ModulationLevel   = buf[14];
  KeyVelocityLevel  = buf[15];
  Level             = buf[16];
  Mode              = buf[17];
  Coarse            = buf[18];
  Fine              = buf[19];
  Detune            = (int)buf[20] - 7;
}

void DX7Operator::load_packed(const uint8_t *buf)
{
  EGRate1           = buf[0];
  EGRate2           = buf[1];
  EGRate3           = buf[2];
  EGRate4           = buf[3];
  EGLevel1          = buf[4];
  EGLevel2          = buf[5];
  EGLevel3          = buf[6];
  EGLevel4          = buf[7];
  KSLBreakPoint     = buf[8];
  KSLLeftDepth      = buf[9];
  KSLRightDepth     = buf[10];
  KSLLeftCurve      = (buf[11] >> 0) & 0x3;
  KSLRightCurve     = (buf[11] >> 2) & 0x3;
  RateScaling       = (buf[12] >> 0) & 0x7;
  ModulationLevel   = (buf[13] >> 0) & 0x3;
  KeyVelocityLevel  = (buf[13] >> 2) & 0x7;
  Level             = buf[14];
  Mode              = (buf[15] >> 0) & 0x1;
  Coarse            = (buf[15] >> 1) & 0x1f;
  Fine              = buf[16];
  Detune            = (int)(buf[12] >> 3) - 7;
}

void DX7PitchEG::load(const uint8_t *buf)
{
  EGRate1           = buf[0];
  EGRate2           = buf[1];
  EGRate3           = buf[2];
  EGRate4           = buf[3];
  EGLevel1          = buf[4];
  EGLevel2          = buf[5];
  EGLevel3          = buf[6];
  EGLevel4          = buf[7];
}

void DX7Interface::load_voice(const uint8_t *buf)
{
  char name[11];

  for(int i = 5; i >= 0; i--)
  {
    ops[i].load(buf);
    buf += 21;
  }
  pitcheg.load(buf);
  buf += 8;

  Algorithm         = buf[0] + 1;
  Feedback          = buf[1];
  OscillatorSync    = buf[2];
  LFOSpeed          = buf[3];
  LFODelay          = buf[4];
  LFOPitchModDepth  = buf[5];
  LFOAmpModDepth    = buf[6];
  LFOSync           = buf[7];
  LFOWaveform       = buf[8];
  PitchModLevel     = buf[9];
  Transpose         = (int)buf[10] - 24;

  memcpy(name, buf + 11, 10);
  name[10] = '\0';
  Name = name;
}

void DX7Interface::load_voice_packed(const uint8_t *buf)
{
  char name[11];

  for(int i = 5; i >= 0; i--)
  {
    ops[i].load_packed(buf);
    buf += 17;
  }
  pitcheg.load(buf);
  buf += 8;

  Algorithm         = buf[0] + 1;
  Feedback          = (buf[1] >> 0) & 0x7;
  OscillatorSync    = (buf[1] >> 3) & 0x1;
  LFOSpeed          = buf[2];
  LFODelay          = buf[3];
  LFOPitchModDepth  = buf[4];
  LFOAmpModDepth    = buf[5];
  LFOSync           = (buf[6] >> 0) & 0x1;
  LFOWaveform       = (buf[6] >> 1) & 0x7;
  PitchModLevel     = (buf[6] >> 4) & 0x7;
  Transpose         = (int)buf[7] - 24;

  memcpy(name, buf + 8, 10);
  name[10] = '\0';
  Name = name;
}

void DX7Interface::load_param(unsigned param, unsigned value)
{
  DX7Operator &op = (param < 126) ? ops[param / 21] : ops[0];
  if(param < 126)
    param %= 21;

  switch(param)
  {
  /* Operators */
  case   0: op.EGRate1 = value; break;
  case   1: op.EGRate2 = value; break;
  case   2: op.EGRate3 = value; break;
  case   3: op.EGRate4 = value; break;
  case   4: op.EGLevel1 = value; break;
  case   5: op.EGLevel2 = value; break;
  case   6: op.EGLevel3 = value; break;
  case   7: op.EGLevel4 = value; break;
  case   8: op.KSLBreakPoint = value; break;
  case   9: op.KSLLeftDepth = value; break;
  case  10: op.KSLRightDepth = value; break;
  case  11: op.KSLLeftCurve = value; break;
  case  12: op.KSLRightCurve = value; break;
  case  13: op.RateScaling = value; break;
  case  14: op.ModulationLevel = value; break;
  case  15: op.KeyVelocityLevel = value; break;
  case  16: op.Level = value; break;
  case  17: op.Mode = value; break;
  case  18: op.Coarse = value; break;
  case  19: op.Fine = value; break;
  case  20: op.Detune = (int)value - 7; break;
  /* Pitch EG */
  case 126: pitcheg.EGRate1 = value; break;
  case 127: pitcheg.EGRate2 = value; break;
  case 128: pitcheg.EGRate3 = value; break;
  case 129: pitcheg.EGRate4 = value; break;
  case 130: pitcheg.EGLevel1 = value; break;
  case 131: pitcheg.EGLevel2 = value; break;
  case 132: pitcheg.EGLevel3 = value; break;
  case 133: pitcheg.EGLevel4 = value; break;
  /* Patch */
  case 134: Algorithm = value + 1; break;
  case 135: Feedback = value; break;
  case 136: OscillatorSync = value; break;
  case 137: LFOSpeed = value; break;
  case 138: LFODelay = value; break;
  case 139: LFOPitchModDepth = value; break;
  case 140: LFOAmpModDepth = value; break;
  case 141: LFOSync = value; break;
  case 142: LFOWaveform = value; break;
  case 143: PitchModLevel = value; break;
  case 144: Transpose = (int)value - 24; break;
  case 145: Name[0] = value; break;
  case 146: Name[1] = value; break;
  case 147: Name[2] = value; break;
  case 148: Name[3] = value; break;
  case 149: Name[4] = value; break;
  case 150: Name[5] = value; break;
  case 151: Name[6] = value; break;
  case 152: Name[7] = value; break;
  case 153: Name[8] = value; break;
  case 154: Name[9] = value; break;

  /* Voice enable */
  case 155:
    for(int i = 0; i < 6; i++)
      ops[i].Enable = (value >> (5 - i)) & 1;
    break;

  // FIXME: function parameters
  };
}

bool DX7Interface::load()
{
  std::vector<uint8_t> in;
  uint8_t buf[4097];

  if(!SysExPath[0] || !MIDIInterface::load_file(in, SysExPath))
    return false;

  unsigned inpos = 0;
  unsigned stop = in.size() - 7;
  while(inpos < stop)
  {
    if(in[inpos++] != 0xf0)
      continue;
    if(in[inpos++] != 0x43)
      continue;

    uint8_t sub  = in[inpos + 0];
    uint8_t type = in[inpos + 1];
    uint8_t hi   = in[inpos + 2];
    uint8_t lo   = in[inpos + 3];

    if(sub >= 0x80 || type >= 0x80 || hi >= 0x80 || lo >= 0x80)
      continue;

    inpos += 4;

    if((sub & 0x70) == 1)
    {
      /* Parameter change */
      if(in[inpos++] != 0xf7)
        return false;

      load_param((type << 7) | hi, lo);
      continue;
    }
    else
    /* Bulk data */
    if((sub & 0x70) != 0)
      return false;

    unsigned sum = 0;
    unsigned count = 0;
    unsigned sz = (hi << 7) | lo;
    bool read = true;

    type |= (sub & 0x70) << 4;
    switch(type)
    {
    case 0x00:  /* Single voice */
      if(sz != 155)
        return false;
      break;

    case 0x09:  /* 32-voice bank */
      if(sz != 4096)
        return false;
      break;

    default:
      read = false;
      break;
    }

    if(read)
    {
      /* Read sz plus one extra byte into buf (checksum) */
      while(inpos < in.size() && in[inpos] < 0x80 && count <= sz)
      {
        sum += in[inpos];
        buf[count++] = in[inpos++];
      }
    }
    else
    {
      while(inpos < in.size() && in[inpos] < 0x80)
      {
        sum += in[inpos++];
        count++;
      }
    }
    if(inpos >= in.size() || in[inpos] != 0xf7 || count != sz + 1 || (sum & 0x7f))
      return false;
    inpos++;

    switch(type)
    {
    case 0x000:  /* Single voice */
      load_voice(buf);
      return true;

    case 0x009:  /* 32-voice bank */
      unsigned offset = (SysExPatch - 1) * 128;
      load_voice_packed(buf + offset);
      return true;
    }
  }
  return false;
}


static class DX7Register : public ConfigRegister
{
public:
  DX7Register(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new DX7Interface(ctx, tag, id));
  }
} reg("DX7");

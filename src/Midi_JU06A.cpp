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

#include "Config.hpp"
#include "Midi.hpp"

namespace CC
{
  enum
  {
    LFORate         = 0x03,
    PortamentoTime  = 0x05,
    LFODelay        = 0x09,
    DCORange        = 0x0c,
    DCOLFOLevel     = 0x0d,
    DCOPWMLevel     = 0x0e,
    DCOPWMSource    = 0x0f,
    DCOPW           = 0x10,
    DCOSaw          = 0x11,
    DCOSubLevel     = 0x12,
    DCONoiseLevel   = 0x13,
    HPFCutoff       = 0x14,
    VCFEnvPolarity  = 0x15,
    VCFEnvLevel     = 0x16,
    VCFLFOLevel     = 0x17,
    VCFKeyLevel     = 0x18,
    VCAEnv          = 0x19,
    VCALevel        = 0x1a,
    EnvSustain      = 0x1b,
    DCOSub          = 0x1c,
    LFOWaveform     = 0x1d,
    LFOKeyTrigger   = 0x1e,
    Hold            = 0x40,
    Portamento      = 0x41,
    VCFResonance    = 0x47,
    EnvRelease      = 0x48,
    EnvAttack       = 0x49,
    VCFCutoff       = 0x4a,
    EnvDecay        = 0x4b,
    DelayTime       = 0x52,
    DelayFeedback   = 0x53,
    AssignMode      = 0x56,
    BendRange       = 0x57,
    TempoSync       = 0x58,
    Delay           = 0x59,
    DelayLevel      = 0x5b,
    Chorus          = 0x5d,
  };
};

class JU06AInterface final : public MIDIInterface
{
public:
  static constexpr EnumValue Modes[] =
  {
    { "60", 60 },
    { "106", 106 },
    { }
  };
  static constexpr EnumValue Waveforms[] =
  {
    { "TRI", 0 },
    { "triangle", 0 },
    { "SQR", 1 },
    { "square", 1 },
    { "SA1", 2 },
    { "saw1", 2 },
    { "sawup", 2 },
    { "SA2", 3 },
    { "saw2", 3 },
    { "sawdown", 3 },
    { "SIN", 4 },
    { "sine", 4 },
    { "RD1", 5 },
    { "rnd1", 5 },
    { "random1", 5 },
    { "RD2", 6 },
    { "rnd2", 6 },
    { "random2", 6 },
    { }
  };
  static constexpr EnumValue Ranges[] =
  {
    { "16", 0 },
    { "8", 1 },
    { "4", 2 },
    { }
  };
  static constexpr EnumValue Polarities[] =
  {
    { "pos", 1 },
    { "positive", 1 },
    { "neg", 0 },
    { "negative", 0 },
    { }
  };
  static constexpr EnumValue AssignModes[] =
  {
    { "poly", 0 },
    { "solo", 2 },
    { "unison", 3 },
    { }
  };
  static constexpr EnumValue ChorusModes[] =
  {
    { "off", 0 },
    { "0", 0 },
    { "I", 1 },
    { "1", 1 },
    { "II", 2 },
    { "2", 2 },
    { "I+II", 3 },
    { "3", 3 },
    { }
  };
  OptionString<512> SysExPath;      // optional
  OptionBool        SendSysEx;      // on: send SysEx  off: send MIDI CC
  Enum<Modes>       JunoMode;       // 60 or 106; requires manual switch
  OptionString<16>  Name;           // undocumented

  /* Note: all values in the range 0-255 are HALVED when sent as MIDI CC. */
  Option<unsigned>  LFORate;        // 0-255
  Option<unsigned>  LFODelay;       // 0-255
  Enum<Waveforms>   LFOWaveform;    // tri sqr sa1 sa2 sin rd1 rd2
  OptionBool        LFOKeyTrigger;

  Enum<Ranges>      DCORange;       // 4 8 16
  Option<unsigned>  DCOLFOLevel;    // 0-255
  Option<unsigned>  DCOPWMLevel;    // 0-255
  Option<unsigned>  DCOPWMSource;   // 0=manual 1=LFO 2=envelope
  OptionBool        DCOPW;
  OptionBool        DCOSaw;
  OptionBool        DCOSub;
  Option<unsigned>  DCOSubLevel;    // 0-255
  Option<unsigned>  DCONoiseLevel;  // 0-255

  Option<unsigned>  HPFCutoff;      // 0-255
  Option<unsigned>  VCFCutoff;      // 0-255
  Option<unsigned>  VCFResonance;   // 0-255
  Enum<Polarities>  VCFEnvPolarity; // pos or neg
  Option<unsigned>  VCFEnvLevel;    // 0-255
  Option<unsigned>  VCFLFOLevel;    // 0-255
  Option<unsigned>  VCFKeyLevel;    // 0-255

  OptionBool        VCAEnv;
  Option<unsigned>  VCALevel;       // 0-255

  Option<unsigned>  EnvAttack;      // 0-255
  Option<unsigned>  EnvDecay;       // 0-255
  Option<unsigned>  EnvSustain;     // 0-255
  Option<unsigned>  EnvRelease;     // 0-255

  Enum<AssignModes> AssignMode;     // 0=poly 2=solo 3=unison
  Enum<ChorusModes> Chorus;         // 0=off 1=I 2=II 3=I+II
  OptionBool        Delay;
  Option<unsigned>  DelayTime;      // 0-15
  Option<unsigned>  DelayLevel;     // 0-15
  Option<unsigned>  DelayFeedback;  // 0-15
  OptionBool        Hold;
  OptionBool        Portamento;
  Option<unsigned>  PortamentoTime; // 0-255
  OptionBool        TempoSync;      // synchronizes LFO to tempo... kind of useless here
  Option<unsigned>  BendRange;      // 0-24

  JU06AInterface(ConfigContext &_ctx, const char *_tag, int _id):
   MIDIInterface(_ctx, _tag, _id),
   /* Options */
   SysExPath(options, "", "SysExPath"),
   SendSysEx(options, true, "SendSysEx"),
   JunoMode(options, "60", "JunoMode"),
   Name(options, "<default>", "Name"),
   LFORate(options, 0, 0, 255, "LFORate"),
   LFODelay(options, 0, 0, 255, "LFODelay"),
   LFOWaveform(options, "tri", "LFOWaveform"),
   LFOKeyTrigger(options, false, "LFOKeyTrigger"),
   DCORange(options, "8", "DCORange"),
   DCOLFOLevel(options, 0, 0, 255, "DCOLFOLevel"),
   DCOPWMLevel(options, 0, 0, 255, "DCOPWMLevel"),
   DCOPWMSource(options, 0, 0, 2, "DCOPWMSource"),
   DCOPW(options, true, "DCOPW"),
   DCOSaw(options, false, "DCOSaw"),
   DCOSub(options, false, "DCOSub"),
   DCOSubLevel(options, 0, 0, 255, "DCOSubLevel"),
   DCONoiseLevel(options, 0, 0, 255, "DCONoiseLevel"),
   HPFCutoff(options, 0, 0, 255, "HPFFrequency"),
   VCFCutoff(options, 0, 0, 255, "VCFFrequency"),
   VCFResonance(options, 0, 0, 255, "VCFResonance"),
   VCFEnvPolarity(options, "pos", "VCFEnvPolarity"),
   VCFEnvLevel(options, 0, 0, 255, "VCFEnvLevel"),
   VCFLFOLevel(options, 0, 0, 255, "VCFLFOLevel"),
   VCFKeyLevel(options, 0, 0, 255, "VCFKeyLevel"),
   VCAEnv(options, true, "VCAEnv"),
   VCALevel(options, 255, 0, 255, "VCALevel"),
   EnvAttack(options, 0, 0, 255, "EnvAttack"),
   EnvDecay(options, 127, 0, 255, "EnvDecay"),
   EnvSustain(options, 190, 0, 255, "EnvSustain"),
   EnvRelease(options, 94, 0, 255, "EnvRelease"),
   AssignMode(options, "poly", "AssignMode"),
   Chorus(options, "off", "Chorus"),
   Delay(options, false, "Delay"),
   DelayTime(options, 11, 0, 15, "DelayTime"),
   DelayLevel(options, 8, 0, 15, "DelayLevel"),
   DelayFeedback(options, 8, 0, 15, "DelayFeedback"),
   Hold(options, false, "Hold"),
   Portamento(options, false, "Portamento"),
   PortamentoTime(options, 100, 0, 255, "PortamentoTime"),
   TempoSync(options, false, "TempoSync"),
   BendRange(options, 2, 0, 24, "BendRange")
  {}

  virtual ~JU06AInterface() {}

  /**
   * JU-06A memory map:
   * Every field is 2 nibbles. Sections have unusual mappings, e.g.
   * xx 0A 00 is 9 fields long and then immediately continues into xx 10 00.
   * Boundaries between sections should probably not be relied upon.
   *
   *              Len   00          02          04            06
   * 03 00 06 00   7w   LFORate     LFODelay    LFOWaveform   LFOKeyTrigger
   *          08        0           0           0
   * 03 00 07 00  13w   DCORange    DCOLFOLevel DCOPWMLevel   DCOPWMSource
   *          08        DCOPW       DCOSaw      DCOSubLevel   DCONoiseLevel
   *          10        DCOSub      0           0             0
   *          18        0
   * 03 00 08 00  12w   HPFCutoff   VCFCutoff   VCFResonance  VCFEnvPolarity
   *          08        VCFEnvLevel VCFLFOLevel VCFKeyLevel   0
   *          10        0           0           0             0
   * 03 00 09 00   7w   VCAEnv      VCALevel    0             0
   *          08        0           0           0
   * 03 00 0A 00   9w   EnvAttack   EnvDecay    EnvSustain    EnvRelease
   *          08        0           0           0             0
   *          10        0
   * 03 00 10 00  10w   Chorus      DelayLevel  DelayTime     DelayFeedback
   *          08        ?           Delay       0             0
   *          10        0           0
   * 03 00 11 00  11w   Portamento  PortaTime   ?             AssignMode
   *          08        BendRange   TempoSync   0             0
   *          10        0           0           0
   * 03 00 13 00  16b   Patch name - ASCII, single byte per character, 20h padded.
   *       14-17 are all 20h padding. 16 bytes into 17h looks like the start of
   *       a patch, but it's not clear if there's a way to access it.
   */

  template<class V>
  void sysex_write(uint8_t *buf, V value) const
  {
    buf[0] = (value >> 4) & 0xf;
    buf[1] = (value >> 0) & 0xf;
  }

  template<class V, class... REST>
  void sysex_write(uint8_t *buf, V value, REST... values) const
  {
    sysex_write<V>(buf, value);
    sysex_write<REST...>(buf + 2, values...);
  }

  /* Note: addr is encoded with bit 7 as padding.
   * Note: a certain blog post claims that the entire message minus the SysEx
   * control codes is part of the checksum, but this worked only because the
   * original JU-06 bytes 1 through 7 add to 0x80 (JU-06 model is 00 00 00 1D).
   * The actual calculation involves bytes 8 through (end - 2) only.
   */
  template<class... REST>
  void sysex(std::vector<uint8_t> &out, unsigned addr, REST... values) const
  {
    constexpr unsigned len = 2 * sizeof...(values);
    uint8_t buf[len + 14];
    buf[0]  = 0xf0; /* SysEx */
    buf[1]  = 0x41; /* Roland */
    buf[2]  = 0x10; /* Device number */
    buf[3]  = 0x00; /* Model ID (4): Boutique JU-06A */
    buf[4]  = 0x00;
    buf[5]  = 0x00;
    buf[6]  = 0x62;
    buf[7]  = 0x12; /* Command: set parameter */
    buf[8]  = 0x03; /* Address (4) */
    buf[9]  = 0x00;
    buf[10] = addr >> 8;
    buf[11] = addr & 0x7f;

    sysex_write<REST...>(buf + 12, values...);

    buf[len + 12] = MIDIInterface::roland_checksum(buf + 8, len + 4);
    buf[len + 13] = 0xf7; /* End SysEx */

    out.insert(out.end(), std::begin(buf), std::end(buf));
  }

  /* Special case for the name string... */
  template<size_t N>
  void sysex_name(std::vector<uint8_t> &out, unsigned addr, const OptionString<N> &s) const
  {
    uint8_t buf[N + 14];
    buf[0]  = 0xf0; /* SysEx */
    buf[1]  = 0x41; /* Roland */
    buf[2]  = 0x10; /* Device Number */
    buf[3]  = 0x00; /* Model ID (4): Boutique JU-06A */
    buf[4]  = 0x00;
    buf[5]  = 0x00;
    buf[6]  = 0x62;
    buf[7]  = 0x12; /* Command: set parameter */
    buf[8]  = 0x03; /* Address (4) */
    buf[9]  = 0x00;
    buf[10] = addr >> 8;
    buf[11] = addr & 0x7f;

    memcpy(buf + 12, s.value(), N);

    buf[N + 12] = MIDIInterface::roland_checksum(buf + 8, N + 4);
    buf[N + 13] = 0xf7; /* End SysEx */

    out.insert(out.end(), std::begin(buf), std::end(buf));
  }

  virtual void program(EventSchedule &ev) const
  {
    std::vector<uint8_t> out;
    char buf[256];

    /* Programmable parameters. */
    if(SendSysEx)
    {
      sysex(out, 0x6000,
       LFORate, LFODelay, LFOWaveform, LFOKeyTrigger);

      sysex(out, 0x0700,
       DCORange, DCOLFOLevel, DCOPWMLevel, DCOPWMSource,
       DCOPW, DCOSaw, DCOSubLevel, DCONoiseLevel, DCOSub);

      sysex(out, 0x0800,
       HPFCutoff, VCFCutoff, VCFResonance, VCFEnvPolarity,
       VCFEnvLevel, VCFLFOLevel, VCFKeyLevel);

      sysex(out, 0x0900,
       VCAEnv, VCALevel);

      sysex(out, 0x0a00,
       EnvAttack, EnvDecay, EnvSustain, EnvRelease);

      sysex(out, 0x1000,
       Chorus, DelayLevel, DelayTime, DelayFeedback, 0, Delay);

      sysex(out, 0x1100,
       Portamento, PortamentoTime, 0, AssignMode, BendRange, TempoSync);

      sysex_name(out, 0x1300, Name);
    }
    else
    {
      cc(out, CC::LFORate, LFORate >> 1);
      cc(out, CC::LFODelay, LFODelay >> 1);
      cc(out, CC::LFOWaveform, LFOWaveform);
      cc(out, CC::LFOKeyTrigger, LFOKeyTrigger);

      cc(out, CC::DCORange, DCORange);
      cc(out, CC::DCOLFOLevel, DCOLFOLevel >> 1);
      cc(out, CC::DCOPWMLevel, DCOPWMLevel >> 1);
      cc(out, CC::DCOPWMSource, DCOPWMSource);
      cc(out, CC::DCOPW, DCOPW);
      cc(out, CC::DCOSaw, DCOSaw);
      cc(out, CC::DCOSub, DCOSub);
      cc(out, CC::DCOSubLevel, DCOSubLevel >> 1);
      cc(out, CC::DCONoiseLevel, DCONoiseLevel >> 1);

      cc(out, CC::HPFCutoff, HPFCutoff >> 1);
      cc(out, CC::VCFCutoff, VCFCutoff >> 1);
      cc(out, CC::VCFResonance, VCFResonance >> 1);
      cc(out, CC::VCFEnvPolarity, VCFEnvPolarity);
      cc(out, CC::VCFEnvLevel, VCFEnvLevel >> 1);
      cc(out, CC::VCFLFOLevel, VCFLFOLevel >> 1);
      cc(out, CC::VCFKeyLevel, VCFKeyLevel >> 1);

      cc(out, CC::VCAEnv, VCAEnv);
      cc(out, CC::VCALevel, VCALevel >> 1);

      cc(out, CC::EnvAttack, EnvAttack >> 1);
      cc(out, CC::EnvDecay, EnvDecay >> 1);
      cc(out, CC::EnvSustain, EnvSustain >> 1);
      cc(out, CC::EnvRelease, EnvRelease >> 1);

      cc(out, CC::AssignMode, AssignMode);
      cc(out, CC::Chorus, Chorus);
      cc(out, CC::Delay, Delay);
      cc(out, CC::DelayTime, DelayTime);
      cc(out, CC::DelayLevel, DelayLevel);
      cc(out, CC::DelayFeedback, DelayFeedback);
      cc(out, CC::Hold, Hold ? 0x7f : 0x00);
      cc(out, CC::Portamento, Portamento ? 0x7f : 0x00);
      cc(out, CC::PortamentoTime, PortamentoTime >> 1);
      cc(out, CC::TempoSync, TempoSync);
      cc(out, CC::BendRange, BendRange);
    }

    MIDIEvent::schedule(ev, *this, std::move(out), EventSchedule::PROGRAM_TIME);

    /* Manual parameters. */
    snprintf(buf, sizeof(buf),
     "-> set parameter 'JunoMode' to: %u", JunoMode.value());
    NoticeEvent::schedule(ev, buf);
  }

  void load_cc(unsigned param, unsigned value)
  {
    switch(param)
    {
    case CC::LFORate:
      LFORate = value << 1;
      break;
    case CC::LFODelay:
      LFODelay = value << 1;
      break;
    case CC::LFOWaveform:
      LFOWaveform = value;
      break;
    case CC::LFOKeyTrigger:
      LFOKeyTrigger = value;
      break;
    case CC::DCORange:
      DCORange = value;
      break;
    case CC::DCOLFOLevel:
      DCOLFOLevel = value << 1;
      break;
    case CC::DCOPWMLevel:
      DCOPWMLevel = value << 1;
      break;
    case CC::DCOPWMSource:
      DCOPWMSource = value;
      break;
    case CC::DCOPW:
      DCOPW = value;
      break;
    case CC::DCOSaw:
      DCOSaw = value;
      break;
    case CC::DCOSub:
      DCOSub = value;
      break;
    case CC::DCOSubLevel:
      DCOSubLevel = value << 1;
      break;
    case CC::DCONoiseLevel:
      DCONoiseLevel = value << 1;
      break;
    case CC::HPFCutoff:
      HPFCutoff = value << 1;
      break;
    case CC::VCFCutoff:
      VCFCutoff = value << 1;
      break;
    case CC::VCFResonance:
      VCFResonance = value << 1;
      break;
    case CC::VCFEnvPolarity:
      VCFEnvPolarity = value;
      break;
    case CC::VCFEnvLevel:
      VCFEnvLevel = value << 1;
      break;
    case CC::VCFLFOLevel:
      VCFLFOLevel = value << 1;
      break;
    case CC::VCFKeyLevel:
      VCFKeyLevel = value << 1;
      break;
    case CC::VCAEnv:
      VCAEnv = value;
      break;
    case CC::VCALevel:
      VCALevel = value << 1;
      break;
    case CC::EnvAttack:
      EnvAttack = value << 1;
      break;
    case CC::EnvDecay:
      EnvDecay = value << 1;
      break;
    case CC::EnvSustain:
      EnvSustain = value << 1;
      break;
    case CC::EnvRelease:
      EnvRelease = value << 1;
      break;
    case CC::AssignMode:
      AssignMode = value;
      break;
    case CC::Chorus:
      Chorus = value;
      break;
    case CC::Delay:
      Delay = value;
      break;
    case CC::DelayTime:
      DelayTime = value;
      break;
    case CC::DelayLevel:
      DelayLevel = value;
      break;
    case CC::DelayFeedback:
      DelayFeedback = value;
      break;
    case CC::Hold:
      Hold = value >= 0x80 ? 1 : 0;
      break;
    case CC::Portamento:
      Portamento = value >= 0x80 ? 1 : 0;
      break;
    case CC::PortamentoTime:
      PortamentoTime = value << 1;
      break;
    case CC::TempoSync:
      TempoSync = value;
      break;
    case CC::BendRange:
      BendRange = value;
      break;
    }
  }

  unsigned value(unsigned hi, unsigned lo)
  {
    return ((hi & 0x0f) << 4) | (lo & 0x0f);
  }

  virtual bool load()
  {
    std::vector<uint8_t> in;

    if(!SysExPath[0] || !MIDIInterface::load_file(in, SysExPath))
      return false;

    if(in.size() < 3)
      return false;

    unsigned inpos = 0;
    unsigned end = in.size() - 3;
    while(inpos < end)
    {
      unsigned code = in[inpos++];
      if(code == 0xb0)
      {
        /* MIDI CC */
        unsigned param = in[inpos++];
        unsigned value = in[inpos++];
        load_cc(param, value);
        continue;
      }

      /* SysEx */
      if(code != 0xf0 || inpos + 12 > in.size())
        continue;
      if(in[inpos++] != 0x41)
        continue;

      uint8_t unitid = in[inpos + 0];  /* 10h */
      uint8_t model0 = in[inpos + 1];  /*  0h */
      uint8_t model1 = in[inpos + 2];  /*  0h */
      uint8_t model2 = in[inpos + 3];  /*  0h */
      uint8_t model3 = in[inpos + 4];  /* 62h (JU-06A) or 0x1d (JU-06)*/
      uint8_t command = in[inpos + 5]; /* 12h */
      uint8_t addr0 = in[inpos + 6];
      uint8_t addr1 = in[inpos + 7];
      uint8_t addr2 = in[inpos + 8];
      uint8_t addr3 = in[inpos + 9];

      if((unitid & 0x80) || (command != 0x12) || model0 != 0 || model1 != 0 ||
         model2 != 0 || (model3 != 0x62 && model3 != 0x1d))
        continue;

      inpos += 10;

      unsigned sum = (unsigned)addr0 + addr1 + addr2 + addr3;
      unsigned addr = (addr2 << 8) | addr3;

      unsigned end2 = inpos;
      while(end2 < in.size() && in[end2] < 0x80)
        end2++;
      if(end2 >= in.size() || in[end2] != 0xf7 || end2 - inpos < 2)
        return false;
      /* Do not load checksum. */
      end2--;

      /* Skip messages starting at unmapped memory areas. */
      if(addr < 0x0600 ||
         (addr >= 0x0606 && addr < 0x0700) ||
         (addr >= 0x071a && addr < 0x0800) ||
         (addr >= 0x0818 && addr < 0x0900) ||
         (addr >= 0x090e && addr < 0x0a00) ||
         (addr >= 0x0a12 && addr < 0x1000) ||
         (addr >= 0x1014 && addr < 0x1100) ||
         (addr >= 0x1116 && addr < 0x1300) ||
         (addr >= 0x1310))
      {
        while(inpos < end2)
          sum += in[inpos++];

        sum += in[inpos++];
        fprintf(stderr, "bad addr %04x, sum=%02x\n", addr, sum);
        if(sum & 0x7f)
          return false;

        continue;
      }

      unsigned hi = 0;
      while(inpos < end2 && addr < 0x1310)
      {
        unsigned lo = in[inpos++];
        sum += lo;

        switch(addr)
        {
        default:
          hi = lo;
          break;

        case 0x0601:  /* LFO */
          LFORate = value(hi, lo);
          break;
        case 0x0603:
          LFODelay = value(hi, lo);
          break;
        case 0x0605:
          LFOWaveform = value(hi, lo);
          break;
        case 0x0607:
          LFOKeyTrigger = value(hi, lo);
          break;
        case 0x060f:
          addr = 0x0701;
          /* fall-through */

        case 0x0701: /* DCO */
          DCORange = value(hi, lo);
          break;
        case 0x0703:
          DCOLFOLevel = value(hi, lo);
          break;
        case 0x0705:
          DCOPWMLevel = value(hi, lo);
          break;
        case 0x0707:
          DCOPWMSource = value(hi, lo);
          break;
        case 0x0709:
          DCOPW = value(hi, lo);
          break;
        case 0x070b:
          DCOSaw = value(hi, lo);
          break;
        case 0x070d:
          DCOSubLevel = value(hi, lo);
          break;
        case 0x070f:
          DCONoiseLevel = value(hi, lo);
          break;
        case 0x0711:
          DCOSub = value(hi, lo);
          break;
        case 0x071b:
          addr = 0x0801;
          /* fall-through */

        case 0x0801: /* VCF */
          HPFCutoff = value(hi, lo);
          break;
        case 0x0803:
          VCFCutoff = value(hi, lo);
          break;
        case 0x0805:
          VCFResonance = value(hi, lo);
          break;
        case 0x0807:
          VCFEnvPolarity = value(hi, lo);
          break;
        case 0x0809:
          VCFEnvLevel = value(hi, lo);
          break;
        case 0x080b:
          VCFLFOLevel = value(hi, lo);
          break;
        case 0x080d:
          VCFKeyLevel = value(hi, lo);
          break;
        case 0x0819:
          addr = 0x0901;
          /* fall-through */

        case 0x0901: /* VCA */
          VCAEnv = value(hi, lo);
          break;
        case 0x0903:
          VCALevel = value(hi, lo);
          break;
        case 0x090f:
          addr = 0x0a01;
          /*  fall-through */

        case 0x0a01: /* Envelope */
          EnvAttack = value(hi, lo);
          break;
        case 0x0a03:
          EnvDecay = value(hi, lo);
          break;
        case 0x0a05:
          EnvSustain = value(hi, lo);
          break;
        case 0x0a07:
          EnvRelease = value(hi, lo);
          break;
        case 0x0a13:
          addr = 0x1001;
          /* fall-through */

        case 0x1001: /* Effects 1 */
          Chorus = value(hi, lo);
          break;
        case 0x1003:
          DelayLevel = value(hi, lo);
          break;
        case 0x1005:
          DelayTime = value(hi, lo);
          break;
        case 0x1007:
          DelayFeedback = value(hi, lo);
          break;
        case 0x100b:
          Delay = value(hi, lo);
          break;
        case 0x1015:
          addr = 0x1101;
          /* fall-through */

        case 0x1101: /* Effects 2 */
          Portamento = value(hi, lo);
          break;
        case 0x1103:
          PortamentoTime = value(hi, lo);
          break;
        case 0x1107:
          AssignMode = value(hi, lo);
          break;
        case 0x1109:
          BendRange = value(hi, lo);
          break;
        case 0x110b:
          TempoSync = value(hi, lo);
          break;
        case 0x1106:
          addr = 0x1300;
          /* fall-through */

        case 0x1300:
        case 0x1301:
        case 0x1302:
        case 0x1303:
        case 0x1304:
        case 0x1305:
        case 0x1306:
        case 0x1307:
        case 0x1308:
        case 0x1309:
        case 0x130a:
        case 0x130b:
        case 0x130c:
        case 0x130d:
        case 0x130e:
        case 0x130f:
          Name[addr & 0xf] = lo;
          break;
        }
        addr++;
      }
      sum += in[inpos++];
      if(sum & 0x7f)
        return false;
    }
    return true;
  }
};


static class JU06ARegister : public ConfigRegister
{
public:
  JU06ARegister(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new JU06AInterface(ctx, tag, id));
  }
} reg("JU-06A");

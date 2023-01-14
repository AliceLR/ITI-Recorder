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
  OptionString<512> SysExPath;      // optional
  OptionBool        JunoMode;       // 0=60 1=106; requires manual switch
  OptionString<16>  Name;           // undocumented

  // FIXME: ALL of the 0-127s are actually 0-255.
  Option<unsigned>  LFORate;        // 0-127
  Option<unsigned>  LFODelay;       // 0-127
  Option<unsigned>  LFOWaveform;    // 0=tri 1=sqr 2=rampup 3=rampdown 4=sine 5=rnd1 6=rnd2
  OptionBool        LFOKeyTrigger;

  Option<unsigned>  DCORange;       // 0=16' 1=8' 2=4'
  Option<unsigned>  DCOLFOLevel;    // 0-127
  Option<unsigned>  DCOPWMLevel;    // 0-127
  Option<unsigned>  DCOPWMSource;   // 0=manual 1=LFO 2=envelope
  OptionBool        DCOPW;
  OptionBool        DCOSaw;
  OptionBool        DCOSub;
  Option<unsigned>  DCOSubLevel;    // 0-127
  Option<unsigned>  DCONoiseLevel;  // 0-127

  Option<unsigned>  HPFCutoff;      // 0-127
  Option<unsigned>  VCFCutoff;      // 0-127
  Option<unsigned>  VCFResonance;   // 0-127
  OptionBool        VCFEnvPolarity; // FIXME: which direction is 1?
  Option<unsigned>  VCFEnvLevel;    // 0-127
  Option<unsigned>  VCFLFOLevel;    // 0-127
  Option<unsigned>  VCFKeyLevel;    // 0-127

  OptionBool        VCAEnv;
  Option<unsigned>  VCALevel;       // 0-127

  Option<unsigned>  EnvAttack;      // 0-127
  Option<unsigned>  EnvDecay;       // 0-127
  Option<unsigned>  EnvSustain;     // 0-127
  Option<unsigned>  EnvRelease;     // 0-127

  Option<unsigned>  AssignMode;     // 0=poly 2=solo 3=unison
  Option<unsigned>  Chorus;         // 0=off 1=I 2=II 3=I+II
  OptionBool        Delay;
  Option<unsigned>  DelayTime;      // 0-15
  Option<unsigned>  DelayLevel;     // 0-15
  Option<unsigned>  DelayFeedback;  // 0-15
  OptionBool        Hold;
  OptionBool        Portamento;
  Option<unsigned>  PortamentoTime; // 0-255; divide by 2
  OptionBool        TempoSync;      // synchronizes LFO to tempo... kind of useless here
  Option<unsigned>  BendRange;      // 0-24

  JU06AInterface(ConfigContext &_ctx, const char *_tag, int _id):
   MIDIInterface(_ctx, _tag, _id),
   /* Options */
   SysExPath(options, "", "SysExPath"),
   JunoMode(options, false, "JunoMode"),
   Name(options, "<default>", "Name"),
   LFORate(options, 0, 0, 127, "LFORate"),
   LFODelay(options, 0, 0, 127, "LFODelay"),
   LFOWaveform(options, 0, 0, 6, "LFOWaveform"),
   LFOKeyTrigger(options, false, "LFOKeyTrigger"),
   DCORange(options, 1, 0, 2, "DCORange"),
   DCOLFOLevel(options, 0, 0, 127, "DCOLFOLevel"),
   DCOPWMLevel(options, 0, 0, 127, "DCOPWMLevel"),
   DCOPWMSource(options, 0, 0, 2, "DCOPWMSource"),
   DCOPW(options, true, "DCOPW"),
   DCOSaw(options, false, "DCOSaw"),
   DCOSub(options, false, "DCOSub"),
   DCOSubLevel(options, 0, 0, 127, "DCOSubLevel"),
   DCONoiseLevel(options, 0, 0, 127, "DCONoiseLevel"),
   HPFCutoff(options, 0, 0, 127, "HPFFrequency"),
   VCFCutoff(options, 0, 0, 127, "VCFFrequency"),
   VCFResonance(options, 0, 0, 127, "VCFResonance"),
   VCFEnvPolarity(options, false, "VCFEnvPolarity"),
   VCFEnvLevel(options, 0, 0, 127, "VCFEnvLevel"),
   VCFLFOLevel(options, 0, 0, 127, "VCFLFOLevel"),
   VCFKeyLevel(options, 0, 0, 127, "VCFKeyLevel"),
   VCAEnv(options, true, "VCAEnv"),
   VCALevel(options, 127, 0, 127, "VCALevel"),
   EnvAttack(options, 0, 0, 127, "EnvAttack"),
   EnvDecay(options, 63, 0, 127, "EnvDecay"),
   EnvSustain(options, 95, 0, 127, "EnvSustain"),
   EnvRelease(options, 127, 0, 127, "EnvRelease"),
   AssignMode(options, 47, 0, 3, "AssignMode"),
   Chorus(options, 0, 0, 3, "Chorus"),
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
  void sysex_write(uint8_t *buf, V value, REST...values) const
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
    buf[0]  = 0xf0; /* SysEx*/
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

  virtual void program(EventSchedule &ev) const
  {
    std::vector<uint8_t> out;
    char buf[256];

    /* KILL ME */
    sysex(out, 0x0700,
     DCORange, DCOLFOLevel, DCOPWMLevel, DCOPWMSource,
     DCOPW, DCOSaw, DCOSubLevel, DCONoiseLevel, DCOSub);

    /* Programmable parameters. */
    cc(out, CC::LFORate, LFORate);
    cc(out, CC::LFODelay, LFODelay);
    cc(out, CC::LFOWaveform, LFOWaveform);
    cc(out, CC::LFOKeyTrigger, LFOKeyTrigger);
    /*
    cc(out, CC::DCORange, DCORange);
    cc(out, CC::DCOLFOLevel, DCOLFOLevel);
    cc(out, CC::DCOPWMLevel, DCOPWMLevel);
    cc(out, CC::DCOPWMSource, DCOPWMSource);
    cc(out, CC::DCOPW, DCOPW);
    cc(out, CC::DCOSaw, DCOSaw);
    cc(out, CC::DCOSub, DCOSub);
    cc(out, CC::DCOSubLevel, DCOSubLevel);
    cc(out, CC::DCONoiseLevel, DCONoiseLevel);
    */
    cc(out, CC::HPFCutoff, HPFCutoff);
    cc(out, CC::VCFCutoff, VCFCutoff);
    cc(out, CC::VCFResonance, VCFResonance);
    cc(out, CC::VCFEnvPolarity, VCFEnvPolarity);
    cc(out, CC::VCFEnvLevel, VCFEnvLevel);
    cc(out, CC::VCFLFOLevel, VCFLFOLevel);
    cc(out, CC::VCFKeyLevel, VCFKeyLevel);
    cc(out, CC::VCAEnv, VCAEnv);
    cc(out, CC::VCALevel, VCALevel);
    cc(out, CC::EnvAttack, EnvAttack);
    cc(out, CC::EnvDecay, EnvDecay);
    cc(out, CC::EnvSustain, EnvSustain);
    cc(out, CC::EnvRelease, EnvRelease);
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

    MIDIEvent::schedule(ev, *this, std::move(out), EventSchedule::PROGRAM_TIME);

    /* Manual parameters. */
    snprintf(buf, sizeof(buf),
     "-> set parameter 'JunoMode' to: %u", JunoMode ? 106 : 60);
    NoticeEvent::schedule(ev, buf);
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

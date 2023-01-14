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
#include <iterator>

#include "Config.hpp"
#include "Midi.hpp"

class PSR36Interface final : public MIDIInterface
{
public:
  Option<unsigned> Patch;       // 0-31
  Option<unsigned> Sustain;     // 0-2, manual
  Option<unsigned> Synthesizer; // 0-1, manual
  Option<unsigned> Spectrum;    // 1-5, manual
  Option<unsigned> Brilliance;  // 1-5, manual
  Option<unsigned> Envelope;    // 1-5, manual
  Option<unsigned> Vibrato;     // 1-5, manual
  Option<unsigned> Detune;      // 1-5, manual

  PSR36Interface(ConfigContext &_ctx, const char *_tag, int _id):
   MIDIInterface(_ctx, _tag, _id),
   /* options */
   Patch(options, 0, 0, 31, "Patch"),
   Sustain(options, 0, 0, 2, "Sustain"),
   Synthesizer(options, 1, 0, 1, "Synthesizer"),
   Spectrum(options, 3, 1, 5, "Spectrum"),
   Brilliance(options, 3, 1, 5, "Brilliance"),
   Envelope(options, 3, 1, 5, "Envelope"),
   Vibrato(options, 1, 1, 5, "Vibrato"),
   Detune(options, 1, 1, 5, "Detune")
  {}

  virtual ~PSR36Interface() {}

  void notice(EventSchedule &ev, const char *param, unsigned value) const
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "-> set parameter '%s' to: %u", param, value);
    NoticeEvent::schedule(ev, buf);
  }

  virtual void program(EventSchedule &ev) const
  {
    std::vector<uint8_t> out;

    /* Programmable parameters. */
    program_change(out, Patch);
    MIDIEvent::schedule(ev, *this, std::move(out), EventSchedule::PROGRAM_TIME);

    /* Manual parameters. */
    notice(ev, "Sustain", Sustain);
    notice(ev, "Synthesizer", Synthesizer);
    notice(ev, "Spectrum", Spectrum);
    notice(ev, "Brilliance", Brilliance);
    notice(ev, "Envelope", Envelope);
    notice(ev, "Vibrato", Vibrato);
    notice(ev, "Detune", Detune);
  }
};


static class PSR36Register : public ConfigRegister
{
public:
  PSR36Register(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new PSR36Interface(ctx, tag, id));
  }
} reg("PSR-36");

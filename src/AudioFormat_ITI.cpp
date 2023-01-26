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
#include "Buffer.hpp"

#include "stdio.h"

class ITIConfig : public ConfigInterface
{
public:
  OptionString<25>  Name;
  Option<unsigned>  MaxHalfSteps;

  ITIConfig(ConfigContext &_ctx, const char *_tag, int _id):
   ConfigInterface(_ctx, _tag, _id),
   Name(options, "<default>", "Name"),
   MaxHalfSteps(options, 3, 0, 120, "MaxHalfSteps")
  {}

  virtual ~ITIConfig() {}
};

class ITIConfigRegister : public ConfigRegister
{
public:
  ITIConfigRegister(const char *_tag): ConfigRegister(_tag) {}

  std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new ITIConfig(ctx, tag, id));
  }
} reg_iti("ITI");


/* Sample conversion functions. */
static void cvt(std::vector<uint8_t> &out, uint8_t d)
{
  out.push_back(d);
}
static void cvt(std::vector<uint8_t> &out, int16_t d)
{
  out.push_back((d >> 0) & 0xff);
  out.push_back((d >> 8) & 0xff);
}
static void cvt(std::vector<uint8_t> &out, int32_t d)
{
  out.push_back((d >> 16) & 0xff);
  out.push_back((d >> 24) & 0xff);
}

static const class AudioFormatITI : public AudioFormat
{
  /* To clarify the nonsense in the documentation: the instrument IS 554 bytes.
   * There are *4* extra bytes of padding at the end. The documentation count
   * includes the three envelope padding bytes in its count for no reason
   * other than to confuse the reader.
   */
  static constexpr unsigned IMPI_LENGTH = 0x40 + 240 + 3 * 82 + 4;
  static constexpr unsigned IMPS_LENGTH = 0x50;

  struct Note
  {
    unsigned note;
    unsigned file_offset;
    size_t start;
    size_t end;

    size_t length() const
    {
      return end - start;
    }
  };

  /**
   * Write an IT instrument header to the output buffer.
   *
   * @param out     Buffer for output ITI file.
   * @param notes   List of all cued notes within the AudioBuffer.
   * @param cfg     Global configuration info.
   */
  void write_impi(std::vector<uint8_t> &out, std::vector<Note> &notes,
   ConfigContext &ctx) const
  {
    const auto iti = ctx.get_interface_as<ITIConfig>("ITI");
    if(!iti)
      return;

    uint8_t keymap[240]{};
    char dosname[12]{};
    char insname[26]{};

    snprintf(insname, sizeof(insname), "%s", iti->Name.value());

    /* Build initial keymap from notes with existing samples. */
    for(size_t i = 0; i < notes.size(); i++)
    {
      const Note &note = notes[i];
      unsigned idx = note.note * 2;
      if(idx < sizeof(keymap) - 1)
      {
        keymap[idx + 0] = MIDIInterface::C4; /* = IT C5 */
        keymap[idx + 1] = i + 1;
      }
    }

    /* Fill in unmapped keys by transposing up/down to the half steps limit. */
    for(size_t i = 0; i < iti->MaxHalfSteps; i++)
    {
      for(size_t j = 0; j < 240; j += 2)
      {
        if(keymap[j + 1] == 0)
        {
          unsigned prev = (j > 0) ? keymap[j - 2 + 1] : 0;
          unsigned next = (j < 238) ? keymap[j + 2 + 1] : 0;
          if(prev)
          {
            keymap[j + 0] = std::min(119, keymap[j - 2] + 1);
            keymap[j + 1] = keymap[j - 1];
            j += 2; // Skip next gap
          }
          else

          if(next)
          {
            keymap[j + 0] = std::max(0, keymap[j + 2] - 1);
            keymap[j + 1] = keymap[j + 3];
          }
        }
      }
    }

    uint8_t buf[IMPI_LENGTH]{};
    Buffer<IMPI_LENGTH> tmp = Buffer<IMPI_LENGTH>(buf)
      .append('I','M','P','I')
      .append(dosname)
      .append<uint8_t>(0)                 /* padding */
      .append<uint8_t>(2)                 /* NNA: Note off */
      .append<uint8_t>(1)                 /* DCA: Note check */
      .append<uint8_t>(2)                 /* DCA: Note fade */
      .append<uint16_t>(128)              /* Fade out */
      .append<int8_t>(0)                  /* Pitch pan separation, -32 to 32 */
      .append<uint8_t>(60)                /* Pitch pan center = C5 */
      .append<uint8_t>(128)               /* Global volume */
      .append<uint8_t>(128)               /* Default pan (128 = don't use) */
      .append<uint8_t>(0)                 /* Random volume variation */
      .append<uint8_t>(0)                 /* Random pan variation */
      .append<uint16_t>(0x0202)           /* Tracker version = 2.02 */
      .append<uint8_t>(notes.size())      /* Number of samples */
      .append<uint8_t>(0)                 /* padding */
      .append(insname)
      .append<uint8_t>(0x7f)              /* initial filter cutoff (127, unused) */
      .append<uint8_t>(0x00)              /* initial filter resonance (0, unused) */
      .append<uint8_t>(0)                 /* MIDI channel */
      .append<uint8_t>(0)                 /* MIDI program */
      .append<uint16_t>(0)                /* MIDI bank */
      .append(keymap)
      .skip<82>()                             /* Volume envelope */
      .skip<82>()                             /* Pan envelope */
      .skip<82>()                             /* Filter envelope */
      .skip<4>()                              /* "7" bytes of padding */
      .check();

    out.insert(out.end(), tmp.begin(), tmp.end());
  }

  /**
   * Write an IT sample header to the output buffer.
   *
   * @param out     Buffer for output ITI file.
   * @param note    Data for the current note/sample.
   * @param cfg     Global configuration info.
   * @param buffer  AudioBuffer containing the current sample's data.
   */
  template<class T>
  void write_imps(std::vector<uint8_t> &out, Note &note,
   ConfigContext &ctx, const AudioBuffer<T> &buffer) const
  {
    const auto cfg = ctx.get_interface_as<GlobalConfig>("Global");
    const auto iti = ctx.get_interface_as<ITIConfig>("ITI");
    if(!cfg || !iti)
      return;

    char dosname[12]{};
    char smpname[26]{};
    unsigned flags = (1 << 0);                /* sample is set */

    snprintf(smpname, sizeof(smpname), "%s", iti->Name.value());
    snprintf(dosname, sizeof(dosname), "%s",
     MIDIInterface::get_note(note.note));

    if(sizeof(T) >= 2)
      flags |= (1 << 1);                      /* 16-bit */
    if(buffer.channels >= 2)
      flags |= (1 << 2);                      /* stereo */
    /* TODO: compression (configurable) */

    uint8_t buf[IMPS_LENGTH];
    Buffer<IMPS_LENGTH> tmp = Buffer<IMPS_LENGTH>(buf)
      .append('I','M','P','S')
      .append(dosname)
      .append<uint8_t>(0)                 /* padding */
      .append<uint8_t>(64)                /* Global volume */
      .append<uint8_t>(flags)             /* Flags */
      .append<uint8_t>(64)                /* Default volume */
      .append(smpname)
      .append<uint8_t>(0x01)              /* Convert: bit 0 = samples are signed */
      .append<int8_t>(0)                  /* Default pan = off */
      .append<uint32_t>(note.length())    /* Sample length in frames */
      .append<uint32_t>(0)                /* Loop start */
      .append<uint32_t>(0)                /* Loop end */
      .append<uint32_t>(cfg->audio_rate)  /* C5 speed */
      .append<uint32_t>(0)                /* Sustain loop start */
      .append<uint32_t>(0)                /* Sustain loop end */
      .append<uint32_t>(note.file_offset) /* Sample offset in file */
      .append<uint8_t>(0)                 /* Vibrato speed */
      .append<uint8_t>(0)                 /* Vibrato depth */
      .append<uint8_t>(0)                 /* Vibrato waveform */
      .append<uint8_t>(0)                 /* Vibrato rate */
      .check();

    out.insert(out.end(), tmp.begin(), tmp.end());
  }

  /**
   * Get the saved length of a sample after 32->16 conversion and channel removal.
   *
   * @param note    Data for the current note/sample, including start/end in buffer.
   * @param buffer  AudioBuffer containing the current sample's data.
   */
  template<class T>
  size_t sample_length(Note &note, const AudioBuffer<T> &buffer) const
  {
    return std::min((size_t)2, sizeof(T)) * std::min(2U, buffer.channels) * note.length();
  }

  /**
   * Write sample data to the output buffer.
   *
   * @param out     Buffer for output ITI file.
   * @param note    Data for the current note/sample, including start/end in buffer.
   * @param buffer  AudioBuffer containing the current sample's data.
   */
  template<class T>
  void write_sample(std::vector<uint8_t> &out,
   Note &note, const AudioBuffer<T> &buffer) const
  {
    // TODO: compression: temporary buffer, bitstream, 8 bit and 16 bit
    //std::vector<uint8_t> tmp;
    std::vector<uint8_t> &dest = out;

    dest.reserve(dest.size() + sample_length(note, buffer));

    /* Left channel */
    size_t j = note.start * buffer.channels;
    // TODO: compression: feed into compression stream
    for(size_t i = note.start; i < note.end; i++, j += buffer.channels)
      cvt(dest, buffer[j]);

    /* Right channel */
    if(buffer.channels >= 2)
    {
      j = note.start * buffer.channels + 1;
      // TODO: compression: feed into compression stream
      for(size_t i = note.start; i < note.end; i++, j += buffer.channels)
        cvt(dest, buffer[j]);
    }
    // TODO: compression: flush compressed data to out
  }

  /**
   * Convert all cued notes in an audio buffer to an Impulse Tracker instrument
   * file. Combined conversion function for all sample formats.
   *
   * @param cfg     Global configuration info.
   * @param out     Buffer for output ITI file.
   * @param buffer  AudioBuffer containing audio data and all note cues.
   * @param start   unused
   * @param end     unused
   * @returns       `true` on success, otherwise `false`.
   */
  template<class T>
  bool _convert(ConfigContext &ctx, std::vector<uint8_t> &out,
   const AudioBuffer<T> &buffer, const AudioCue &start, const AudioCue &end) const
  {
    /* Reject individual note saves */
    if(start.value >= 0)
      return false;

    std::vector<Note> notes;

    const std::vector<AudioCue> &cues = buffer.get_cues();
    for(size_t i = 1; i < cues.size(); i++)
    {
      if(notes.size() >= 255) /* IT: 99, make configurable? */
        break;

      const AudioCue &on = cues[i - 1];
      const AudioCue &off = cues[i];
      if(on.type == AudioCue::NoteOn && off.type == AudioCue::NoteOff &&
       on.value == off.value && on.frame < off.frame)
      {
        notes.push_back({
          static_cast<unsigned>(on.value),
          0,
          on.frame,
          off.frame
        });
      }
    }

    write_impi(out, notes, ctx);

    unsigned sample_pos = IMPI_LENGTH + notes.size() * IMPS_LENGTH + 4;
    for(Note &note : notes)
    {
      note.file_offset = sample_pos;
      sample_pos += sample_length(note, buffer);

      write_imps(out, note, ctx, buffer);
    }

    /* Do NOT interpret sample data as a header */
    uint8_t no_tag[4]{};
    out.insert(out.end(), std::begin(no_tag), std::end(no_tag));

    for(Note &note : notes)
      write_sample(out, note, buffer);

    return true;
  }

  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<uint8_t> &buffer,
   const AudioCue &start, const AudioCue &end) const override
  {
    return _convert(ctx, out, buffer, start, end);
  }

  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<int16_t> &buffer,
   const AudioCue &start, const AudioCue &end) const override
  {
    return _convert(ctx, out, buffer, start, end);
  }

  virtual bool convert(ConfigContext &ctx,
   std::vector<uint8_t> &out, const AudioBuffer<int32_t> &buffer,
   const AudioCue &start, const AudioCue &end) const override
  {
    return _convert(ctx, out, buffer, start, end);
  }
} iti;

const AudioFormat &AudioFormatITI = iti;

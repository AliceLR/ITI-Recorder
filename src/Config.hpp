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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>

class ConfigOption
{
public:
  const char * const key;

  ConfigOption(std::vector<ConfigOption *> &v, const char *_key): key(_key)
  {
    v.push_back(this);
  }
  virtual bool handle(const char *value) = 0;
  virtual void print() = 0;
};

struct EnumValue
{
  const char *key = nullptr;
  unsigned value = 0;
};

/* VALUES must be terminated by { nullptr, # }. */
template<const EnumValue VALUES[]>
class Enum : public ConfigOption
{
protected:
  unsigned val;
  unsigned index;

public:
  Enum(std::vector<ConfigOption *> &v, const char *def, const char *_key):
   ConfigOption(v, _key), val(0)
  {
    val = VALUES[0].value;
    index = 0;
    if(!Enum::handle(def))
      throw "invalid enum default value!";
  }

  virtual ~Enum() {}
  virtual bool handle(const char *value)
  {
    for(size_t i = 0; VALUES[i].key; i++)
    {
      if(!strcasecmp(VALUES[i].key, value))
      {
        val = VALUES[i].value;
        index = i;
        return true;
      }
    }
    return false;
  }

  virtual void print()
  {
    printf("%s=%s\n", key, VALUES[index].key);
  }

  Enum &operator=(unsigned value)
  {
    for(size_t i = 0; VALUES[i].key; i++)
    {
      if(VALUES[i].value == value)
      {
        val = value;
        index = i;
        break;
      }
    }
    return *this;
  }

  operator unsigned() const { return val; }
  unsigned value() const { return val; }
};

extern const EnumValue BoolValues[];

class OptionBool : public Enum<BoolValues>
{
public:
  OptionBool(std::vector<ConfigOption *> &v, bool def, const char *_key):
   Enum<BoolValues>(v, def ? "on" : "off", _key) {}

  OptionBool &operator=(bool value)
  {
    Enum::operator=(value);
    return *this;
  }
};

template<class T>
class Option : public ConfigOption
{
  T val;
  T min;
  T max;

public:
  Option(std::vector<ConfigOption *> &v,
   T def, T _min, T _max, const char *_key):
   ConfigOption(v, _key), val(def), min(_min), max(_max) {}

  virtual ~Option() {}
  virtual bool handle(const char *value)
  {
    char *e;
    T v = strtol(value, &e, 10);
    if(*e || v < min || v > max)
      return false;

    val = v;
    return true;
  }

  virtual void print()
  {
    printf("%s=%lld\n", key, (long long)val);
  }

  Option<T> &operator=(T v)
  {
    if(v >= min && v <= max)
      val = v;
    return *this;
  }

  operator T() const { return val; }
  T value() const { return val; }
};

template<size_t SZ>
class OptionString : public ConfigOption
{
  char buf[SZ + 1]{};

public:
  OptionString(std::vector<ConfigOption *> &v,
   const char *def, const char *_key):
   ConfigOption(v, _key)
  {
    snprintf(buf, sizeof(buf), "%s", def);
  }

  virtual ~OptionString() {}
  virtual bool handle(const char *value)
  {
    snprintf(buf, sizeof(buf), "%s", value);
    return true;
  }

  virtual void print()
  {
    printf("%s=%s\n", key, buf);
  }

  OptionString &operator=(const char *value)
  {
    handle(value);
    return *this;
  }

  operator char *() { return buf; }
  operator const char *() const { return buf; }
  const char *value() const { return buf; }
};

class OptionRate : public ConfigOption
{
  unsigned audio_rate;

public:
  OptionRate(std::vector<ConfigOption *> &v, unsigned def, const char *_key):
   ConfigOption(v, _key), audio_rate(def) {}

  static constexpr bool check(unsigned rate)
  {
    return  rate == 8000  || rate == 16000 ||
            rate == 22050 || rate == 44100 ||
            rate == 48000 || rate == 88200 ||
            rate == 96000 || rate == 176400 ||
            rate == 192000;
  }

  virtual ~OptionRate() {}
  virtual bool handle(const char *value)
  {
    char *e;
    unsigned rate = strtoul(value, &e, 10);
    if(!*e && check(rate))
    {
      audio_rate = rate;
      return true;
    }
    return false;
  }

  virtual void print()
  {
    printf("%s=%u\n", key, audio_rate);
  }

  OptionRate &operator=(unsigned rate)
  {
    if(check(rate))
      audio_rate = rate;
    return *this;
  }

  operator unsigned() const { return audio_rate; }
  unsigned value() const { return audio_rate; }
};


class ConfigContext;

class ConfigSubinterface
{
public:
  std::vector<ConfigOption *> options;
  const char *name;

  ConfigSubinterface(std::vector<ConfigSubinterface *> &v, const char *_name): name(_name)
  {
    v.push_back(this);
  }
};

class ConfigInterface
{
protected:
  std::vector<ConfigOption *> options;
  std::vector<ConfigSubinterface *> subs;
  ConfigContext &ctx;
  ConfigSubinterface *subinterface = nullptr;

  bool handle(std::vector<ConfigOption *> &_options, const char *opt, const char *value);

public:
  char tag[32];
  int id;

  ConfigInterface(ConfigContext &_ctx, const char *_tag, int _id):
   ctx(_ctx), id(_id)
  {
    snprintf(tag, sizeof(tag), "%s", _tag);
  }

  virtual ~ConfigInterface() {}

  ConfigSubinterface *get_subinterface(const char *subtag)
  {
    for(ConfigSubinterface *sub : subs)
    {
      if(!strcasecmp(sub->name, subtag))
        return sub;
    }
    return nullptr;
  }

  bool set_subinterface()
  {
    subinterface = nullptr;
    return true;
  }

  bool set_subinterface(const char *subtag)
  {
    ConfigSubinterface *sub = get_subinterface(subtag);
    if(sub)
    {
      subinterface = sub;
      return true;
    }
    return false;
  }

  bool handle(const char *opt, const char *value);
  void print();
};


class ConfigRegister
{
protected:
  static std::vector<const ConfigRegister *> handlers;
  const char *tag;

public:
  ConfigRegister(const char *tag);

  virtual std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const = 0;

  static std::shared_ptr<ConfigInterface> generate_config(ConfigContext &ctx,
   const char *tag, int id);
};

class ConfigContext
{
  std::vector<std::shared_ptr<ConfigInterface>> interfaces;
  int current_cfg;
  int current_line;

  const char *skip_whitespace(const char *in);

public:
  void error(const char *reason, const char *value);

  std::shared_ptr<ConfigInterface> get_interface(const char *tag, int id = 1);
  const std::vector<std::shared_ptr<ConfigInterface>> &get_interfaces()
  {
    return interfaces;
  }

  template<class T>
  std::shared_ptr<T> get_interface_as(const char *tag, int id = 1)
  {
    std::shared_ptr<ConfigInterface> a = get_interface(tag, id);
    return std::dynamic_pointer_cast<T>(a);
  }

  bool parse_config_file(const char *filename);
  bool parse_config(int num, const char * const *cfgs);
  bool parse_config(const char *cfg)
  {
    return parse_config(1, &cfg);
  }

  bool check_filename_string(const char *filename);
  bool init(int argc, const char * const *argv);
};


/* Specific configurations. */
class GlobalConfig : public ConfigInterface
{
public:
  static constexpr unsigned max_inputs = 32;

  /* Audio recording options. */
  OptionString<31>  audio_driver;
  OptionString<31>  audio_device;
  OptionRate        audio_rate;
  OptionBool        output_on;
  OptionBool        output_noise_removal;
  Option<unsigned>  output_noise_ms;
  OptionBool        output_dump;
  OptionBool        output_flac;
  OptionBool        output_wav;
  OptionBool        output_sam;
  OptionBool        output_iti;

  /* Patch playback configuration. */
  OptionBool        program_on;

  GlobalConfig(ConfigContext &_ctx, const char *_tag, int _id):
   ConfigInterface(_ctx, _tag, _id),
   audio_driver(options, "", "Driver"),
   audio_device(options, "0", "Audio"),
   audio_rate(options, 96000, "AudioRate"),
   output_on(options, true, "Output"),
   output_noise_removal(options, true, "OutputNoiseRemoval"),
   output_noise_ms(options, 30*1000, 0, UINT_MAX, "OutputNoiseMS"),
   output_dump(options, false, "OutputDump"),
   output_flac(options, false, "OutputFLAC"),
   output_wav(options, true, "OutputWAV"),
   output_sam(options, false, "OutputSAM"),
   output_iti(options, true, "OutputITI"),
   program_on(options, true, "Program")
  {}

  virtual ~GlobalConfig() {}
};

class InputConfig : public ConfigInterface
{
public:
  OptionString<32>  midi_device;
  Option<unsigned>  midi_channel;

  InputConfig(ConfigContext &_ctx, const char *_tag, int _id):
   ConfigInterface(_ctx, _tag, _id),
   midi_device(options, "hw:1,0,0", "Device"),
   midi_channel(options, 1, 1, 16, "Channel")
  {}

  virtual ~InputConfig() {}
};

#endif /* CONFIG_HPP */

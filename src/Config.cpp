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
#include <memory>
#include <vector>

#include "Config.hpp"

std::vector<const ConfigRegister *> ConfigRegister::handlers;

const EnumValue BoolValues[] =
{
  { "off", 0 },
  { "on", 1 },
  { "false", 0 },
  { "true", 1 },
  { "0", 0 },
  { "1", 1 },
  { }
};

static class GlobalRegister : public ConfigRegister
{
public:
  GlobalRegister(const char *_tag): ConfigRegister(_tag) {}

  virtual std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new GlobalConfig(ctx, tag, id));
  }
} global("global");

static class InputRegister : public ConfigRegister
{
public:
  InputRegister(const char *_tag): ConfigRegister(_tag) {}

  virtual std::shared_ptr<ConfigInterface> generate(ConfigContext &ctx,
   const char *tag, int id) const
  {
    return std::shared_ptr<ConfigInterface>(new InputConfig(ctx, tag, id));
  }
} input("MIDI");


bool ConfigInterface::handle(std::vector<ConfigOption *> &_options,
 const char *key, const char *value)
{
  for(ConfigOption *opt : _options)
  {
    if(!strcasecmp(opt->key, key))
    {
      if(opt->handle(value))
        return true;

      char tmp[256];
      snprintf(tmp, sizeof(tmp), "invalid value for %s.%s", tag, key);
      ctx.error(tmp, value);
      return false;
    }
  }
  ctx.error("unknown option", key);
  return false;
}

bool ConfigInterface::handle(const char *key, const char *value)
{
  if(subinterface)
    return handle(subinterface->options, key, value);

  return handle(options, key, value);
}

void ConfigInterface::print()
{
  printf("\n[%s:%d]\n", tag, id);

  for(ConfigOption *opt : options)
    opt->print();

  for(ConfigSubinterface *sub : subs)
  {
    printf("\n[.%s]\n", sub->name);

    for(ConfigOption *opt : sub->options)
      opt->print();
  }
}


ConfigRegister::ConfigRegister(const char *t): tag(t)
{
  handlers.push_back(this);
}

std::shared_ptr<ConfigInterface> ConfigRegister::generate_config(ConfigContext &ctx,
 const char *tag, int id)
{
  for(auto &h : handlers)
  {
    if(!strcasecmp(tag, h->tag))
      return h->generate(ctx, tag, id);
  }
  return std::shared_ptr<ConfigInterface>(nullptr);
}

std::shared_ptr<ConfigInterface> ConfigContext::get_interface(const char *tag, int id)
{
  for(auto &interface : interfaces)
  {
    if(!strcasecmp(tag, interface->tag) && interface->id == id)
      return interface;
  }

  // Create new interface.
  auto interface = ConfigRegister::generate_config(*this, tag, id);
  if(interface != nullptr)
    interfaces.push_back(interface);
  else
    error("unknown group", tag);

  return interface;
}

void ConfigContext::error(const char *reason, const char *value)
{
  if(current_cfg)
  {
    fprintf(stderr, "error at argument %d:%d: %s: %s\n",
     current_cfg, current_line, reason?reason:"null", value?value:"null");
  }
  else
  {
    fprintf(stderr, "error at line %d: %s: %s\n",
     current_line, reason?reason:"null", value?value:"null");
  }
}

const char *ConfigContext::skip_whitespace(const char *in)
{
  while(true)
  {
    if(*in == '\n')
    {
      return in;
    }
    else

    if(isspace((unsigned)*in))
    {
      in++;
    }
    else
      return in;
  }
}

#define STR_EV(num) #num
#define STR(num) STR_EV(num)
#define INPUT_SIZE 1024

static bool is_ident(unsigned char ch)
{
  static constexpr class ident_table
  {
  public:
    char tbl[256]{};
    constexpr ident_table()
    {
      tbl['-'] = 1;
      tbl['_'] = 1;
      for(int i = '0'; i <= '9'; i++)
        tbl[i] = 1;
      for(int i = 'A'; i <= 'Z'; i++)
        tbl[i] = 1, tbl[i + 32] = 1;
    }
  } ident_table;
  return ident_table.tbl[ch];
}

bool ConfigContext::parse_config(int num, const char * const *cfgs)
{
  current_cfg = 0;
  current_line = 0;

  std::shared_ptr<ConfigInterface> current = get_interface("global");
  char buffer[INPUT_SIZE + 1];
  char key[INPUT_SIZE + 1];
  unsigned id;

  /* Create MIDI input interface 1 if it doesn't already exist. */
  get_interface("MIDI");

  if(current == nullptr)
    return false;

  for(current_cfg = 0; current_cfg < num; current_cfg++)
  {
    const char *pos = cfgs[current_cfg];
    const char *next = pos;
    const char *end;
    const char *tend;
    char *t;
    current_line = 0;

    while(*pos)
    {
      pos = skip_whitespace(next);
      if(!*pos)
        break;

      // Find line end.
      end = pos;
      while(*end && *end != '\n')
        end++;
      next = end + 1;
      current_line++;

      // Comment or blank line
      if(pos == end || *pos == '#' || *pos == ';')
      {
      }
      else

      // Group
      // too many bugs with sscanf gotta do this the shitty way i guess :(
      if(*pos == '[')
      {
        pos++;
        t = key;
        tend = key + sizeof(key) - 1;
        while(pos < end && t < tend && is_ident(*pos))
          *(t++) = *(pos++);

        *t = '\0';
        if(pos >= end)
          goto err;

        t = buffer;
        tend = buffer + sizeof(buffer) - 1;
        if(*pos == '.')
        {
          pos++;
          while(pos < end && t < tend && (is_ident(*pos) || *pos == '.'))
            *(t++) = *(pos++);
        }
        *t = '\0';
        if(pos >= end || (!key[0] && !buffer[0]))
          goto err;

        id = 1;
        if(*pos == ':')
        {
          pos++;
          id = strtoul(pos, &t, 10);
          pos = t;
        }
        if(pos >= end || *(pos++) != ']')
          goto err;

        if(key[0])
        {
          current = get_interface(key, id);
          if(current == nullptr)
          {
            error("unknown group", key);
            return false;
          }
          current->set_subinterface();
        }
        if(buffer[0])
        {
          if(!current->set_subinterface(buffer))
          {
            error("unknown group", buffer);
            return false;
          }
        }
      }
      // Key
      else
      {
        t = key;
        tend = key + sizeof(key) - 1;
        while(pos < end && t < tend && is_ident(*pos))
          *(t++) = *(pos++);

        *t = '\0';
        if(pos >= end)
          goto err;

        pos = skip_whitespace(pos);
        if(pos >= end)
          goto err;

        if(*pos != '=')
          goto err;

        pos = skip_whitespace(pos + 1);
        if(pos > end)
          goto err;

        t = buffer;
        tend = buffer + sizeof(buffer) - 1;
        while(pos < end && t < tend && (!isspace(*pos) && *pos != ';' && *pos != '#'))
          *(t++) = *(pos++);
        *t = '\0';

        if(!current->handle(key, buffer))
          return false;
      }
      continue;

    err:
      error("syntax error", pos);
      return false;
    }
  }
  return true;
}

bool ConfigContext::parse_config_file(const char *filename)
{
  std::vector<char> contents;
  bool ret = false;

  FILE *fp = fopen(filename, "rb");
  if(!fp)
    return false;

  int fd = fileno(fp);
  if(fd < 0)
    goto err;

  struct stat st;
  if(fstat(fd, &st) != 0)
    goto err;

  contents.resize(st.st_size);

  if(fread(contents.data(), st.st_size, 1, fp) < 1)
    goto err;

  ret = parse_config(contents.data());

err:
  fclose(fp);
  return ret;
}

bool ConfigContext::check_filename_string(const char *filename)
{
  while(*filename)
  {
    if(!isalnum((unsigned)*filename) &&
     *filename != ' ' && *filename != '-' && *filename != '_' &&
     *filename != '/' && *filename != '.')
      return false;

    filename++;
  }
  return true;
}

bool ConfigContext::init(int argc, const char * const *argv)
{
  int cmd_start = 1;

  if(!parse_config_file("config.ini"))
    return false;

  if(argc >= 2 && check_filename_string(argv[1]))
  {
    char buf[256];
    if(snprintf(buf, sizeof(buf), "%s", argv[1]) < (int)sizeof(buf))
    {
      if(!parse_config_file(buf))
        return false;

      cmd_start++;
    }
  }

  if(argc >= cmd_start)
  {
    if(!parse_config(argc - cmd_start, argv + cmd_start))
      return false;
  }
  return true;
}

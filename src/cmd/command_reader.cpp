/*
 * hCraft 2 - A revised custom Minecraft server.
 * Copyright (C) 2015 Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cmd/command_reader.hpp"
#include <sstream>
#include <cctype>
#include <memory>
#include <cstdlib>


namespace hc {
  
  command_reader::~command_reader ()
  {
    for (argument *arg : this->args)
      delete arg;
    for (auto& opt : this->opts)
      delete opt.arg;
  }
  
  
  
//------------------------------------------------------------------------------
  
#define _EOF std::istringstream::traits_type::eof ()
  
  
  void
  command_reader::skip_whitespace ()
  {
    int spc = 0;
    int c;
    for (;;)
      {
        c = this->ss->peek ();
        if (c == _EOF || !std::isspace (c))
          break;
        this->ss->get ();
        ++ spc;
      }
    
    this->spc = spc;
  }
  
  
  
  command_reader::argument*
  command_reader::parse_string ()
  {
    this->ss->get (); // skip "
    
    int c;
    std::unique_ptr<argument> arg { new argument () };
    arg->spc = this->spc;
    for (;;)
      {
        c = this->ss->get ();
        if (c == '"')
          break;
        else if (c == _EOF)
          throw command_parse_error ("encountered unterminated string");
        
        if (c == '\\')
          {
            // handle escape sequence
            c = this->ss->get ();
            if (c == _EOF)
              throw command_parse_error ("expected escape sequence after '\\'");
            
            switch (c)
              {
              case '\\': arg->val.push_back ('\\'); break;
              case '"': arg->val.push_back ('"'); break;
              
              default:
                throw command_parse_error ("invalid escape sequence in string");
              }
          }
        
        arg->val.push_back (c);
      }
    
    return arg.release ();
  }
  
  command_reader::argument*
  command_reader::parse_arg ()
  {
    int c = this->ss->peek ();
    if (c == _EOF)
      return nullptr;
    else if (c == '"')
      return this->parse_string ();
    
    std::unique_ptr<argument> arg { new argument () };
    arg->spc = this->spc;
    for (;;)
      {
        c = this->ss->peek ();
        if (c == _EOF || std::isspace (c))
          break;
        
        arg->val.push_back (this->ss->get ());
      }
    
    return arg.release ();
  }
  
  
  void
  command_reader::parse_long_opt ()
  {
    this->ss->get (); // skip -
    
    int c = this->ss->peek ();
    if (std::isspace (c) || c == _EOF)
      {
        // encountered --
        // end of options
        this->p_opts = false;
        return;
      }
    
    std::string name;
    for (;;)
      {
        c = this->ss->peek ();
        if (c == _EOF || c == '=' || std::isspace (c))
          break;
        
        name.push_back (this->ss->get ());
      }
    
    option *opt = nullptr;
    for (auto& op : this->opts)
      if (op.long_name == name)
        {
          opt = &op;
          break;
        }
    
    if (!opt)
      throw command_parse_error ("unknown option --" + name);
    
    opt->enc = true;
    
    if (c == '=')
      {
        // read argument
        this->ss->get ();
        opt->arg = this->parse_arg ();
      }
    
    if (opt->arg_req && !opt->arg)
      throw command_parse_error (
        "expected argument after option (--" + opt->long_name + ")");
  }
  
  void
  command_reader::parse_opt ()
  {
    this->ss->get (); // skip -
    
    if (this->ss->peek () == '-')
      return this->parse_long_opt ();
    
    std::vector<option *> enc_opts;
    for (;;)
      {
        int optc = this->ss->peek ();
        if (!std::isalpha (optc))
          break;
        this->ss->get ();
        
        bool found = false;
        for (auto& opt : this->opts)
          {
            if (opt.short_name == optc)
              {
                opt.enc = true;
                enc_opts.push_back (&opt);
                found = true;
                break;
              }
          }
        
        if (!found)
          throw command_parse_error ("unknown option -" + std::string (1, (char)optc));
      }
    
    if (enc_opts.size () > 1)
      {
        for (option *opt : enc_opts)
          if (opt->arg_req)
            throw command_parse_error ("options must not take arguments when grouped");
        return;
      }
    
    option *opt = enc_opts.front ();
    if (opt->arg_req)
      {
        this->skip_whitespace ();
        opt->arg = this->parse_arg ();
        if (!opt->arg)
          throw command_parse_error (
            "expected argument after option (-" + std::string (1, opt->short_name) + ")");
      }
  }
  
  
  void
  command_reader::parse_next ()
  {
    int c = this->ss->peek ();
    if (c == _EOF)
      return;
    
    if (c == '-')
      {
        this->ss->get ();
        c = this->ss->peek ();
        this->ss->unget ();
        if (c == '-' || std::isalpha (c))
          {
            if (this->p_opts)
              return this->parse_opt ();
          }
      }
    
    this->args.push_back (this->parse_arg ());
  }
  
  
  
  /* 
   * Parses the specified input string.
   * 
   * Throws `command_parse_error' on failure.
   */
  void
  command_reader::parse (const std::string& input)
  {
    std::unique_ptr<std::istringstream> ss { new std::istringstream (input) };
    this->ss = ss.get ();
    this->p_opts = true;
    this->spc = 0;
    
    for (;;)
      {
        this->skip_whitespace ();
        if (ss->peek () == _EOF)
          break;
        
        this->parse_next ();
      }
    
    this->argi = 0;
  }
  
  
  
  /*
   * Registers a new option.
   */
  void
  command_reader::add_opt (char short_name, const char *long_name,
    bool arg_req, bool req)
  {
    this->opts.emplace_back (short_name, long_name, arg_req, req);
  }
  
  /*
   * Finds and returns the option with the given name.
   */
  command_reader::option&
  command_reader::opt (const char *long_name)
  {
    for (auto& opt: this->opts)
      if (opt.long_name == long_name)
        return opt;
    
    throw std::runtime_error ("opt: unknown option " + std::string (long_name));
  }
  
  
  
  /* 
   * Returns false if all arguments have been read using next(); otherwise,
   * returns true.
   */
  bool
  command_reader::has_next ()
  {
    return this->argi < (int)this->args.size ();
  }
  
  /* 
   * Returns a reference to the next argment.
   * NOTE: has_next() must be checked before calling next()!
   */
  const command_reader::argument&
  command_reader::next ()
  {
    return *this->args[this->argi ++];
  }
  
  
  
//------------------------------------------------------------------------------
  
  bool
  command_reader::argument::is_int () const
  {
    for (char c : this->val)
      if (!std::isdigit (c))
        return false;
    return true;
  }
  
  int
  command_reader::argument::as_int () const
  {
    return std::atoi (this->val.c_str ());
  }
  
  
  bool
  command_reader::argument::is_double () const
  {
    int dot = 0;
    for (char c : this->val)
      {
        if (c == '.')
          {
            if (++dot > 1)
              return false;
          }
        else if (!std::isdigit (c))
          return false;
      }
    
    return true;
  }
  
  double
  command_reader::argument::as_double () const
  {
    return std::strtod (this->val.c_str (), nullptr);
  }
}


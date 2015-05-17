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

#ifndef _hCraft2__CMD__COMMAND_PARSER__H_
#define _hCraft2__CMD__COMMAND_PARSER__H_

#include <string>
#include <stdexcept>
#include <string>
#include <iosfwd>
#include <vector>


namespace hc {
  
  class command_parse_error: public std::runtime_error
  {
  public:
    command_parse_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  
  /* 
   * Used to parse and supply input to a command handler during execution.
   */
  class command_reader
  {
  public:
    class argument
    {
      friend class command_reader;
      
      std::string val;
      int spc;        // space count before arg
      
    public:
      inline const std::string& str () const { return this->val; }
    
      bool is_int () const;
      int as_int () const;
      
      bool is_double () const;
      double as_double () const;
    };
    
    class option
    {
      friend class command_reader;
      
      char short_name;
      std::string long_name;
      bool arg_req;
      bool req;
      
      argument *arg;
      bool enc;
      
    public:
      inline bool found () const { return this->enc; }
      inline bool got_arg () const { return this->arg != nullptr; }
      inline argument* get_arg () { return this->arg; }
      
    public:
      option (char short_name, const char *long_name, bool arg_req, bool req)
        : short_name (short_name), long_name (long_name), arg_req (arg_req), req (req)
        { this->enc = false; this->arg = nullptr; }
    };
    
  private:
    std::vector<option> opts;
    std::vector<argument *> args;
    int argi; // arg index
    
    // parse state:
    std::istringstream *ss;
    bool p_opts; // false if stopped parsing opts (after --)
    int spc;     // space count before last arg
    
  public:
    inline int arg_count () const { return (int)this->args.size (); }
    inline bool got_args () const { return !this->args.empty (); }
    
  public:
    ~command_reader ();
    
  private:
    void skip_whitespace ();
    
    void parse_next ();
    
    argument* parse_string ();
    
    argument* parse_arg ();
    
    void parse_opt ();
    
    void parse_long_opt ();
  
  public:
    /* 
     * Parses the specified input string.
     * 
     * Throws `command_parse_error' on failure.
     */
    void parse (const std::string& input);
    
    
    /*
     * Registers a new option.
     */
    void add_opt (char short_name, const char *long_name, bool arg_req,
      bool required);
    
    /*
     * Finds and returns the option with the given name.
     */
    option& opt (const char *long_name);
    
  public:
    /* 
     * Returns false if all arguments have been read using next(); otherwise,
     * returns true.
     */
    bool has_next ();
    
    /* 
     * Returns a reference to the next argment.
     * NOTE: has_next() must be checked before calling next()!
     */
    const argument& next ();
  };
}

#endif


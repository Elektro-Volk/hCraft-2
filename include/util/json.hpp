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

#ifndef _hCraft2__UTIL__JSON__H_
#define _hCraft2__UTIL__JSON__H_

#include <unordered_map>
#include <vector>
#include <cstddef>
#include <string>
#include <iosfwd>
#include <stdexcept>


namespace hc {
  
  namespace json {
    
    struct json_format
    {
      // general:
      int indent;
      
      // containers:
      bool elem_per_line;    // put elements in separate lines
      int elem_dist;         // number of lines between elements
      int comma_spaces;      // number of spaces after ,
      
      // objects:
      int col_space_before;  // number of spaces before :
      int col_space_after;   // number of spaces after :
      bool brace_in_line;    // whether to put first { in its own line.
      
      // arrays:
      bool bracket_in_line;  // whether to put first [ in its own line.
      int elems_per_line;    // number of elements per line
      
    public:
      static void init_packed (json_format& fmt);
      static void init_nice (json_format& fmt);
    };
    
    
    enum value_type
    {
      JSON_NULL,
      JSON_BOOL,
      JSON_NUMBER,
      JSON_STRING,
      JSON_ARRAY,
      JSON_OBJECT,
    };
    
    
    // forward decs:
    class j_object;
    class j_array;
    class j_bool;
    
    /* 
     * Base class of all JSON types.
     */
    class j_value
    {
    public:
      virtual ~j_value () { }
      
    public:
      virtual value_type type () const = 0;
      
    public:
      double as_number () const;
      std::string as_string () const;
      j_object* as_object ();
      j_array* as_array ();
      j_bool* as_bool ();
    };
    
    
    class j_object: public j_value
    {
      std::unordered_map<std::string, j_value *> vals;
      
    public:
      inline size_t size () const { return this->vals.size (); }
      inline decltype (j_object::vals)& get_values () { return this->vals; }
      
      virtual value_type type () const override { return JSON_OBJECT; }
      
    public:
      ~j_object ()
      {
        for (auto p : vals)
          delete p.second;
      }
      
    public:
      void
      set (const std::string& name, j_value *val)
        { this->vals[name] = val; }
      
      j_value*
      get (const std::string& str)
      {
        auto itr = this->vals.find (str);
        return (itr == this->vals.end ()) ? nullptr : itr->second;
      }
    };
    
    
    class j_array: public j_value
    {
      std::vector<j_value *> vals;
      
    public:
      inline size_t size () const { return this->vals.size (); }
      inline decltype (j_array::vals)& get_values () { return this->vals; }
      
      virtual value_type type () const override { return JSON_ARRAY; }
      
    public:
      ~j_array ()
      {
        for (j_value *v : vals)
          delete v;
      }
      
    public:
      void
      add (j_value *val)
        { this->vals.push_back (val); }
      
      void
      set (int i, j_value *val)
        { this->vals[i] = val; }
      
      j_value*
      get (int i)
        { return (i >= (int)this->vals.size ()) ? nullptr : this->vals[i]; }
    };
    
    
    class j_string: public j_value
    {
      std::string val;
      
    public:
      virtual value_type type () const override { return JSON_STRING; }
      
    public:
      j_string (const std::string& str)
        : val (str)
        { }
      
    public:
      const std::string&
      get () const
        { return this->val; }
      
      void
      set (const std::string& str)
        { this->val = str; }
    };
    
    
    class j_number: public j_value
    {
      double val;
      
    public:
      virtual value_type type () const override { return JSON_NUMBER; }
      
    public:
      j_number (double val)
        : val (val)
        { }
      
    public:
      double
      get () const
        { return this->val; }
      
      void
      set (double v)
        { this->val = v; }
    };
    
    
    class j_bool: public j_value
    {
      bool val;
      
    public:
      virtual value_type type () const override { return JSON_BOOL; }
      
    public:
      j_bool (bool val)
        : val (val)
        { }
      
    public:
      bool
      get () const
        { return this->val; }
      
      void
      set (bool v)
        { this->val = v; }
    };
    
    
    class j_null: public j_value
    {
    public:
      virtual value_type type () const override { return JSON_NULL; }
    };
  }
  
//------------------------------------------------------------------------------
  
  class json_parse_error: public std::runtime_error
  {
    int ln, col;
    
  public:
    inline int line () const { return this->ln; }
    inline int column () const { return this->col; }
    
  public:
    json_parse_error (const std::string& str, int ln, int col)
      : std::runtime_error (str)
    {
      this->ln = ln;
      this->col = col;
    }
  };
  
  class json_reader
  {
    std::istream& strm;
    
  public:
    json_reader (std::istream& strm);
    
  public:
    /* 
     * Throws `json_parse_error' on failure.
     */
    json::j_object* read ();
  };
  
  
  
//------------------------------------------------------------------------------
  
  class json_writer
  {
    json::json_format def_fmt;
    std::ostream& strm;
    
  public:
    inline json::json_format& fmt () { return this->def_fmt; }
    
  public:
    json_writer (std::ostream& strm);
    
  public:
    void write (json::j_object *obj);
  };
}

#endif


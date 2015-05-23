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

#include "util/json.hpp"
#include <istream>
#include <stack>
#include <cctype>
#include <memory>
#include <sstream>


namespace hc {
  
  namespace json {
    
    void
    json_format::init_packed (json_format& fmt)
    {
      fmt.indent = 0;
      fmt.elem_per_line = false;
      fmt.elem_dist = 0;
      fmt.col_space_before = 0;
      fmt.col_space_after = 0;
      fmt.brace_in_line = false;
      fmt.bracket_in_line = false;
      fmt.elems_per_line = 1;
      fmt.comma_spaces = 0;
    }
    
    void
    json_format::init_nice (json_format& fmt)
    {
      fmt.indent = 4;
      fmt.elem_per_line = true;
      fmt.elem_dist = 0;
      fmt.col_space_before = 0;
      fmt.col_space_after = 1;
      fmt.brace_in_line = false;
      fmt.bracket_in_line = false;
      fmt.elems_per_line = 2;
      fmt.comma_spaces = 1;
    }
    
    
    
    double
    j_value::as_number () const
    {
      if (this->type () != JSON_NUMBER)
        return 0.0;
      return (static_cast<const j_number *> (this))->get ();
    }
    
    std::string
    j_value::as_string () const
    {
      if (this->type () != JSON_STRING)
        return "";
      return (static_cast<const j_string *> (this))->get ();
    }
    
    j_object*
    j_value::as_object ()
    {
      if (this->type () != JSON_OBJECT)
        return nullptr;
      return (static_cast<j_object *> (this));
    }
    
    j_array*
    j_value::as_array ()
    {
      if (this->type () != JSON_ARRAY)
        return nullptr;
      return (static_cast<j_array *> (this));
    }
    
    bool
    j_value::as_bool ()
    {
      if (this->type () != JSON_BOOL)
        return nullptr;
      return (static_cast<j_bool *> (this))->get ();
    }
  }
  
  
  
//------------------------------------------------------------------------------
  
  namespace {
    
#ifndef EOF
# define EOF (-1)
#endif
    
    class lexer_stream
    {
      std::istream& strm;
      int ln, col;
      std::stack<int> pcol;
      
    public:
      inline int line () const { return this->ln; }
      inline int column () const { return this->col; }
      
    public:
      lexer_stream (std::istream& strm)
        : strm (strm)
      {
        this->ln = this->col = 1;
      }
      
    public:
      int
      get ()
      {
        int n = this->strm.get ();
        if (n == std::istream::traits_type::eof ())
          return EOF;
        
        if (n == '\n')
          {
            ++ this->ln;
            this->pcol.push (this->col);
            this->col = 1;
          }
        else
          ++ this->col;
        
        return n;
      }
      
      int
      peek ()
      {
        int n = this->strm.peek ();
        if (n == std::istream::traits_type::eof ())
          return EOF;
        return n;
      }
      
      void
      unget ()
      {
        this->strm.seekg (-1, std::ios_base::cur);
        if (this->peek () == '\n')
          {
            -- this->ln;
            this->col = this->pcol.top ();
            this->pcol.pop ();
          }
        else
          -- this->col;
      }
    };
  }
  
  
  
//------------------------------------------------------------------------------
  
  json_reader::json_reader (std::istream& strm)
    : strm (strm)
  {
  }
  
  
  
  static void
  _skip_white (lexer_stream& strm)
  {
    int c;
    while ((c = strm.peek ()) != EOF && std::isspace (c))
      strm.get ();
  }
  
  static void
  _read_str (std::string& out, lexer_stream& strm)
  {
    _skip_white (strm);
    
    if (strm.get () != '"')
      throw json_parse_error ("expected '\"' at start of JSON string",
        strm.line (), strm.column ());
    
    int c;
    for (;;)
      {
        c = strm.get ();
        if (c == EOF)
          throw json_parse_error ("unexpected EOF inside JSON string",
            strm.line (), strm.column ());
        else if (c == '"')
          break;
        
        if (c == '\\')
          {
            c = strm.get ();
            switch (c)
              {
              case '"': out.push_back ('"'); break;
              case '\\': out.push_back ('\\'); break;
              case '/': out.push_back ('/'); break;
              case 'b': out.push_back ('\b'); break;
              case 'f': out.push_back ('\f'); break;
              case 'n': out.push_back ('\n'); break;
              case 'r': out.push_back ('\r'); break;
              case 't': out.push_back ('\t'); break;
              
              case EOF:
                throw json_parse_error ("unexpected EOF inside JSON string",
                  strm.line (), strm.column ());
              }
          }
        else
          out.push_back ((char)c);
      }
  }
  
  
  
  // forward dec:
  static json::j_value* _read (lexer_stream& strm);
  
  static json::j_object*
  _read_object (lexer_stream& strm)
  {
    int c;
    _skip_white (strm);
    
    if (strm.get () != '{')
      throw json_parse_error ("expected '{' at start of JSON object",
        strm.line (), strm.column ());
    
    std::unique_ptr<json::j_object> obj { new json::j_object () };
    
    for (;;)
      {
        _skip_white (strm);
        if (strm.peek () == '}')
          {
            strm.get ();
            break;
          }
        
        std::string key;
        _read_str (key, strm);
        
        _skip_white (strm);
        if (strm.get () != ':')
          throw json_parse_error ("expected ':' between name/value pair",
            strm.line (), strm.column ());
        
        _skip_white (strm);
        json::j_value *val = _read (strm);
        obj->set (key, val);
        
        _skip_white (strm);
        c = strm.peek ();
        if (c == ',')
          strm.get ();
        else if (c != '}')
          throw json_parse_error ("expected ',' or '}' after name/value pair",
            strm.line (), strm.column ());
      }
    
    return obj.release ();
  }
  
  static json::j_array*
  _read_array (lexer_stream& strm)
  {
    int c;
    _skip_white (strm);
    
    if (strm.get () != '[')
      throw json_parse_error ("expected '[' at start of JSON array",
        strm.line (), strm.column ());
    
    std::unique_ptr<json::j_array> arr { new json::j_array () };
    
    for (;;)
      {
        _skip_white (strm);
        if (strm.peek () == ']')
          {
            strm.get ();
            break;
          }
        
        json::j_value *val = _read (strm);
        arr->add (val);
        
        _skip_white (strm);
        c = strm.peek ();
        if (c == ',')
          strm.get ();
        else if (c != ']')
          throw json_parse_error ("expected ',' or ']' after array element",
            strm.line (), strm.column ());
      }
    
    return arr.release ();
  }
  
  static json::j_string*
  _read_string (lexer_stream& strm)
  {
    std::string str;
    _read_str (str, strm);
    return new json::j_string (str);
  }
  
  static json::j_number*
  _read_number (lexer_stream& strm)
  {
    std::string str;
    int c;
    
    for (;;)
      {
        c = strm.peek ();
        if (c == EOF)
          break;
        
        if (std::isdigit (c) || c == '.' || c == '-')
          str.push_back ((char)strm.get ());
        else
          break;
      }
    
    double v;
    std::istringstream ss (str);
    ss >> v;
    return new json::j_number (v);
  }
  
  static json::j_bool*
  _read_bool (lexer_stream& strm)
  {
    _skip_white (strm);
    int c = strm.get ();
    if (c == 't')
      {
        if (strm.get () != 'r' ||
            strm.get () != 'u' ||
            strm.get () != 'e')
          throw json_parse_error ("expected 'true' or 'false'",
            strm.line (), strm.column ());
         
        return new json::j_bool (true);
      }
    else if (c == 'f')
      {
        if (strm.get () != 'a' ||
            strm.get () != 'l' ||
            strm.get () != 's' ||
            strm.get () != 'e')
          throw json_parse_error ("expected 'true' or 'false'",
            strm.line (), strm.column ());
        
        return new json::j_bool (false);
      }
    else
      throw json_parse_error ("expected 'true' or 'false'",
        strm.line (), strm.column ());
  }
  
  static json::j_null*
  _read_null (lexer_stream& strm)
  {
    _skip_white (strm);
    if (strm.get () != 'n' ||
        strm.get () != 'u' ||
        strm.get () != 'l' ||
        strm.get () != 'l')
      throw json_parse_error ("expected 'null'", strm.line (), strm.column ());
    
    return new json::j_null ();
  }
  
  json::j_value*
  _read (lexer_stream& strm)
  {
    _skip_white (strm);
    int c = strm.peek ();
    switch (c)
      {
      case '{': return _read_object (strm);
      case '[': return _read_array (strm);
      case '"': return _read_string (strm);
      
      case 't':
      case 'f':
        return _read_bool (strm);
      
      case 'n': return _read_null (strm);
      }
    
    if (std::isdigit (c))
      return _read_number (strm);
    
   throw json_parse_error ("invalid JSON value", strm.line (), strm.column ());
  }
  
  
  
  /* 
   * Throws `json_parse_error' on failure.
   */
  json::j_object*
  json_reader::read ()
  {
    lexer_stream ls (this->strm);
    return _read_object (ls);
  }
  
  
  
//------------------------------------------------------------------------------
 
  json_writer::json_writer (std::ostream& strm)
     : strm (strm)
  {
    json::json_format::init_packed (this->def_fmt);
  }
  
  
  
  namespace {
    
    struct write_state
    {
      int depth;
    };
  }
  
  static void
  _encode_str (std::ostream& strm, const std::string& str)
  {
    strm << '"';
    
    for (char c : str)
      {
        switch (c)
          {
          case '"':    strm << "\\\""; break;
          case '\\':    strm << "\\\\"; break;
          case '\b':    strm << "\\b"; break;
          case '\f':    strm << "\\f"; break;
          case '\n':    strm << "\\n"; break;
          case '\r':    strm << "\\r"; break;
          case '\t':    strm << "\\t"; break;
          
          default:
            strm << c;
            break;
          }
      }
    
    strm << '"';
  }
  
  
  // forward decs:
  static void _write (std::ostream& strm, json::j_value *val,
    const json::json_format& fmt, write_state& st);
  
  
  static void
  _write_object (std::ostream& strm, json::j_object *obj,
    const json::json_format& fmt, write_state& st)
  {
    if (fmt.brace_in_line)
      {
        strm << '\n';
        strm << std::string (fmt.indent * st.depth, ' ') << '{';
      }
    else
      strm << '{';
    
    if (fmt.elem_per_line)
      strm << '\n';
    
    int i = 0;
    int count = (int)obj->size ();
    for (auto p : obj->get_values ())
      {
        if (fmt.indent)
          strm << std::string (fmt.indent * (st.depth + 1), ' ');
        
        _encode_str (strm, p.first);
        
        strm << std::string (fmt.col_space_before, ' ');
        strm << ':';
        strm << std::string (fmt.col_space_after, ' ');
        
        ++ st.depth;
        _write (strm, p.second, fmt, st);
        -- st.depth;
        
        if (i != count - 1)
          {
            strm << ',';
            strm << std::string (fmt.comma_spaces, ' ');
            strm << std::string (fmt.elem_dist, '\n');
          }
        
        if (fmt.elem_per_line)
          strm << '\n';
        
        ++ i;
      }
    
    strm << std::string (fmt.indent * st.depth, ' ') << '}';
  }
  
  
  static void
  _write_array (std::ostream& strm, json::j_array *arr,
    const json::json_format& fmt, write_state& st)
  {
    if (fmt.bracket_in_line)
      {
        strm << '\n';
        strm << std::string (fmt.indent * st.depth, ' ') << '[';
      }
    else
      strm << '[';
    
    if (fmt.elem_per_line)
      strm << '\n';
    
    int count = (int)arr->get_values ().size ();
    for (int i = 0; i < (int)arr->size (); ++i)
      {
        if (fmt.indent)
          if (i % fmt.elems_per_line == 0)
            strm << std::string (fmt.indent * (st.depth + 1), ' ');
        
        ++ st.depth;
        _write (strm, arr->get (i), fmt, st);
        -- st.depth;
        
        if (i != count - 1)
          {
            strm << ',';
            strm << std::string (fmt.comma_spaces, ' ');
            strm << std::string (fmt.elem_dist, '\n');
          }
        
        if (fmt.elem_per_line)
          if ((i == count - 1) || ((i + 1) % fmt.elems_per_line == 0))
            strm << '\n';
      }
    
    strm << std::string (fmt.indent * st.depth, ' ') << ']';
  }
  
  
  static void
  _write_string (std::ostream& strm, json::j_string *val,
    const json::json_format& fmt, write_state& st)
  {
    _encode_str (strm, val->get ());
  }
  
  
  static void
  _write_number (std::ostream& strm, json::j_number *val,
    const json::json_format& fmt, write_state& st)
  {
    strm << val->get ();
  }
  
  
  static void
  _write_bool (std::ostream& strm, json::j_bool *val,
    const json::json_format& fmt, write_state& st)
  {
    strm << (val->get () ? "true" : "false");
  }
  
  
  static void
  _write_null (std::ostream& strm, json::j_null *val,
    const json::json_format& fmt, write_state& st)
  {
    strm << "null";
  }
  
  
  static void
  _write (std::ostream& strm, json::j_value *val, const json::json_format& fmt,
    write_state& st)
  {
    switch (val->type ())
      {
      case json::JSON_OBJECT:
        _write_object (strm, static_cast<json::j_object *> (val), fmt, st);
        break;
      
      case json::JSON_ARRAY:
        _write_array (strm, static_cast<json::j_array *> (val), fmt, st);
        break;
      
      case json::JSON_STRING:
        _write_string (strm, static_cast<json::j_string *> (val), fmt, st);
        break;
      
      case json::JSON_NUMBER:
        _write_number (strm, static_cast<json::j_number *> (val), fmt, st);
        break;
      
      case json::JSON_BOOL:
        _write_bool (strm, static_cast<json::j_bool *> (val), fmt, st);
        break;
      
      case json::JSON_NULL:
        _write_null (strm, static_cast<json::j_null *> (val), fmt, st);
        break;
      
      default: ;
      }
  }
  
  
  
  void
  json_writer::write (json::j_object *obj)
  {
    write_state st;
    st.depth = 0;
    
    _write_object (this->strm, obj, this->def_fmt, st);
  }
}


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

#ifndef _hCraft2__UTIL__NBT__H_
#define _hCraft2__UTIL__NBT__H_

#include "util/byte_buffer.hpp"
#include <iosfwd>
#include <string>
#include <stack>
#include <stdexcept>
#include <vector>
#include <unordered_map>


namespace hc {
  
  enum nbt_tag_type
  {
    NBT_TAG_END = 0,
    NBT_TAG_BYTE,
    NBT_TAG_SHORT,
    NBT_TAG_INT,
    NBT_TAG_LONG,
    NBT_TAG_FLOAT,
    NBT_TAG_DOUBLE,
    NBT_TAG_BYTE_ARRAY,
    NBT_TAG_STRING,
    NBT_TAG_LIST,
    NBT_TAG_COMPOUND,
    NBT_TAG_INT_ARRAY,
  };
  
  
  
//------------------------------------------------------------------------------
  
  // forward decs:
  class nbt_tag_compound;
  class nbt_tag_list;
  class nbt_tag_byte_array;
  class nbt_tag_int_array;
  
  class nbt_tag
  {
  protected:
    std::string name;
    
  public:
    virtual nbt_tag_type get_type () const = 0;
    
    inline const std::string& get_name () const { return this->name; }
    inline void set_name (const std::string& name) { this->name = name; }
    
  public:
    virtual ~nbt_tag () { }
    
  public:
    // convenient methods that perform type casting:
    
    unsigned char as_byte () const;
    short as_short () const;
    int as_int () const;
    long long as_long () const;
    float as_float () const;
    double as_double () const;
    std::string as_string () const;
    nbt_tag_byte_array* as_byte_array ();
    nbt_tag_int_array* as_int_array ();
    nbt_tag_list* as_list ();
    nbt_tag_compound* as_compound ();
  };
  
  class nbt_tag_byte: public nbt_tag
  {
    char val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_BYTE; }
    inline char get_value () const { return this->val; }
    
  public:
    nbt_tag_byte (const char val)
      : val (val)
      { }
  };
  
  class nbt_tag_short: public nbt_tag
  {
    short val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_SHORT; }
    inline short get_value () const { return this->val; }
    
  public:
    nbt_tag_short (short val)
      : val (val)
      { }
  };
  
  class nbt_tag_int: public nbt_tag
  {
    int val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_INT; }
    inline int get_value () const { return this->val; }
    
  public:
    nbt_tag_int (int val)
      : val (val)
      { }
  };
  
  class nbt_tag_long: public nbt_tag
  {
    long long val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_LONG; }
    inline long long get_value () const { return this->val; }
    
  public:
    nbt_tag_long (long long val)
      : val (val)
      { }
  };
  
  class nbt_tag_float: public nbt_tag
  {
    float val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_FLOAT; }
    inline float get_value () const { return this->val; }
    
  public:
    nbt_tag_float (float val)
      : val (val)
      { }
  };
  
  class nbt_tag_double: public nbt_tag
  {
    double val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_DOUBLE; }
    inline double get_value () const { return this->val; }
    
  public:
    nbt_tag_double (double val)
      : val (val)
      { }
  };
  
  class nbt_tag_string: public nbt_tag
  {
    std::string val;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_STRING; }
    inline const std::string& get_value () const { return this->val; }
    
  public:
    nbt_tag_string (const std::string& val)
      : val (val)
      { }
  };
  
  class nbt_tag_byte_array: public nbt_tag
  {
    unsigned char *data;
    unsigned int len;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_BYTE_ARRAY; }
    inline const unsigned char* get_data () const { return this->data; }
    inline unsigned int get_length () const { return this->len; }
    
  public:
    nbt_tag_byte_array (const unsigned char *data, unsigned int len);
    ~nbt_tag_byte_array ();
  };
  
  class nbt_tag_int_array: public nbt_tag
  {
    int *data;
    unsigned int len;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_INT_ARRAY; }
    inline const int* get_data () const { return this->data; }
    inline unsigned int get_length () const { return this->len; }
    
  public:
    nbt_tag_int_array (const int *data, unsigned int len);
    ~nbt_tag_int_array ();
  };
  
  class nbt_tag_list: public nbt_tag
  {
    nbt_tag_type eltype;
    std::vector<nbt_tag *> tags;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_LIST; }
    inline nbt_tag_type get_value_type () const { return this->eltype; }
    inline std::vector<nbt_tag *>& get_values () { return this->tags; }
    
  public:
    ~nbt_tag_list ();
  };
  
  class nbt_tag_compound: public nbt_tag
  {
    std::unordered_map<std::string, nbt_tag *> tags;
    
  public:
    virtual nbt_tag_type get_type () const override { return NBT_TAG_COMPOUND; }
    inline std::unordered_map<std::string, nbt_tag *>& get_map () { return this->tags; }
    
  public:
    ~nbt_tag_compound ();
  
  public:
    nbt_tag* get (const std::string& name);
  };
  
  
  

//------------------------------------------------------------------------------
  
  enum nbt_compression_strategy
  {
    NBT_RAW,
    NBT_ZLIB,
    NBT_GZIP,
  };
  
  
  /* 
   * Can be thrown by nbt_writer when compression fails.
   */
  class nbt_write_error: public std::runtime_error
  {
  public:
    nbt_write_error (const std::string& str)
    : std::runtime_error (str)
      { }
  };
  
  
  /* 
   * Writes NBT data onto a byte stream.
   */
  class nbt_writer
  {
  private:
    struct frame
    {
      nbt_tag_type type;
      int len_pos;
      int len;
    };
    
  private:
    byte_buffer buf;
    std::stack<frame> frms;
    
  public:
    nbt_writer ();
    ~nbt_writer ();
    
  public:
    void put_byte (unsigned char val, const char *name = nullptr);
    void put_short (unsigned short val, const char *name = nullptr);
    void put_int (unsigned int val, const char *name = nullptr);
    void put_long (unsigned long long val, const char *name = nullptr);
    void put_float (float val, const char *name = nullptr);
    void put_double (double val, const char *name = nullptr);
    void put_byte_array (const unsigned char *arr, int len, const char *name = nullptr);
    void put_int_array (const int *arr, int len, const char *name = nullptr);
    void put_string (const std::string& val, const char *name = nullptr);
    
    void start_list (int tag_type, const char *name = nullptr);
    void end_list ();
    
    void start_compound (const char *name = nullptr);
    void end_compound ();
    
  public:
    /* 
     * Commits the data created in memory to the specified stream, optionally
     * compressing it.
     * 
     * Throws exceptions of type `nbt_write_error' on failure.
     */
    void write (std::ostream& strm, nbt_compression_strategy strategy = NBT_RAW);
  };



//------------------------------------------------------------------------------
  
  /* 
   * Thrown by nbt_reader when it can't properly parse the input stream.
   */
  class nbt_read_error: public std::runtime_error
  {
  public:
    nbt_read_error (const std::string& str)
    : std::runtime_error (str)
      { }
  };
  
  
  /* 
   * Reads NBT data from a stream.
   */
  class nbt_reader
  {
  public:
    nbt_reader ();
    
  public:
    /* 
     * Reads and returns an NBT Compound tag from the specified byte array.
     * The returned object must be destroyed by the caller.
     * 
     * Throws exceptions of type `nbt_reader' on failure.
     * Returns null if there are no more tags to read.
     */
    nbt_tag_compound* read (const unsigned char *data, int len,
      nbt_compression_strategy strategy);
  };
}

#endif


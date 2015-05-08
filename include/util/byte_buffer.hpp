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

#ifndef _hCraft2__UTIL__BYTE_BUFFER__H_
#define _hCraft2__UTIL__BYTE_BUFFER__H_


namespace hc {
  
  /* 
   * A dynamically expandable byte array.
   */
  class byte_buffer
  {
    unsigned char *data;
    unsigned int len;
    unsigned int cap;
    unsigned int pos;
    
  public:
    inline unsigned char* get_data () { return this->data; }
    inline unsigned int get_length () { return this->len; }
    inline unsigned int get_capacity () { return this->cap; }
    
    inline unsigned int get_pos () { return this->pos; }
    inline void seek (unsigned int npos) { this->pos = npos; }
    inline void seek_to_end () { this->seek (this->len); }
    inline void seek_to_beg () { this->seek (0); }
    inline void reset () { this->len = this->pos = 0; }
    
  public:
    byte_buffer (unsigned int init_cap = 32);
    ~byte_buffer ();
    
  public:
    /* 
     * Extends the byte buffer so that it could accommodate at least the
     * specified amount of bytes.
     */
    void extend (unsigned int new_cap);
  
  public:
    /* 
     * `put' methods:
     */
//------------------------------------------------------------------------------
 
    void put_bytes (const unsigned char *arr, unsigned int len);
    void put_byte (unsigned char val);
    void put_short_le (unsigned short val);
    void put_short_be (unsigned short val);
    void put_int_le (unsigned int val);
    void put_int_be (unsigned int val);
    void put_long_le (unsigned long long val);
    void put_long_be (unsigned long long val);
    void put_float_le (float val);
    void put_float_be (float val);
    void put_double_le (double val);
    void put_double_be (double val);
    
//------------------------------------------------------------------------------
  };
}

#endif


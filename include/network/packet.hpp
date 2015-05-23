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

#ifndef _hCraft2__NETWORK__PACKET__H_
#define _hCraft2__NETWORK__PACKET__H_

#include <string>


namespace hc {
  
  /* 
   * Implements a convenient interface to extract fields from a packet.
   */
  class packet_reader
  {
    unsigned char *arr;
    unsigned int len;
    unsigned int pos;
    bool copy;
    
  public:
    inline unsigned int length () const { return this->len; }
    inline unsigned int get_pos () const { return this->pos; }
    
    inline void rewind () { this->pos = 0; }
    
  public:
    packet_reader (const unsigned char *arr, unsigned int len, bool copy = false);
    ~packet_reader ();
    
  public:
    /* 
     * `read' methods:
     */
    //--------------------------------------------------------------------------
    
    bool read_bool ();
    unsigned char read_byte ();
    short read_short ();
    int read_int ();
    long long read_long ();
    float read_float ();
    double read_double ();
    int read_varint ();
    long long read_varlong ();
    bool read_string (char *out, int len);
    void read_bytes (unsigned char *out, int len);
    
    //--------------------------------------------------------------------------
  };
  
  
  
  /* 
   * Wraps around a byte array and provides convenient methods to write binary
   * data to it.
   */
  class packet
  {
    unsigned char *arr;
    unsigned int pos;
    unsigned int len;
    unsigned int cap;
    unsigned int rbeg; // number of bytes reserved at the beginning
    
  public:
    inline const unsigned char* get_data () const { return this->arr; }
    inline unsigned int get_pos () const { return this->pos; }
    inline unsigned int get_length () const { return this->len; }
    inline unsigned int get_capacity () const { return this->cap; }
    
    inline void rewind () { this->pos = 0; }
    inline void reset () { this->pos = this->len = 0; }
    
  public:
    packet (unsigned int init_cap = 16, unsigned int reserve_beg = 5);
    ~packet ();
    
  public:
    /* 
     * Expands the packet's internal capacity to the specified value.
     */
    void expand (unsigned int new_cap);
    
    /* 
     * Turns the specified number of bytes of reserved bytes at the beginning
     * of the packet a part of the packet's contents.
     * Sets the position pointer to the beginning of that reserved part.
     */
    void use_reserved (int count);
    
    /* 
     * DEBUG
     */
    void print_hex ();
    
  public:
    /* 
     * `put' methods:
     */
    //--------------------------------------------------------------------------
    
    void put_bool (bool val);
    void put_byte (unsigned char val);
    void put_short (unsigned short val);
    void put_int (unsigned int val);
    void put_long (unsigned long long val);
    void put_float (float val);
    void put_double (double val);
    void put_varint (int val);
    void put_varlong (unsigned long long val);
    void put_string (const std::string& str);
    void put_bytes (const void *ptr, unsigned int len);
    
    //--------------------------------------------------------------------------
  };
}

#endif


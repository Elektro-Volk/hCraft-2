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

#include "network/packet.hpp"
#include "util/binary.hpp"
#include <cstring>


namespace hc {
  
  packet_reader::packet_reader (const unsigned char *arr, unsigned int len, bool copy)
  {
    this->len = len;
    this->pos = 0;
    this->copy = copy;
    
    if (this->copy)
      {
        this->arr = new unsigned char [len];
        std::memcpy (this->arr, arr, len);
      }
    else
      this->arr = const_cast<unsigned char *> (arr);
  }
  
  packet_reader::~packet_reader ()
  {
    if (this->copy)
      delete[] this->arr;
  }
  
  
  
//------------------------------------------------------------------------------
  
  bool
  packet_reader::read_bool ()
  {
    return this->arr[this->pos++] ? true : false;
  }
  
  unsigned char
  packet_reader::read_byte ()
  {
    return this->arr[this->pos++];
  }
  
  short
  packet_reader::read_short ()
  {
    short v = bin::read_short (this->arr + this->pos);
    this->pos += 2;
    return v;
  }
  
  int
  packet_reader::read_int ()
  {
    int v = bin::read_int (this->arr + this->pos);
    this->pos += 4;
    return v;
  }
  
  long long
  packet_reader::read_long ()
  {
    long long v = bin::read_long (this->arr + this->pos);
    this->pos += 8;
    return v;
  }
  
  float
  packet_reader::read_float ()
  {
    float v = bin::read_float (this->arr + this->pos);
    this->pos += 4;
    return v;
  }
  
  double
  packet_reader::read_double ()
  {
    double v = bin::read_double (this->arr + this->pos);
    this->pos += 8;
    return v;
  }
  
  int
  packet_reader::read_varint ()
  {
    int vl;
    int v = bin::read_varint (this->arr + this->pos, &vl);
    this->pos += vl;
    return v;
  }
  
  long long
  packet_reader::read_varlong ()
  {
    int vl;
    long long v = bin::read_varlong (this->arr + this->pos, &vl);
    this->pos += vl;
    return v;
  }
  
  bool
  packet_reader::read_string (char *out, int len)
  {
    int vl;
    if (!bin::read_string (this->arr + this->pos, out, len, &vl))
      return false;
    this->pos += vl;
    return true;
  }
  
  void
  packet_reader::read_bytes (unsigned char *out, int len)
  {
    std::memcpy (out, this->arr + this->pos, len);
    this->pos += len;
  }
  
//------------------------------------------------------------------------------
  
  packet::packet (unsigned int init_cap, unsigned int reserve_beg)
  {
    this->arr = new unsigned char [reserve_beg + init_cap];
    this->cap = init_cap;
    this->len = 0;
    this->pos = 0;
    
    this->rbeg = reserve_beg;
    this->arr += this->rbeg;
  }
  
  packet::~packet ()
  {
    delete[] (this->arr - this->rbeg);
  }
  
  
  
  /* 
   * Expands the packet's internal capacity to the specified value.
   */
  void
  packet::expand (unsigned int new_cap)
  {
    if (this->cap >= new_cap)
      return;
    
    unsigned char *narr = new unsigned char [this->rbeg + new_cap];
    std::memcpy (narr + this->rbeg, this->arr, this->len);
    delete[] (this->arr - this->rbeg);
    
    this->cap = new_cap;
    this->arr = narr;
    this->arr += this->rbeg;
  }
  
  /* 
   * Turns the specified number of bytes of reserved bytes at the beginning
   * of the packet a part of the packet's contents.
   * Sets the position pointer to the beginning of that reserved part.
   */
  void
  packet::use_reserved (int count)
  {
    this->arr -= count;
    this->len += count;
    this->pos = 0;
    this->rbeg -= count;
  }
  
  
  
  void
  packet::put_bool (bool val)
  {
    if ((this->pos + 1) > this->cap)
      this->expand ((this->cap * 16)/10 + 1);
    
    this->arr[this->pos ++] = val ? 1 : 0;
    if (this->pos > this->len)
      this->len = this->pos;
  } 
  
  void
  packet::put_byte (unsigned char val)
  {
    if ((this->pos + 1) > this->cap)
      this->expand ((this->cap * 16)/10 + 1);
    
    this->arr[this->pos ++] = val;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_short (unsigned short val)
  {
    if ((this->pos + 2) > this->cap)
      this->expand ((this->cap * 16)/10 + 2);
    
    bin::write_short (this->arr + this->pos, val);
    this->pos += 2;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_int (unsigned int val)
  {
    if ((this->pos + 4) > this->cap)
      this->expand ((this->cap * 16)/10 + 4);
    
    bin::write_int (this->arr + this->pos, val);
    this->pos += 4;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_long (unsigned long long val)
  {
    if ((this->pos + 8) > this->cap)
      this->expand ((this->cap * 16)/10 + 8);
    
    bin::write_long (this->arr + this->pos, val);
    this->pos += 8;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_float (float val)
  {
    if ((this->pos + 4) > this->cap)
      this->expand ((this->cap * 16)/10 + 4);
    
    bin::write_float (this->arr + this->pos, val);
    this->pos += 4;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_double (double val)
  {
    if ((this->pos + 8) > this->cap)
      this->expand ((this->cap * 16)/10 + 8);
    
    bin::write_double (this->arr + this->pos, val);
    this->pos += 8;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_varint (int val)
  {
    if ((this->pos + 5) > this->cap)
      this->expand ((this->cap * 16)/10 + 5);
    
    int vl = 0;
    bin::write_varint (this->arr + this->pos, val, &vl);
    this->pos += vl;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  packet::put_varlong (unsigned long long val)
  {
    if ((this->pos + 10) > this->cap)
      this->expand ((this->cap * 16)/10 + 10);
    
    int vl = 0;
    bin::write_varlong (this->arr + this->pos, val, &vl);
    this->pos += vl;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  
  static int
  _strlen_upper_bound (const std::string& str)
  {
    return 5 + (int)str.size ();
  }
  
  void
  packet::put_string (const std::string& str)
  {
    int ub = _strlen_upper_bound (str);
    if ((this->pos + ub) > this->cap)
      this->expand ((this->cap * 14)/10 + ub);
    
    int vl = 0;
    bin::write_string (this->arr + this->pos, str.c_str (), &vl);
    this->pos += vl;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  
  void
  packet::put_bytes (const void *ptr, unsigned int len)
  {
    const unsigned char *arr = (const unsigned char *)ptr;
    if ((this->pos + len) > this->cap)
      this->expand ((this->cap * 11)/10 + len);
    
    std::memcpy (this->arr + this->pos, arr, len);
    this->pos += len;
    if (this->pos > this->len)
      this->len = this->pos;
  }
}


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

#include "util/byte_buffer.hpp"
#include <cstring>


namespace hc {
  
  byte_buffer::byte_buffer (unsigned int init_cap)
  {
    this->cap = init_cap;
    this->len = 0;
    this->data = new unsigned char [this->cap];
    this->pos = 0;
  }
  
  byte_buffer::~byte_buffer ()
  {
    delete[] this->data;
  }
  
  
  
  /* 
   * Extends the byte buffer so that it could accommodate at least the
   * specified amount of bytes.
   */
  void
  byte_buffer::extend (unsigned int new_cap)
  {
    if (this->cap >= new_cap)
      return;
    
    unsigned char *narr = new unsigned char [new_cap];
    std::memcpy (narr, this->data, this->cap);
    delete[] this->data;
    this->data = narr;
    this->cap = new_cap;
  }
  
  
  
#define ENSURE_CAP(N) \
  if ((this->len + (N)) > this->cap)  \
    this->extend (this->cap * 15 / 10 + (N));
  
  void
  byte_buffer::put_bytes (const unsigned char *arr, unsigned int len)
  {
    ENSURE_CAP (len)
    
    std::memcpy (this->data + this->pos, arr, len);
    this->pos += len;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_byte (unsigned char val)
  {
    ENSURE_CAP (1)
    
    this->data[this->pos ++] = val;
    
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_short_le (unsigned short val)
  {
    ENSURE_CAP (2)
    
    this->data[this->pos + 0] = val & 0xFF;
    this->data[this->pos + 1] = val >> 8;
    
    this->pos += 2;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_short_be (unsigned short val)
  {
    ENSURE_CAP (2)
    
    this->data[this->pos + 0] = val >> 8;
    this->data[this->pos + 1] = val & 0xFF;
    
    this->pos += 2;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_int_le (unsigned int val)
  {
    ENSURE_CAP (4)
    
    this->data[this->pos + 0] = val & 0xFF;
    this->data[this->pos + 1] = (val >> 8) & 0xFF;
    this->data[this->pos + 2] = (val >> 16) & 0xFF;
    this->data[this->pos + 3] = val >> 24;
    
    this->pos += 4;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_int_be (unsigned int val)
  {
    ENSURE_CAP (4)
    
    this->data[this->pos + 0] = val >> 24;
    this->data[this->pos + 1] = (val >> 16) & 0xFF;
    this->data[this->pos + 2] = (val >> 8) & 0xFF;
    this->data[this->pos + 3] = val & 0xFF;
    
    this->pos += 4;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_long_le (unsigned long long val)
  {
    ENSURE_CAP (8)
    
    this->data[this->pos + 0] = val & 0xFF;
    this->data[this->pos + 1] = (val >> 8) & 0xFF;
    this->data[this->pos + 2] = (val >> 16) & 0xFF;
    this->data[this->pos + 3] = (val >> 24) & 0xFF;
    this->data[this->pos + 4] = (val >> 32) & 0xFF;
    this->data[this->pos + 5] = (val >> 40) & 0xFF;
    this->data[this->pos + 6] = (val >> 48) & 0xFF;
    this->data[this->pos + 7] = val >> 56;
    
    this->pos += 8;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_long_be (unsigned long long val)
  {
    ENSURE_CAP (8)
    
    this->data[this->pos + 0] = val >> 56;
    this->data[this->pos + 1] = (val >> 48) & 0xFF;
    this->data[this->pos + 2] = (val >> 40) & 0xFF;
    this->data[this->pos + 3] = (val >> 32) & 0xFF;
    this->data[this->pos + 4] = (val >> 24) & 0xFF;
    this->data[this->pos + 5] = (val >> 16) & 0xFF;
    this->data[this->pos + 6] = (val >> 8) & 0xFF;
    this->data[this->pos + 7] = val & 0xFF;
    
    this->pos += 8;
    if (this->pos > this->len)
      this->len = this->pos;
  }
  
  void
  byte_buffer::put_float_le (float val)
  {
    this->put_int_le (*((int *)&val));
  }
  
  void
  byte_buffer::put_float_be (float val)
  {
    this->put_int_be (*((int *)&val));
  }
  
  void
  byte_buffer::put_double_le (double val)
  {
    this->put_long_le (*((long long *)&val));
  }
  
  void
  byte_buffer::put_double_be (double val)
  {
    this->put_long_be (*((long long *)&val));
  }
}


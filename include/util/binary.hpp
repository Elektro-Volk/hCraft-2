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

#ifndef _hCraft2__UTIL__BINARY__H_
#define _hCraft2__UTIL__BINARY__H_


namespace hc {
  
  namespace bin {
    
    int read_varint (const void *ptr, int *olen = nullptr);
    
    long long read_varlong (const void *ptr, int *olen = nullptr);
    
    inline bool
    read_bool (const unsigned char *ptr)
      { return *ptr ? true : false; }
    
    inline short
    read_short (const unsigned char *ptr)
      { return (short)(((unsigned short)ptr[0] << 8) |
                        (unsigned short)ptr[1]); }
    
    inline int
    read_int (const unsigned char *ptr)
      { return (int)(((unsigned int)ptr[0] << 24) |
                     ((unsigned int)ptr[1] << 16) |
                     ((unsigned int)ptr[2] << 8)  |
                      (unsigned int)ptr[3]); }
    
    inline long long
    read_long (const unsigned char *ptr)
      { return (long long)(((unsigned long long)ptr[0] << 56) |
                           ((unsigned long long)ptr[1] << 48) |
                           ((unsigned long long)ptr[2] << 40) |
                           ((unsigned long long)ptr[3] << 32) |
                           ((unsigned long long)ptr[4] << 24) |
                           ((unsigned long long)ptr[5] << 16) |
                           ((unsigned long long)ptr[6] << 8)  |
                            (unsigned long long)ptr[7]); }
    
    inline float
    read_float (const unsigned char *ptr)
    {
      unsigned char arr[4];
      arr[0] = ptr[3];
      arr[1] = ptr[2];
      arr[2] = ptr[1];
      arr[3] = ptr[0];
      return *((float *)arr);
    }
    
    inline double
    read_double (const unsigned char *ptr)
    {
      unsigned char arr[8];
      arr[0] = ptr[7];
      arr[1] = ptr[6];
      arr[2] = ptr[5];
      arr[3] = ptr[4];
      arr[4] = ptr[3];
      arr[5] = ptr[2];
      arr[6] = ptr[1];
      arr[7] = ptr[0];
      return *((double *)arr);
    }
    
    /* 
     * Extracts an MC string (UTF-8 string prefixed with its size in bytes as a
     * varint). Returns false is the contained string is not valid.
     */
    bool read_string (const unsigned char *ptr, char *out, int out_len,
      int *olen = nullptr);
    
    
    
//------------------------------------------------------------------------------
    
    inline void
    write_bool (unsigned char *ptr, bool val)
      { *ptr = val ? 1 : 0; }
    
    inline void
    write_byte (unsigned char *ptr, unsigned char val)
      { *ptr = val; }
    
    inline void
    write_short (unsigned char *ptr, unsigned short val)
    {
      ptr[0] = val >> 8;
      ptr[1] = val & 0xFF;
    }
    
    inline void
    write_int (unsigned char *ptr, unsigned int val)
    {
      ptr[0] = val >> 24;
      ptr[1] = (val >> 16) & 0xFF;
      ptr[2] = (val >> 8) & 0xFF;
      ptr[3] = val & 0xFF;
    }
    
    inline void
    write_long (unsigned char *ptr, unsigned long long val)
    {
      ptr[0] = val >> 56;
      ptr[1] = (val >> 48) & 0xFF;
      ptr[2] = (val >> 40) & 0xFF;
      ptr[3] = (val >> 32) & 0xFF;
      ptr[4] = (val >> 24) & 0xFF;
      ptr[5] = (val >> 16) & 0xFF;
      ptr[6] = (val >> 8) & 0xFF;
      ptr[7] = val & 0xFF;
    }
    
    inline void
    write_float (unsigned char *ptr, float f)
    {
      unsigned int val = *((unsigned int *)&f);
      ptr[0] = val >> 24;
      ptr[1] = (val >> 16) & 0xFF;
      ptr[2] = (val >> 8) & 0xFF;
      ptr[3] = val & 0xFF;
    }
    
    inline void
    write_double (unsigned char *ptr, double f)
    {
      unsigned long long val = *((unsigned long long *)&f);
      ptr[0] = val >> 56;
      ptr[1] = (val >> 48) & 0xFF;
      ptr[2] = (val >> 40) & 0xFF;
      ptr[3] = (val >> 32) & 0xFF;
      ptr[4] = (val >> 24) & 0xFF;
      ptr[5] = (val >> 16) & 0xFF;
      ptr[6] = (val >> 8) & 0xFF;
      ptr[7] = val & 0xFF;
    }
    
    void write_varint (unsigned char *ptr, unsigned int val,
      int *olen = nullptr);
    
    void write_varlong (unsigned char *ptr, unsigned long long val,
      int *olen = nullptr);
    
    void write_string (unsigned char *ptr, const char *str,
      int *olen = nullptr);
  
  
  
//------------------------------------------------------------------------------
  
    int got_varint (const void *ptr, int len);
    
    int varint_size (int num);
  }
}

#endif


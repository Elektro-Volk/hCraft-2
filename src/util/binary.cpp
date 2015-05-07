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

#include "util/binary.hpp"
#include <cstring>


namespace hc {
  
  namespace bin {
    
    int
    got_varint (const void *ptr, int len)
    {
      const unsigned char *arr = (const unsigned char *)ptr;
      
      int li = -1;
      for (int i = 0; i < len; ++i)
        {
          if (!(arr[i] & 0x80))
            {
              li = i;
              break;
            }
        }
      
      if (li > 5)
        return -1;
      else if (li == -1)
        return 0;
      return 1;
    }
    
    int
    varint_size (int num)
    {
      if (num == 0)
        return 1;
      
      int s = 0;
      while (num)
        {
          ++ s;
          num >>= 7;
        }
      
      return s;
    }
    
    
    
    int
    read_varint (const void *ptr, int *olen)
    {
      int n = 0;
      const unsigned char *arr = (const unsigned char *)ptr;
      for (int i = 0; i < 5; ++i)
        {
          n |= (arr[i] & 0x7F) << (i * 7);
          if (!(arr[i] & 0x80))
            {
              if (olen)
                *olen = i + 1;
              break;
            }
        }
      
      return n;
    }
    
    long long
    read_varlong (const void *ptr, int *olen)
    {
      long long n = 0;
      const unsigned char *arr = (const unsigned char *)ptr;
      for (int i = 0; i < 10; ++i)
        {
          n |= (unsigned long long)(arr[i] & 0x7F) << (i * 7);
          if (!(arr[i] & 0x80))
            {
              if (olen)
                *olen = i + 1;
              break;
            }
        }
      
      return n;
    }
    
    
    
    /* 
     * Extracts an MC string (UTF-8 string prefixed with its size in bytes as a
     * varint). Returns false is the contained string is not valid.
     */
    bool
    read_mc_string (const unsigned char *ptr, char *out, int out_len, int *olen)
    {
      int vl = 0;
      int len = read_varint (ptr, &vl);
      if (olen)
        *olen = len + vl;
      ptr += vl;
      
      -- out_len;
      int mlen = 0;
      for (int i = 0; i < len; ++i)
        {
          int bc = 0;
          int f = *ptr;
          while (f & 0x80)
            {
              ++ bc;
              f <<= 1;
            }
          
          if (bc == 1 || bc > 6)
            return false;
          
          if (mlen++ >= out_len)
            return false;
          
          *out++ = *ptr++;
          if (bc >= 2)
            i += bc - 1;
        }
      
      *out = '\0';
      
      return true;
    }
    
    
    
//------------------------------------------------------------------------------
    
    int
    write_varint (unsigned char *ptr, unsigned int val)
    {
      if (!val)
        {
          *ptr = 0;
          return 1;
        }
      
      int n = 0;
      
      while (val)
        {
          unsigned char p = val & 0x7F;
          val >>= 7;
          
          if (val)
            p |= 0x80;
          ptr[n++] = p;
        }
      
      return n;
    }
    
    int
    write_varlong (unsigned char *ptr, unsigned long long val)
    {
      if (!val)
        {
          *ptr = 0;
          return 1;
        }
      
      int n = 0;
      
      while (val)
        {
          unsigned char p = val & 0x7F;
          val >>= 7;
          
          if (val)
            p |= 0x80;
          ptr[n++] = p;
        }
      
      return n;
    }
    
    int
    write_mc_string (unsigned char *ptr, const char *str)
    {
      int n = 0;
      n += write_varint (ptr, (unsigned int)std::strlen (str));
      
      while (*str)
        ptr[n++] = *str++;
      
      return n;
    }
  }
}


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

#include "network/packet_delimiter.hpp"
#include "util/binary.hpp"


namespace hc {
  
  /* 
   * A special packet delimiter that is used to detect the protocol version
   * implemented by connecting clients.
   */
  int
  infer_packet_delimiter::remaining (const unsigned char *arr, int got)
  {
    return this->delim.remaining (arr, got);
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Starting with client versions 1.7, packets are prefixed with their length
   * as a VarInt, which is used to accurately calculate a packet's boundaries.
   */
  int
  mc17_packet_delimiter::remaining (const unsigned char *arr, int got)
  {
    int i;
    
    // make sure we got the whole length field
    bool got_len = false;
    for (i = 0; i < got; ++i)
      {
        if (!(arr[i] & 0x80))
          {
            got_len = true;
            break;
          }
      }
     
    if (i >= 5)
      return -1;
    else if (!got_len)
      return 1; // get one more byte
    
    int len = bin::read_varint (arr);
    return len + i + 1 - got;
  }
}


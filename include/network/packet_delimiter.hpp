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

#ifndef _hCraft2__NETWORK__PACKET_DELIMITER__H_
#define _hCraft2__NETWORK__PACKET_DELIMITER__H_


namespace hc {
  
  /* 
   * Used to determine the boundaries of individual packets in a byte
   * array/stream.
   */
  class packet_delimiter
  {
  public:
    virtual ~packet_delimiter () { }
    
  public:
    /* 
     * Checks and returns the number of bytes that are needed to complete the
     * packet in the specified array or compute a more estimate. A return value
     * of zero signifies that the packet has been fully read.
     * Returns -1 on invalid packet formats.
     * 
     * NOTE: This function is not guaranteed to return the absolute number of
     * bytes left (the return value may change with more input for the same
     * packet; this function should be called as many times needed until it
     * returns zero).
     */
    virtual int remaining (const unsigned char *arr, int got) = 0;
  };
  
  
  
  /* 
   * Starting with client versions 1.7, packets are prefixed with their length
   * as a VarInt, which is used to accurately calculate a packet's boundaries.
   */
  class mc17_packet_delimiter: public packet_delimiter
  {
  public:
    virtual int remaining (const unsigned char *arr, int got) override;
  };
  
  
  
  /* 
   * A special packet delimiter that is used to detect the protocol version
   * implemented by connecting clients.
   */
  class infer_packet_delimiter: public packet_delimiter
  {
    mc17_packet_delimiter delim;
    
  public:
    virtual int remaining (const unsigned char *arr, int got) override;
  };
}

#endif


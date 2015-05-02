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

#ifndef _hCraft2__NETWORK__PACKET_TRANSFORMER__H_
#define _hCraft2__NETWORK__PACKET_TRANSFORMER__H_


namespace hc {
  
  /* 
   * A part of a protocol implementation.
   * The transformer is used to apply a transformation (e.g. encryption/
   * decryption) on incoming and outgoing packets.
   */
  class packet_transformer
  {
  protected:
    bool on;
    
  public:
    inline bool is_on () const { return this->on; }
    
  public:
    packet_transformer ();
    virtual ~packet_transformer () { }
    
  public:
    /* 
     * Applies the transformation to incoming data.
     * Returns false on failure.
     * 
     * NOTE: The input may represent only a part of a whole packet.
     */
    virtual bool transform_in (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len, int *consumed) = 0;
    
    /* 
     * Applies the transformation to outgoing data.
     * Returns false on failure.
     */
    virtual bool transform_out (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len) = 0;
    
    /*
     * Calculates an upper bound on the size of the output data that would be
     * returned by transform_in() with the specified input data size.
     */
    virtual int estimate_in (unsigned int len) = 0;
    
    /*
     * Calculates an upper bound on the size of the output data that would be
     * returned by transform_out() with the specified input data size.
     */
    virtual int estimate_out (unsigned int len) = 0;
    
    /* 
     * Returns 1 if the given input is enough data for transform_in(); 0 if
     * more data must be read; and -1 on invalid data.
     */
    virtual int in_enough (const unsigned char *data, unsigned int len) = 0;
    
  public:
    /* 
     * Begins applying transformations to packets.
     */
    virtual void start ();
    
    /* 
     * Stops applying transformations to packets.
     */
    virtual void stop ();
  };
}

#endif


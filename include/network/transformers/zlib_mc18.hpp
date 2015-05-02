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

#ifndef _hCraft2__NETWORK__TRANSFORMERS__ZLIB__H_
#define _hCraft2__NETWORK__TRANSFORMERS__ZLIB__H_

#include "network/packet_transformer.hpp"
#include <zlib.h>


namespace hc {
  
  /* 
   * Provides ZLIB compression/decompression.
   */
  class zlib_mc18_transformer: public packet_transformer
  {
    int threshold;
    int level;
    z_stream strm_out, strm_in;
  
  public:
    zlib_mc18_transformer ();
    ~zlib_mc18_transformer ();
    
  public:
    virtual bool transform_in (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len, int *consumed) override;
    
    virtual bool transform_out (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len) override;
    
    virtual int estimate_in (unsigned int len) override;
    
    virtual int estimate_out (unsigned int len) override;
    
    virtual int in_enough (const unsigned char *data, unsigned int len) override;
    
  public:
    /* 
     * Initializes the transformer with the specified threshold value and
     * compression level.
     * Packets bigger than the given threshold will be compressed.
     */
    void setup (int threshold, int level);
    
    
    virtual void start () override;
    
    virtual void stop () override;
  };
}

#endif


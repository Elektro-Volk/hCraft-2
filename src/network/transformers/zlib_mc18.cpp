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

#include "network/transformers/zlib_mc18.hpp"
#include "util/binary.hpp"
#include <cstring>


namespace hc {
  
  zlib_mc18_transformer::zlib_mc18_transformer ()
  {
    this->threshold = 256;
  }
  
  zlib_mc18_transformer::~zlib_mc18_transformer ()
  {
    this->stop ();
  }
  
  
  
  bool
  zlib_mc18_transformer::transform_in (const unsigned char *data, 
    unsigned int len, unsigned char **outp, int *out_len, int *consumed)
  {
    int outc = len * 4;
    int outl = 0;
    unsigned char *out = new unsigned char [outc];
    
    // process as many packets as possible
    const unsigned char *ptr = data;
    *consumed = 0;
    for (;;)
      {
        int plen_len, dlen_len;
        int plen = bin::read_varint (ptr, &plen_len);
        int dlen = bin::read_varint (ptr + plen_len, &dlen_len);
        
        if (dlen == 0)
          {
            // uncompressed packet
            
            int size = plen - dlen_len + 5;
            if (outl + size >= outc)
              {
                unsigned char *nout = new unsigned char [outc * 2];
                std::memcpy (nout, out, outc);
                
                delete[] out;
                out = nout;
                outc *= 2;
              }
            
            outl += bin::write_varint (out + outl, plen - dlen_len);
            
            std::memcpy (out + outl, ptr + plen_len + dlen_len, plen - dlen_len);
            outl += plen - dlen_len;
          }
        else
          {
            this->strm_in.next_in = (Bytef *) (ptr + plen_len + dlen_len);
            this->strm_in.avail_in = plen - dlen_len;
            inflateReset (&this->strm_in);
            
            // put data length
            {
              if (outl + 5 >= outc)
                {
                  unsigned char *nout = new unsigned char [outc * 2];
                  std::memcpy (nout, out, outc);
                  
                  delete[] out;
                  out = nout;
                  outc *= 2;
                }
              
              outl += bin::write_varint (out + outl, dlen);
            }
            
            for (;;)
              {
                this->strm_in.next_out = out + outl;
                this->strm_in.avail_out = outc - outl;
                
                auto prev_avail = this->strm_in.avail_out;
                auto ret = inflate (&this->strm_in, Z_NO_FLUSH);
                outl += prev_avail - this->strm_in.avail_out;
                switch (ret)
                  {
                  case Z_STREAM_END:
                    goto inflate_done;
                  
                  case Z_OK:
                    break;
                  
                  default:
                    delete[] out;
                    return false;
                  }
                
                if (this->strm_in.avail_out == 0)
                  {
                    // increase buffer size
                    
                    unsigned char *nout = new unsigned char [outc * 2];
                    std::memcpy (nout, out, outc);
                    
                    delete[] out;
                    out = nout;
                    outc *= 2;
                  }
              }
          inflate_done: ;
          }
        
        *consumed += plen + plen_len;
        
        // check for more packets
        ptr += plen + plen_len;
        switch (bin::got_varint (ptr, len - *consumed))
          {
          case -1:
            delete[] out;
            break;
          
          case 0:
            goto done;
          }
      }
    
  done:
    *outp = out;
    *out_len = outl;
    return true;
  }
  
  bool
  zlib_mc18_transformer::transform_out (const unsigned char *data,
    unsigned int len, unsigned char **outp, int *out_len)
  {
    int dlen_len;
    int dlen = bin::read_varint (data, &dlen_len);
    
    if (dlen < this->threshold)
      {
        // no compression needed, simply wrap in proper packet format
        int plen = len - dlen_len + 1;
        int plen_len = bin::varint_size (plen);
        
        unsigned char *out = new unsigned char [plen_len + plen];
        bin::write_varint (out, plen);
        bin::write_varint (out + plen_len, 0);
        std::memcpy (out + plen_len + 1, data + dlen_len, dlen);
        
        *outp = out;
        *out_len = plen_len + plen;
      }
    else
      {
        // compress packet
        
        deflateReset (&this->strm_out);
        
        int out_cap = deflateBound (&this->strm_out, len);
        unsigned char *out = new unsigned char [out_cap + 10];
        
        int dlen_len;
        int dlen = bin::read_varint (data, &dlen_len);
        
        this->strm_out.avail_in = len - dlen_len;
        this->strm_out.next_in = (Bytef *)(data + dlen_len);
        this->strm_out.avail_out = out_cap;
        this->strm_out.next_out = out;
        
        auto ret = deflate (&this->strm_out, Z_FINISH);
        if (ret != Z_STREAM_END)
          {
            delete[] out;
            return false;
          }
        
        int compressed_len = out_cap - this->strm_out.avail_out;
        int plen = compressed_len + dlen_len;
        int plen_len = bin::varint_size (plen);
        
        unsigned char *fout = new unsigned char [plen + plen_len];
        bin::write_varint (fout, plen);
        bin::write_varint (fout + plen_len, dlen);
        std::memcpy (fout + plen_len + dlen_len, out, compressed_len);
        delete[] out;
        
        *outp = fout;
        *out_len = plen + plen_len;
      }
    
    return true;
  }
  
  int
  zlib_mc18_transformer::estimate_in (unsigned int len)
  {
    return len * 4;
  }
  
  int
  zlib_mc18_transformer::estimate_out (unsigned int len)
  {
    return deflateBound (&this->strm_out, len);
  }
  
  int
  zlib_mc18_transformer::in_enough (const unsigned char *data,
    unsigned int len)
  {
    switch (bin::got_varint (data, len))
      {
      case 0:
        return 0;
      
      case 1:
        {
          int plen_len;
          int plen = bin::read_varint (data, &plen_len);
          if ((int)len >= plen + plen_len)
            return 1;
          return 0;
        }
      
      default:
        return -1;
      }
  }
  
  
  
  /* 
   * Initializes the transformer with the specified threshold value and
   * compression level.
   * Packets bigger than the given threshold will be compressed.
   */
  void
  zlib_mc18_transformer::setup (int threshold, int level)
  {
    this->threshold = threshold;
    this->level = level;
  }
  
  
  
  void
  zlib_mc18_transformer::start ()
  {
    this->strm_out.zalloc = Z_NULL;
    this->strm_out.zfree = Z_NULL;
    this->strm_out.opaque = Z_NULL;
    if (deflateInit (&this->strm_out, this->level) != Z_OK)
      return;
    
    this->strm_in.zalloc = Z_NULL;
    this->strm_in.zfree = Z_NULL;
    this->strm_in.opaque = Z_NULL;
    this->strm_in.avail_in = 0;
    this->strm_in.next_in = Z_NULL;
    if (inflateInit (&this->strm_in) != Z_OK)
      return;
    
    packet_transformer::start ();
  }
  
  void
  zlib_mc18_transformer::stop ()
  {
    if (this->on)
      {
        deflateEnd (&this->strm_out);
        inflateEnd (&this->strm_in);
      }
    
    packet_transformer::stop ();
  }
}


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

#include "world/chunk.hpp"
#include "world/blocks.hpp"
#include <cstring>


namespace hc {
  
  sub_chunk::sub_chunk ()
  {
    std::memset (this->types, 0, sizeof this->types);
    std::memset (this->sl, 0, sizeof this->sl);
    std::memset (this->bl, 0, sizeof this->bl);
  }
  
  
  
  chunk::chunk (int x, int z)
    : pos (x, z)
  {
    for (int i = 0; i < 16; ++i)
      this->subs[i] = nullptr;
    std::memset (this->biomes, 1, sizeof this->biomes);
    std::memset (this->hmap, 0, sizeof this->hmap);
    
    for (int i = 0; i < 4; ++i)
      this->neighbours[i] = nullptr;
  }
  
  chunk::~chunk ()
  {
    for (int i = 0; i < 16; ++i)
      delete this->subs[i];
  }
  
  
  
  void
  chunk::recalc_height_at (int x, int z, int from)
  {
    int h = from;
    for (; h >= 0; --h)
      {
        auto binf = block_info::from_id (this->get_id (x, h, z));
        if (binf && binf->state == BS_SOLID && binf->opaque)
          break;
      }
    
    this->hmap[(z << 4) | x] = h + 1;
  }
  
  
  
  void
  chunk::set_id (int x, int y, int z, unsigned short id)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    sub->types[index] &= 0x000F;
    sub->types[index] |= id << 4;
    
    // update heightmap
    int hind = (z << 4) | x;
    auto binf = block_info::from_id (id);
    if (binf && binf->opaque && binf->state == BS_SOLID)
      {
        if (y >= this->hmap[hind])
          this->hmap[hind] = y + 1;
      }
    else if (this->hmap[hind] == y + 1)
      this->recalc_height_at (x, z, y - 1);
  }
  
  unsigned short
  chunk::get_id (int x, int y, int z)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      return 0;
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    return sub->types[index] >> 4;
  }
  
  
  void
  chunk::set_meta (int x, int y, int z, unsigned char meta)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    sub->types[index] &= 0xFFF0;
    sub->types[index] |= meta;
  }
  
  unsigned char
  chunk::get_meta (int x, int y, int z)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    return sub->types[index] & 0xF;
  }
  
  
  void
  chunk::set_sky_light (int x, int y, int z, unsigned char sl)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    int si = index >> 1;
    if (index & 1)
      {
        sub->sl[si] &= 0x0F;
        sub->sl[si] |= sl << 4;
      }
    else
      {
        sub->sl[si] &= 0xF0;
        sub->sl[si] |= sl;
      }
  }
  
  unsigned char
  chunk::get_sky_light (int x, int y, int z)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    return (index & 1)
      ? (sub->sl[index >> 1] >> 4)
      : (sub->sl[index >> 1] & 0x0F);
  }
  
  
  void
  chunk::set_block_light (int x, int y, int z, unsigned char bl)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    int si = index >> 1;
    if (index & 1)
      {
        sub->bl[si] &= 0x0F;
        sub->bl[si] |= bl << 4;
      }
    else
      {
        sub->bl[si] &= 0xF0;
        sub->bl[si] |= bl;
      }
  }
  
  unsigned char
  chunk::get_block_light (int x, int y, int z)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    return (index & 1)
      ? (sub->bl[index >> 1] >> 4)
      : (sub->bl[index >> 1] & 0x0F);
  }
  
  
  void
  chunk::set_id_and_meta (int x, int y, int z, unsigned short id,
    unsigned char meta)
  {
    int sy = y >> 4;
    sub_chunk *sub = this->subs[sy];
    if (!sub)
      sub = this->subs[sy] = new sub_chunk ();
    
    int index = ((y & 0xF) << 8) | (z << 4) | x;
    sub->types[index] = (id << 4) | meta;
    
    // update heightmap
    int hind = (z << 4) | x;
    auto binf = block_info::from_id (id);
    if (binf && binf->opaque && binf->state == BS_SOLID)
      {
        if (y >= this->hmap[hind])
          this->hmap[hind] = y + 1;
      }
    else if (this->hmap[hind] == y + 1)
      this->recalc_height_at (x, z, y - 1);
  }
}


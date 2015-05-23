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

#ifndef _hCraft2__UTIL__POSITION__H_
#define _hCraft2__UTIL__POSITION__H_

#include <functional>


namespace hc {
  
  // forward decs:
  struct block_pos;
  struct chunk_pos;
  struct entity_pos;
  
  
  struct block_pos
  {
    int x, y, z;
    
  public:
    block_pos (const chunk_pos& pos);
    block_pos (const block_pos& pos);
    block_pos (const entity_pos& pos);
    block_pos (int x, int y, int z);
    block_pos ();
    
  public:
    inline bool
    operator== (block_pos other) const
      { return this->x == other.x && this->y == other.y && this->z == other.z; }
  };
  
  struct chunk_pos
  {
    int x, z;
    
  public:
    chunk_pos (const chunk_pos& pos);
    chunk_pos (const block_pos& pos);
    chunk_pos (const entity_pos& pos);
    chunk_pos (int x, int z);
    chunk_pos ();
    
  public:
    inline bool
    operator== (chunk_pos other) const
      { return this->x == other.x && this->z == other.z; }
    
    inline bool
    operator!= (chunk_pos other) const
      { return !this->operator== (other); }
  };
  
  struct entity_pos
  {
    double x, y, z;
    float yaw, pitch;
    bool on_ground;
    
  public:
    entity_pos (const chunk_pos& pos);
    entity_pos (const block_pos& pos);
    entity_pos (const entity_pos& pos);
    entity_pos (double x, double y, double z, float yaw = 0.0,
      float pitch = 0.0, bool on_ground = false);
    entity_pos ();
  };
  
  
  
  struct vector3
  {
    double x, y, z;
    
  public:
    vector3 ();
    vector3 (double x, double y, double z);
  };
}


/* 
 * std::hash specializations.
 */
namespace std {
  
  template<>
  struct hash<hc::chunk_pos>
  {
    std::size_t
    operator() (hc::chunk_pos pos) const
    {
      return pos.x * 65531 + pos.z * 31;
    }
  };
}

#endif


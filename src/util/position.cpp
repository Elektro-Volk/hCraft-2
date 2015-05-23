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

#include "util/position.hpp"


namespace hc {
  
  block_pos::block_pos (const chunk_pos& pos)
  {
    this->x = pos.x << 4;
    this->y = 0;
    this->z = pos.z << 4;
  }
  
  block_pos::block_pos (const block_pos& pos)
  {
    this->x = pos.x;
    this->y = pos.y;
    this->z = pos.z;
  }
  
  block_pos::block_pos (const entity_pos& pos)
  {
    this->x = (int)pos.x;
    this->y = (int)pos.y;
    this->z = (int)pos.z;
  }
  
  block_pos::block_pos (int x, int y, int z)
  {
    this->x = x;
    this->y = y;
    this->z = z;
  }
  
  block_pos::block_pos ()
  {
    this->x = 0;
    this->y = 0;
    this->z = 0;
  }
  
  
  
//------------------------------------------------------------------------------
  
  chunk_pos::chunk_pos (const chunk_pos& pos)
  {
    this->x = pos.x;
    this->z = pos.z;
  }
  
  chunk_pos::chunk_pos (const block_pos& pos)
  {
    this->x = pos.x >> 4;
    this->z = pos.z >> 4;
  }
  
  chunk_pos::chunk_pos (const entity_pos& pos)
  {
    this->x = (int)pos.x >> 4;
    this->z = (int)pos.z >> 4;
  }
  
  chunk_pos::chunk_pos (int x, int z)
  {
    this->x = x;
    this->z = z;
  }
  
  chunk_pos::chunk_pos ()
  {
    this->x = 0;
    this->z = 0;
  }
  
  
  
//------------------------------------------------------------------------------
  
  entity_pos::entity_pos (const chunk_pos& pos)
  {
    this->x = pos.x * 16.0;
    this->y = 0.0;
    this->z = pos.z * 16.0;
    this->yaw = 0.0;
    this->pitch = 0.0;
    this->on_ground = false;
  }
  
  entity_pos::entity_pos (const block_pos& pos)
  {
    this->x = pos.x;
    this->y = pos.y;
    this->z = pos.z;
    this->yaw = 0.0;
    this->pitch = 0.0;
    this->on_ground = false;
  }
  
  entity_pos::entity_pos (const entity_pos& pos)
  {
    this->x = pos.x;
    this->y = pos.y;
    this->z = pos.z;
    this->yaw = pos.yaw;
    this->pitch = pos.pitch;
    this->on_ground = pos.on_ground;
  }
  
  entity_pos::entity_pos (double x, double y, double z, float yaw, float pitch,
    bool on_ground)
  {
    this->x = x;
    this->y = y;
    this->z = z;
    this->yaw = yaw;
    this->pitch = pitch;
    this->on_ground = on_ground;
  }
  
  entity_pos::entity_pos ()
  {
    this->x = 0.0;
    this->y = 0.0;
    this->z = 0.0;
    this->yaw = 0.0f;
    this->pitch = 0.0f;
    this->on_ground = false;
  }
  
  
  
//------------------------------------------------------------------------------
  
  vector3::vector3 ()
  {
    this->x = this->y = this->z = 0.0;
  }
  
  vector3::vector3 (double x, double y, double z)
  {
    this->x = x;
    this->y = y;
    this->z = z;
  }
}


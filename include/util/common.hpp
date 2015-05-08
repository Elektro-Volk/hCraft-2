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

#ifndef _hCraft2__COMMON__H_
#define _hCraft2__COMMON__H_


namespace hc {
  // 
  // Common structs and enums.
  // Not really sure where to put this...
  // 
  
  
  
  enum digging_state
  {
    DIG_START         = 0,
    DIG_CANCEL        = 1,
    DIG_FINISH        = 2,
    DIG_DROP_STACK    = 3,
    DIG_DROP_ITEM     = 4,
    DIG_FINISH_EATING = 5,
  };
  
  
  enum block_face
  {
    BFACE_Y_NEG = 0,
    BFACE_Y_POS = 1,
    BFACE_Z_NEG = 2,
    BFACE_Z_POS = 3,
    BFACE_X_NEG = 4,
    BFACE_X_POS = 5,
  };
  
  
  enum direction
  {
    DIR_NORTH = 0, // -z
    DIR_WEST  = 1, // -x
    DIR_SOUTH = 2, // +z
    DIR_EAST  = 3, // +x
    DIR_DOWN  = 4, // -y
    DIR_UP    = 5, // +y
  };
}

#endif


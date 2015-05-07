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

#include "world/generators/flatgrass.hpp"
#include "world/chunk.hpp"
#include "slot/blocks.hpp"


namespace hc {
  
  void
  flatgrass_world_generator::generate (chunk *ch)
  {
    for (int y = 0; y < 58; ++y)
      for (int x = 0; x < 16; ++x)
        for (int z = 0; z < 16; ++z)
          ch->set_id (x, y, z, BT_STONE);
    
    for (int y = 58; y < 64; ++y)
      for (int x = 0; x < 16; ++x)
        for (int z = 0; z < 16; ++z)
          ch->set_id (x, y, z, BT_DIRT);
     
    for (int x = 0; x < 16; ++x)
      for (int z = 0; z < 16; ++z)
        ch->set_id (x, 64, z, BT_GRASS);
  }
  
  
  
  entity_pos
  flatgrass_world_generator::find_spawn ()
  {
    return entity_pos (0.0, 66.0, 0.0, 0.0, 0.0);
  }
}


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

#include "world/world_generator.hpp"
#include "world/chunk.hpp"
#include "world/blocks.hpp"
#include <unordered_map>

// generators:
#include "world/generators/flatgrass.hpp"


namespace hc {
  
  static world_generator*
  _create_flatgrass ()
  {
    return new flatgrass_world_generator ();
  }
  
  /* 
   * Creates a new world generator from the given name and initializes it
   * using the specified initialization string.
   */
  world_generator*
  world_generator::create (const char *name, const std::string& init_str)
  {
    static std::unordered_map<std::string, world_generator* (*) ()> _map {
       { "flatgrass", &_create_flatgrass }
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return nullptr;
    
    auto gen = itr->second ();
    gen->setup (init_str);
    return gen;
  }
  
  
  
  /* 
   * Generates an "edge" chunk.
   * These are the chunks that are used outside the boundaries of a finite
   * world.
   */
  void
  world_generator::generate_edge (chunk *ch)
  {
    for (int y = 0; y < 64; ++y)
      for (int x = 0; x < 16; ++x)
        for (int z = 0; z < 16; ++z)
          ch->set_id (x, y, z, BT_BEDROCK);
    
    for (int x = 0; x < 16; ++x)
      for (int z = 0; z < 16; ++z)
        ch->set_id (x, 64, z, BT_STILL_WATER);
  }
}


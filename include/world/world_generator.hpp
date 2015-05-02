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

#ifndef _hCraft2__WORLD__WORLD_GENERATOR__H_
#define _hCraft2__WORLD__WORLD_GENERATOR__H_

#include "util/position.hpp"


namespace hc {
  
  // forward decs:
  class chunk;
  
  /* 
   * Base class for all world generators.
   * A world generator is responsible for generating chunks when one does not
   * exist neither in memory nor on disk.
   */
  class world_generator
  {
  public:
    virtual ~world_generator () { }
  
  public:
    /* 
     * Generates a chunk at the specified chunk coordinates.
     */
    virtual void generate (chunk *ch, int cx, int cz) = 0;
    
    /* 
     * Generates an "edge" chunk.
     * These are the chunks that are used outside the boundaries of a finite
     * world.
     */
    virtual void generate_edge (chunk *ch);
    
    /* 
     * Finds an appropriate spawn position.
     */
    virtual entity_pos find_spawn () = 0;
  };
}

#endif


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

#ifndef _hCraft2__WORLD__WORLD__H_
#define _hCraft2__WORLD__WORLD__H_

#include "util/position.hpp"
#include "world/async_generator.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <functional>


namespace hc {
  
  // forward decs:
  class chunk;
  class world_generator;
  class player;
  class server;
  
  /* 
   * An organized collection of chunks.
   */
  class world
  {
    server& srv;
    
    std::unordered_map<unsigned long long, chunk *> chunks;
    world_generator *gen;
    std::mutex ch_mtx;
    entity_pos spawn_pos;
    async_generator async_gen;
    
    std::vector<player *> pls;
    std::mutex pl_mtx;
    
  public:
    inline server& get_server () { return this->srv; }
    inline entity_pos get_spawn_pos () const { return this->spawn_pos; }
    inline async_generator& get_async_gen () { return this->async_gen; }
    
  public:
    world (server& srv, world_generator *gen);
    ~world ();
    
  public:
    /* 
     * Chunk manipulation:
     */
  //----------------------------------------------------------------------------
  
    /*
     * Places the specified chunk in the given chunk coordinates.
     */
    void set_chunk (int x, int y, chunk *ch);
    
    /* 
     * Returns the chunk located in the specified coordiantes, or null if there
     * is none.
     */
    chunk* get_chunk (int x, int y);
    
    /* 
     * Performs the first thing that works out of the following three:
     *     1. Load the chunk from memory if it exists.
     *     2. Load the chunk from disk.
     *     3. Generate the chunk.
     */
    chunk* load_chunk (int x, int y);
  
  //----------------------------------------------------------------------------
    
    
    
    /* 
     * Player management:
     */
  //----------------------------------------------------------------------------
    
    /* 
     * Inserts the specified player into the world's player list.
     */
    void add_player (player *pl);
    
    /* 
     * Removes the specified player from the world's player list.
     */
    void remove_player (player *pl);
    
  //----------------------------------------------------------------------------
  };
}

#endif


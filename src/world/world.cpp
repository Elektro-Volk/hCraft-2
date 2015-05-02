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

#include "world/world.hpp"
#include "world/chunk.hpp"
#include "world/world_generator.hpp"
#include "player/player.hpp"
#include "system/server.hpp"
#include <algorithm>


namespace hc {
  
  world::world (server& srv, world_generator *gen)
    : srv (srv), async_gen (*this, srv.get_gen_seq ())
  {
    this->gen = gen;
    this->spawn_pos = this->gen->find_spawn ();
  }
  
  world::~world ()
  {
    for (auto p : this->chunks)
      delete p.second;
    
    delete this->gen;
  }
  
  
  
//------------------------------------------------------------------------------

  /*
   * Places the specified chunk in the given chunk coordinates.
   */
  void
  world::set_chunk (int x, int y, chunk *ch)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    unsigned long long index =
      (unsigned int)x | ((unsigned long long)(unsigned int)y << 32);
    
    auto itr = this->chunks.find (index);
    if (itr != this->chunks.end ())
      delete itr->second;
    
    this->chunks[index] = ch;
  }
  
  /* 
   * Returns the chunk located in the specified coordiantes, or null if there
   * is none.
   */
  chunk*
  world::get_chunk (int x, int y)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    unsigned long long index =
      (unsigned int)x | ((unsigned long long)(unsigned int)y << 32);
    
    auto itr = this->chunks.find (index);
    if (itr == this->chunks.end ())
      return nullptr;
    
    return itr->second;
  }
  
  /* 
   * Performs the first thing that works out of the following three:
   *     1. Load the chunk from memory if it exists.
   *     2. Load the chunk from disk.
   *     3. Generate the chunk.
   */
  chunk*
  world::load_chunk (int x, int y)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    unsigned long long index =
      (unsigned int)x | ((unsigned long long)(unsigned int)y << 32);
    
    auto itr = this->chunks.find (index);
    if (itr != this->chunks.end ())
      return itr->second;
    
    // generate the chunk
    
    chunk *ch = new chunk ();
    this->gen->generate (ch, x, y);
    this->chunks[index] = ch;
    
    this->srv.get_lighting_manager ().light_chunk (ch);
    
    return ch;
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Inserts the specified player into the world's player list.
   */
  void
  world::add_player (player *pl)
  {
    std::lock_guard<std::mutex> guard (this->pl_mtx);
    this->pls.push_back (pl);
  }
  
  /* 
   * Removes the specified player from the world's player list.
   */
  void
  world::remove_player (player *pl)
  {
    std::lock_guard<std::mutex> guard (this->pl_mtx);
    this->pls.erase (std::remove (this->pls.begin (), this->pls.end (), pl),
      this->pls.end ());
  }
}


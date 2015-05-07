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
  class world_provider;
  class player;
  class server;
  class logger;
  
  
  /* 
   * Stores static information about a world that can be loaded and saved
   * from/to disk.
   */
  struct world_data
  {
    std::string name;     // world name
    long long seed;       // used for world generation
    std::string gen_name; // name of world generator
    entity_pos spawn_pos; // spawn position
  };
  
  
  /* 
   * An organized collection of chunks.
   */
  class world
  {
    server& srv;
    logger& log;
    
    world_data inf;
    
    std::unordered_map<unsigned long long, chunk *> chunks;
    world_generator *gen;
    std::mutex ch_mtx;
    async_generator async_gen;
    world_provider *prov;
    
    std::vector<player *> pls;
    std::mutex pl_mtx;
    
  public:
    inline server& get_server () { return this->srv; }
    inline entity_pos get_spawn_pos () const { return this->inf.spawn_pos; }
    inline async_generator& get_async_gen () { return this->async_gen; }
    inline world_generator* get_generator () { return this->gen; }
    
    inline world_data& get_info () { return this->inf; }
    inline const std::string& get_name () { return this->inf.name; }
    
  private:
    chunk* get_chunk_no_lock (int x, int z);
    
    void set_chunk_neighbours (chunk *ch);
    
  private:
    // used by world::load_from ()
    world (const world_data& wd, server& srv, world_generator *gen,
      world_provider *prov);
    
  public:
    world (const std::string& name, server& srv, world_generator *gen,
      world_provider *prov);
    ~world ();
  
  public:
    /*
     * Loads a world from the specified path.
     */
    static world* load_from (const std::string& path, server& srv);
    
  public:
    /* 
     * Chunk manipulation:
     */
  //----------------------------------------------------------------------------
  
    /*
     * Inserts the specified chunk into the world.
     */
    void put_chunk (chunk *ch);
    
    /* 
     * Returns the chunk located in the specified coordiantes, or null if there
     * is none.
     */
    chunk* get_chunk (int x, int z);
    
    /* 
     * Performs the first thing that works out of the following three:
     *     1. Load the chunk from memory if it exists.
     *     2. Load the chunk from disk.
     *     3. Generate the chunk.
     */
    chunk* load_chunk (int x, int z);
  
  //----------------------------------------------------------------------------
  
  
  
    /* 
     * Block manipulation:
     */
  //----------------------------------------------------------------------------
  
    void set_id (int x, int y, int z, unsigned short id);
    unsigned short get_id (int x, int y, int z);
    
    void set_sky_light (int x, int y, int z, unsigned char sl);
    unsigned char get_sky_light (int x, int y, int z);
    
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
    
    
    
    /* 
     * Saves the world to disk.
     */
    void save_all ();
  };
}

#endif


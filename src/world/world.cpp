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
#include "system/logger.hpp"
#include "world/chunk.hpp"
#include "world/world_generator.hpp"
#include "world/world_provider.hpp"
#include "player/player.hpp"
#include "system/server.hpp"
#include <algorithm>
#include <chrono>


namespace hc {
  
  world::world (const std::string& name, server& srv, world_generator *gen,
    world_provider *prov, int width, int depth)
    : srv (srv), log (srv.get_logger ()), async_gen (*this, srv.get_gen_seq ())
  {
    this->inf.name = name;
    this->inf.seed = std::chrono::duration_cast<std::chrono::nanoseconds> (
      std::chrono::high_resolution_clock::now ().time_since_epoch ()).count ();
    this->inf.width = width;
    this->inf.depth = depth;
    
    this->gen = gen;
    this->prov = prov;
    this->inf.spawn_pos = this->gen->find_spawn ();
    this->inf.gen_name = this->gen->name ();
    
    if (this->prov)
      {
        this->prov->open (
          this->prov->get_specifier ()->path_from_name (this->inf.name));
        this->prov->save_world_data (this->inf);
      }
    
    this->prepare_oob_chunk ();
  }
  
  // used by world::load_from ()
  world::world (const world_data& wd, server& srv, world_generator *gen,
    world_provider *prov)
    : srv (srv), log (srv.get_logger ()), inf (wd),
      async_gen (*this, srv.get_gen_seq ())
  {
    this->gen = gen;
    this->prov = prov;
    
    if (this->prov)
      {
        this->prov->open (
          this->prov->get_specifier ()->path_from_name (this->inf.name));
      }
    
    this->prepare_oob_chunk ();
  }
  
  world::~world ()
  {
    this->save_all ();
    
    for (auto p : this->chunks)
      delete p.second;
    delete this->edge_ch;
    
    delete this->gen;
    delete this->prov;
  }
  
  
  
  /*
   * Loads a world from the specified path.
   */
  world*
  world::load_from (const std::string& path, server& srv)
  {
    const char *fmt = world_provider_specifier::determine (path);
    if (!fmt)
      {
        srv.get_logger () (LT_ERROR) << "Could not load world at \"" << path
          << " \": unknown format." << std::endl;
        return nullptr;
      }
    
    world_data wd;
    world_provider *prov = world_provider::create (fmt);
    prov->open (path);
    try
      {
        prov->load_world_data (wd);
        prov->close ();
      }
    catch (const world_load_error& ex)
      {
        srv.get_logger () (LT_ERROR) << "Could not load world at \"" << path
          << " \": corrupt world? (" << ex.what () << ")" << std::endl;
        delete prov;
        return nullptr;
      }
    
    world_generator *gen = world_generator::create (wd.gen_name.c_str (), "");
    if (!gen)
      {
        srv.get_logger () (LT_ERROR) << "World \"" << wd.name
          << "\" has invalid generator (\"" << wd.gen_name << "\")" << std::endl;
        gen = world_generator::create ("flatgrass", "");
      }
    
    srv.get_logger () (LT_SYSTEM) << "Loaded world \"" << wd.name
      << "\" (format: " << fmt << ") from \"" << path << "\"" << std::endl;
    return new world (wd, srv, gen, prov);
  }
  
  
  
  void
  world::prepare_oob_chunk ()
  {
    this->edge_ch = new chunk (0, 0);
    this->gen->generate_edge (this->edge_ch);
    this->srv.get_lighting_manager ().light_chunk (this->edge_ch);
  }
  
  
  
//------------------------------------------------------------------------------
  
  void
  world::set_chunk_neighbours (chunk *ch)
  {
    int cx = ch->get_pos ().x;
    int cz = ch->get_pos ().z;
    chunk *nch;
    
    nch = this->get_chunk_no_lock (cx + 1, cz);
    ch->set_neighbour (DIR_EAST, nch);
    if (nch)
      nch->set_neighbour (DIR_WEST, ch);
    
    nch = this->get_chunk_no_lock (cx - 1, cz);
    ch->set_neighbour (DIR_WEST, nch);
    if (nch)
      nch->set_neighbour (DIR_EAST, ch); 
    
    nch = this->get_chunk_no_lock (cx, cz + 1);
    ch->set_neighbour (DIR_SOUTH, nch);
    if (nch)
      nch->set_neighbour (DIR_NORTH, ch);
    
    nch = this->get_chunk_no_lock (cx, cz - 1);
    ch->set_neighbour (DIR_NORTH, nch);
    if (nch)
      nch->set_neighbour (DIR_SOUTH, ch);
  }
  
  
  /*
   * Inserts the specified chunk into the world.
   */
  void
  world::put_chunk (chunk *ch)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    unsigned long long index =
      (unsigned int)ch->get_pos ().x |
      ((unsigned long long)(unsigned int)ch->get_pos ().z << 32);
    
    auto itr = this->chunks.find (index);
    if (itr != this->chunks.end ())
      delete itr->second;
    
    this->chunks[index] = ch;
    this->set_chunk_neighbours (ch);
  }
  
  
  /* 
   * Returns the chunk located in the specified coordiantes, or null if there
   * is none.
   */
  chunk*
  world::get_chunk (int x, int z)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    return this->get_chunk_no_lock (x, z);
  }
  
  chunk*
  world::get_chunk_no_lock (int x, int z)
  {
    if (this->inf.width > -1)
      if (x < 0 || x >= (this->inf.width >> 4))
        return this->edge_ch;
    if (this->inf.depth > -1)
      if (z < 0 || z >= (this->inf.depth >> 4))
        return this->edge_ch;
    
    unsigned long long index =
      (unsigned int)x | ((unsigned long long)(unsigned int)z << 32);
    
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
  world::load_chunk (int x, int z)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    unsigned long long index =
      (unsigned int)x | ((unsigned long long)(unsigned int)z << 32);
    
    if (this->inf.width > -1)
      if (x < 0 || x >= (this->inf.width >> 4))
        return this->edge_ch;
    if (this->inf.depth > -1)
      if (z < 0 || z >= (this->inf.depth >> 4))
        return this->edge_ch;
    
    auto itr = this->chunks.find (index);
    if (itr != this->chunks.end ())
      return itr->second;
    
    // try to load from disk
    chunk *ch = nullptr;
    if (this->prov)
      {
        try
          {
            ch = this->prov->load_chunk (x, z);
          }
        catch (const world_load_error& ex)
          {
            log (LT_ERROR) << "World \"" << this->inf.name
              << "\": Failed to load chunk at (" << x << ", " << z << ") ("
              << ex.what () << ")" << std::endl;
          }
      }
    if (ch)
      {
        this->chunks[index] = ch;
        this->set_chunk_neighbours (ch);
        return ch;
      }
    
    // generate the chunk
   
    ch = new chunk (x, z);
    this->gen->generate (ch);
    this->chunks[index] = ch;
    this->set_chunk_neighbours (ch);
    
    this->srv.get_lighting_manager ().light_chunk (ch);
    
    return ch;
  }
  
  
  
//------------------------------------------------------------------------------

  void
  world::set_id (int x, int y, int z, unsigned short id)
  {
    chunk *ch = this->get_chunk (x >> 4, z >> 4);
    if (!ch)
      {
        if (id == 0)
          return;
        ch = this->load_chunk (x >> 4, z >> 4);
      }
    
    ch->set_id (x & 15, y, z & 15, id);
    this->srv.get_lighting_manager ().enqueue (this, x, y, z);
  }
  
  unsigned short
  world::get_id (int x, int y, int z)
  {
    chunk *ch = this->get_chunk (x >> 4, z >> 4);
    if (!ch)
      return 0;
    
    return ch->get_id (x & 15, y, z & 15);
  }
  
  
  void
  world::set_sky_light (int x, int y, int z, unsigned char sl)
  {
    chunk *ch = this->get_chunk (x >> 4, z >> 4);
    if (!ch)
      return;
    
    ch->set_sky_light (x & 15, y, z & 15, sl);
  }
  
  unsigned char
  world::get_sky_light (int x, int y, int z)
  {
    chunk *ch = this->get_chunk (x >> 4, z >> 4);
    if (!ch)
      return 15;
    
    return ch->get_sky_light (x & 15, y, z & 15);
  }
  
  
  void
  world::set_id_and_meta (int x, int y, int z, unsigned short id,
    unsigned char meta)
  {
    chunk *ch = this->get_chunk (x >> 4, z >> 4);
    if (!ch)
      {
        if (id == 0)
          return;
        ch = this->load_chunk (x >> 4, z >> 4);
      }
    
    ch->set_id_and_meta (x & 15, y, z & 15, id, meta);
    this->srv.get_lighting_manager ().enqueue (this, x, y, z);
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
  
  /* 
   * Calls the specified callback function for every player in the world.
   */
  void
  world::all_players (std::function<void (player *)>&& cb)
  {
    std::lock_guard<std::mutex> guard (this->pl_mtx);
    for (player *pl : this->pls)
      cb (pl);
  }
  
  
  
  /* 
   * Saves the world to disk.
   */
  void
  world::save_all ()
  {
    if (!this->prov)
      return;
    
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    for (auto p : this->chunks)
      {
        chunk *ch = p.second;
        this->prov->save_chunk (ch);
      }
    
    this->prov->save_world_data (this->inf);
  }
}


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

#ifndef _hCraft2__WORLD__CHUNK__H_
#define _hCraft2__WORLD__CHUNK__H_

#include "util/common.hpp"
#include "util/position.hpp"
#include <mutex>
#include <vector>


namespace hc {
  
  // forward decs:
  class entity;
  
  
  /* 
   * A 16x16x16 chunk of blocks.
   * 16 of these stacked on top of each other make up a single 16x256x16 chundk.
   */
  struct sub_chunk
  {
    unsigned short types[4096];  // block ids + meta
    unsigned char sl[2048];   // sky light
    unsigned char bl[2048];   // block light
    
  public:
    sub_chunk ();
  };
  
  
  /* 
   * A 16x256x16 chunk of blocks.
   * Represented as 16 sub-chunks.
   */
  class chunk
  {
    chunk_pos pos;
    sub_chunk *subs[16];
    unsigned char biomes[256];
    int hmap[256];
    
    // neighbours
    chunk *neighbours[4];
    
    std::vector<entity *> ents;
    std::mutex ent_mtx;
    
  public:
    inline chunk_pos get_pos () { return this->pos; }
    inline sub_chunk* get_sub (int sy) { return this->subs[sy]; }
    inline unsigned char* get_biomes () { return this->biomes; }
    inline int* get_heightmap () { return this->hmap; }
    
    inline int
    get_height (int x, int z)
      { return this->hmap[(z << 4) | x]; }
    
    inline chunk*
    get_neighbour (direction dir)
      { return this->neighbours[(int)dir]; }
    
    inline void
    set_neighbour (direction dir, chunk *ch)
      { this->neighbours[(int)dir] = ch; }
    
  public:
    chunk (int x, int z);
    ~chunk ();
    
  private:
    void recalc_height_at (int x, int z, int from = 255);
    
  public:
    void set_id (int x, int y, int z, unsigned short id);
    unsigned short get_id (int x, int y, int z);
    
    void set_meta (int x, int y, int z, unsigned char meta);
    unsigned char get_meta (int x, int y, int z);
    
    void set_sky_light (int x, int y, int z, unsigned char sl);
    unsigned char get_sky_light (int x, int y, int z);
    
    void set_block_light (int x, int y, int z, unsigned char bl);
    unsigned char get_block_light (int x, int y, int z);
    
    void set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta);
    
  public:
    /* 
     * Entity management:
     */
    //--------------------------------------------------------------------------
    
    /* 
     * Inserts the specified entity into the chunk's entity list.
     */
    void register_entity (entity *ent);
    
    /* 
     * Removes the specified entity from the chunk's entity list.
     */
    void deregister_entity (entity *ent);
    
    /* 
     * Calls the specified callback function for every entity in the chunk.
     */
    void all_entities (std::function<void (entity *)>&& cb);
    
    //--------------------------------------------------------------------------
  };
}

#endif


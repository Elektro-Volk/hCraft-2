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

#ifndef _hCraft2__ENTITY__ENTITY__H_
#define _hCraft2__ENTITY__ENTITY__H_

#include "util/position.hpp"
#include <unordered_set>
#include <mutex>


namespace hc {
  
  // forward decs:
  class player;
  class world;
  class chunk;
  class entity_metadata;
  
  
  /* 
   * A dynamic moving object (e.g. pigs, sheep, players, dropped items).
   */
  class entity
  {
  protected:
    int eid;
    entity_pos pos;
    vector3 vel;      // velocity
    vector3 vol;      // volume occupied
    int health;       // in half-heart units
    
    world *w;
    chunk *curr_ch;   // the chunk the entity is currently in.
    std::mutex ch_mtx;
    
  public:
    inline int get_eid () const { return this->eid; }
    inline entity_pos get_pos () const { return this->pos; }
    inline vector3 get_velocity () const { return this->vel; }
    inline vector3 get_volume () const { return this->vol; }
    inline int get_health () const { return this->health; }
  
  public:
    entity (int eid);
    virtual ~entity () { }
    
  public:
    /* 
     * Populates the specified metadata dictionary with values for this entity.
     */
    virtual void build_metadata (entity_metadata& metadata);
    
    
    
    /* 
     * Makes the entity visible to the specified player.
     */
    virtual void spawn_to (player *pl) = 0;
    
    /* 
     * Removes the entity from the sight of the specified player.
     */
    virtual void despawn_from (player *pl);
    
    
    
    /* 
     * Spawns the entity into the specified world at the given coordinates.
     * NOTE: The chunk in which the entity is to be spawned in, must be already
     *       loaded.
     */
    virtual void spawn (world *w, entity_pos pos);
    
    /* 
     * Despawns the entity from the world's it's currently in.
     */
    virtual void despawn ();
    
    
    
    /* 
     * Moves the entity to the specified position.
     */
    void move (entity_pos pos);
  };
}

#endif


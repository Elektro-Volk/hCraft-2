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

#include "entity/entity.hpp"
#include "player/player.hpp"
#include "world/world.hpp"
#include "world/chunk.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "entity/metadata.hpp"


namespace hc {
  
  entity::entity (int eid)
  {
    this->eid = eid;
  }
  
  
  
  /* 
   * Populates the specified metadata dictionary with values for this entity.
   */
  void
  entity::build_metadata (entity_metadata& metadata)
  {
    metadata.put_byte (0, 0); // flags
    metadata.put_short (1, 0); // air
  }
  
  
  
  /* 
   * Removes the entity from the sight of the specified player.
   */
  void
  entity::despawn_from (player *pl)
  {
    
  }
  
  
  
  /* 
   * Spawns the entity into the specified world at the given coordinates.
   * NOTE: The chunk in which the entity is to be spawned in, must be already
   *       loaded.
   */
  void
  entity::spawn (world *w, entity_pos pos)
  {
    std::lock_guard<std::mutex> guard (this->ch_mtx);
    
    server& srv = w->get_server ();
    logger& log = srv.get_logger ();
    
    chunk_pos cpos = pos;
    chunk *ch = w->get_chunk (cpos.x, cpos.z);
    if (!ch)
      {
        log (LT_ERROR) << "Attempted to spawn entity in unloaded chunk." << std::endl;
        return;
      }
    
    this->w = w;
    this->pos = pos;
    this->curr_ch = ch;
    ch->register_entity (this);
    
    
  }
  
  /* 
   * Despawns the entity from the world's it's currently in.
   */
  void
  entity::despawn ()
  {
    if (!this->curr_ch)
      return;
    
    this->curr_ch->deregister_entity (this);
    
    this->w = nullptr;
    this->curr_ch = nullptr;
  }
  
  
  
  /* 
   * Moves the entity to the specified position.
   */
  void
  entity::move (entity_pos pos)
  {
    if (!this->w)
      return;
    
    chunk_pos pcp = this->pos;
    chunk_pos ncp = pos;
    
    this->pos = pos;
    
    if (pcp != ncp)
      {
        std::lock_guard<std::mutex> guard (this->ch_mtx);
        this->curr_ch = this->w->get_chunk (ncp.x, ncp.z);
      }
  }
}


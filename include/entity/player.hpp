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

#ifndef _hCraft2__ENTITY__PLAYER__H_
#define _hCraft2__ENTITY__PLAYER__H_

#include "entity/entity.hpp"


namespace hc {
  
  // forward decs:
  class player;
  
  
  /* 
   * A special entity type that represents a player.
   */
  class player_entity: public entity
  {
    player *pl;
    
  public:
    player_entity (player *pl, int eid);
    
  public:
    virtual void spawn_to (player *pl) override;
    
    virtual void spawn (world *w, entity_pos pos) override;
    
    virtual void despawn () override;
    
    virtual void build_metadata (entity_metadata& metadata) override;
  };
}

#endif


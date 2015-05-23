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

#include "entity/player.hpp"
#include "player/player.hpp"
#include "world/world.hpp"
#include "network/connection.hpp"
#include "network/protocol.hpp"
#include "network/packet_builder.hpp"
#include "entity/metadata.hpp"
#include "system/server.hpp"
#include "world/chunk.hpp"

#include <iostream> // DEBUG


namespace hc {
  
  player_entity::player_entity (player *pl, int eid)
    : entity (eid)
  {
    this->pl = pl;
  }
  
  
  
  void
  player_entity::build_metadata (entity_metadata& metadata)
  {
    entity::build_metadata (metadata);
  }
  
  
  
  void
  player_entity::spawn_to (player *pl)
  {
    connection& conn = pl->get_connection ();
    auto builder = conn.get_protocol ()->get_builder ();
    
    entity_metadata meta;
    this->build_metadata (meta);
    
    conn.send (builder->make_spawn_player (this->get_eid (),
      this->pl->get_uuid (), this->pos.x, this->pos.y, this->pos.z,
      this->pos.yaw, this->pos.pitch, 0, meta));
  }
  
  void
  player_entity::spawn (world *w, entity_pos pos)
  {
    entity::spawn (w, pos);
    
    // add self to tab player list of online players, and vice versa.
    player *me = this->pl;
    w->all_players (
      [me] (player *pl)
        {
          // TODO: send proper ping
          
          connection& pl_conn = pl->get_connection ();
          auto pl_builder = pl_conn.get_protocol ()->get_builder ();
          pl_conn.send (pl_builder->make_player_list_add (me->get_uuid (),
            me->get_username (), me->get_gm (), 80));
          
          connection& me_conn = me->get_connection ();
          auto me_builder = me_conn.get_protocol ()->get_builder ();
          me_conn.send (me_builder->make_player_list_add (pl->get_uuid (),
            pl->get_username (), pl->get_gm (), 80));
        });
    
    // show existing entities
    chunk_pos cpos = this->pos;
    std::cout << "Spawning existing entities to " << this->pl->get_username () << std::endl;
    int vrad = w->get_server ().get_config ().view_dist;
    for (int x = (cpos.x - vrad); x <= (cpos.x + vrad); ++x)
      for (int z = (cpos.z - vrad); z <= (cpos.z + vrad); ++z)
        {
          chunk *ch = w->get_chunk (x, z);
          if (!ch)
            continue;
          
          player_entity *that = this;
          ch->all_entities (
            [that] (entity *ent)
              {
                if (ent == that)
                  return;
                
                ent->spawn_to ((static_cast<player_entity *> (that))->pl);
                //std::cout << "  | spawning ent at [" << ent->get_pos ().x << ", " << ent->get_pos ().y << ", " << ent->get_pos ().z << "]" << std::endl;
                
                player_entity *pent = dynamic_cast<player_entity *> (ent);
                if (pent)
                  {
                    std::cout << "  | spawned " << pent->pl->get_username () << " at [" << ent->get_pos ().x << ", " << ent->get_pos ().y << ", " << ent->get_pos ().z << "] to " << that->pl->get_username () << std::endl;
                    
                    // spawn self to player
                    that->spawn_to (pent->pl);
                    std::cout << "  | > | ent is player (" << pent->pl->get_username () << "), spawning self (" << that->pl->get_username () << " to ent at [" << that->get_pos ().x << ", " << that->get_pos ().y << ", " << that->get_pos ().z << "]" << std::endl;
                  }
              });
        }
  }
  
  void
  player_entity::despawn ()
  {
    player *me = this->pl;
    w->all_players (
      [me] (player *pl)
        {
          connection& conn = pl->get_connection ();
          auto builder = conn.get_protocol ()->get_builder ();
          conn.send (builder->make_player_list_remove (me->get_uuid (),
            me->get_username ()));
        });
    
    entity::despawn ();
  }
}


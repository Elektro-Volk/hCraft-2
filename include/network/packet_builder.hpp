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

#ifndef _hCraft2__NETWORK__PACKET_BUILDER__H_
#define _hCraft2__NETWORK__PACKET_BUILDER__H_

#include "util/uuid.hpp"
#include "util/common.hpp"
#include <string>

#ifdef WIN32
# ifdef uuid_t
#   undef uuid_t
# endif
#endif


namespace hc {
  
  // forward decs:
  class packet;
  class entity_metadata;
  
  /* 
   * Base class for packet builders.
   */
  class packet_builder
  {
  public:
    virtual ~packet_builder () { }
    
  public:
    virtual packet* make_login_disconnect (const std::string& msg) = 0;
    
    
    virtual packet* make_keep_alive (int id) = 0;
    
    virtual packet* make_disconnect (const std::string& msg) = 0;
    
    virtual packet* make_chat_message (const std::string& msg) = 0;
    
    virtual packet* make_player_list_add (uuid_t uuid, const std::string& name,
      game_mode gm, int ping) = 0;
    
    virtual packet* make_player_list_remove (uuid_t uuid,
      const std::string& name) = 0;
    
    virtual packet* make_spawn_player (int eid, uuid_t uuid, double x,
      double y, double z, float yaw, float pitch, short curr_item,
      const entity_metadata& meta) = 0;
  };
}

#endif


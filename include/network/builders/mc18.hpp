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

#ifndef _hCraft2__NETWORK__BUILDERS__MC18__H_
#define _hCraft2__NETWORK__BUILDERS__MC18__H_

#include "network/packet_builder.hpp"
#include <cryptopp/rsa.h>
#include <string>


namespace hc {
  
  // forward decs:
  class chunk;
  
  /* 
   * A packet builder conforming to protocol version #47 (client version: 1.8).
   */
  class mc18_packet_builder: public packet_builder
  {
  public:
    // status:
    
    virtual packet* make_status_response (const std::string& str);
    
    virtual packet* make_status_ping (unsigned long long time);
    
    
    // login:
    
    virtual packet* make_encryption_request (const std::string& sid,
      CryptoPP::RSA::PublicKey& pkey, unsigned char *vtoken);
    
    virtual packet* make_login_success (const std::string& uuid,
      const std::string& username);
    
    virtual packet* make_login_disconnect (const std::string& msg) override;
    
    
    // play:
    
    virtual packet* make_keep_alive (int id) override;
    
    virtual packet* make_join_game (int eid, unsigned char gm, unsigned char dim,
      unsigned char diff, unsigned char maxp, const std::string& level_type,
      bool reduced_debug);
    
    virtual packet* make_chat_message (const std::string& js, int pos);
    virtual packet* make_chat_message (const std::string& msg) override;
    
    virtual packet* make_spawn_position (int x, int y, int z);
    
    virtual packet* make_player_position_and_look (double x, double y, double z,
      float yaw, float pitch, unsigned char flags);
    
    virtual packet* make_chunk_data (int cx, int cz, bool cont,
      unsigned short mask, chunk *ch);
    
    virtual packet* make_unload_chunk (int cx, int cz);
    
    virtual packet* make_disconnect (const std::string& msg) override;
    
    virtual packet* make_set_compression (int threshold);
    
    virtual packet* make_player_list_add (uuid_t uuid, const std::string& name,
      game_mode gm, int ping) override;
    
    virtual packet* make_player_list_remove (uuid_t uuid,
      const std::string& name) override;
    
    virtual packet* make_spawn_player (int eid, uuid_t uuid, double x,
      double y, double z, float yaw, float pitch, short curr_item,
      const entity_metadata& meta) override;
  };
}

#endif


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

#include "network/handlers/mc18.hpp"
#include "network/packet.hpp"
#include "network/connection.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "network/protocol.hpp"
#include "network/builders/mc18.hpp"
#include "network/transformers/aes.hpp"
#include "network/transformers/zlib_mc18.hpp"
#include "util/json.hpp"
#include "player/player.hpp"
#include "world/world.hpp"
#include "world/chunk.hpp"
#include <sstream>
#include <memory>
#include <stdexcept>
#include <random>
#include <chrono>
#include <cryptopp/osrng.h>
#include <cstring>


namespace hc {
  
  mc18_packet_handler::mc18_packet_handler ()
  {
    this->state = PS_HANDSHAKE;
    this->pl = nullptr;
  }
  
  mc18_packet_handler::~mc18_packet_handler ()
  {
    delete this->pl;
  }
  
  
  
  void
  mc18_packet_handler::login ()
  {
    server& srv = this->conn->get_server ();
    
    this->conn->send (this->builder->make_login_success (
      pl->get_uuid ().str (), pl->get_username ()));
    this->conn->send (this->builder->make_join_game (
      1, 1, 0, 0, 20, "default", true));
    this->state = PS_PLAY;
    
    // ---
    
    // begin compression
    this->conn->send (this->builder->make_set_compression (srv.get_config ().compress_threshold));
    for (auto trans : this->conn->get_protocol ()->get_transformers ())
      {
        zlib_mc18_transformer *tr =
          dynamic_cast<zlib_mc18_transformer *> (trans);
        if (!tr)
          continue;
        
        tr->setup (srv.get_config ().compress_threshold,
          srv.get_config ().compress_level);
        tr->start ();
        break;
      }
    
    // ---
    
    this->pl->on_login ();
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Handshake:
  // 
  
  void
  mc18_packet_handler::handle_h00 (packet_reader& reader)
  {
    // 
    // 0x00: Handshake
    // 
    
    int proto = reader.read_varint ();
    if (proto != 47)
      { this->conn->disconnect (); return; }
    
    char addr[256];
    reader.read_string (addr, sizeof addr);
    reader.read_short (); // port
    
    // next state
    int st = reader.read_varint ();
    if (st != 1 && st != 2)
      { this->conn->disconnect (); return; }
    this->state = (protocol_state)st;
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Status:
  // 
  
  void
  mc18_packet_handler::handle_s00 (packet_reader& reader)
  {
    // 
    // 0x00: Status Request
    // 
    
    std::unique_ptr<json::j_object> js { new json::j_object () };
    {
      using namespace json;
      
      j_object *obj = new j_object ();
      obj->set ("name", new j_string ("1.8"));
      obj->set ("protocol", new j_number (47));
      js->set ("version", obj);
      
      obj = new j_object ();
      obj->set ("max", new j_number (12));
      obj->set ("online", new j_number (0));
      obj->set ("sample", new j_array ());
      js->set ("players", obj);
      
      obj = new j_object ();
      obj->set ("text", new j_string ("Hello world"));
      js->set ("description", obj);
    }
    
    std::ostringstream ss;
    json_writer writer (ss);
    writer.write (js.get ());
    
    this->conn->send (this->builder->make_status_response (ss.str ()));
  }
  
  void
  mc18_packet_handler::handle_s01 (packet_reader& reader)
  {
    // 
    // 0x01: Status Ping
    // 
    
    unsigned long long time = reader.read_long ();
    this->conn->send (this->builder->make_status_ping (time));
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Login:
  // 
  
  void
  mc18_packet_handler::handle_l00 (packet_reader& reader)
  {
    // 
    // 0x00: Login Start
    //
    
    server& srv = this->conn->get_server ();
    logger& log = srv.get_logger ();
    
    char name[256];
    if (!reader.read_string (name, sizeof name))
      { this->conn->disconnect (); return; }
    // TODO: check whether username is valid
    
    player *pl = new player (*this->conn, uuid_t::random (), name);
    this->conn->set_player (pl);
    srv.register_player (pl);
    this->pl = pl;
    log (LT_SYSTEM) << "Player `" << pl->get_username () << "' logging in from @"
      << this->conn->get_ip () << " (UUID: " << pl->get_uuid ().str () << ")" << std::endl;
    
    // initialize verification token
    std::mt19937 rnd;
    rnd.seed ((unsigned long)std::chrono::duration_cast<std::chrono::nanoseconds> (
      std::chrono::high_resolution_clock::now ().time_since_epoch ()).count ());
    std::uniform_int_distribution<> dis (0x00, 0xFF);
    for (int i = 0; i < 4; ++i)
      this->vtoken[i] = dis (rnd);
    
    if (!srv.get_config ().encryption)
      {
        this->login ();
        return;
      }
    
    // send encryption request
    auto pkey = srv.get_pub_key ();
    this->conn->send (
      this->builder->make_encryption_request ("", pkey, vtoken));
  }
  
  void
  mc18_packet_handler::handle_l01 (packet_reader& reader)
  {
    // 
    // 0x01: Encryption Response
    //
    
    server& srv = this->conn->get_server ();
    logger& log = srv.get_logger ();
    
    // read encrypted shared secret
    int ss_len = reader.read_varint ();
    if (ss_len < 0 || ss_len > 128)
      { this->disconnect (); return; }
    std::unique_ptr<unsigned char[]> ss_enc { new unsigned char [ss_len] };
    reader.read_bytes (ss_enc.get (), ss_len);
    
    // read encrypted verification token
    int vtoken_len = reader.read_varint ();
    if (vtoken_len < 0 || vtoken_len > 128)
      { this->disconnect (); return; }
    std::unique_ptr<unsigned char[]> vtoken_enc { new unsigned char [vtoken_len] };
    reader.read_bytes (vtoken_enc.get (), vtoken_len);
    
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSAES_PKCS1v15_Decryptor decrypt (srv.get_priv_key ());
    
    // decrypt verification token
    {
      CryptoPP::SecByteBlock ct_vtoken (vtoken_enc.get (), vtoken_len);
      size_t dpl = decrypt.MaxPlaintextLength (ct_vtoken.size ());
      CryptoPP::SecByteBlock rec ((dpl));
      auto res = decrypt.Decrypt (rng, ct_vtoken, ct_vtoken.size (), rec);
      if (res.messageLength != 4)
        {
          log (LT_WARNING) << "Player \"" << this->pl->get_username ()
                           << " failed token verification" << std::endl;
          this->conn->disconnect ();
          return;
        }
      
      // make sure tokens match
      for (int i = 0; i < 4; ++i)
        if (this->vtoken[i] != rec[i])
          {
            log (LT_WARNING) << "Player \"" << this->pl->get_username ()
                             << " failed token verification" << std::endl;
            this->conn->disconnect ();
            return;
          }
    }
    
    // decrypt shared secret
    unsigned char ssec[16];
    {
      CryptoPP::SecByteBlock ct_ssec (ss_enc.get (), ss_len);
      size_t dpl = decrypt.MaxPlaintextLength (ct_ssec.size ());
      CryptoPP::SecByteBlock rec ((dpl));
      auto res = decrypt.Decrypt (rng, ct_ssec, ct_ssec.size (), rec);
      if (res.messageLength != 16)
        {
          log (LT_WARNING) << "Player \"" << this->pl->get_username ()
                           << " sent invalid shared secret (length =/= 16)" << std::endl;
          this->conn->disconnect ();
          return;
        }
      
      std::memcpy (ssec, rec.data (), 16);
    }
    
    // begin encryption
    for (auto trans : this->conn->get_protocol ()->get_transformers ())
      {
        aes_transformer *tr =
          dynamic_cast<aes_transformer *> (trans);
        if (!tr)
          continue;
        
        tr->setup (ssec);
        tr->start ();
        break;
      }
    
    // ---
    
    this->login ();
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Play:
  // 
  
  void
  mc18_packet_handler::handle_p00 (packet_reader& reader)
  {
    // 
    // 0x00: Keep Alive
    //
    
    int id = reader.read_varint ();
    this->pl->handle_keep_alive (id);
  }
  
  
  
  void
  mc18_packet_handler::handle_p01 (packet_reader& reader)
  {
    // 
    // 0x01: Chat Message
    //
    
    char str[256];
    if (!reader.read_string (str, 119))
      {
        this->conn->disconnect ();
        return;
      }
    
    this->pl->on_chat (str);
  }
  
  
  
  
  void
  mc18_packet_handler::handle_p03 (packet_reader& reader)
  {
    // 
    // 0x03: Player
    //
    
    auto pos = this->pl->position ();
    pos.on_ground = reader.read_bool ();
    this->pl->on_move (pos);
  }
  
  void
  mc18_packet_handler::handle_p04 (packet_reader& reader)
  {
    // 
    // 0x04: Player Position
    //
    
    auto pos = this->pl->position ();
    pos.x = reader.read_double ();
    pos.y = reader.read_double ();
    pos.z = reader.read_double ();
    pos.on_ground = reader.read_bool ();
    this->pl->on_move (pos);
  }
  
  void
  mc18_packet_handler::handle_p05 (packet_reader& reader)
  {
    // 
    // 0x05: Player Look
    //
    
    auto pos = this->pl->position ();
    pos.yaw = reader.read_float ();
    pos.pitch = reader.read_float ();
    pos.on_ground = reader.read_bool ();
    this->pl->on_move (pos);
  }
  
  void
  mc18_packet_handler::handle_p06 (packet_reader& reader)
  {
    // 
    // 0x06: Player Position And Look
    //
    
    auto pos = this->pl->position ();
    pos.x = reader.read_double ();
    pos.y = reader.read_double ();
    pos.z = reader.read_double ();
    pos.yaw = reader.read_float ();
    pos.pitch = reader.read_float ();
    pos.on_ground = reader.read_bool ();
    this->pl->on_move (pos);
  }
  
  
  
//------------------------------------------------------------------------------
  
  void
  mc18_packet_handler::handle_xx (packet_reader& reader)
    { /* dummy handler */ }
  
  
  /* 
   * Handles the specified packet, by dispatching it to the appropriate
   * handler function.
   */
  void
  mc18_packet_handler::handle (packet_reader& reader)
  {
    if (!this->conn)
      return;
    
    logger& log = this->conn->get_server ().get_logger ();
    
    int len = reader.read_varint ();
    int opc = reader.read_varint ();
    //log (LT_DEBUG) << "got packet [" << len << " bytes] ::id(" << opc << ")" << std::endl;
    
    static void (mc18_packet_handler:: *_handshake_table[]) (packet_reader&) = {
      &mc18_packet_handler::handle_h00, 
    };
    
    static void (mc18_packet_handler:: *_status_table[]) (packet_reader&) = {
      &mc18_packet_handler::handle_s00,
      &mc18_packet_handler::handle_s01,
    };
    
    static void (mc18_packet_handler:: *_login_table[]) (packet_reader&) = {
      &mc18_packet_handler::handle_l00,
      &mc18_packet_handler::handle_l01,
    };
    
    static void (mc18_packet_handler:: *_play_table[]) (packet_reader&) = {
      &mc18_packet_handler::handle_p00, &mc18_packet_handler::handle_p01,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_p03,
      &mc18_packet_handler::handle_p04, &mc18_packet_handler::handle_p05,
      &mc18_packet_handler::handle_p06, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
      &mc18_packet_handler::handle_xx, &mc18_packet_handler::handle_xx,
    };
    
    switch (this->state)
      {
      case PS_HANDSHAKE:
        if (opc > 0x00)
          { this->conn->disconnect (); return; }
        (this ->* _handshake_table[opc])(reader);
        break;
      
      case PS_STATUS:
        if (opc > 0x01)
          { this->conn->disconnect (); return; }
        (this ->* _status_table[opc])(reader);
        break;
      
      case PS_LOGIN:
        if (opc > 0x01)
          { this->conn->disconnect (); return; }
        (this ->* _login_table[opc])(reader);
        break;
      
      case PS_PLAY:
        if (opc > 0x19)
          { this->conn->disconnect (); return; }
        (this ->* _play_table[opc])(reader);
        break;
      }
  }
  
  
  
  void
  mc18_packet_handler::set_connection (connection *conn)
  {
    this->conn = conn;
    
    this->builder = dynamic_cast<mc18_packet_builder *> (
      this->conn->get_protocol ()->get_builder ());
    if (!this->builder)
      throw std::runtime_error (
        "a 1.8 packet handler requires a packet builder supporting 1.8");
  }
  
  
  
  void
  mc18_packet_handler::disconnect ()
  {
    server& srv = this->conn->get_server ();
    
    if (this->pl)
      {
        srv.deregister_player (this->pl);
      }
  }
  
  
  
  void
  mc18_packet_handler::tick ()
  {
    if (this->pl)
      this->pl->tick ();
  }
}


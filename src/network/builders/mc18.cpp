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

#include "network/builders/mc18.hpp"
#include "network/packet.hpp"
#include "util/binary.hpp"
#include "world/chunk.hpp"
#include "util/json.hpp"
#include "entity/metadata.hpp"
#include <sstream>
#include <memory>
#include <cmath>


namespace hc {
  
  // adds a length field to the packet at its beginning.
  inline packet*
  _put_len (packet *pack)
  {
    unsigned char a[5];
    int vl = bin::write_varint (a, pack->get_length ());
    pack->use_reserved (vl);
    pack->put_bytes (a, vl);
    return pack;
  }
  
  
  static unsigned char
  _to_angle (float v)
  {
    return (unsigned char)((std::fmod (v, 360.0)/360.0) * 256.0);
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Status:
  // 
  
  packet*
  mc18_packet_builder::make_status_response (const std::string& str)
  {
    packet *pack = new packet ();
    pack->put_varint (0x00); // opcode
    pack->put_string (str.c_str ());
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_status_ping (unsigned long long time)
  {
    packet *pack = new packet ();
    pack->put_varint (0x01); // opcode
    pack->put_long (time);
    
    return _put_len (pack);
  }
  
  
  
//------------------------------------------------------------------------------
  // 
  // Login:
  // 
  
  packet*
  mc18_packet_builder::make_encryption_request (const std::string& sid,
    CryptoPP::RSA::PublicKey& pkey, unsigned char *vtoken)
  {
    CryptoPP::ByteQueue q;
    pkey.Save (q);
    
    unsigned char buf[384];
    int keylen = (int)q.Get (buf, sizeof buf);
    
    packet *pack = new packet ();
    pack->put_varint (0x01); // opcode
    pack->put_string (sid.c_str ()); // server id
    pack->put_varint (keylen);
    pack->put_bytes (buf, (int)keylen);
    pack->put_varint (4);
    pack->put_bytes (vtoken, 4);
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_login_success (const std::string& uuid,
    const std::string& username)
  {
    packet *pack = new packet ();
    pack->put_varint (0x02); // opcode
    pack->put_string (uuid.c_str ());
    pack->put_string (username.c_str ());
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_login_disconnect (const std::string& msg)
  {
    packet *pack = new packet ();
    pack->put_varint (0x00); // opcode
    
    std::ostringstream ss;
    json_writer writer (ss);
    json::j_object *js = new json::j_object ();
    js->set ("text", new json::j_string (msg));
    js->set ("color", new json::j_string ("red"));
    writer.write (js);
    pack->put_string (ss.str ().c_str ());
    delete js;
    
    return _put_len (pack);
  }
  
  
  
  
//------------------------------------------------------------------------------
  // 
  // Play:
  // 
  
  packet*
  mc18_packet_builder::make_keep_alive (int id)
  {
    packet *pack = new packet ();
    pack->put_varint (0x00); // opcode
    pack->put_varint (id);
    
    return _put_len (pack);
  }
  
  
  packet*
  mc18_packet_builder::make_join_game (int eid, unsigned char gm,
    unsigned char dim, unsigned char diff, unsigned char maxp,
    const std::string& level_type, bool reduced_debug)
  {
    packet *pack = new packet ();
    pack->put_varint (0x01); // opcode
    pack->put_int (eid);
    pack->put_byte (gm);
    pack->put_byte (dim);
    pack->put_byte (diff);
    pack->put_byte (maxp);
    pack->put_string (level_type.c_str ());
    pack->put_byte (reduced_debug);
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_chat_message (const std::string& js, int pos)
  {
    packet *pack = new packet ();
    pack->put_varint (0x02); // opcode
    pack->put_string (js.c_str ());
    pack->put_byte (pos);
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_chat_message (const std::string& msg)
  {
    using namespace json;
    
    std::unique_ptr<j_object> js { new j_object () };
    js->set ("text", new j_string (msg));
    
    std::ostringstream ss;
    json_writer writer (ss);
    writer.write (js.get ());
    
    return this->make_chat_message (ss.str (), 0);
  }
  
  
  
  packet*
  mc18_packet_builder::make_spawn_position (int x, int y, int z)
  {
    packet *pack = new packet ();
    pack->put_varint (0x05); // opcode
    pack->put_long (((long long)(x & 0x3FFFFFF) << 38) |
      ((long long)(y & 0xFFF) << 26) | (z & 0x3FFFFFF));
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_player_position_and_look (double x, double y, double z,
    float yaw, float pitch, unsigned char flags)
  {
    packet *pack = new packet ();
    pack->put_varint (0x08); // opcode
    pack->put_double (x);
    pack->put_double (y);
    pack->put_double (z);
    pack->put_float (yaw);
    pack->put_float (pitch);
    pack->put_byte (flags);
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_chunk_data (int cx, int cz, bool cont,
    unsigned short mask, chunk *ch)
  {
    packet *pack = new packet ();
    pack->put_varint (0x21); // opcode
    pack->put_int (cx);
    pack->put_int (cz);
    pack->put_bool (cont);
    pack->put_short (mask);
    
    int data_size = cont ? 256 : 0;
    for (int i = 0; i < 16; ++i)
      if (mask & (1 << i))
        data_size += 12288;
    pack->put_varint (data_size);
    
    for (int i = 0; i < 16; ++i)
      if (mask & (1 << i))
          pack->put_bytes (ch->get_sub (i)->types, 8192);
    for (int i = 0; i < 16; ++i)
      if (mask & (1 << i))
          pack->put_bytes (ch->get_sub (i)->bl, 2048);
    for (int i = 0; i < 16; ++i)
      if (mask & (1 << i))
          pack->put_bytes (ch->get_sub (i)->sl, 2048);
    if (cont)
      pack->put_bytes (ch->get_biomes (), 256);
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_unload_chunk (int cx, int cz)
  {
    packet *pack = new packet ();
    pack->put_varint (0x21); // opcode
    pack->put_int (cx);
    pack->put_int (cz);
    pack->put_bool (true); // ground-up continuous
    pack->put_short (0);   // primary bitmask
    pack->put_varint (0);  // data size
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_disconnect (const std::string& msg)
  {
    packet *pack = new packet ();
    pack->put_varint (0x40); // opcode
    
    std::ostringstream ss;
    json_writer writer (ss);
    json::j_object *js = new json::j_object ();
    js->set ("text", new json::j_string (msg));
    js->set ("color", new json::j_string ("red"));
    writer.write (js);
    pack->put_string (ss.str ().c_str ());
    delete js;
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_set_compression (int threshold)
  {
    packet *pack = new packet ();
    pack->put_varint (0x46); // opcode
    pack->put_varint (threshold);
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_player_list_add (uuid_t uuid,
    const std::string& name, game_mode gm, int ping)
  {
    packet *pack = new packet ();
    
    pack->put_varint (0x38); // opcode
    pack->put_varint (0);    // action (add player)
    pack->put_varint (1);    // number of elements
    for (int i = 0; i < 16; ++i)
      pack->put_byte (uuid.parts[i]);
    pack->put_string (name.c_str ());
    pack->put_varint (0);    // number of properties
    pack->put_varint ((gm == GM_CREATIVE) ? 1 : 0);
    pack->put_varint (ping);
    pack->put_bool (false);      // has display name
    
    return _put_len (pack);
  }
  
  packet*
  mc18_packet_builder::make_player_list_remove (uuid_t uuid,
    const std::string& name)
  {
    packet *pack = new packet ();
    
    pack->put_varint (0x38); // opcode
    pack->put_varint (4);    // action (remove player)
    pack->put_varint (1);    // number of elements
    for (int i = 0; i < 16; ++i)
      pack->put_byte (uuid.parts[i]);
    
    return _put_len (pack);
  }
  
  
  
  packet*
  mc18_packet_builder::make_spawn_player (int eid, uuid_t uuid, double x,
    double y, double z, float yaw, float pitch, short curr_item,
    const entity_metadata& meta)
  {
    packet *pack = new packet ();
    
    pack->put_varint (0x0C); // opcode
    pack->put_varint (eid);
    for (int i = 0; i < 16; ++i)
      pack->put_byte (uuid.parts[i]);
    pack->put_int ((int)(x * 32.0));
    pack->put_int ((int)(y * 32.0));
    pack->put_int ((int)(z * 32.0));
    pack->put_byte (_to_angle (yaw));
    pack->put_byte (_to_angle (pitch));
    pack->put_short (curr_item);
    meta.encode (pack);
    
    return _put_len (pack);
  }
}


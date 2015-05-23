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

#include "system/authenticator.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "os/http.hpp"
#include "util/json.hpp"
#include "util/uuid.hpp"
#include "player/uuid_manager.hpp"
#include <cryptopp/sha.h>
#include <cstring>
#include <sstream>

#include <iostream> // DEBUG


namespace hc {
  
  authenticator::authenticator (server& srv)
    : srv (srv)
  {
    
  }
  
  
  
  static std::string
  _digest_str (const unsigned char *ptr)
  {
    unsigned char digest[20];
    std::memcpy (digest, ptr, 20);
    bool neg = digest[0] & 0x80;
    
    if (neg)
      {
        // two's complement
        bool c = true;
        for (int i = 19; i >= 0; --i)
          {
            digest[i] = ~digest[i];
            if (c)
              {
                c = (digest[i] == 0xFF);
                ++ digest[i];
              }
          }
      }
    
    std::string str;
    if (neg)
      str.push_back ('-');
    
    static const char *_hex = "0123456789abcdef";
    for (int i = 0; i < 20; ++i)
      {
        str.push_back (_hex[digest[i] >> 4]);
        str.push_back (_hex[digest[i] & 15]);
      }
    
    for (auto itr = str.begin (); itr != str.end (); )
      {
        if (*itr == '0')
          itr = str.erase (itr);
        else
          break;
      }
    
    return str;
  }
  
  /*
   * Validates the authenticity of the specified player's connection and
   * returns the result to the specified callback when done.
   */
  void
  authenticator::authenticate (const std::string& name,
    const unsigned char *ssec, std::function<void (bool)>&& cb)
  {
    auto pkey = this->srv.get_pub_key ();
    CryptoPP::ByteQueue q;
    pkey.Save (q);
    unsigned char buf[384];
    int keylen = (int)q.Get (buf, sizeof buf);
    
    CryptoPP::SHA1 hash;
    unsigned char digest[20];
    hash.Update (ssec, 16);
    hash.Update (buf, keylen);
    hash.Final (digest);
    std::string digest_str = _digest_str (digest);
    
    std::string resp;
    try
      {
        resp = https_get ("sessionserver.mojang.com",
          "/session/minecraft/hasJoined?username=" + name + "&serverId="
          + digest_str, 5);
      }
    catch (const http_error&)
      {
        logger& log = this->srv.get_logger ();
        log (LT_ERROR)
          << "Could not connect to Mojang's session server to authenticate player '"
          << name << "'" << std::endl;
        cb (false);
        return;
      }
    
    if (resp.empty ())
      { cb (false); return; }
    
    std::istringstream iss (resp);
    json_reader reader (iss);
    std::unique_ptr<json::j_object> obj;
    
    try
      {
        obj.reset (reader.read ());
      }
    catch (const json_parse_error&)
      {
        cb (false);
        return;
      }
    
    auto id_o = obj->get ("id");
    if (!id_o || id_o->type () != json::JSON_STRING)
      {
        cb (false);
        return;
      }
    
    // TODO: do something with skin blob?
    
    uuid_t uuid = uuid_t::parse_hex (id_o->as_string ());
    this->srv.get_uuid_manager ().insert (name, uuid);
    cb (true);
  }
}


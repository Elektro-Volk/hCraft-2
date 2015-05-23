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

#include "player/uuid_manager.hpp"
#include "system/server.hpp"
#include "util/thread_pool.hpp"
#include "os/http.hpp"
#include "util/json.hpp"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <cstring>
#include <sstream>
#include <ctime>
#include <memory>

#ifndef WIN32
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
#else
# ifdef uuid_t
#   undef uuid_t
# endif
#endif


namespace hc {
  
  namespace {
    
    struct uuid_ctx_t {
      std::string name;
      std::function<void (uuid_t uuid)> cb;
      std::function<void ()> fail_cb;
    };
  }
  
  
  uuid_manager::uuid_manager (server& srv)
    : srv (srv)
  {
    this->online = false;
  }
  
  
  
  void
  uuid_manager::worker (const std::string& name,
      std::function<void (uuid_t uuid)>& cb,
      std::function<void ()>& fail_cb)
  {
    std::time_t t;
    std::time (&t);
    
    std::ostringstream ss;
    ss << "/users/profiles/minecraft/";
    ss << name << "?at=" << (unsigned long long)t;
    
    std::string resp;
    try
      {
        resp = https_get ("api.mojang.com", ss.str (), 5);
      }
    catch (const http_error&)
      {
        fail_cb ();
        return;
      }
    
    std::istringstream iss (resp);
    json_reader reader (iss);
    std::unique_ptr<json::j_object> obj;
    
    try
      {
        obj.reset (reader.read ());
      }
    catch (const json_parse_error&)
      {
        fail_cb ();
        return;
      }
    
    json::j_value *id_v = obj->get ("id");
    if (!id_v || id_v->type () != json::JSON_STRING)
      { fail_cb (); return; }
    
    uuid_t uuid = uuid_t::parse_hex (id_v->as_string ());
    this->cache[name] = uuid;
    cb (uuid);
  }
  
  
  
  /* 
   * If online, performs a Mojang API request to obtain a UUID from the
   * specified username; otherwise, generates a random UUIDv3-like UUID.
   */
  void
  uuid_manager::from_username (const std::string& name,
    std::function<void (uuid_t uuid)>&& cb,
    std::function<void ()>&& fail_cb)
  {
    auto itr = this->cache.find (name);
    if (itr != this->cache.end ())
      { cb (itr->second); return; }
    
    if (this->online)
      {
        uuid_ctx_t *ctx = new uuid_ctx_t;
        ctx->name = name;
        ctx->cb = cb;
        ctx->fail_cb = fail_cb;
        
        uuid_manager *that = this;
        this->srv.get_thread_pool ().enqueue (
          [that] (void *ptr)
            {
              uuid_ctx_t *ctx = static_cast<uuid_ctx_t *> (ptr);
              that->worker (ctx->name, ctx->cb, ctx->fail_cb);
              delete ctx;
            }, ctx);
      }
    else
      {
        uuid_t uuid = uuid_t::generate_v3 (name);
        //this->cache[name] = uuid;
        cb (uuid);
      }
  }
 
  /*
   * Puts the UUID manager in online mode.
   */
  void
  uuid_manager::set_online ()
  {
    if (!this->online)
      this->cache.clear ();
    this->online = true;
  }
  
  /* 
   * Inserts an entry into the manager's cache.
   */
  void
  uuid_manager::insert (const std::string& name, uuid_t uuid)
  {
    this->cache[name] = uuid;
  }
}


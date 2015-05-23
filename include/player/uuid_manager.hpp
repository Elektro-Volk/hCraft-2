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

#ifndef _hCraft2__PLAYER__UUID_MANAGER__H_
#define _hCraft2__PLAYER__UUID_MANAGER__H_

#include "util/uuid.hpp"
#include <string>
#include <unordered_map>
#include <functional>


namespace hc {
  
  // forward decs:
  class server;
  
  
  /* 
   * Provides an interface that can be used to obtain UUIDs from usernames.
   */
  class uuid_manager
  {
    server& srv;
    std::unordered_map<std::string, uuid_t> cache;
    bool online;
    
  public:
    uuid_manager (server& srv);
    
  private:
    void worker (const std::string& name,
      std::function<void (uuid_t uuid)>& cb,
      std::function<void ()>& fail_cb);
    
  public:
    /* 
     * If online, performs a Mojang API request to obtain a UUID from the
     * specified username; otherwise, generates a random UUIDv3-like UUID.
     */
    void from_username (const std::string& name,
      std::function<void (uuid_t uuid)>&& cb,
      std::function<void ()>&& fail_cb);
    
    /*
     * Puts the UUID manager in online mode.
     */
    void set_online ();
    
    /* 
     * Inserts an entry into the manager's cache.
     */
    void insert (const std::string& name, uuid_t uuid);
  };
}

#endif


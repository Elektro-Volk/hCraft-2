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

#ifndef _hCraft2__SYSTEM__AUTHENTICATOR__H_
#define _hCraft2__SYSTEM__AUTHENTICATOR__H_

#include <functional>


namespace hc {
  
  // forward decs:
  class server;
  
  /*
   * Handles the process of verifying whether a connected player has
   * authenticated with Mojang's servers.
   */
  class authenticator
  {
    server& srv;
    
  public:
    authenticator (server& srv);
    
  public:
    /*
     * Validates the authenticity of the specified player's connection and
     * returns the result to the specified callback when done.
     */
    void authenticate (const std::string& name, const unsigned char *ssec,
      std::function<void (bool)>&& cb);
  };
} 

#endif


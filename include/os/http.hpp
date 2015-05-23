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

#ifndef _hCraft2__OS__HTTP__H_
#define _hCraft2__OS__HTTP__H_

#include <string>
#include <stdexcept>


namespace hc {
  
  class http_error: public std::runtime_error
  {
  public:
    http_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  /* 
   * Sends an HTTPS GET request to the specified server for the given resource.
   * Returns the returned response's body as a string.
   * 
   * Throws `http_error' on failure.
   */
  std::string https_get (const char *server, const std::string& resource,
    int timeout_sec);
}

#endif


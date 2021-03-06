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

#ifndef _hCraft2__UTIL__UUID__H_
#define _hCraft2__UTIL__UUID__H_

#include <string>


namespace hc {
  
  struct uuid_t
  {
    unsigned char parts[16];
    
  public:
    bool operator== (const uuid_t other) const;
    bool operator!= (const uuid_t other) const;
    
  public:
    /* 
     * Returns a human-readable representation of the UUID.
     */
    std::string str ();
    
  public:
    /* 
     * Returns a random UUID.
     */
    static uuid_t generate_v4 ();
    
    /* 
     * Generates and returns a UUIDv3 from the specified string.
     */
    static uuid_t generate_v3 (const std::string& str);
    
    /*
     * Parses and returns a UUID from a 32 digit hex string.
     */
    static uuid_t parse_hex (const std::string& str);
  };
}

#endif


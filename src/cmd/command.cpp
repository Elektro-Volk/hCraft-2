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

#include "cmd/command.hpp"
#include <unordered_map>
#include <string>

// commands:
#include "cmd/info/help.hpp"


namespace hc {
  
  /* 
   * Creates and returns a command from the specified command name.
   */
  command*
  command::create (const char *name)
  {
#define CREATE_HANDLER(NAME) [] { return static_cast<command *> (new cmd_##NAME ()); }
#define DEFINE_CMD(NAME) { #NAME , CREATE_HANDLER(NAME) }
    
    const static std::unordered_map<std::string, command* (*) ()> _map {
      DEFINE_CMD(help)
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return nullptr;
    return itr->second ();
  }
}


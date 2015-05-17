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

#ifndef _hCraft2__CMD__COMMAND__H_
#define _hCraft2__CMD__COMMAND__H_

#include <string>


namespace hc {
  
  // forward decs:
  class player;
  
  
  /* 
   * Base class for all server/player commands.
   * Commands are invoked by prefixing their name with a forward slash '/'.
   */
  class command
  {
  public:
    virtual ~command () { }
    
  public:
    /* 
     * Returns the name of the command.
     */
    virtual const char* name () = 0;
    
    /* 
     * Executes the command for the specified player.
     */
    virtual void execute (player *pl, const std::string& args) = 0;
  
  public:
    /* 
     * Creates and returns a command from the specified command name.
     */
    static command* create (const char *name);
  };
}

#endif


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

#ifndef _hCraft2__UTIL__REFC__H_
#define _hCraft2__UTIL__REFC__H_

#include <atomic>


namespace hc {
  
  class ref_counter
  {
    std::atomic_int refc;
    
  public:
    inline void increment () { ++ this->refc; }
    inline void decrement () { -- this->refc; }
    inline bool zero () { return this->refc == 0; }
    inline int get () { return this->refc.load (); }
    
  public:
    ref_counter () { }
  };
}

#endif


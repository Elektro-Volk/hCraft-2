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

#include "util/uuid.hpp"
#include <random>
#include <chrono>
#include <mutex>


namespace hc {
  
  bool
  uuid_t::operator== (const uuid_t other) const
  {
    for (int i = 0; i < 4; ++i)
      if (((unsigned int *)this->parts)[i] != ((unsigned int *)other.parts)[i])
        return false;
    
    return true;
  }
  
  bool
  uuid_t::operator!= (const uuid_t other) const
  {
    return !(this->operator== (other));
  }
  
  
  
  /* 
   * Returns a human-readable representation of the UUID.
   */
  std::string
  uuid_t::str ()
  {
    static const char *hex = "0123456789abcdef";
    
    std::string out;
    
    for (int i = 0; i < 16; ++i)
      {
        if (i == 4 || i == 6 || i == 8 || i == 10)
          out.push_back ('-');
        
        unsigned char p = this->parts[i];
        out.push_back (hex[p >> 4]);
        out.push_back (hex[p & 15]);
      }
    
    return out;
  }
  
  
  
  /* 
   * Returns a random UUID.
   */
  uuid_t
  uuid_t::random ()
  {
    static bool _init = false;
    static std::mt19937 _rnd;
    static std::uniform_int_distribution<> _dis (0x00, 0xFF);
    static std::mutex mtx;
    
    std::lock_guard<std::mutex> guard (mtx);
    
    if (!_init)
      {
        _init = true;
        _rnd.seed ((unsigned long)std::chrono::duration_cast<std::chrono::nanoseconds> (
          std::chrono::high_resolution_clock::now ().time_since_epoch ())
            .count ());
      }
    
    uuid_t uuid;
    for (int i = 0; i < 16; ++i)
      uuid.parts[i] = _dis (_rnd);
    return uuid;
  }
}


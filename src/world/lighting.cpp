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

#include "world/lighting.hpp"
#include "util/thread.hpp"
#include "world/chunk.hpp"
#include "util/position.hpp"
#include "world/blocks.hpp"
#include <functional>
#include <chrono>
#include <queue>


namespace hc {
  
  lighting_manager::lighting_manager ()
  {
    this->running = false;
  }
  
  lighting_manager::~lighting_manager ()
  {
    this->stop ();
  }
  
  
  
  void
  lighting_manager::worker_func ()
  {
#define UPDATE_PER_CYCLE    1000
#define SLEEP_TIME             2
    
    while (this->running)
      {
        std::this_thread::sleep_for (std::chrono::milliseconds (SLEEP_TIME));
        
        
      }
  }
  
  
  
  /* 
   * Starts the specified amount of worker threads.
   */
  void
  lighting_manager::start (int count)
  {
    if (this->running)
      return;
    
    this->running = true;
    
    for (int i = 0; i < count; ++i)
      {
        worker *w = new worker;
        w->th = new hc::thread (
          std::bind (std::mem_fn (&lighting_manager::worker_func), this));
        
        this->workers.push_back (w);
      }
  }
  
  /* 
   * Stops all worker threads.
   */
  void
  lighting_manager::stop ()
  {
    if (!this->running)
      return;
    
    this->running = false;
    
    for (worker *w : this->workers)
      {
        if (w->th->joinable ())
          w->th->join ();
        delete w->th;
        
        delete w;
      }
  }
  
  
  
  /* 
   * (Re)lights the specified chunk as much as possible, without taking its
   * adjacent neighbours into consideration.
   */
  void
  lighting_manager::light_chunk (chunk *ch)
  {
    std::queue<block_pos> slu;
    
    for (int x = 0; x < 16; ++x)
      for (int z = 0; z < 16; ++z)
        {
          // skylight value at y=255 is always 15.
          ch->set_sky_light (x, 255, z, 15);
          
          int sl = 15;
          int h = ch->get_height (x, z);
          for (int y = 254; y >= h; --y)
            {
              auto binf = block_info::from_id (ch->get_id (x, y, z));
              if (binf)
                {
                  sl -= binf->opacity;
                  if (sl < 0)
                    sl = 0;
                }
              
              ch->set_sky_light (x, y, z, sl);
              slu.emplace (x, y, z);
            }
          
          for (int y = h - 1; y >= 0; --y)
            ch->set_sky_light (x, y, z, 0);
        }
  }
}


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
#include "world/world.hpp"
#include <functional>
#include <chrono>
#include <queue>

#include <iostream> // DEBUG


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
#define UPDATES_PER_CYCLE       1000
#define SLEEP_TIME                 2
#define OVERLOAD_THRESHOLD   2000000
    
    while (this->running)
      {
        std::this_thread::sleep_for (std::chrono::milliseconds (SLEEP_TIME));
        if (this->updates.empty ())
          continue;
        
        std::lock_guard<std::mutex> guard { this->mtx };
        
        int processed;
        for (processed = 0; processed < UPDATES_PER_CYCLE; ++processed)
          {
            if (this->updates.empty ())
              break;
            work_item item = this->updates.back ();
            this->updates.pop_back ();
            
            this->calc_sl (item.w, item.x, item.y, item.z);
            
          }
      }
  }
  
  
  
#ifndef MAX
# define MAX(A, B) (((A) > (B)) ? (A) : (B))
#endif
  
  /* 
   * Calculates the right sky light value for the specified block and
   * queues its neighbours for further checking.
   */
  void
  lighting_manager::calc_sl (world *w, int x, int y, int z)
  {
    chunk *ch = w->get_chunk (x >> 4, z >> 4);
    if (!ch)
      return;
    
    int bx = x & 15;
    int bz = z & 15;
    
    // neighbouring chunks:
    chunk *east = ch->get_neighbour (DIR_EAST);
    chunk *west = ch->get_neighbour (DIR_WEST);
    chunk *north = ch->get_neighbour (DIR_NORTH);
    chunk *south = ch->get_neighbour (DIR_SOUTH);
    
    int sl;
    if (y >= ch->get_height (bx, bz))
      sl = 15;
    else
      {
        // neighbouring block sl values
        int n_xp = (bx == 15)     // +x
          ? (east ? east->get_sky_light (0, y, bz) : 0)
          : ch->get_sky_light (bx + 1, y, bz);
        int n_xn = (bx == 0)      // -x
          ? (west ? west->get_sky_light (15, y, bz) : 0)
          : ch->get_sky_light (bx - 1, y, bz);
        int n_zp = (bz == 15)     // +z
          ? (south ? south->get_sky_light (bx, y, 0) : 0)
          : ch->get_sky_light (bx, y, bz + 1);
        int n_zn = (bz == 0)      // -z
          ? (north ? north->get_sky_light (bx, y, 15) : 0)
          : ch->get_sky_light (bx, y, bz - 1);
        int n_yp = (y == 255) ? 15 : ch->get_sky_light (bx, y + 1, bz);
        int n_yn = (y == 0) ? 0 : ch->get_sky_light (bx, y - 1, bz);
        
        int max =
          MAX(n_xp, MAX(n_xn, MAX(n_zp, MAX(n_zn, MAX(n_yp, MAX(n_yn, 0))))));
        sl = max - 1;
        if (sl < 0)
          sl = 0;
      }
      
    if (sl != ch->get_sky_light (bx, y, bz))
      {
        //std::cout << "(" << x << ", " << y << ", " << z << "): " << sl << std::endl;
        ch->set_sky_light (bx, y, bz, sl);
        this->enqueue_sl_neighbours (w, x, y, z);
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
  
  
  
  /* 
   * Queues a lighting update for the specified block.
   */
  void
  lighting_manager::enqueue (world *w, int x, int y, int z)
  {
    std::lock_guard<std::mutex> guard { this->mtx };
    this->enqueue_sl_no_lock (w, x, y, z);
  }
  
  
  // no locking
  void
  lighting_manager::enqueue_sl_no_lock (world *w, int x, int y, int z)
  {
    chunk *ch = w->get_chunk (x >> 4, z >> 4);
    int bx = x & 15;
    int bz = z & 15;
    
    // block must be able to pass light
    block_info *binf = block_info::from_id (ch->get_id (bx, y, bz));
    if (!binf || binf->opaque)
      {
        unsigned char nsl = 15 - binf->opacity;
        if (nsl != ch->get_sky_light (bx, y, bz))
          {
            ch->set_sky_light (bx, y, bz, nsl);
            this->enqueue_sl_neighbours (w, x, y, z);
          }
      }
    else
      {
        work_item item;
        item.w = w;
        item.x = x;
        item.y = y;
        item.z = z;
        this->updates.push_back (item);
      }
  }
  
  void
  lighting_manager::enqueue_sl_neighbours (world *w, int x, int y, int z)
  {
    this->enqueue_sl_no_lock (w, x - 1, y, z);
    this->enqueue_sl_no_lock (w, x + 1, y, z);
    this->enqueue_sl_no_lock (w, x, y, z - 1);
    this->enqueue_sl_no_lock (w, x, y, z + 1);
    if (y > 0)
      this->enqueue_sl_no_lock (w, x, y - 1, z);
    if (y < 255)
      this->enqueue_sl_no_lock (w, x, y + 1, z);
  }
}


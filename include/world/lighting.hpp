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

#ifndef _hCraft2__WORLD__LIGHTING__H_
#define _hCraft2__WORLD__LIGHTING__H_

#include <vector>
#include <deque>
#include <mutex>


namespace hc {
  
  // forward decs:
  class thread;
  class chunk;
  class world;
  
  /* 
   * In charge of properly setting lighting values for blocks.
   * Processes updates in one or more threads dedicated to lighting work.
   */
  class lighting_manager
  {
  private:
    struct worker
    {
      hc::thread *th;
    };
    
    struct work_item
    {
      world *w;
      int x, y, z;
    };
    
  private:
    std::vector<worker *> workers;
    std::deque<work_item> updates;
    std::mutex mtx;
    bool running;
    
  public:
    lighting_manager ();
    ~lighting_manager ();
    
  private:
    void worker_func ();
    
  private:
    /* 
     * Calculates the right sky light value for the specified block and
     * queues its neighbours for further checking.
     */
    void calc_sl (world *w, int x, int y, int z);
    
    void enqueue_sl_no_lock (world *w, int x, int y, int z);
    
    void enqueue_sl_neighbours (world *w, int x, int y, int z);
    
  public:
    /* 
     * Starts the specified amount of worker threads.
     */
    void start (int count);
    
    /* 
     * Stops all worker threads.
     */
    void stop ();
    
    
    
    /* 
     * (Re)lights the specified chunk as much as possible, without taking its
     * adjacent neighbours into consideration.
     */
    void light_chunk (chunk *ch);
    
    /* 
     * Queues a lighting update for the specified block.
     */
    void enqueue (world *w, int x, int y, int z);
  };
}

#endif


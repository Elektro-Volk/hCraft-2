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

#include "util/thread.hpp"
#include <mutex>
#include <unordered_map>


namespace hc {
  
  static std::unordered_map<std::thread::id, thread *> th_map;
  static std::mutex th_map_mtx;
  
  
  
  /* 
   * Called right after a thread is created and starts running.
   */
  void
  thread::on_start ()
  {
    this->fin_mtx = new std::mutex ();
    
    std::lock_guard<std::mutex> guard { th_map_mtx };
    th_map[std::this_thread::get_id ()] = this;
  }
  
  /* 
   * Called right after a thread finishes running.
   */
  void
  thread::on_exit ()
  {
    std::lock_guard<std::mutex> guard { *this->fin_mtx };
    for (auto itr = this->fins.rbegin (); itr != this->fins.rend (); ++itr)
      (*itr) ();
    this->fins.clear ();
    
    std::lock_guard<std::mutex> guard2 { th_map_mtx };
    th_map.erase (std::this_thread::get_id ());
  }
  
  
  /* 
   * Called by the main function.
   */
  void
  thread::on_main_exit ()
  {
    hc::thread *mt;
    
    {
      std::lock_guard<std::mutex> guard { *this->fin_mtx };
      for (auto itr = this->fins.rbegin (); itr != this->fins.rend (); ++itr)
        (*itr) ();
      this->fins.clear ();
      
      std::lock_guard<std::mutex> guard2 { th_map_mtx };
      auto itr = th_map.find (std::this_thread::get_id ());
      mt = itr->second;
      th_map.erase (itr);
    }
    
    delete mt;
  }
  
  
  
  /* 
   * Adds a destructor that will be called when the thread exits.
   */
  void
  thread::add_destructor (std::function<void ()>&& fn)
  {
    std::lock_guard<std::mutex> guard { *this->fin_mtx };
    this->fins.push_back (fn);
  }
  
  
  
  /*
   * Returns an instance to the calling thread.
   */
  thread&
  thread::this_thread ()
  {
    std::lock_guard<std::mutex> guard { th_map_mtx };
    return *th_map[std::this_thread::get_id ()];
  }
  
  /* 
   * Should be called from the main thread.
   * Sets up things so that stuff will work from within the main function.
   */
  void
  thread::init_main ()
  {
    std::lock_guard<std::mutex> guard { th_map_mtx };
    th_map[std::this_thread::get_id ()] = new thread ();
  }
}


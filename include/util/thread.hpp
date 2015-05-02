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

#ifndef _hCraft2__UTIL__THREAD__H_
#define _hCraft2__UTIL__THREAD__H_

#include <thread>
#include <functional>
#include <vector>
#include <mutex>


namespace hc {
  
  /* 
   * Extends a regular standard library thread, and provides a way to
   * execute finalizer code when a thread terminates.
   */
  class thread: public std::thread
  {
    std::vector<std::function<void ()>> fins;
    std::mutex *fin_mtx;
    
  private:
    thread ()
      : std::thread ()
      { this->fin_mtx = new std::mutex (); }
    
  public:
    thread (thread&& other) = delete;
    thread (const thread& other) = delete;
    
    thread (std::function<void (void *)>&& fn, void *ptr = nullptr)
      : std::thread (
        [=] {
          this->on_start ();
          fn (ptr);
          this->on_exit ();
        })
      { }
    
    ~thread ()
      { delete this->fin_mtx; }
    
  private:
    /* 
     * Called right after a thread is created and starts running.
     */
    void on_start ();
    
    /* 
     * Called right after a thread finishes running.
     */
    void on_exit ();
  
  public:
    /* 
     * Called by the main function.
     */
    void on_main_exit ();
    
  public:
    /* 
     * Adds a destructor that will be called when the thread exits.
     */
    void add_destructor (std::function<void ()>&& fn);
  
  public:
    /*
     * Returns an instance to the calling thread.
     */
    static thread& this_thread ();
    
    /* 
     * Should be called from the main thread.
     * Sets up things so that stuff will work from within the main function.
     */
    static void init_main ();
  };
  
  
  
  /* 
   * RAII class for handling things in the main thread.
   */
  class main_thread_raii
  {
  public:
    main_thread_raii ()
      { hc::thread::init_main (); }
    
    ~main_thread_raii ()
      { hc::thread::this_thread ().on_main_exit (); }
  };
}

#endif


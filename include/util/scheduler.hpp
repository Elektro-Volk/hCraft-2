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

#ifndef _hCraft2__UTIL__SCHEDULER__H_
#define _hCraft2__UTIL__SCHEDULER__H_

#include <functional>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>


namespace hc {
  
  // forward decs:
  class thread;
  
  /* 
   * Runs scheduled tasks in a separate thread.
   */
  class scheduler
  {
  public:
    class task
    {
    private:
      friend class scheduler;
      
      scheduler& sch;
      std::function<void (task&)> fn;
      void *ctx;
      int interval;
      std::chrono::system_clock::time_point next;
      bool once;
      bool active;
      
    public:
      inline void* get_context () { return this->ctx; }
      
    public:
      task (scheduler& sch)
        : sch (sch)
        { }
        
    public:
      void run_once (int delay = 0);
      void run (int interval, int delay = 0);
    };
    
  private:
    std::deque<task *> tasks;
    hc::thread *th;
    bool running;
    std::mutex task_mtx;
    
  public:
    scheduler ();
    ~scheduler ();
    
  private:
    void worker_func ();
    
  public: 
    /* 
     * Starts the scheduler's thread and begins processing tasks.
     */
    void start ();
    
    /* 
     * Stops the scheduler's thread.
     */
    void stop ();
    
  public:
    /* 
     * Creates and returns a new task.
     */
    task& create (std::function<void (task&)> fn, void *ctx = nullptr);
  };
}

#endif


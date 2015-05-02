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

#include "util/scheduler.hpp"
#include "util/thread.hpp"


namespace hc {
  
  void
  scheduler::task::run_once (int delay)
  {
    this->next = std::chrono::system_clock::now () + std::chrono::milliseconds (delay);
    this->interval = 0;
    this->once = true;
    
    this->active = true;
  }
  
  void
  scheduler::task::run (int interval, int delay)
  {
    this->next = std::chrono::system_clock::now () + std::chrono::milliseconds (delay);
    this->interval = interval;
    this->once = false;
    
    this->active = true;
  }
  
  
  
  scheduler::scheduler ()
  {
    this->th = nullptr;
    this->running = false;
  }
  
  scheduler::~scheduler ()
  {
    this->stop ();
  }
  
  
  
  void
  scheduler::worker_func ()
  {
    while (this->running)
      {
        std::this_thread::sleep_for (std::chrono::milliseconds (20));
        std::lock_guard<std::mutex> guard { this->task_mtx };
        
        auto now = std::chrono::system_clock::now ();
        
        int s = (int)this->tasks.size ();
        for (int i = 0; i < s; ++i)
          {
            task *t = this->tasks.front ();
            this->tasks.pop_front ();
            
            if (t->active && (now >= t->next))
              {
                t->fn (*t);
                now = std::chrono::system_clock::now ();
                
                if (!t->once)
                  {
                    t->next = now + std::chrono::milliseconds (t->interval);
                    this->tasks.push_back (t);
                  }
              }
            else
              this->tasks.push_back (t);
          }
      }
  }
  
  
  
  /* 
   * Starts the scheduler's thread and begins processing tasks.
   */
  void
  scheduler::start ()
  {
    if (this->running)
      return;
    
    this->running = true;
    this->th = new hc::thread (std::bind (
      std::mem_fn (&scheduler::worker_func), this));
  }
  
  /* 
   * Stops the scheduler's thread.
   */
  void
  scheduler::stop ()
  {
    if (!this->running)
      return;
    
    this->running = false;
    if (this->th->joinable ())
      this->th->join ();
    delete this->th;
    
    std::lock_guard<std::mutex> guard { this->task_mtx };
    for (auto t : this->tasks)
      delete t;
  }
  
  
  
  /* 
   * Creates and returns a new task.
   */
  scheduler::task&
  scheduler::create (std::function<void (task&)> fn, void *ctx)
  {
    task *t = new task (*this);
    t->fn = fn;
    t->ctx = ctx;
    t->active = false;
    
    std::lock_guard<std::mutex> guard { this->task_mtx };
    this->tasks.push_back (t);
    return *t;
  }
}


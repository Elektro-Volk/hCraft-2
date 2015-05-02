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

#ifndef _hCraft2__UTIL__THREAD_POOL__H_
#define _hCraft2__UTIL__THREAD_POOL__H_

#include <vector>
#include <condition_variable>
#include <queue>
#include <deque>
#include <mutex>
#include <functional>


namespace hc {
  
  // forward decs:
  class thread;
  class ref_counter;
  
  class thread_pool
  {
  private:
    struct worker_thread
    {
      hc::thread *th;
      
    public:
      worker_thread (hc::thread *th)
        : th (th)
        { }
    };
    
    struct job
    {
      std::function<void (void *)> fn;
      void *ctx;
      ref_counter *refc;
    };
    
  public:
    /* 
     * Used to allow sequential execution of jobs.
     * Jobs in the same "sequence class" will execute sequentially.
     */
    struct seq_class
    {
      std::mutex mtx;
      bool free;
      bool accepting;
      std::deque<job *> jobs;
    };
  
  private:
    std::vector<worker_thread *> ths;
    bool running, accepting;
    std::condition_variable cv, cv_done;
    std::queue<job *> jobs;
    std::mutex mtx;
  
  public:
    thread_pool ();
    ~thread_pool ();
    
  private:
    void worker_func (void *ctx);
    
    static void seq_wrapper (void *ptr);
    
  public:
    /* 
     * Initializes the thread pool by creating the specified amount of worker
     * threads.
     */
    void init (int count);
    
    /* 
     * Waits for all jobs to complete.
     */
    void join ();
    
    /* 
     * Stops all worker threads and clears them.
     */
    void stop ();
    
  public:
    /* 
     * Queues a new job for the thread pool worker threads to handle.
     */
    bool enqueue (std::function<void (void *)> fn, void *ctx = nullptr);
    
    bool enqueue (std::function<void (void *)> fn, void *ctx, ref_counter& refc);
    
  public:
    /* 
     * Creates a new job sequence class.
     * release_seq() should be called once the returned object is no longer
     * needed.
     */
    seq_class* create_seq ();
    
    /* 
     * Frees a job sequence class.
     * The context pointers of all unfinished jobs are passed to the specified
     * callback function to free resources.
     */
    void release_seq (seq_class *seq, std::function<void (void *)>&& cb);
    
    /* 
     * Stops all jobs in the specified sequence and disables adding future jobs.
     * The context pointers of all unfinished jobs are passed to the specified
     * callback function to free resources.
     */
    void disable_seq (seq_class *seq, std::function<void (void *)>&& cb);
    
    /* 
     * Stops all jobs in the specified sequence for which the return value of
     * the given callback function is true.  The callback function accepts
     * the context pointer of each job, and serves as a finalizer function
     * when it returns true.
     */
    void stop_jobs (seq_class *seq, std::function<bool (void *)>&& cb);
    
    /* 
     * Queues a new job to be executed in the specified sequence.
     */
    bool enqueue_seq (seq_class *seq, std::function<void (void *)>&& fn,
      void *ctx = nullptr);
    
    bool enqueue_seq (seq_class *seq, std::function<void (void *)>&& fn,
      void *ctx, ref_counter& refc);
  };
}

#endif


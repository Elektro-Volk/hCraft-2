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

#include "util/thread_pool.hpp"
#include "util/thread.hpp"
#include "util/refc.hpp"


namespace hc {
  
  thread_pool::thread_pool ()
  {
    this->running = false;
    this->accepting = false;
  }
  
  thread_pool::~thread_pool ()
  {
    this->stop ();
  }
  
  
  
  void
  thread_pool::worker_func (void *ctx)
  {
    std::unique_lock<std::mutex> guard { this->mtx };
    while (this->running)
      {
        this->cv.wait (guard,
          [&] { return !this->running || !this->jobs.empty (); });
        
        if (!this->running)
          break;
        else
          {
            job *j = this->jobs.front ();
            this->jobs.pop ();
            guard.unlock ();
            
            j->fn (j->ctx);
            delete j;
            
            if (this->jobs.empty ())
              this->cv_done.notify_one ();
            guard.lock ();
          }
      }
  }
  
  
  
  /* 
   * Initializes the thread pool by creating the specified amount of worker
   * threads.
   */
  void
  thread_pool::init (int count)
  {
    this->running = true;
    this->accepting = true;
    
    for (int i = 0; i < count; ++i)
      {
        hc::thread *th = new hc::thread (
          std::bind (std::mem_fn (&thread_pool::worker_func), this,
            std::placeholders::_1));
        worker_thread *wth = new worker_thread (th);        
        this->ths.push_back (wth);
      }
  }
  
  /* 
   * Waits for all jobs to complete.
   */
  void
  thread_pool::join ()
  {
    std::unique_lock<std::mutex> guard { this->mtx };
    
    this->accepting = false;
    this->cv_done.wait (guard, [&] { return this->jobs.empty (); });
  }
  
  /* 
   * Stops all worker threads and clears them.
   */
  void
  thread_pool::stop ()
  {
    if (!this->running)
      return;
    
    this->running = false;
    this->cv.notify_all ();
    
    for (worker_thread *wth : this->ths)
      {
        if (wth->th->joinable ())
          wth->th->join ();
        delete wth->th;
        delete wth;
      }
  }
  
  
  
  /* 
   * Queues a new job for the thread pool worker threads to handle.
   */
  bool
  thread_pool::enqueue (std::function<void (void *)> fn, void *ctx)
  {
    std::unique_lock<std::mutex> guard { this->mtx };
    if (!this->accepting)
      return false;
    
    job *j = new job ();
    j->fn = fn;
    j->ctx = ctx;
    j->refc = nullptr;
    this->jobs.push (j);
    
    guard.unlock ();
    this->cv.notify_one ();
    return true;
  }
  
  bool
  thread_pool::enqueue (std::function<void (void *)> fn, void *ctx,
    ref_counter& refc)
  {
    std::unique_lock<std::mutex> guard { this->mtx };
    if (!this->accepting)
      return false;
    
    ref_counter *rfc = &refc;
    
    job *j = new job ();
    j->fn = [fn, rfc] (void *ctx)
      {
        fn (ctx);
        rfc->decrement ();
      };
    j->ctx = ctx;
    j->refc = &refc;
    this->jobs.push (j);
    
    refc.increment ();
    
    guard.unlock ();
    this->cv.notify_one ();
    return true;
  }
  
  
  
  /* 
   * Creates a new job sequence class.
   * release_seq() should be called once the returned object is no longer
   * needed.
   */
  thread_pool::seq_class*
  thread_pool::create_seq ()
  {
    seq_class *seq = new seq_class ();
    seq->free = true;
    seq->accepting = true;
    return seq;
  }
  
  /* 
   * Frees a job sequence class.
   */
  void
  thread_pool::release_seq (seq_class *seq, std::function<void (void *)>&& cb)
  {
    this->disable_seq (seq, std::move (cb));
    delete seq;
  }
  
  /* 
   * Stops all jobs in the specified sequence and disables adding future jobs.
   * The context pointers of all unfinished jobs are passed to the specified
   * callback function to free resources.
   */
  void
  thread_pool::disable_seq (seq_class *seq, std::function<void (void *)>&& cb)
  {
    std::lock_guard<std::mutex> guard { seq->mtx };
    
    seq->accepting = false;
    seq->free = true;
    while (!seq->jobs.empty ())
      {
        auto job = seq->jobs.front ();
        seq->jobs.pop_front ();
        
        cb (job->ctx);
        delete job;
      }
  }
  
  /* 
   * Stops all jobs in the specified sequence for which the return value of
   * the given callback function is true.  The callback function accepts
   * the context pointer of each job, and serves as a finalizer function
   * when it returns true.
   */
  void
  thread_pool::stop_jobs (seq_class *seq, std::function<bool (void *)>&& cb)
  {
    std::lock_guard<std::mutex> guard { seq->mtx };
    
    for (auto itr = seq->jobs.begin (); itr != seq->jobs.end (); )
      {
        auto job = *itr;
        if (cb (job->ctx))
          itr = seq->jobs.erase (itr);
        else
          ++ itr;
      }
    
    if (seq->jobs.empty ())
      {
        seq->free = true;
      }
  }
  
  
  
  namespace {
    
    struct seq_ctx_t
    {
      std::function<void (void *)> fn;
      void *ctx;
      
      thread_pool::seq_class *seq;
      thread_pool *pool;
    };
  }
  
  void
  thread_pool::seq_wrapper (void *ptr)
  {
    seq_ctx_t *ctx = static_cast<seq_ctx_t *> (ptr);
    auto *seq = ctx->seq;
    
    ctx->fn (ctx->ctx);
    
    std::lock_guard<std::mutex> guard { seq->mtx };
    
    if (seq->jobs.empty ())
      {
        delete ctx;
        seq->free = true;
      }
    else
      {
        thread_pool::job *j = seq->jobs.front ();
        seq->jobs.pop_front ();
        
        ctx->fn = j->fn;
        ctx->ctx = j->ctx;
        if (j->refc)
          ctx->pool->enqueue (&thread_pool::seq_wrapper, ctx, *j->refc);
        else
          ctx->pool->enqueue (&thread_pool::seq_wrapper, ctx);
        delete j;
      }
  }
  
  /* 
   * Queues a new job to be executed in the specified sequence.
   */
  bool
  thread_pool::enqueue_seq (seq_class *seq, std::function<void (void *)>&& fn,
    void *ctx)
  {
    std::lock_guard<std::mutex> guard { seq->mtx };
    if (!seq->accepting)
      return false;
    
    if (seq->free)
      {
        seq->free = false;
        
        seq_ctx_t *ptr = new seq_ctx_t ();
        ptr->fn = fn;
        ptr->ctx = ctx;
        ptr->seq = seq;
        ptr->pool = this;
        
        bool res = this->enqueue (&thread_pool::seq_wrapper, ptr);
        if (!res)
          delete ptr;
        return res;
      }
    else
      {
        job *j = new job ();
        j->fn = fn;
        j->ctx = ctx;
        j->refc = nullptr;
        seq->jobs.push_back (j);
      }
    
    return true;
  }
  
  bool
  thread_pool::enqueue_seq (seq_class *seq, std::function<void (void *)>&& fn,
    void *ctx, ref_counter& refc)
  {
    std::lock_guard<std::mutex> guard { seq->mtx };
    if (!seq->accepting)
      return false;
    
    if (seq->free)
      {
        seq->free = false;
        
        seq_ctx_t *ptr = new seq_ctx_t ();
        ptr->fn = fn;
        ptr->ctx = ctx;
        ptr->seq = seq;
        ptr->pool = this;
        
        bool res = this->enqueue (&thread_pool::seq_wrapper, ptr, refc);
        if (!res)
          delete ptr;
        return res;
      }
    else
      {
        job *j = new job ();
        j->fn = fn;
        j->ctx = ctx;
        j->refc = &refc;
        seq->jobs.push_back (j);
      }
    
    return true;
  }
}


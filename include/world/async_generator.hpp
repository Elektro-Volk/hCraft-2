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

#ifndef _hCraft2__WORLD__ASYNC_GENERATOR__H_
#define _hCraft2__WORLD__ASYNC_GENERATOR__H_

#include "util/thread_pool.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>


namespace hc {
  
  // forward decs:
  class chunk;
  class world;
  class player;
  
  /* 
   * Provides a convenient interface to generate chunks asynchronously.
   */
  class async_generator
  {
  private:
    struct token
    {
      int id;
      bool enabled;
    };
    
  private:
    world& w;
    thread_pool::seq_class *seq;
    int next_tok;
    std::mutex gen_mtx;
    std::unordered_map<int, token *> toks;
    
  public:
    async_generator (world& w, thread_pool::seq_class *seq);
    ~async_generator ();
    
  private:
    static void gen_func (void *ptr);
    
  public:
    /* 
     * Returns a new token that can be used to make generation requests.
     */
    int make_token ();
    
    /* 
     * Stops all generation requests associated with the specified token and
     * disables it.
     */
    void free_token (int tok);
    
    
    
    /* 
     * First checks if the chunk located at the specified coordinates is
     * already loaded. If it is, it is returned; otherwise, its generation is
     * deferred to a separate task that will call the given callback function
     * when done and null is returned.
     */
    chunk* generate (int token, int x, int z,
      std::function<void (world *w, chunk *, int, int)>&& cb,
      ref_counter *refc = nullptr);
  };
}

#endif


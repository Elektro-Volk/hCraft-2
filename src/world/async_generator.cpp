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

#include "world/async_generator.hpp"
#include "world/world.hpp"
#include "system/server.hpp"
#include "player/player.hpp"
#include <memory>


namespace hc {
  
  async_generator::async_generator (world& w, thread_pool::seq_class *seq)
    : w (w)
  {
    this->seq = seq;
    this->next_tok = 1;
  }
  
  async_generator::~async_generator ()
  {
    
  }
  
  
  
  /* 
   * Returns a new token that can be used to make generation requests.
   */
  int
  async_generator::make_token ()
  {
    std::lock_guard<std::mutex> guard { this->gen_mtx };
    
    token *tok = new token ();
    tok->id = this->next_tok++;
    tok->enabled = true;
    
    this->toks[tok->id] = tok;
    return tok->id;
  }
  
  /* 
   * Stops all generation requests associated with the specified token and
   * disables it.
   */
  void
  async_generator::free_token (int tid)
  {
    std::lock_guard<std::mutex> guard { this->gen_mtx };
    
    auto itr = this->toks.find (tid);
    if (itr == this->toks.end ())
      return;
      
    token *tok = itr->second;
    this->toks.erase (itr);
    delete tok;
  }
  
  
  
  namespace {
    
    struct gen_ctx_t
    {
      async_generator *agen;
      world *w;
      int token;
      int x, z;
      std::function<void (world *w, chunk *, int, int)> cb;
    };
  }
  
  void
  async_generator::gen_func (void *ptr)
  {
    std::unique_ptr<gen_ctx_t> ctx { static_cast<gen_ctx_t *> (ptr) };
    token *tok = ctx->agen->toks[ctx->token];
    
    if (!tok->enabled)
      return;
    
    chunk *ch = ctx->w->load_chunk (ctx->x, ctx->z);
    ctx->cb (ctx->w, ch, ctx->x, ctx->z);
  }
  
  /* 
   * First checks if the chunk located at the specified coordinates is
   * already loaded. If it is, it is returned; otherwise, its generation is
   * deferred to a separate task that will call the given callback function
   * when done and null is returned.
   */
  chunk*
  async_generator::generate (int tid, int x, int z,
    std::function<void (world *w, chunk *, int, int)>&& cb, ref_counter *refc)
  {
    token *tok;
    {
      std::lock_guard<std::mutex> guard { this->gen_mtx };
      
      auto itr = this->toks.find (tid);
      if (itr == this->toks.end ())
        return nullptr;
      
      tok = itr->second;
    }
    
    if (!tok->enabled)
      return nullptr;
    
    chunk *ch = this->w.get_chunk (x, z);
    if (ch)
      return ch;
    
    gen_ctx_t *ctx = new gen_ctx_t;
    ctx->agen = this;
    ctx->w = &this->w;
    ctx->token = tid;
    ctx->x = x;
    ctx->z = z;
    ctx->cb = cb;
    
    if (refc)
      this->w.get_server ().get_thread_pool ().enqueue_seq (this->seq,
        &async_generator::gen_func, ctx, *refc);
    else
      this->w.get_server ().get_thread_pool ().enqueue_seq (this->seq,
        &async_generator::gen_func, ctx);
    return nullptr;
  }
}


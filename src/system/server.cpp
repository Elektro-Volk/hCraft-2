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

#include "system/server.hpp"
#include "system/logger.hpp"
#include "network/connection.hpp"
#include "util/scheduler.hpp"
#include "util/thread.hpp"
#include "network/protocol.hpp"
#include "world/world.hpp"
#include "world/world_generator.hpp"
#include "world/world_provider.hpp"
#include "player/player.hpp"
#include "os/fs.hpp"
#include <chrono>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <event2/event.h>
#include <event2/listener.h>
#include <sstream>
#include <memory>
#include <cryptopp/osrng.h>
#include <signal.h>

#ifdef WIN32
# include "os/windows/stdafx.hpp"
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif


namespace hc {
  
  server::server (logger& log)
    : log (log)
  {
    this->running = false;
    this->started = false;
    
    // <init, fin> pairs
    this->inits.emplace_back (&server::first_init, &server::last_fin);
    this->inits.emplace_back (&server::init_config, &server::fin_config);
    this->inits.emplace_back (&server::init_crypt, &server::fin_crypt);
    this->inits.emplace_back (&server::init_worlds, &server::fin_worlds);
    this->inits.emplace_back (&server::init_workers, &server::fin_workers);
    this->inits.emplace_back (&server::init_listener, &server::fin_listener);
    this->inits.emplace_back (&server::init_scheduler, &server::fin_scheduler);
    this->inits.emplace_back (&server::last_init, &server::first_fin);
  }
  
  /* 
   * NOTE: Will stop the server if it is still running.
   */
  server::~server ()
  {
    this->stop ();
  }
  
  
  
//----------
  
  namespace {
    
    struct ctx_t {
      int index;
    };
  }
  
  void
  server::worker_func (void *ctx)
  {
    std::unique_ptr<ctx_t> sctx (static_cast<ctx_t *> (ctx));
    int index = sctx->index;
    
    // wait for all worker threads to get created
    for (;;)
      {
        std::this_thread::sleep_for (std::chrono::milliseconds (100));
        if (!this->running)
          return;
        else if (this->workers_created)
          break;
      }
    
    worker *w = this->workers[index];
    
    while (this->running)
      {
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
        
        event_base_loop (w->evbase, EVLOOP_ONCE | EVLOOP_NONBLOCK);
      }
  }
  
  
  
  /* 
   * Returns the server worker that is currently handling the least amount
   * of events, and increases its event count.
   */
  server::worker*
  server::min_worker ()
  {
    std::lock_guard<std::mutex> guard { this->worker_mtx };
    
    worker *min = nullptr;
    for (worker *w : this->workers)
      if (!min || w->evs > min->evs)
        min = w;
    
    if (min)
      ++ min->evs;
    return min;
  }
  
  
  
  /* 
   * Called when the listener accepts a new connection.
   */
  void
  server::on_accept (struct evconnlistener *listener, evutil_socket_t sock,
    struct sockaddr *addr, int len, void *ptr)
  {
    server *srv = static_cast<server *> (ptr);
    
    // get IP address
    char ip[INET_ADDRSTRLEN];
    if (!inet_ntop (AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), ip, INET_ADDRSTRLEN))
      {
        srv->log (LT_DEBUG)
          << "Accepted invalid connection (could not get IP address)" << std::endl;
        evutil_closesocket (sock);
        return;
      }
    
    connection *conn = new connection (*srv, sock, ip);
    {
      std::lock_guard<std::recursive_mutex> guard { srv->conn_mtx };
      srv->conns.push_back (conn);
    }
    
    srv->log (LT_DEBUG) << "Accepted connection from @" << ip << std::endl;
    conn->infer_protocol ();
    conn->start_io ();
  }
  
  /* 
   * Handles SIGPIPE signals that can sometimes be caused by writing to a
   * socket that terminated the connection from the other end.
   */
  void
  server::on_signal (evutil_socket_t sock, short what, void *ptr)
  {
    // ... do nothing ...
  }
  
  
  
//----------
  
  /* 
   * Starts the server up.
   * Does nothing if the server is already running.
   */
  void
  server::start ()
  {
    if (this->started)
      return;
    
    for (int i = 0; i < (int)this->inits.size (); ++i)
      {
        try
          {
            (this->* this->inits[i].init) ();
          }
        catch (const server_start_error&)
          {
            for (--i; i >= 0; --i)
              (this->* this->inits[i].fin) ();
            
            throw;
          }
      }
    
    this->started = true;
  }
 
  
  /* 
   * Stops the server.
   */
  void
  server::stop ()
  {
    if (!this->started)
      return;
    
    for (auto itr = this->inits.rbegin (); itr != this->inits.rend (); ++itr)
      {
        (this->* itr->fin) ();
      }
    
    this->started = false;
  }
  
  
  
//----------
  
  /* 
   * Removes a connection from the server's connection list, and places it
   * in the gray list.
   * NOTE: Shouldn't be called directly, instead use connection.disconnect ().
   */
  void
  server::disconnect_connection (connection *conn)
  {
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    
    this->conns.erase (
      std::remove (this->conns.begin (), this->conns.end (), conn),
      this->conns.end ());
    this->gray_conns.push_back (conn);
  }
  
  /* 
   * Destroys gray connections.
   */
  void
  server::clean_gray ()
  {
    if (this->gray_conns.empty ())
      return;
    
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    
    std::vector<connection *> nconns;
    for (auto conn : this->gray_conns)
      {
        // it's possible that a connection's destructor will be in the middle
        // of execution when we try to delete it.
        std::unique_lock<std::recursive_mutex> dc_guard { conn->get_dc_mutex () };
        
        player *pl = conn->get_player ();
        if (!pl || conn->get_player ()->get_refc ().zero ())
          {
            dc_guard.unlock ();
            delete conn;
          }
        else
          nconns.push_back (conn);
      }
    
    this->gray_conns = nconns;
  }
  
  
  
  /*
   * Inserts the specified player to the server's player list.
   * 
   * Throws `server_full_error' when the player cannot be registered because
   * the server is full.
   */
  void
  server::register_player (player *pl)
  {
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    
    if ((int)this->players.size () == this->cfg.max_players)
      throw server_full_error ();
      
    this->players.push_back (pl);
  }
  
  /* 
   * Removes a player from the server's player list.
   */
  void
  server::deregister_player (player *pl)
  {
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    
    this->players.erase (
      std::remove (this->players.begin (), this->players.end (), pl),
      this->players.end ());
  }
  
  /* 
   * Calls the specified callback function for every player in the server.
   */
  void
  server::all_players (std::function<void (player *)>&& cb)
  {
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    for (player *pl : this->players)
      cb (pl);
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Cleans up after gray connections.
   */
  void
  server::cleanup_conns (scheduler::task& task)
  {
    this->clean_gray ();
  }
  
  
  
  /* 
   * Sends Keep-Alive packets to all connected players; and checks whether
   * there exist any connections that did not respond with a keep alive
   * packet within 20 seconds.
   */
  void
  server::keep_alive (scheduler::task& task)
  {
    std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
    for (player *pl : this->players)
      {
        pl->send_keep_alive ();
      }
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * The initialization function is called before any other initialization
   * function; the finalization function is called after all others are
   * called.
   */
  
  void
  server::first_init ()
  {
#define TPOOL_THREAD_COUNT    4

    log (LT_SYSTEM) << "Creating thread pool with " << TPOOL_THREAD_COUNT
                    << " threads..." << std::endl;
    this->tpool = new thread_pool ();
    this->tpool->init (4);
    
    this->gen_seq = this->tpool->create_seq ();
  }
  
  void
  server::last_fin ()
  {
    this->tpool->release_seq (this->gen_seq,
      [] (void *ctx)
        {
          
        });
    
    this->tpool->join ();
    this->tpool->stop ();
    delete this->tpool;
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Sets up encryption-related stuff.
   */
  
  void
  server::init_crypt ()
  {
    // generate 1024-bit RSA key pair
    CryptoPP::AutoSeededRandomPool rnd;
    this->rsa_p.GenerateRandomWithKeySize (rnd, 1024);
  }
  
  void
  server::fin_crypt ()
  {
    
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Loads and sets up the server's worlds, and its main world in particular.
   */
  
  void
  server::init_worlds ()
  {
    fs::create_dir ("worlds");
    
    this->lman.start (1);
    
    std::vector<std::string> paths;
    fs::get_files ("worlds", paths);
    fs::get_dirs ("worlds", paths);
    for (auto& path : paths)
      {
        std::string full_path = "worlds/" + path;
        const char *fmt = world_provider_specifier::determine (full_path);
        if (!fmt)
          continue;
        
        world *w = world::load_from (full_path, *this);
        if (!w)
          continue;
        
        this->worlds.push_back (w);
      }
    
    // setup main world
    world *mw = nullptr;
    for (world *w : this->worlds)
      if (w->get_name () == this->cfg.mainw)
        {
          mw = w;
          break;
        }
    
    if (mw)
      {
        this->mainw = mw;
        log (LT_SYSTEM) << "World \"" << mw->get_name ()
          << "\" has been as set as the main world." << std::endl;
      }
    else
      {
        log (LT_WARNING) << "Main world (\""
          << this->cfg.mainw << "\") not found, creating default..." << std::endl;
        
        this->mainw = new world (this->cfg.mainw, *this,
          world_generator::create ("flatgrass", ""),
          world_provider::create ("anvil"));
        this->worlds.push_back (this->mainw);
      }
  }
  
  void
  server::fin_worlds ()
  {
    std::lock_guard<std::mutex> guard (this->world_mtx);
    
    for (world *w : this->worlds)
      delete w;
    this->worlds.clear ();
    
    this->lman.stop ();
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Sets up the server's worker threads.
   */
  
  void
  server::init_workers ()
  {
#define DEFAULT_WORKER_THREAD_COUNT   2
  
    std::lock_guard<std::mutex> guard { this->worker_mtx };

    int wcount = DEFAULT_WORKER_THREAD_COUNT;
    
    log (LT_SYSTEM) << "Starting " << wcount << " workers thread(s)..." << std::endl;
    
    this->running = true;
    this->workers_created = false;
    
    for (int i = 0; i < wcount; ++i)
      {
        ctx_t *ctx = new ctx_t;
        ctx->index = i;
        hc::thread *th = new hc::thread (std::bind (std::mem_fn (&server::worker_func),
          this, std::placeholders::_1), ctx);
        
        worker *w = new worker;
        w->th = th;
        w->evs = 0;
        
        // create event base
        w->evbase = event_base_new ();
        if (!w->evbase)
          {
            throw server_start_error ("failed to create worker thread: "
              "could not create event base");
          }
        log (LT_DEBUG) << " - Method: " << event_base_get_method (w->evbase) << std::endl;
        
        this->workers.push_back (w);
      }
    
    this->workers_created = true;
  }
  
  void
  server::fin_workers ()
  {
    log (LT_SYSTEM) << "Stopping worker threads..." << std::endl;
    std::lock_guard<std::mutex> guard { this->worker_mtx };
    this->running = false;
    
    for (worker *w : this->workers)
      {
        if (w->th->joinable ())
          w->th->join ();
        delete w->th;
        
        event_base_free (w->evbase);
        
        delete w;
      }
    
    this->workers.clear ();
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Creates the server's listening socket and starts listening or incoming
   * connections.
   */
  
  void
  server::init_listener ()
  {
    struct addrinfo hints, *res;
    
    std::ostringstream ss;
    ss << this->cfg.port;
    
    std::memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo (NULL, ss.str ().c_str (), &hints, &res);
    
    worker *w = this->min_worker ();
    std::lock_guard<std::mutex> guard { this->worker_mtx };
    
    this->listener = evconnlistener_new_bind (w->evbase, &server::on_accept,
      this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, res->ai_addr,
      (int)res->ai_addrlen);
    freeaddrinfo (res);
      
    if (!this->listener)
      {
        log (LT_FATAL) << "Could not start listening on port " << this->cfg.port << std::endl;
        throw server_start_error ("could not start listening");
      }
    
    log (LT_SYSTEM) << "Started listening on port " << this->cfg.port << std::endl;
    
    // we also bind a handler for SIGPIPE here
#ifndef WIN32
    this->pipe_event = evsignal_new (w->evbase, SIGPIPE, &server::on_signal,
      this);
    evsignal_add (this->pipe_event, NULL);
#endif
  }
  
  void
  server::fin_listener ()
  {
    evconnlistener_free (this->listener);
    
#ifndef WIN32
    event_free (this->pipe_event);
#endif
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Initializes the server's scheduler and schedules important tasks.
   */
  
  void
  server::init_scheduler ()
  {
    log (LT_SYSTEM) << "Initializing scheduler..." << std::endl;
    this->sched.start ();
    
    this->sched.create (
      [&] (scheduler::task& task) { this->cleanup_conns (task); }).run (1000);
    this->sched.create (
      [&] (scheduler::task& task) { this->keep_alive (task); }).run (15000);
  }
  
  void
  server::fin_scheduler ()
  {
    this->sched.stop ();
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * First finaliation and last initialization functions.
   */
  
  void
  server::last_init ()
  {
    
  }
  
  void
  server::first_fin ()
  {
    log (LT_SYSTEM) << "Closing all connections..." << std::endl;
    {
      std::lock_guard<std::recursive_mutex> guard { this->conn_mtx };
      for (connection *conn : this->conns)
        {
          conn->disconnect ();
        }
      this->clean_gray ();
    }
  }
}


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

#ifndef _hCraft2__SYSTEM__SERVER__H_
#define _hCraft2__SYSTEM__SERVER__H_

#include "util/scheduler.hpp"
#include "util/thread_pool.hpp"
#include "world/lighting.hpp"
#include <vector>
#include <stdexcept>
#include <string>
#include <mutex>
#include <functional>
#include <event2/util.h>
#include <cryptopp/rsa.h>
#include <unordered_map>


// forward decs:
struct event;
struct event_base;
struct evconnlistener;

namespace hc {
  
  // forward decs:
  class logger;
  class connection;
  class player;
  class thread;
  class world;
  class lighting_manager;
  class command;
  
  
  /* 
   * Thrown by the server when it can not properly start up.
   */
  class server_start_error: public std::runtime_error
  {
  public:
    server_start_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  /* 
   * Thrown by server::register_player () when the server is full.
   */
  class server_full_error: public std::runtime_error
  {
  public:
    server_full_error ()
      : std::runtime_error ("server full")
      { }
  };
  
  
  /* 
   * The hCraft server.
   */
  class server
  {
  private:
    struct init_fin_pair
    {
      void (server:: *init) ();
      void (server:: *fin) ();
      
    public:
      init_fin_pair (void (server:: *init) (), void (server:: *fin) ())
        : init (init), fin (fin)
        { }
    };
    
  public:
    struct worker
    {
      hc::thread *th;
      struct event_base *evbase;
      int evs;  // number of events
    };
    
    /* 
     * Loaded from `config.json' at runtime.
     */
    struct configuration
    {
      std::string motd;
      int max_players;
      
      int port;
      bool encryption;
      int compress_threshold;
      int compress_level;
      
      std::string mainw;
    };
  
  private:
    logger& log;
    bool running, started;
    std::vector<init_fin_pair> inits;
    
    configuration cfg;
    
    std::mutex worker_mtx;
    std::vector<worker *> workers;
    bool workers_created;
    struct event *pipe_event;
    
    struct evconnlistener *listener;
    
    std::vector<connection *> conns;
    std::vector<connection *> gray_conns;
    std::vector<player *> players;
    std::recursive_mutex conn_mtx;
    
    scheduler sched;
    thread_pool *tpool;
    
    std::vector<world *> worlds;
    world *mainw;
    std::mutex world_mtx;
    thread_pool::seq_class *gen_seq; // used for world generation
    lighting_manager lman;
    
    std::unordered_map<std::string, command *> cmds;
    
    // encryption/authentication:
    CryptoPP::RSA::PrivateKey rsa_p;
    
  public:
    inline bool is_running () const { return this->running; }
    
    inline logger& get_logger () { return this->log; }
    inline scheduler& get_scheduler () { return this->sched; }
    inline thread_pool& get_thread_pool () { return *this->tpool; }
    inline const configuration& get_config () const { return this->cfg; }
    inline world* get_main_world () { return this->mainw; }
    inline thread_pool::seq_class* get_gen_seq () { return this->gen_seq; }
    inline lighting_manager& get_lighting_manager () { return this->lman; }
    inline int get_player_count () const { return (int)this->players.size (); }
    
    inline CryptoPP::RSA::PublicKey get_pub_key ()
      { return CryptoPP::RSA::PublicKey (this->rsa_p); }
    inline CryptoPP::RSA::PrivateKey& get_priv_key () { return this->rsa_p; }
    
  public:
    server (logger& log);
    
    /* 
     * NOTE: Will stop the server if it is still running.
     */
    ~server ();
    
  private:
    void worker_func (void *ctx);
    
    
    /* 
     * Called when the listener accepts a new connection.
     */
    static void on_accept (struct evconnlistener *listener, evutil_socket_t sock,
      struct sockaddr *addr, int len, void *ptr);
    
    /* 
     * Handles SIGPIPE signals that can sometimes be caused by writing to a
     * socket that terminated the connection from the other end.
     */
    static void on_signal (evutil_socket_t sock, short what, void *arg);
    
  public:
    /* 
     * Starts the server up.
     * Does nothing if the server is already running.
     */
    void start ();
    
    /* 
     * Stops the server.
     */
    void stop ();
    
    
    
    /* 
     * Returns the server worker that is currently handling the least amount
     * of events, and increases its event count.
     */
    worker* min_worker ();
    
  public:
    /* 
     * Removes a connection from the server's connection list, and places it
     * in the gray list.
     * NOTE: Shouldn't be called directly, instead use connection.disconnect ().
     */
    void disconnect_connection (connection *conn);
    
    /* 
     * Destroys gray connections.
     */
    void clean_gray ();
    
    
    /*
     * Inserts the specified player to the server's player list.
     * 
     * Throws `server_full_error' when the player cannot be registered because
     * the server is full.
     */
    void register_player (player *pl);
    
    /* 
     * Removes a player from the server's player list.
     */
    void deregister_player (player *pl);
    
    /* 
     * Calls the specified callback function for every player in the server.
     */
    void all_players (std::function<void (player *)>&& cb);
    
    
    
    /* 
     * Finds and returns a command whose name matches the one specified from
     * the server's list of registered commands.
     */
    command* find_command (const std::string& name);
    
  private:
    /* 
     * Scheduled functions:
     */
    //--------------------------------------------------------------------------
    
    /* 
     * Cleans up after gray connections.
     * Basically, just calls clean_gray ().
     */
    void cleanup_conns (scheduler::task& task);
    
    /* 
     * Sends Keep-Alive packets to all connected players; and checks whether
     * there exist any connections that did not respond with a keep alive
     * packet within 20 seconds.
     */
    void keep_alive (scheduler::task& task);
    
    //--------------------------------------------------------------------------
    
  private:
    /* 
     * <init, fin> functions:
     */
    //--------------------------------------------------------------------------
    
    /* 
     * The initialization function is called before any other initialization
     * function; the finalization function is called after all others are
     * called.
     */
    void first_init ();
    void last_fin ();
    
    /* 
     * Loads properties from the server's configuration file at `config.json'.
     */
    void init_config ();
    void fin_config ();
    
    /* 
     * Sets up encryption-related stuff.
     */
    void init_crypt ();
    void fin_crypt ();
    
    /* 
     * Loads and sets up the server's worlds, and its main world in particular.
     */
    void init_worlds ();
    void fin_worlds ();
    
    /* 
     * Loads and sets up server/player commands.
     */
    void init_cmds ();
    void fin_cmds ();
    
    /* 
     * Sets up the server's worker threads.
     */
    void init_workers ();
    void fin_workers ();
    
    /* 
     * Creates the server's listening socket and starts listening or incoming
     * connections.
     */
    void init_listener ();
    void fin_listener ();
    
    /* 
     * Initializes the server's scheduler and schedules important tasks.
     */
    void init_scheduler ();
    void fin_scheduler ();
    
    /* 
     * First finaliation and last initialization functions.
     */
    void last_init ();
    void first_fin ();
    
    //--------------------------------------------------------------------------
  };
}

#endif


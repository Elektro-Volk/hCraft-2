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

#ifndef _hCraft2__PLAYER__PLAYER__H_
#define _hCraft2__PLAYER__PLAYER__H_

#include "util/uuid.hpp"
#include "util/position.hpp"
#include "util/thread_pool.hpp"
#include "util/refc.hpp"
#include "util/common.hpp"
#include <string>
#include <random>
#include <mutex>
#include <unordered_set>
#include <sstream>
#include <atomic>

#include <iostream> // DEBUG


namespace hc {
  
  // forward decs:
  class logger;
  class connection;
  class world;
  class chunk;
  class server;
  
  
//------------------------------------------------------------------------------
  
  // forward decs:
  class player;
  class player_message_stream;
  player_message_stream& endm (player_message_stream& strm);
  
  class player_message_stream
  {
  private:
    player& pl;
    std::ostringstream *ss;
    
  public:
    player_message_stream (player& pl)
      : pl (pl), ss (new std::ostringstream ())
      { }
    
  public:
    template<typename T>
    friend player_message_stream&
    operator<< (player_message_stream& strm, T val)
      { *strm.ss << val; return strm; }
    
    friend player_message_stream&
    operator<< (player_message_stream& strm,
      player_message_stream& (*fn) (player_message_stream&))
      { fn (strm); return strm; }
    
    friend player_message_stream& endm (player_message_stream& strm);
  };
  
  
  
//-----------------------------------------------------------------------------
  
  
  /* 
   * Represents a player.
   * Wraps on top of a connection, and is responsible for handling packets.
   */
  class player
  {
    server& srv;
    connection& conn;
    logger& log;
    
    uuid_t uuid;
    std::string name;
    std::mt19937 rnd;
    
    // keep-alive related:
    bool ka_expecting;  // whether we're expecting a response keep-alive packet.
    int ka_id;          // the keep alive ID we're expecting.
    
    world *w; // current world
    std::recursive_mutex w_mtx;
    std::unordered_set<chunk_pos> vis_chunks;
    entity_pos pos;
    bool spawned;
    entity_pos spawn_pos;
    chunk_pos last_cp;
    int gen_tok;
    
    // the player will be destroyed by the server's cleanup_conns() task once
    // this reference counter drops to zero.
    ref_counter refc;
    
  public:
    inline server& get_server () { return this->srv; }
    inline connection& get_connection () { return this->conn; }
    inline ref_counter& get_refc () { return this->refc; }
    
    inline uuid_t get_uuid () const { return this->uuid; }
    inline const std::string& get_username () const { return this->name; }
    inline world* get_world () { return this->w; }
    inline entity_pos& position () { return this->pos; }
    
  public:
    player (connection& conn, uuid_t uuid, const std::string& name);
    ~player ();
    
  private:
    /* 
     * Sends new chunks that are close to the player, and unloads distant
     * chunks.
     */
    void stream_chunks ();
    
  public:
    /* 
     * Called once a chunk generation job has finished, or when the chunk is
     * already loaded.
     */
    void on_chunk_loaded (chunk *ch, int x, int z);
    
  public:
    /* 
     * Functions called by the underlying packet handler.
     */
//------------------------------------------------------------------------------
    
    /* 
     * Called right after the player logs into the server.
     */
    void on_login ();
  
    /* 
     * Invoked when the player's position/orientation changes.
     * Streams chunks if the player crossed chunk boundaries.
     */
    void on_move (entity_pos pos);
    
    /* 
     * Invoked when the player attempts to send out a chat message.
     */
    void on_chat (const std::string& msg);
    
    /* 
     * Invoked when the player attempts to destroy a block.
     */
    void on_digging (int x, int y, int z, digging_state state, block_face face);
    
//------------------------------------------------------------------------------
    
  public:
    /* 
     * Kicks the player with the specified message.
     */
    void kick (const std::string& msg);
    
    
    
    /* 
     * Returns a stream wrapper that can be used to build the message that is
     * to be sent.
     */
    player_message_stream& message ();
    
    /* 
     * Sends the specified string to the player (old-style formatting is used).
     */
    void message (const std::string& msg);
    
    
    
    /* 
     * Sends a Keep-Alive packet to the player.
     * After the packet is sent, the client is then expected to respond with
     * another Keep-Alive packet within 30 seconds.
     */
    void send_keep_alive ();
    
    /* 
     * Notifies the player that a Keep-Alive packet has been received from the
     * client with the specified ID.
     */
    void handle_keep_alive (int id);
    
    
    
    /* 
     * Teleports the player into the specified world.
     */
    void join_world (world *w, entity_pos pos);
    void join_world (world *w);
    
    
    
    /* 
     * Called every 20ms by the packet handler.
     */
    void tick ();
  };
}

#endif


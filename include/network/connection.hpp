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

#ifndef _hCraft2__NETWORK__CONNECTION__H_
#define _hCraft2__NETWORK__CONNECTION__H_

#include "util/thread_pool.hpp"
#include <event2/util.h>
#include <mutex>
#include <functional>
#include <deque>


// forward decs:
struct bufferevent;
struct event_base;
struct evbuffer;
struct event;

namespace hc {
  
  // forward decs:
  class server;
  class logger;
  class protocol;
  class packet;
  class player;
  
#define READ_BUFFER_SIZE    512

// terminates the connection after the packet has been sent:
#define CONN_SEND_DISCONNECT  0x1
  
  /* 
   * Wraps around a socket, and handles all the IO related things.
   */
  class connection
  {
  private:
    struct packet_container
    {
      int id;
      packet *pack;
      unsigned int flags;
    };
    
  private:
    server& srv;
    logger& log;
    
    evutil_socket_t sock;
    char ip[20];
    bool disconnected;
    bool disconnect_req;
    std::recursive_mutex dc_mtx;
    
    struct bufferevent *bev;
    struct event_base *evb;
    unsigned char rbuf[READ_BUFFER_SIZE];
    int rbuf_len;
    thread_pool::seq_class *pseq;
    struct event *tev;
    
    // used to transform data:
    std::vector<struct evbuffer *> tbs;
    struct evbuffer *ftb; // final transformed data
    
    std::deque<packet_container *> outq;
    bool can_send;
    packet_container *init_pack; // used to initiate writes
    int next_packet_id;
    
    protocol *proto;
    player *pl;
    
  public:
    inline server& get_server () { return this->srv; }
    inline const char* get_ip () const { return this->ip; }
    inline bool is_disconnected () const { return this->disconnected; }
    inline protocol* get_protocol () { return this->proto; }
    
    inline player* get_player () { return this->pl; }
    inline void set_player (player *pl) { this->pl = pl; }
    
  public:
    connection (server& srv, evutil_socket_t sock, const char *ip);
    ~connection ();
    
  private:
    /* 
     * Does all the work of disconnecting the connection.
     * disconnect() merely makes disconnect_() get called by the connection's
     * timer event to prevent locking the bufferevent.
     */
    void disconnect_ ();
    
    /* 
     * Processes as much read data as possible using the protocol's
     * transformers.
     */
    void apply_in_transformations ();
    
    /* 
     * Applies the protocol's out transformers to the contents of the specified
     * packet.
     */
    void apply_out_transformations (packet *pack);
    
  public:
    /* 
     * Deactivates the connection, and places it into the server's "gray" list.
     */
    void disconnect ();
    
    /* 
     * Starts handling reading and writing.
     */
    void start_io ();
    
  public:
    /* 
     * Sets the protocol implementation the connection will use.
     * NOTE: Ownership of the protocol object is passed to the connection.
     */
    void set_protocol (protocol *proto);
    
    /* 
     * Makes the connection guess which protocol implementation to use based
     * on first few packets sent by the client.
     */
    void infer_protocol ();
    
  public:
    /* 
     * Queues the specified packet to be sent.
     * NOTE: Ownership of the packet is passed to the connection.
     */
    void send (packet *pack, unsigned int flags = 0);
  
  private:
    /* 
     * libevent callbacks:
     */
    //--------------------------------------------------------------------------
    
    static void on_read (struct bufferevent *bev, void *ctx);
    
    static void on_write (struct bufferevent *bev, void *ctx);
    
    static void on_event (struct bufferevent *bev, short events, void *ctx);
    
    static void on_time (evutil_socket_t fd, short events, void *ctx);
    
    //--------------------------------------------------------------------------
  };
}

#endif


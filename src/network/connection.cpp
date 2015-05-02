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

#include "network/connection.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "util/binary.hpp"
#include "network/packet.hpp"
#include "network/packet_delimiter.hpp"
#include "network/packet_handler.hpp"
#include "network/packet_transformer.hpp"
#include "network/handlers/infer.hpp"
#include "network/protocol.hpp"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <cstring>
#include <stdexcept>


namespace hc {
  
  connection::connection (server& srv, evutil_socket_t sock, const char *ip)
    : srv (srv), log (srv.get_logger ())
  {
    this->sock = sock;
    std::strcpy (this->ip, ip);
    this->disconnected = false;
    this->proto = nullptr;
    this->can_send = true;
    this->ftb = evbuffer_new ();
    this->init_pack = nullptr;
    this->disconnect_req = false;
    this->next_packet_id = 1;
    this->pl = nullptr;
    
    this->pseq = srv.get_thread_pool ().create_seq ();
  }
  
  connection::~connection ()
  {
    this->disconnect ();
    
    delete this->proto;
    srv.get_thread_pool ().release_seq (this->pseq, [] (void *) { });
    
    bufferevent_free (this->bev);
    for (auto tb : this->tbs)
      evbuffer_free (tb);
    evbuffer_free (this->ftb);
  }
  
  
  
  /* 
   * Does all the work of disconnecting the connection.
   * disconnect() merely makes disconnect_() get called by the connection's
   * timer event to prevent locking the bufferevent.
   */
  void
  connection::disconnect_ ()
  {
    std::lock_guard<std::recursive_mutex> dc_guard { this->dc_mtx };
    if (this->disconnected)
      return;
    
    // free unsent packets
    {
      for (packet_container *cont : this->outq)
        {
          delete cont->pack;
          delete cont;
        }
    }
    
    event_free (this->tev);
    bufferevent_disable (this->bev, EV_READ | EV_WRITE);
    
    this->srv.get_thread_pool ().disable_seq (this->pseq,
      [] (void *ptr) {
        delete static_cast<packet_reader *> (ptr);
      });
    
    this->proto->get_handler ()->disconnect ();
    
    this->disconnected = true;
    this->srv.disconnect_connection (this);
    log (LT_SYSTEM) << "@" << this->ip << " has disconnected" << std::endl;
  }
  
  /* 
   * Deactivates the connection, and places it into the server's "gray" list.
   */
  void
  connection::disconnect ()
  {
    this->disconnect_req = true;
  }
  
  
  /* 
   * Starts handling reading and writing.
   */
  void
  connection::start_io ()
  {
    if (!this->proto)
      throw std::runtime_error ("connection::start_io: no protocol");
    
    server::worker *w = this->srv.min_worker ();
    this->evb = w->evbase;
    
    this->rbuf_len = 0;
    
    this->bev = bufferevent_socket_new (this->evb, this->sock,
      BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    bufferevent_setcb (this->bev, &connection::on_read, &connection::on_write,
      &connection::on_event, this);
    bufferevent_enable (this->bev, EV_READ | EV_WRITE);
    
    struct timeval tv = { 0, 10000 };
    this->tev = evtimer_new (w->evbase, &connection::on_time, this);
    evtimer_add (this->tev, &tv);
  }
  
  
  
  /* 
   * Processes as much read data as possible using the protocol's transformers.
   */
  void
  connection::apply_in_transformations ()
  {
    struct evbuffer *input = bufferevent_get_input (this->bev);
    
    bool needed = false;
    auto trs = this->proto->get_transformers ();
    for (auto tr : trs)
      if (tr->is_on ())
        {
          needed = true;
          break;
        }
    if (!needed)
      {
        // move straight to final data buffer
        evbuffer_add_buffer (this->ftb, input);
        return;
      }
    
    struct evbuffer *lastbuf = input;
    for (int i = trs.size () - 1; i >= 0; --i)
      {
        auto tr = trs[i];
        if (!tr->is_on ())
          continue;
        
        evbuffer_add_buffer (this->tbs[i], lastbuf);
        
        // transform as much data as possible
        
        int from_len = (int)evbuffer_get_length (this->tbs[i]);
        unsigned char *from = new unsigned char[from_len];
        evbuffer_remove (this->tbs[i], from, from_len);
        
        int res = tr->in_enough (from, from_len);
        if (res == -1)
          {
            // invalid data
            delete[] from;
            this->disconnect ();
            return;
          }
        else if (res != 1)
          {
            // need more data
            evbuffer_add (this->tbs[i], from, from_len);
            delete[] from;
            break;
          }
        
        int to_len = 0;
        int consumed = 0;
        unsigned char *to = nullptr;
        tr->transform_in (from, from_len, &to, &to_len, &consumed);
        
        struct evbuffer *nextbuf = nullptr;
        for (int j = i - 1; j >= 0; --j)
          if (trs[j]->is_on ())
            nextbuf = this->tbs[j];
        if (!nextbuf)
          nextbuf = this->ftb;
        evbuffer_add (nextbuf, to, to_len);
        
        evbuffer_add (this->tbs[i], from + consumed, from_len - consumed);
        delete[] from;
        delete[] to;
        
        lastbuf = this->tbs[i];
      }
  }
  
  /* 
   * Applies the protocol's out transformers to the contents of the specified
   * packet.
   */
  void
  connection::apply_out_transformations (packet *pack)
  {
    int on_count = 0;
    auto trs = this->proto->get_transformers ();
    for (auto tr : trs)
      if (tr->is_on ())
        ++ on_count;
    if (on_count == 0)
      return;
    
    int ti = 0;
    unsigned char *init_data = const_cast<unsigned char *> (pack->get_data ());
    unsigned char *last_data = init_data;
    int last_len = pack->get_length ();
    for (int i = 0; i < on_count; ++i)
      {
        while (!trs[ti]->is_on ())
          ++ ti;
        
        auto tr = trs[ti];
        int out_len = 0;
        unsigned char *out = nullptr;
        if (!tr->transform_out (last_data, last_len, &out, &out_len))
          {
            log (LT_ERROR) << "Could not apply transformation to packet, disconnecting player." << std::endl;
            delete[] out;
            if (last_data != init_data)
              delete[] last_data;
            return;
          }
        
        if (last_data != init_data)
          delete[] last_data;
        last_data = out;
        last_len = out_len;
        ++ ti;
      }
    
    pack->reset ();
    pack->put_bytes (last_data, last_len);
    delete[] last_data;
  }
  
  
  
//------------------------------------------------------------------------------
  
  void
  connection::on_read (struct bufferevent *bev, void *ctx)
  {
    connection *conn = static_cast<connection *> (ctx);
    std::lock_guard<std::recursive_mutex> dc_guard { conn->dc_mtx };
    
    struct evbuffer *input = bufferevent_get_input (conn->bev);
    int len = (int)evbuffer_get_length (input);
    if (len <= 0)
      { conn->disconnect (); return; }
    
    conn->apply_in_transformations ();
    
    do
      {
        len = (int)evbuffer_get_length (conn->ftb);
        
        int rem = conn->proto->get_delimiter ()->remaining (conn->rbuf, conn->rbuf_len);
        if (rem < 0)
          {
            conn->log (LT_WARNING)
              << "Received invalid packet from @" << conn->get_ip () << std::endl;
            conn->disconnect ();
            return;
          }
        else if (rem == 0)
          {
            // finished reading packet
            if (conn->proto->get_handler ())
              {
                // execute handler in different (pooled) thread.
                packet_reader *reader = new packet_reader (conn->rbuf, conn->rbuf_len, true);
                conn->srv.get_thread_pool ().enqueue_seq (conn->pseq,
                  [conn] (void *ptr) {
                    auto reader = static_cast<packet_reader *> (ptr);
                    conn->proto->get_handler ()->handle (*reader);
                    delete reader;
                  }, reader);
              }
            
            conn->rbuf_len = 0;
          }
        else
          {
            if ((conn->rbuf_len + rem) > READ_BUFFER_SIZE)
              {
                conn->log (LT_WARNING)
                  << "Packet received from @" << conn->get_ip () << " too big" << std::endl;
                conn->disconnect ();
                return;
              }
            
            if (rem > len)
              rem = len;
            int n = evbuffer_remove (conn->ftb, conn->rbuf + conn->rbuf_len, rem);
            if (n < 0)
              { conn->disconnect (); return; }
            conn->rbuf_len += n;
          }
      }
    while (len > 0);
  }
  
  void
  connection::on_write (struct bufferevent *bev, void *ctx)
  {
    connection *conn = static_cast<connection *> (ctx);
    std::lock_guard<std::recursive_mutex> dc_guard { conn->dc_mtx };
    if (conn->outq.empty ())
      return;
    
    packet_container *cont = conn->outq.front ();
    if (cont == conn->init_pack) // hasn't been sent yet
      return;
    
    packet *pack = cont->pack;
    unsigned int flags = cont->flags;
    conn->outq.pop_front ();
    delete cont;
    delete pack;
    
    if (flags & CONN_SEND_DISCONNECT)
      {
        conn->disconnect ();
        return;
      }
    
    if (!conn->outq.empty ())
      {
        cont = conn->outq.front ();
        pack = cont->pack;
        bufferevent_write (conn->bev, pack->get_data (), pack->get_length ());
      }
  }
  
  void
  connection::on_event (struct bufferevent *bev, short events, void *ctx)
  {
    connection *conn = static_cast<connection *> (ctx);
    conn->disconnect ();
  }
  
  void
  connection::on_time (evutil_socket_t fd, short events, void *ctx)
  {
    connection *conn = static_cast<connection *> (ctx);
    std::lock_guard<std::recursive_mutex> dc_guard { conn->dc_mtx };
    bufferevent_lock (conn->bev);
    
    if (conn->disconnect_req)
      {
        event_del (conn->tev);
        bufferevent_unlock (conn->bev);
        conn->disconnect_ ();
        return;
      }
    
    if (conn->init_pack)
      {
        // initiate write
        auto cont = conn->init_pack;
        bufferevent_write (conn->bev,
          cont->pack->get_data (), cont->pack->get_length ());
        conn->init_pack = nullptr;
      }
    
    bufferevent_unlock (conn->bev);
    
    if (conn->proto->get_handler ())
      conn->proto->get_handler ()->tick ();
    
    if (!evtimer_pending (conn->tev, NULL))
      {
        struct timeval tv = { 0, 20000 };
        event_del (conn->tev);
        evtimer_add (conn->tev, &tv );
      }
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Sets the protocol implementation the connection will use.
   * NOTE: Ownership of the protocol object is passed to the connection.
   */
  void
  connection::set_protocol (protocol *proto)
  {
    if (this->proto)
      delete this->proto;
    
    this->proto = proto;
    this->proto->get_handler ()->set_connection (this);
    
    for (auto evb : this->tbs)
      evbuffer_free (evb);
    this->tbs.clear ();
    for (int i = 0; i < (int)this->proto->get_transformers ().size (); ++i)
      this->tbs.push_back (evbuffer_new ());
  }
  
  /* 
   * Makes the connection guess which protocol implementation to use based
   * on first few packets sent by the client.
   */
  void
  connection::infer_protocol ()
  {
    this->proto = new protocol ("<infer>",
      new infer_packet_delimiter (), new infer_packet_handler (),
      nullptr);
    this->proto->get_handler ()->set_connection (this);
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Queues the specified packet to be sent.
   * NOTE: Ownership of the packet is passed to the connection.
   */
  void
  connection::send (packet *pack, unsigned int flags)
  {
    std::lock_guard<std::recursive_mutex> dc_guard { this->dc_mtx };
    if (!this->can_send || this->disconnected || this->disconnect_req)
      { delete pack; return; }
    
    int pack_id = this->next_packet_id++;
    
    // apply transformation first
    this->apply_out_transformations (pack);
    
    packet_container *cont = new packet_container;
    cont->pack = pack;
    cont->flags = flags;
    cont->id = pack_id;
    if (flags & CONN_SEND_DISCONNECT)
      this->can_send = false;
    
    this->outq.push_back (cont);
    if (this->outq.size () == 1)
      this->init_pack = cont;
  }
}


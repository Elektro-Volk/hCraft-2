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

#include "player/player.hpp"
#include "network/packet.hpp"
#include "network/connection.hpp"
#include "network/protocol.hpp"
#include "network/packet_builder.hpp"
#include "network/builders/mc18.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "world/world.hpp"
#include "world/chunk.hpp"
#include "world/blocks.hpp"
#include "cmd/command.hpp"
#include "entity/player.hpp"
#include <chrono>
#include <algorithm>


namespace hc {
  
//------------------------------------------------------------------------------
  
  player_message_stream&
  endm (player_message_stream& strm)
  {
    strm.pl.message (strm.ss->str ());
    
    delete strm.ss;
    delete &strm;
    return strm;
  }
 
//------------------------------------------------------------------------------
  
  
  
  player::player (connection& conn, uuid_t uuid, const std::string& name)
    : srv (conn.get_server ()), conn (conn),
      log (conn.get_server ().get_logger ()), uuid (uuid), name (name)
  {
    this->rnd.seed ((unsigned long)std::chrono::duration_cast<std::chrono::nanoseconds> (
      std::chrono::high_resolution_clock::now ().time_since_epoch ()).count ());
    
    this->ka_expecting = false;
    this->w = nullptr;
    this->spawned = false;
    this->gen_tok = 0;
    this->openw = nullptr;
    this->cur_slot = 0;
    this->gm = GM_SURVIVAL;
    
    this->pent = new player_entity (this, srv.next_entity_id ());
  }
  
  player::~player ()
  {
    if (this->w)
      {
        this->pent->despawn ();
        delete this->pent;
        
        // abort all chunk generation requests
        this->w->get_async_gen ().free_token (this->gen_tok);
        
        this->w->remove_player (this);
      }
  }
  
  
  
  /* 
   * Kicks the player with the specified message.
   */
  void
  player::kick (const std::string& msg)
  {
    auto builder = this->conn.get_protocol ()->get_builder ();
    this->conn.send (builder->make_disconnect (msg), CONN_SEND_DISCONNECT);
    log (LT_SYSTEM) << "Player `" << this->name
      << "' has been kicked (message: `" << msg << "')" << std::endl;
  }
  
  
  
  /* 
   * Returns a stream wrapper that can be used to build the message that is
   * to be sent.
   */
  player_message_stream&
  player::message ()
  {
    // NOTE: the returned object is destroyed after being passed the endm
    //       manipulator.
    return *new player_message_stream (*this);
  }
  
  /* 
   * Sends the specified string to the player (old-style formatting is used).
   */
  void
  player::message (const std::string& msg)
  {
    auto builder = this->conn.get_protocol ()->get_builder ();
    this->conn.send (builder->make_chat_message ("§e" + msg));
  }
  
  
  
  /* 
   * Sends a Keep-Alive packet to the player.
   * After the packet is sent, the client is then expected to respond with
   * another Keep-Alive packet within 30 seconds.
   */
  void
  player::send_keep_alive ()
  {
    if (this->ka_expecting)
      {
        // TODO: kick the player (reason: timed out)
        this->conn.disconnect ();
        return;
      }
    
    this->ka_id = this->rnd ();
    this->ka_expecting = true;
    
    auto builder = this->conn.get_protocol ()->get_builder ();
    this->conn.send (builder->make_keep_alive (this->ka_id));
  }
  
  /* 
   * Notifies the player that a Keep-Alive packet has been received from the
   * client with the specified ID.
   */
  void
  player::handle_keep_alive (int id)
  {
    if (this->ka_expecting && id == this->ka_id)
      this->ka_expecting = false;
  }
  
  
  
  /* 
   * Called once a chunk generation job has finished, or when the chunk is
   * already loaded.
   */
  void
  player::on_chunk_loaded (chunk *ch, int x, int z)
  {
    if (this->conn.is_disconnected ())
      return;
    
    // TODO: generalize
    auto builder = dynamic_cast<mc18_packet_builder *> (
      this->conn.get_protocol ()->get_builder ());
    
    unsigned short mask = 0;
    for (int i = 0; i < 16; ++i)
      if (ch->get_sub (i))
        mask |= 1 << i;
    
    this->conn.send (builder->make_chunk_data (x, z, true, mask, ch));
    this->vis_chunks.emplace (x, z);
    
    if (!this->spawned && (chunk_pos (x, z) == chunk_pos (this->spawn_pos)))
      {
        // the chunk the player is supposed to spawn on has been sent.
        // spawn the player.
        
        this->spawned = true;
        this->pos = this->spawn_pos;
        this->last_cp = this->pos;
        
        block_pos bpos = this->spawn_pos;
        this->conn.send (builder->make_spawn_position (bpos.x, bpos.y, bpos.z));
        this->conn.send (
          builder->make_player_position_and_look (
            this->pos.x, this->pos.y, this->pos.z, this->pos.yaw, this->pos.pitch, 0));
        
        this->pent->spawn (this->w, this->pos);
      }
  }
  
  /* 
   * Sends new chunks that are close to the player, and unloads distant
   * chunks.
   */
  void
  player::stream_chunks ()
  {
#define VISIBILITY_RADIUS   this->srv.get_config ().view_dist
    
    // TODO: generalize
    auto builder = dynamic_cast<mc18_packet_builder *> (
      this->conn.get_protocol ()->get_builder ());
    
    std::lock_guard<std::recursive_mutex> guard { this->w_mtx };
    if (!this->w)
      return;
    
    chunk_pos me_pos = this->pos;
  
    // build list of chunks that we the player should see
    std::vector<chunk_pos> in_sight;
    for (int x = me_pos.x - VISIBILITY_RADIUS; x <= me_pos.x + VISIBILITY_RADIUS; ++x)
      for (int z = me_pos.z - VISIBILITY_RADIUS; z <= me_pos.z + VISIBILITY_RADIUS; ++z)
        {
          in_sight.emplace_back (x, z);
        }
    
    // build list of chunks that are no longer in range.
    std::vector<chunk_pos> to_unload;
    for (chunk_pos cp : this->vis_chunks)
      if (!(cp.x >= (me_pos.x - VISIBILITY_RADIUS) &&
        cp.x <= (me_pos.x + VISIBILITY_RADIUS) && 
        cp.z >= (me_pos.z - VISIBILITY_RADIUS) && 
        cp.z <= (me_pos.z + VISIBILITY_RADIUS)))
        {
          to_unload.push_back (cp);
        }
    
    // sort in-range chunks so that the closer ones are sent first
    std::sort (in_sight.begin (), in_sight.end (),
      [me_pos] (chunk_pos a, chunk_pos b) -> bool
        {
          int ad2 = (a.x - me_pos.x) * (a.x - me_pos.x) +
            (a.z - me_pos.z) * (a.z - me_pos.z);
          int bd2 = (b.x - me_pos.x) * (b.x - me_pos.x) +
            (b.z - me_pos.z) * (b.z - me_pos.z);
          return ad2 < bd2;
        });
    
    // unload distant chunks
    for (chunk_pos cp : to_unload)
      {
        this->conn.send (builder->make_unload_chunk (cp.x, cp.z));
        this->vis_chunks.erase (cp);
      }
    
    // send new chunks
    for (chunk_pos cp : in_sight)
      {
        if (this->vis_chunks.find (cp) == this->vis_chunks.end ())
          {
            player *me = this;
            chunk *ch = this->w->get_async_gen ().generate (this->gen_tok,
              cp.x, cp.z,
              [me] (world *w, chunk *ch, int x, int z)
                {
                  if (w != me->get_world ())
                    return;
                  me->on_chunk_loaded (ch, x, z);
                }, &this->refc);
            
            if (ch)
              this->on_chunk_loaded (ch, cp.x, cp.z);
          }
      }
  }
  
  
  /* 
   * Teleports the player into the specified world.
   */
  void
  player::join_world (world *w, entity_pos pos)
  {
    std::lock_guard<std::recursive_mutex> guard { this->w_mtx };
    
    world *prev_w = this->w;
    if (prev_w)
      {
        // abort all chunk generation requests
        prev_w->get_async_gen ().free_token (this->gen_tok);
        
        prev_w->remove_player (this);
      }
    
    w->add_player (this);
    this->w = w;
    this->spawned = false;
    this->spawn_pos = this->pos = pos;
    this->gen_tok = this->w->get_async_gen ().make_token ();
    
    this->stream_chunks ();
  }
  
  void
  player::join_world (world *w)
  {
    this->join_world (w, w->get_spawn_pos ());
  }
  
  
  
  /* 
   * Respawns the player with a different game mode.
   */
  void
  player::set_gm (game_mode gm)
  {
    this->gm = gm;
    
    // TODO: respawn
  }
  
  
  
  /* 
   * Opens the specified window (and shows it to the player).
   */
  void
  player::open_window (window *w)
  {
    this->openw = w;
    
    // TODO: show window
  }
  
  /* 
   * Close currently open window.
   */
  void
  player::close_window ()
  {
    delete this->openw;
    this->openw = nullptr;
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Called right after the player logs into the server.
   */
  void
  player::on_login ()
  {
    this->message ("§eWelcome to an §6hCraft 2 §eserver§f!");
    
    this->join_world (srv.get_main_world ());
  }
  
  
  
  
  /* 
   * Invoked by the underlying packet handler when the player's
   * position/orientation changes.
   * Streams chunks if the player crossed chunk boundaries.
   */
  void
  player::on_move (entity_pos pos)
  {
    if (this->conn.is_disconnected () || !this->spawned)
      return;
    
    this->pos = pos;
    this->pent->move (this->pos);
    
    // stream chunks if needed
    chunk_pos cp = pos;
    if (cp != this->last_cp)
      {
        // crossed chunk boundaries
        this->last_cp = cp;
        this->stream_chunks ();
      }
  }
  
  
  
  void
  player::handle_command (const std::string& msg)
  {
    std::string name, rest;
    for (size_t i = 1; i < msg.length (); ++i)
      {
        if (msg[i] != ' ')
          name.push_back (msg[i]);
        else
          {
            while (i < msg.length () && msg[i] == ' ')
              ++ i;
            for (; i < msg.length (); ++i)
              rest.push_back (msg[i]);
            break;
          }
      }
    
    command *cmd = this->srv.find_command (name);
    if (!cmd)
      {
        this->message () << "§cUnknown command§f: §7/" << name << endm;
        return;
      }
    
    cmd->execute (this, rest);
  }
  
  /* 
   * Invoked by the underlying packet handler when the player attempts to
   * send out a chat message.
   */
  void
  player::on_chat (const std::string& msg)
  {
    if (this->conn.is_disconnected () || !this->spawned)
      return;
    if (msg.empty ())
      return;
    
    if (msg[0] == '/')
      {
        this->handle_command (msg);
        return;
      }
    
    log (LT_CHAT) << this->get_username () << ": " << msg << std::endl;
    
    player *me = this;
    this->srv.all_players (
      [me, &msg] (player *pl) {
        pl->message () << "§e" << me->get_username () << "§f: " << msg << endm;
      });
  }
  
  
  
  static bool
  _block_in_range (entity_pos ppos, block_pos bpos)
  {
#define MAX_DIGGING_DISTANCE    6
    
    double xd = ppos.x - bpos.x;
    double yd = ppos.y - bpos.y;
    double zd = ppos.z - bpos.z;
    int max_d = MAX_DIGGING_DISTANCE;
    
    return (xd * xd + yd * yd + zd * zd) <= (max_d * max_d);
  }
  
  /* 
   * Invoked when the player attempts to destroy a block.
   */
  void
  player::on_dig (int x, int y, int z, digging_state state, block_face face)
  {
    if (this->conn.is_disconnected () || !this->spawned)
      return;
    
    if (y < 0 || y > 255)
      { this->kick ("You can't dig there (Y coordinate out of bounds)"); return; }
    
    // make sure the block the player is trying to break is within reach.
    if (!_block_in_range (this->pos, block_pos (x, y, z)))
      {
        if (state == DIG_FINISH)
          ; // TODO: resend original block
        return;
      }
    
    switch (state)
      {
      case DIG_START:
        this->w->set_id (x, y, z, BT_AIR);
        break;
      
      default: ;
      }
  }
  
  
  
  /* 
   * Invoked when the player tries to place a block down.
   */
  void
  player::on_place (int x, int y, int z, block_face face)
  {
    if (x == -1 && y == 4095 && z == -1)
      {
        // update held item status
        // TODO
        return;
      }
    
    int nx = x, ny = y, nz = z;
    switch (face)
      {
      case BFACE_X_NEG: -- nx; break;
      case BFACE_X_POS: ++ nx; break;
      case BFACE_Y_NEG: -- ny; break;
      case BFACE_Y_POS: ++ ny; break;
      case BFACE_Z_NEG: -- nz; break;
      case BFACE_Z_POS: ++ nz; break;
      
      default:
        this->conn.disconnect ();
        return;
      }
    
    if (ny < 0 || ny > 255)
      { this->conn.disconnect (); return; }
    else if (!_block_in_range (this->pos, block_pos (nx, ny, nz)))
      {
        // TODO: resend original block
        return;
      }
    
    slot_item *held_item = this->inv.get (36 + this->cur_slot);
    if (!held_item)
      return;
    
    this->w->set_id_and_meta (nx, ny, nz, held_item->get_id (),
      (unsigned char)held_item->get_damage ());
  }
  
  
  
//------------------------------------------------------------------------------
  
  
  
  /* 
   * Called every 20ms by the packet handler.
   */
  void
  player::tick ()
  {
    
  }
}


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

#include "network/handlers/infer.hpp"
#include "network/packet.hpp"
#include "network/connection.hpp"
#include "system/server.hpp"
#include "system/logger.hpp"
#include "network/protocol.hpp"


namespace hc {
  
  /* 
   * Called when the protocol version could not be successfully inferred.
   */
  void
  infer_packet_handler::fail ()
  {
    logger& log = this->conn->get_server ().get_logger ();
    
    log (LT_WARNING) << "Could not infer the protocol version used by @"
                     << this->conn->get_ip () << std::endl;
    this->conn->disconnect ();
  }
  
  /* 
   * Switches the protocol implementation used by the underlying connection.
   */
  void
  infer_packet_handler::success (const char *name, packet_reader& reader)
  {
    protocol *proto = protocol::create (name);
    this->conn->set_protocol (proto);
    
    // re-process first packet by the new protocol implementation
    reader.rewind ();
    proto->get_handler ()->handle (reader);
  }
  
  
  
  void
  infer_packet_handler::handle (packet_reader& reader)
  {
    // assume next field is packet length
    if (reader.read_byte () > 127)
      { this->fail (); return; }
    
    // opcode
    if (reader.read_byte () != 0x00)
      { this->fail (); return; }
    
    // protocol version
    int pv = reader.read_byte ();
    switch (pv)
      {
      case 47:
        this->success ("1.8", reader);
        return;
      
      default:
        this->fail ();
        return;
      }
    
    this->fail ();
    return;
  }
}


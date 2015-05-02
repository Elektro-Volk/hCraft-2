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

#ifndef _hCraft2__NETWORK__HANDLERS__INFER__H_
#define _hCraft2__NETWORK__HANDLERS__INFER__H_

#include "network/packet_handler.hpp"


namespace hc {
  
  /* 
   * A special packet handler used to detect the protocol version of connecting
   * clients.  Once a suitable protocol version is detected, the underlying
   * connection is then made to use an appropriate implementation.
   */
  class infer_packet_handler: public packet_handler
  {
  private:
    /* 
     * Called when the protocol version could not be successfully inferred.
     */
    void fail ();
    
    /* 
     * Switches the protocol implementation used by the underlying connection.
     */
    void success (const char *proto, packet_reader& reader);
  
  public:
    virtual void handle (packet_reader& reader) override;
  };
}

#endif


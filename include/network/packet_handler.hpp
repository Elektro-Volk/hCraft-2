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

#ifndef _hCraft2__NETWORK__PACKET_HANDLER__H_
#define _hCraft2__NETWORK__PACKET_HANDLER__H_


namespace hc {
  
  // forward decs:
  class connection;
  class packet_reader;
  
  /* 
   * In charge of handling the packets received by an underlying connection
   * (delimited by the connection's packet delimiter).
   */
  class packet_handler
  {
  protected:
    connection *conn;
  
  public:
    inline connection* get_connection () { return this->conn; }
  
  public:
    virtual ~packet_handler () { }
    
  public:
    packet_handler ();
    
  public:
    /* 
     * Handles the specified packet, usually by dispatching it to the
     * appropriate handler function.
     */
    virtual void handle (packet_reader& reader) = 0;
    
    /* 
     * Used internally by connections.
     */
    virtual void set_connection (connection *conn);
    
    /* 
     * Called when the connection is being terminated.
     */
    virtual void disconnect () { };
    
    /* 
     * Called every 20ms.
     */
    virtual void tick () { };
  };
}

#endif


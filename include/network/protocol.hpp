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

#ifndef _hCraft2__NETWORK__PROTOCOL__H_
#define _hCraft2__NETWORK__PROTOCOL__H_

#include <string>
#include <vector>


namespace hc {
  
  // forward decs:
  class packet_delimiter;
  class packet_handler;
  class packet_builder;
  class packet_transformer;
  
  /* 
   * Stores all the components that make up a protocol implementation.
   */
  class protocol
  {
    // name of the protocol.
    std::string name;
    
    // determines packet boundaries.
    packet_delimiter *delim;
    
    // processes read packets.
    packet_handler *handler;
    
    // constructs packets to send.
    packet_builder *builder;
    
    // transforms packet contents when reading and before sending.
    std::vector<packet_transformer *> transformers;
    
  public:
    inline const std::string& get_name () const { return this->name; }
    inline packet_delimiter* get_delimiter () { return this->delim; }
    inline packet_handler* get_handler () { return this->handler; }
    inline packet_builder* get_builder () { return this->builder; }
    inline std::vector<packet_transformer*>& get_transformers () { return this->transformers; }
    
  public:
    protocol (const std::string& name, packet_delimiter *delim,
      packet_handler *handler, packet_builder *builder);
    
    ~protocol ();
    
  public:
    /* 
     * Inserts the specified transformer to the end of the transformer
     * list.
     */
    void add_transformer (packet_transformer *trans);
    
  public:
    /* 
     * Creates the protocol that matches the specified client version.
     * Returns null if no matching protocol implementation is found.
     * The returned object should be deleted once no longer needed.
     */
    static protocol* create (const char *version);
  };
}

#endif


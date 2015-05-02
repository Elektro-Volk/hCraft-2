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

#include "network/protocol.hpp"
#include "network/packet_delimiter.hpp"
#include "network/packet_handler.hpp"
#include "network/packet_transformer.hpp"
#include <unordered_map>

#include "network/handlers/mc18.hpp"
#include "network/builders/mc18.hpp"
#include "network/transformers/aes.hpp"
#include "network/transformers/zlib_mc18.hpp"


namespace hc {
  
  protocol::protocol (const std::string& name, packet_delimiter *delim,
    packet_handler *handler, packet_builder *builder)
    : name (name)
  {
    this->delim = delim;
    this->handler = handler;
    this->builder = builder;
  }
  
  protocol::~protocol ()
  {
    delete this->delim;
    delete this->handler;
    delete this->builder;
    
    for (auto trans : this->transformers)
      delete trans;
  }
  
  
  
  /* 
   * Inserts the specified transformer to the end of the transformer
   * list.
   */
  void
  protocol::add_transformer (packet_transformer *trans)
  {
    this->transformers.push_back (trans);
  }
  
  
  
//------------------------------------------------------------------------------
  
  static protocol*
  _create_mc18 ()
  {
    auto proto = new protocol ("1.8",
      new mc17_packet_delimiter (), new mc18_packet_handler (),
      new mc18_packet_builder ());
    
    proto->add_transformer (new zlib_mc18_transformer ());
    proto->add_transformer (new aes_transformer ());
    
    return proto;
  }
  
  
  /* 
   * Creates the protocol that matches the specified client version.
   * Returns null if no matching protocol implementation is found.
   * The returned object should be deleted once no longer needed.
   */
  protocol*
  protocol::create (const char *version)
  {
    const std::unordered_map<std::string, protocol* (*) ()> _map {
      { "1.8.3", &_create_mc18 },
      { "1.8.2", &_create_mc18 },
      { "1.8.1", &_create_mc18 },
      { "1.8", &_create_mc18 },
    };
    
    auto itr = _map.find (version);
    if (itr == _map.end ())
      return nullptr;
    return itr->second ();
  }
}


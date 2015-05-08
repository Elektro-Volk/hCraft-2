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

#include "world/world_provider.hpp"
#include <unordered_map>
#include <memory>
#include <vector>

// providers:
#include "world/providers/anvil/provider.hpp"


namespace hc {
  
  world_provider::world_provider (world_provider_specifier *spec)
  {
    this->spec = spec;
  }
  
  world_provider::~world_provider ()
  {
    delete this->spec;
  }
  
  
  
  
  static world_provider*
  _create_anvil ()
  {
    return new anvil_world_provider ();
  }
  
  /* 
   * Creates and returns a new world provider from the specified proider name.
   */
  world_provider*
  world_provider::create (const char *name)
  {
    static std::unordered_map<std::string, world_provider* (*) ()> _map {
       { "anvil", &_create_anvil }
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return nullptr;
    return itr->second ();
  }
  
  
  
//------------------------------------------------------------------------------
  
  static world_provider_specifier*
  _create_spec_anvil ()
  {
    return new anvil_provider_spec ();
  }
  
  /* 
   * Creates and returns a new world provider specifier from the specified
   * provider name.
   */
  world_provider_specifier*
  world_provider_specifier::create (const char *name)
  {
    static std::unordered_map<std::string, world_provider_specifier* (*) ()> _map {
       { "anvil", &_create_spec_anvil }
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return nullptr;
    return itr->second ();
  }
  
  
  
  /* 
   * Returns the name of the world provider that implements the format of the
   * world located at the specified path.  Null is returned if the format
   * can not be recognized.
   */
  const char*
  world_provider_specifier::determine (const std::string& path)
  {
    static std::vector<std::unique_ptr<world_provider_specifier>> _specs;
    static bool _init = false;
    
    if (!_init)
      {
        _specs.emplace_back (new anvil_provider_spec ());
        _init = true;
      }
    
    for (auto& ptr : _specs)
      {
        auto spec = ptr.get ();
        if (spec->claims (path))
          {
            return spec->name ();
          }
      }
    
    return nullptr;
  }
}


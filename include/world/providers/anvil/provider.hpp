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

#ifndef _hCraft2__WORLD__PROVIDERS__ANVIL__PROVIDER__H_
#define _hCraft2__WORLD__PROVIDERS__ANVIL__PROVIDER__H_

#include "world/world_provider.hpp"


namespace hc {
  
  class anvil_provider_spec: public world_provider_specifier
  {
  public:
    virtual const char*
    name () override
      { return "anvil"; }
    
  public:
    virtual bool
    is_directory_format () override
      { return true; }
    
    virtual std::string path_from_name (const std::string& name) override;
    
    virtual bool claims (const std::string& path) override;
  };
  
  
  
  /*
   * Implements the Anvil world format used by the vanilla server and client.
   */
  class anvil_world_provider: public world_provider
  {
    bool _open;
    std::string wpath;
    
  public:
    virtual const char*
    name () override
      { return "anvil"; }
    
  public:
    anvil_world_provider ();
    ~anvil_world_provider ();
    
  public:
    virtual chunk* load_chunk (int x, int z) override;
    
    virtual void save_chunk (chunk *ch) override;
    
    
    
    virtual void load_world_data (world_data& wd) override;
    
    virtual void save_world_data (const world_data& wd) override;
    
  public:
    virtual void open (const std::string& path) override;
    
    virtual void close () override;
  };
}

#endif


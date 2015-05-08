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

#ifndef _hCraft2__WORLD__WORLD_PROVIDER__H_
#define _hCraft2__WORLD__WORLD_PROVIDER__H_

#include "world/world.hpp"
#include <string>
#include <stdexcept>


namespace hc {
  
  /* 
   * Thrown by a world provider when it fails to load something.
   */
  class world_load_error: public std::runtime_error
  {
  public:
    world_load_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  
  /* 
   * Every world provider implementation has an associated specifier class that
   * provides the means to get a path from a world name; and a way to determine
   * whether a world file/directory is of the format in question.
   */
  class world_provider_specifier
  {
  public:
    virtual ~world_provider_specifier () { }
  
  public:
    /* 
     * Creates and returns a new world provider specifier from the specified
     * provider name.
     */
    static world_provider_specifier* create (const char *name);
    
    /* 
     * Returns the name of the world provider that implements the format of the
     * world located at the specified path.  Null is returned if the format
     * can not be recognized.
     */
    static const char* determine (const std::string& path);
    
  public:
    /* 
     * Returns the name of the provider's format.
     * NOTE: Must have the same name as the provider.
     */
    virtual const char* name () = 0;
    
  public:
    /* 
     * Returns true if the format implemented by this provider keeps world
     * contents inside a separate directory.
     */
    virtual bool is_directory_format () = 0;
    
    /* 
     * Returns the path in which the world with the given name would be located
     * at.
     */
    virtual std::string path_from_name (const std::string& name) = 0;
    
    /* 
     * Returns true if the world located at the specified path is of the format
     * implemented by this provider.
     */
    virtual bool claims (const std::string& path) = 0;
  };
  
  
  
  /* 
   * Base class of all world providers.
   * A world provider is responsible for loading/saving a world from/to disk.
   */
  class world_provider
  {
  protected:
    world_provider_specifier *spec;
    
  public:
    inline world_provider_specifier* get_specifier () { return this->spec; }
    
  public:
    world_provider (world_provider_specifier *spec);
    virtual ~world_provider ();
    
  public:
    /* 
     * Creates and returns a new world provider from the specified provider name.
     */
    static world_provider* create (const char *name);
    
  public:
    /* 
     * Returns the name of the format implemented by this provider.
     */
    virtual const char* name () = 0;
    
  public:
    /* 
     * Attempts to load a chunk at the specified coordinates from disk.
     * Returns null if the requested chunk does not exist.
     * 
     * Throws exceptions of type `chunk_load_error' on failure.
     */
    virtual chunk* load_chunk (int x, int z) = 0;
    
    /* 
     * Saves the specified chunk to disk.
     */
    virtual void save_chunk (chunk *ch) = 0;
    
    
    
    /* 
     * Loads static world information from disk into the specified structure.
     */
    virtual void load_world_data (world_data& wd) = 0;
    
    /*
     * Saves world information to disk.
     */
    virtual void save_world_data (const world_data& wd) = 0;
    
  public:
    /* 
     * Opens the world located at the given path.
     */
    virtual void open (const std::string& path) = 0;
    
    /* 
     * Closes the stream to the world and frees resources.
     */
    virtual void close () = 0;
  };
}

#endif


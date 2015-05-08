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

#ifndef _hCraft2__OS__FS__H_
#define _hCraft2__OS__FS__H_

#include <string>
#include <vector>


namespace hc {
  
  namespace fs {
    
    /* 
     * Creates a new directory at the specified path.
     */
    void create_dir (const std::string& path);
    
    /* 
     * Checks whether the file located at the specified path exists.
     */
    bool file_exists (const std::string& path);
    
    /* 
     * Checks whether a directory exists at the specified path.
     */
    bool dir_exists (const std::string& path);
    
    /* 
     * Populates the specified vector with the list of files contained in
     * the directory located at the given path.
     */
    void get_files (const std::string& path, std::vector<std::string>& out);
    
    /* 
     * Populates the specified vector with the list of sub-directories contained 
     * in the directory located at the given path.
     */
    void get_dirs (const std::string& path, std::vector<std::string>& out);
  }
}

#endif


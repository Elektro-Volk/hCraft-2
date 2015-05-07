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

#include "system/server.hpp"
#include "util/json.hpp"
#include "system/logger.hpp"
#include <fstream>


namespace hc {
  
  static void
  _default_config (server::configuration& cfg)
  {
    cfg.motd = "A new hCraft server has been born!";
    cfg.max_players = 12;
    
    cfg.port = 25565;
    cfg.encryption = true;
    cfg.compress_threshold = 256;
    cfg.compress_level = 6;
    
    cfg.mainw = "Main";
  }
  
  
  
  static void
  _cfg_save_default (logger& log)
  {
    std::ofstream fs ("config.json", std::ios_base::out);
    if (!fs)
      {
        log (LT_FATAL) << "Cannot save configuration file (`config.json')" << std::endl;
        throw server_start_error ("failed to save default configuration");
      }
    
    fs << "{\n";
    fs << "\"general\": {\n";
    fs << "  \"motd\": \"A new hCraft server has been born!\",\n";
    fs << "  \"max-players\": 12,\n";
    fs << "  },\n";
    fs << "\n";
    fs << "  \"net\": {\n";
    fs << "    \"port\": 25565,\n";
    fs << "    \"encryption\": true,\n";
    fs << "    \"compression\": {\n";
    fs << "      \"threshold\": 256,\n";
    fs << "      \"level\": 6,\n";
    fs << "    }\n";
    fs << "  },\n";
    fs << "\n";
    fs << "  \"worlds\": {\n";
    fs << "    \"main-world\": \"Main\",\n";
    fs << "  }\n";
    fs << "}";
    
    fs << std::endl;
    fs.close ();
  }
  
  
  
   static void
  _cfg_load_general (json::j_object *obj, server::configuration& cfg, logger& log)
  {
    if (!obj)
      {
        log (LT_FATAL) << "  config: `general' not found" << std::endl;
        throw server_start_error ("config: `general' not found");
      }
    
    // general.motd
    if (obj->get ("motd"))
      cfg.motd = obj->get ("motd")->as_string ();
    else
      log (LT_WARNING) << "  config: `general.motd' not found, using default." << std::endl;
    
    // general.max-players
    if (obj->get ("max-players"))
      cfg.max_players = (int)obj->get ("max-players")->as_number ();
    else
      log (LT_WARNING) << "  config: `general.max-players' not found, using default." << std::endl;
  }
  
  static void
  _cfg_load_net_compression (json::j_object *obj, server::configuration& cfg, logger& log)
  {
    if (!obj)
      {
        log (LT_FATAL) << "  config: `net.compression' not found" << std::endl;
        throw server_start_error ("config: `net.compression' not found");
      }
    
    // compression.threshold
    if (obj->get ("threshold"))
      cfg.compress_threshold = (int)obj->get ("threshold")->as_number ();
    else
      log (LT_WARNING) << "  config: `net.compression.threshold' not found, using default." << std::endl;
    
    // compression.level
    if (obj->get ("level"))
      cfg.compress_level = (int)obj->get ("level")->as_number ();
    else
      log (LT_WARNING) << "  config: `net.compression.level' not found, using default." << std::endl;
  }
  
  static void
  _cfg_load_net (json::j_object *obj, server::configuration& cfg, logger& log)
  {
    if (!obj)
      {
        log (LT_FATAL) << "  config: `net' not found" << std::endl;
        throw server_start_error ("config: `net' not found");
      }
    
    // net.port
    if (obj->get ("port"))
      cfg.port = (int)obj->get ("port")->as_number ();
    else
      log (LT_WARNING) << "  config: `net.port' not found, using default." << std::endl;
    
    // net.encryption
    if (obj->get ("encryption"))
      cfg.encryption = obj->get ("encryption")->as_bool ();
    else
      log (LT_WARNING) << "  config: `net.encryption' not found, using default." << std::endl;
    
    // net.compression
    _cfg_load_net_compression (obj->get ("compression")->as_object (), cfg, log);
  }
  
  static void
  _cfg_load_worlds (json::j_object *obj, server::configuration& cfg, logger& log)
  {
    if (!obj)
      {
        log (LT_FATAL) << "  config: `worlds' not found" << std::endl;
        throw server_start_error ("config: `worlds' not found");
      }
    
    // worlds.main-world
    if (obj->get ("main-world"))
      cfg.mainw = obj->get ("main-world")->as_string ();
    else
      log (LT_WARNING) << "  config: `worlds.main-world' not found, using default." << std::endl;
  }
  
  static void
  _cfg_load (json::j_object *root, server::configuration& cfg, logger& log)
  {
    _cfg_load_general (root->get ("general")->as_object (), cfg, log);
    _cfg_load_net (root->get ("net")->as_object (), cfg, log);
    _cfg_load_worlds (root->get ("worlds")->as_object (), cfg, log);
  }
  
  
  
//------------------------------------------------------------------------------
  /* 
   * Loads properties from the server's configuration file at `config.json'.
   */
  
  void
  server::init_config ()
  {
    log (LT_SYSTEM) << "Loading server configuration..." << std::endl;
    
    std::ifstream fs ("config.json", std::ios_base::in);
    if (!fs)
      {
        log (LT_WARNING) << "Configuration file (`config.json') not found, saving default." << std::endl;
        
        _default_config (this->cfg);
        _cfg_save_default (this->log);
        return;
      }
    
    json_reader reader (fs);
    json::j_object *root;
    try
      {
        root = reader.read ();
      }
    catch (const json_parse_error& ex)
      {
        log (LT_FATAL) << "Could not parse configuration file:" << std::endl;
        log (LT_FATAL) << "  at line " << ex.line () << ", column "
          << ex.column () << ": " << ex.what () << std::endl;
        throw server_start_error ("Server config parse error");
      }
    
    _default_config (this->cfg);
    try
      {
        _cfg_load (root, this->cfg, this->log);
      }
    catch (const std::exception&)
      {
        delete root;
        throw;
      }
    
    delete root;
  }
  
  void
  server::fin_config ()
  {
    
  }
  
//------------------------------------------------------------------------------
}


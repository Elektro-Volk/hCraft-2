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

#include <iostream>

#include "os/windows/stdafx.hpp"
#include "system/logger.hpp"
#include "system/server.hpp"
#include "util/thread.hpp"
#include <event2/thread.h>
#include <chrono>
#include <cstring>

// TEST:
#include "util/uuid.hpp"
#include "util/thread.hpp"
#include "util/json.hpp"
#include "util/thread_pool.hpp"
#include <random>
#include <sstream>


//------------------------------------------------------------------------------

//#define TEST

static int
_test ()
{
  using namespace hc;
  
  uuid_t id = uuid_t::random ();
  std::cout << id.str () << std::endl;
  
  return 0;
}

//------------------------------------------------------------------------------



static void
_print_intro ()
{
  std::cout << " __      ____                      ___  __            ___     \n";
  std::cout << "/\\ \\    /\\  _`\\                  /'___\\/\\ \\__       /'___`\\   \n";
  std::cout << "\\ \\ \\___\\ \\ \\/\\_\\  _ __    __   /\\ \\__/\\ \\ ,_\\     /\\_\\ /\\ \\  \n";
  std::cout << " \\ \\  _ `\\ \\ \\/_/_/\\`'__\\/'__`\\ \\ \\ ,__\\\\ \\ \\/     \\/_/// /__ \n";
  std::cout << "  \\ \\ \\ \\ \\ \\ \\L\\ \\ \\ \\//\\ \\L\\.\\_\\ \\ \\_/ \\ \\ \\_       // /_\\ \\\n";
  std::cout << "   \\ \\_\\ \\_\\ \\____/\\ \\_\\\\ \\__/.\\_\\\\ \\_\\   \\ \\__\\     /\\______/\n";
  std::cout << "    \\/_/\\/_/\\/___/  \\/_/ \\/__/\\/_/ \\/_/    \\/__/     \\/_____/ \n";
  std::cout << std::endl;
}


static bool
_init_winsock ()
{
#ifdef WIN32
  WSADATA data;
  int err = WSAStartup (MAKEWORD(2, 2), &data);
  if (err)
    return false;
#endif

  return true;
}

static bool
_init_libevent ()
{
#ifdef WIN32
  evthread_use_windows_threads ();
#else
  evthread_use_pthreads ();
#endif
  return true;
}



int
main (int argc, char *argv[])
{
#ifdef TEST
  return _test ();
#endif
  
  hc::main_thread_raii tr;
  hc::logger log;
  
  _print_intro ();
  if (!_init_winsock ())
    {
      log (hc::LT_FATAL) << "Could not initialize Winsock2" << std::endl;
      return -1;
    }
  
  if (!_init_libevent ())
    {
      log (hc::LT_FATAL) << "Could not initialize libevent" << std::endl;
      return -1;
    }
  
  hc::server srv {log};
  srv.start ();
  
  // input loop
  char input[1024];
  for (;;)
    {
      std::cin.getline (input, sizeof input);
      if (std::strcmp (input, "stop") == 0)
        break;
    }
  
#ifdef WIN32
  WSACleanup ();
#endif
  
  return 0;
}


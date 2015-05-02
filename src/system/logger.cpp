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

#include "system/logger.hpp"
#include "util/thread.hpp"
#include <stdexcept>
#include <iostream>
#include <ctime>
#include <iomanip>


namespace hc {
  
  int
  logger::logger_buf::sync ()
  {
    std::lock_guard<std::mutex> guard { this->mtx };
    
    std::cout << this->str () << std::flush;
    
    this->str (std::string ());
    return 0;
  }
  
  
  
  void
  logger::logger_stream::write_start (log_type lt)
  {
    switch (lt)
      {
      case LT_DEBUG:    *this << "debug   | "; break;
      case LT_SYSTEM:   *this << "system  | "; break;
      case LT_WARNING:  *this << "warning | "; break;
      case LT_ERROR:    *this << "error   | "; break;
      case LT_FATAL:    *this << "fatal   | "; break;
      }
    
    std::time_t t;
    struct tm t2;
    
    std::time (&t);
#ifdef WIN32
    localtime_s (&t2, &t);
#else 
    localtime_r (&t, &t2);
#endif
    *this << std::setfill ('0');
    *this << std::setw (2) << t2.tm_hour << ':'
          << std::setw (2) << t2.tm_min << ':'
          << std::setw (2) << t2.tm_sec << " | ";
    *this << std::setfill (' ');
  }
  
  
  
  static void
  _tl_strm_fin (void *ptr)
  {
    delete static_cast<logger::logger_stream *> (ptr);
  }
  
  logger::logger ()
  {
    if (tls_alloc (&this->strm_key, nullptr))
      throw std::runtime_error ("logger: failed to create thread-local key");
  }
  
  
  
  /* 
   * Returns a reference to a thread-local stream.
   */
  logger::logger_stream&
  logger::operator() (log_type lt)
  {
    void *ptr = tls_get (this->strm_key);
    logger_stream *strm;
    if (ptr)
      strm = static_cast<logger_stream *> (ptr);
    else
      {
        strm = new logger_stream ();
        tls_set (this->strm_key, strm);
        
        auto key = this->strm_key;
        hc::thread::this_thread ().add_destructor (
          [key] {
            void *ptr = tls_get (key);
            _tl_strm_fin (ptr);
          });
      }
    
    strm->write_start (lt);
    
    return *strm;
  }
}


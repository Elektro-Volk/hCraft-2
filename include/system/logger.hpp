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

#ifndef _hCraft2__SYSTEM__LOGGER__H_
#define _hCraft2__SYSTEM__LOGGER__H_

#include "os/tls.hpp"
#include <ostream>
#include <sstream>
#include <mutex>
#include <iostream>


namespace hc {
  
  enum log_type
  {
    LT_DEBUG,
    LT_CHAT,
    LT_SYSTEM,
    LT_WARNING,
    LT_ERROR,
    LT_FATAL,
  };
  
  
  
  /* 
   * The logger provides a way to log messages both to the console and to a log-
   * file in a thread-safe manner.
   */
  class logger
  {
  public:
    /* 
     * Logger stream buffer.
     * Stores the pieces of the log message being recorded, and writes
     * everything at once to a file once the stream is flushed atomically.
     */
    class logger_buf: public std::stringbuf
    {
      std::mutex mtx;
      
    public:
      virtual int sync () override;
    };
    
    class logger_stream: public std::ostream
    {
    public:
      logger_stream ()
        : std::ostream (new logger_buf ())
      {
      }
      
      ~logger_stream ()
      {
        delete static_cast<logger_buf *> (this->rdbuf ());
        this->rdbuf (nullptr);
      }
    
    public:
      void write_start (log_type lt);
    };
    
  private:
    tls_key_t strm_key;
    
  public:
    logger ();
    
  public:
    /* 
     * Returns a reference to a thread-local stream.
     */
    logger_stream& operator() (log_type lt);
  };
}

#endif


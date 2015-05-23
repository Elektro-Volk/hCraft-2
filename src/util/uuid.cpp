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

#include "util/uuid.hpp"
#include <random>
#include <chrono>
#include <mutex>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>


namespace hc {
  
  bool
  uuid_t::operator== (const uuid_t other) const
  {
    for (int i = 0; i < 4; ++i)
      if (((unsigned int *)this->parts)[i] != ((unsigned int *)other.parts)[i])
        return false;
    
    return true;
  }
  
  bool
  uuid_t::operator!= (const uuid_t other) const
  {
    return !(this->operator== (other));
  }
  
  
  
  /* 
   * Returns a human-readable representation of the UUID.
   */
  std::string
  uuid_t::str ()
  {
    static const char *hex = "0123456789abcdef";
    
    std::string out;
    
    for (int i = 0; i < 16; ++i)
      {
        if (i == 4 || i == 6 || i == 8 || i == 10)
          out.push_back ('-');
        
        unsigned char p = this->parts[i];
        out.push_back (hex[p >> 4]);
        out.push_back (hex[p & 15]);
      }
    
    return out;
  }
  
  
  
  /* 
   * Returns a random UUID.
   */
  uuid_t
  uuid_t::generate_v4 ()
  {
    static bool _init = false;
    static std::mt19937 _rnd;
    static std::uniform_int_distribution<> _dis (0x00, 0xFF);
    static std::mutex mtx;
    
    std::lock_guard<std::mutex> guard (mtx);
    
    if (!_init)
      {
        _init = true;
        _rnd.seed ((unsigned long)std::chrono::duration_cast<std::chrono::nanoseconds> (
          std::chrono::high_resolution_clock::now ().time_since_epoch ())
            .count ());
      }
    
    uuid_t uuid;
    for (int i = 0; i < 16; ++i)
      uuid.parts[i] = _dis (_rnd);
    
    // make it look like a version 4 UUID.
    uuid.parts[6] = 0x40 | (uuid.parts[6] & 15);
    uuid.parts[8] = 0xB0 | (uuid.parts[8] & 15);
    
    return uuid;
  }
  
  /* 
   * Generates and returns a UUIDv3 from the specified string.
   */
  uuid_t
  uuid_t::generate_v3 (const std::string& str)
  {
    CryptoPP::Weak1::MD5 hash;
    unsigned char digest[16];
    hash.CalculateDigest ((unsigned char *)digest,
      (const unsigned char *)str.c_str (), str.length ());
    
    uuid_t uuid;
    for (int i = 0; i < 16; ++i)
      uuid.parts[i] = digest[i];
    
    // make it look like a version 3 UUID.
    uuid.parts[6] = 0x30 | (uuid.parts[6] & 15);
    uuid.parts[8] = 0xB0 | (uuid.parts[8] & 15);
    
    return uuid;
  }
  
  
  
  static char
  _hex_to_dec (char c)
  {
    if (c >= 'a' && c <= 'z')
      return 10 + c - 'a';
    if (c >= 'A' && c <= 'Z')
      return 10 + c - 'A';
    return c - '0';
  }
  
  /*
   * Parses and returns a UUID from a 32 digit hex string.
   */
  uuid_t
  uuid_t::parse_hex (const std::string& str)
  {
    uuid_t uuid;
    for (int i = 0; i < 16; ++i)
      {
        uuid.parts[i] = (_hex_to_dec (str[i*2 + 0]) << 4)
          | _hex_to_dec (str[i*2 + 1]);
      }
    
    return uuid;
  }
}


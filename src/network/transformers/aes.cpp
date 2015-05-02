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

#include "network/transformers/aes.hpp"
#include <cryptopp/files.h>
#include <cstring>
#include <string>


namespace hc {
  
  aes_transformer::aes_transformer ()
    : packet_transformer ()
  {
    this->encryptor = nullptr;
    this->decryptor = nullptr;
  }
  
  aes_transformer::~aes_transformer ()
  {
    delete this->encryptor;
    delete this->decryptor;
  }
  
  
  
  /* 
   * Sets up encryption and decryption using the specified shared secret.
   * Must be called before start ().
   */
  void
  aes_transformer::setup (const unsigned char *ssec)
  {
    delete this->encryptor;
    delete this->decryptor;
    
    this->encryptor = new CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption (
      ssec, 16, ssec, 1);
    this->decryptor = new CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption (
      ssec, 16, ssec, 1);
  }
  
  
  
  bool
  aes_transformer::transform_in (const unsigned char *data,
    unsigned int len, unsigned char **outp, int *out_len, int *consumed)
  {
    unsigned char *out = new unsigned char[len];
    *outp = out;
    
    std::string in_str ((char *)data, len);
    std::string out_str;
    
    try
      {
        CryptoPP::StringSource (in_str, true,
          new CryptoPP::StreamTransformationFilter (*this->decryptor,
            new CryptoPP::StringSink (out_str)));
      }
    catch (const CryptoPP::Exception&)
      {
        delete[] out;
        return false;
      }
    
    std::memcpy (out, out_str.c_str (), out_str.size ());
    *out_len = (int)out_str.size ();
    *consumed = len;
    
    return true;
  }
  
  bool
  aes_transformer::transform_out (const unsigned char *data,
    unsigned int len, unsigned char **outp, int *out_len)
  {
    unsigned char *out = new unsigned char[len];
    *outp = out;
    
    std::string in_str ((char *)data, len);
    std::string out_str;
    
    try
      {
        CryptoPP::StringSource (in_str, true,
          new CryptoPP::StreamTransformationFilter (*this->encryptor,
            new CryptoPP::StringSink (out_str)));
      }
    catch (const CryptoPP::Exception&)
      {
        delete[] out;
        return false;
      }
    
    std::memcpy (out, out_str.c_str (), out_str.size ());
    *out_len = (int)out_str.size ();
    
    return true;
  }
  
  int
  aes_transformer::estimate_in (unsigned int len)
  {
    return len;
  }
  
  int
  aes_transformer::estimate_out (unsigned int len)
  {
    return len;
  }
  
  int
  aes_transformer::in_enough (const unsigned char *data,
    unsigned int len)
  {
    return 1;
  }
}


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

#ifndef _hCraft2__NETWORK__TRANSFORMERS__AES__H_
#define _hCraft2__NETWORK__TRANSFORMERS__AES__H_

#include "network/packet_transformer.hpp"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>


namespace hc {
  
  /* 
   * Provides AES/CFB8 encryption and decryption.
   */
  class aes_transformer: public packet_transformer
  {
  protected:
    CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption *encryptor;
    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption *decryptor;
    
  public:
    aes_transformer ();
    ~aes_transformer ();
    
  public:
    virtual bool transform_in (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len, int *consumed) override;
    
    virtual bool transform_out (const unsigned char *data, unsigned int len,
      unsigned char **out, int *out_len) override;
    
    virtual int estimate_in (unsigned int len) override;
    
    virtual int estimate_out (unsigned int len) override;
    
    virtual int in_enough (const unsigned char *data, unsigned int len) override;
      
  public:
    /* 
     * Sets up encryption and decryption using the specified shared secret.
     * Must be called before start ().
     */
    void setup (const unsigned char *ssec);
  };
}

#endif


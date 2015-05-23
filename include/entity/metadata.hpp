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

#ifndef _hCraft2__ENTITY__METADATA__H_
#define _hCraft2__ENTITY__METADATA__H_

#include <unordered_map>


namespace hc {
  
  // forward decs:
  class slot_item;
  class packet;
  
  
  enum entity_metadata_type
  {
    EMT_BYTE = 0,
    EMT_SHORT,
    EMT_INT,
    EMT_FLOAT,
    EMT_STRING,
    EMT_SLOT,
    EMT_POS,
    EMT_ROT,
  };
  
  struct entity_metadata_value
  {
    entity_metadata_type type;
    union
      {
        unsigned char u8;
        unsigned short u16;
        unsigned int u32;
        float f32;
        std::string *str;
        slot_item *slot;
        struct
          {
            int x, y, z;
          } pos;
        struct
          {
            float pitch;
            float yaw;
            float roll;
          } rot;
      } val;
  };
  
  
  /*
   * When entities are spawned, their metadata is sent to nearby players using
   * this dictionary format.
   */
  class entity_metadata
  {
    std::unordered_map<int, entity_metadata_value> data;
    
  public:
    ~entity_metadata ();
    
  public:
    /* 
     * `put' methods:
     */
    //--------------------------------------------------------------------------
    
    void put_byte (int index, unsigned char val);
    void put_short (int index, short val);
    void put_int (int index, int val);
    void put_float (int index, float val);
    void put_string (int index, const std::string& str);
    void put_slot (int index, const slot_item& slot);
    void put_pos (int index, int x, int y, int z);
    void put_rot (int index, float pitch, float yaw, float roll);
    
    //--------------------------------------------------------------------------
    
    /* 
     * Encodes the contents of the metadata dictionary to the end of the
     * specified packet.
     */
    void encode (packet *pack) const;
  };
}

#endif


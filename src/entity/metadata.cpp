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

#include "entity/metadata.hpp"
#include "inv/slot.hpp"
#include "network/packet.hpp"


namespace hc {
  
  entity_metadata::~entity_metadata ()
  {
    for (auto p : this->data)
      {
        auto& val = p.second;
        switch (val.type)
          {
          case EMT_STRING:
            delete val.val.str;
            break;
          
          case EMT_SLOT:
            delete val.val.slot;
            break;
          
          default: ;
          }
      }
  }
  
  
  
  void
  entity_metadata::put_byte (int index, unsigned char val)
  {
    auto& v = this->data[index];
    v.type = EMT_BYTE;
    v.val.u8 = val;
  }
  
  void
  entity_metadata::put_short (int index, short val)
  {
    auto& v = this->data[index];
    v.type = EMT_SHORT;
    v.val.u16 = (unsigned short)val;
  }
  
  void
  entity_metadata::put_int (int index, int val)
  {
    auto& v = this->data[index];
    v.type = EMT_INT;
    v.val.u32 = (unsigned int)val;
  }
  
  void
  entity_metadata::put_float (int index, float val)
  {
    auto& v = this->data[index];
    v.type = EMT_FLOAT;
    v.val.f32 = val;
  }
  
  void
  entity_metadata::put_string (int index, const std::string& str)
  {
    auto& v = this->data[index];
    v.type = EMT_STRING;
    v.val.str = new std::string (str);
  }
  
  void
  entity_metadata::put_slot (int index, const slot_item& slot)
  {
    auto& v = this->data[index];
    v.type = EMT_SLOT;
    v.val.slot = new slot_item (slot);
  }
  
  void 
  entity_metadata::put_pos (int index, int x, int y, int z)
  {
    auto& v = this->data[index];
    v.type = EMT_POS;
    v.val.pos.x = x;
    v.val.pos.y = y;
    v.val.pos.z = z;
  }
  
  void
  entity_metadata::put_rot (int index, float pitch, float yaw, float roll)
  {
    auto& v = this->data[index];
    v.type = EMT_ROT;
    v.val.rot.pitch = pitch;
    v.val.rot.yaw = yaw;
    v.val.rot.roll = roll;
  }
  
  
  
  /* 
   * Encodes the contents of the metadata dictionary to the end of the
   * specified packet.
   */
  void
  entity_metadata::encode (packet *pack) const
  {
    for (auto p : this->data)
      {
        unsigned char t = (unsigned char)p.second.type;
        pack->put_byte ((t << 5) | p.first);
        
        switch (p.second.type)
          {
          case EMT_BYTE:
            pack->put_byte (p.second.val.u8);
            break;
          
          case EMT_SHORT:
            pack->put_short (p.second.val.u16);
            break;
          
          case EMT_INT:
            pack->put_int (p.second.val.u32);
            break;
          
          case EMT_FLOAT:
            pack->put_float (p.second.val.f32);
            break;
          
          case EMT_STRING:
            pack->put_varint ((int)p.second.val.str->length ());
            pack->put_bytes ((const unsigned char *)p.second.val.str->c_str (),
              (int)p.second.val.str->length ());
            break;
          
          case EMT_SLOT:
            {
              auto& slot = *p.second.val.slot;
              pack->put_short (slot.get_id ());
              if (slot.get_id () != EMPTY_SLOT_VALUE)
                {
                  pack->put_byte ((slot.get_amount () > 255)
                    ? 255 : slot.get_amount ());
                  pack->put_short (slot.get_damage ());
                  pack->put_byte (0); // NBT
                  
                  // TODO: handle extra NBT data
                }
            }
            break;
          
          case EMT_POS:
            pack->put_int (p.second.val.pos.x);
            pack->put_int (p.second.val.pos.y);
            pack->put_int (p.second.val.pos.z);
            break;
          
          case EMT_ROT:
            pack->put_float (p.second.val.rot.pitch);
            pack->put_float (p.second.val.rot.yaw);
            pack->put_float (p.second.val.rot.roll);
            break;
          }
      }
    
    pack->put_byte (127);
  }
}


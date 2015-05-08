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

#ifndef _hCraft2__SLOT__SLOT__H_
#define _hCraft2__SLOT__SLOT__H_


namespace hc {
  
  /* 
   * Represents a single window/inventory item.
   */
  class slot_item
  {
    unsigned short id;
    int amount;
    unsigned short damage;
    
  public:
    inline unsigned short get_id () const { return this->id; }
    inline int get_amount () const { return this->amount; }
    inline unsigned short get_damage () const { return this->damage; }
    
    inline void set_id (unsigned short id) { this->id = id; }
    inline void set_amount (int amount) { this->amount = amount; }
    inline void set_damage (unsigned short damage) { this->damage = damage; }
    
  public:
    slot_item (unsigned short id, unsigned short damage = 0, int amount = 1);
  };
}

#endif

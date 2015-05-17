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

#include "inv/window.hpp"


namespace hc {
  
  window::window (unsigned char wid, int slot_count)
  {
    this->wid = wid;
    this->slots.resize (slot_count, nullptr);
  }
  
  window::~window ()
  {
    for (slot_item *slot : this->slots)
      delete slot;
  }
  
  
  
  static bool
  _items_compatible (const slot_item *a, const slot_item *b)
  {
    if (!a || !b)
      return true;
    
    if (a->get_id () != b->get_id ())
      return false;
    if (a->get_damage () != b->get_damage ())
      return false;
    
    return true;
  }
  
  /* 
   * Inserts an add-range to the end of the list.
   * When an attempt to insert an item into the window is made, all
   * add-ranges are tried in order.
   */
  void
  window::push_add_range (int start, int end)
  {
    add_range r;
    r.start = start;
    r.end = end;
    this->add_ranges.push_back (r);
  }
  
  int
  window::try_add (int index, const slot_item& it, int count)
  {
    slot_item *slot = this->slots[index];
    if (!slot)
      {
        slot = this->slots[index] = new slot_item (it);
        slot->set_amount (count);
        return count;
      }
    
    if (!_items_compatible (slot, &it))
      return 0;
    
    // TODO: change max stack accordingly
    int max_stack = 64;
    int room = max_stack - slot->get_amount ();
    int to_add = (count > room) ? count : room;
    
    slot->set_amount (slot->get_amount () + to_add);
    
    return to_add;
  }
  
  /* 
   * Attempts to insert the specified item into the window.
   * Returns the number of items not added because of insufficient space.
   */
  int
  window::add (const slot_item& it)
  {
    int left = it.get_amount ();
    for (add_range range : this->add_ranges)
      {
        for (int i = range.start; i <= range.end; ++i)
          {
            left -= this->try_add (i, it, left);
            if (left == 0)
              goto done;
          }
      }
    
  done:
    return left;
  }
  
  
  /* 
   * Changes the contents of the slot at the given index.
   * NOTE: Ownership of the specified value is implicitely passed.
   */
  void
  window::set (int index, slot_item *item)
  {
    if (this->slots[index] == item)
      return;
    
    delete this->slots[index];
    this->slots[index] = item;
  }
  
  void
  window::set (int index, const slot_item& item)
  {
    delete this->slots[index];
    this->slots[index] = new slot_item (item);
  }
}


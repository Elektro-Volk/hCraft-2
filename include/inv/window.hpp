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

#ifndef _hCraft2__INV__WINDOW__H_
#define _hCraft2__INV__WINDOW__H_

#include "inv/slot.hpp"
#include <vector>


namespace hc {
  
  /* 
   * Represents a Minecraft window (e.g. player inventory, chests, workbenches,
   * etc...)
   */
  class window
  {
  private:
    struct add_range
    {
      int start;
      int end;
    };
  
  protected:
    unsigned char wid;
    std::vector<slot_item *> slots;
    std::vector<add_range> add_ranges;
  
  public:
    inline unsigned char get_id () const { return this->wid; }
    inline slot_item* get (int i) { return this->slots[i]; }
  
  public:
    window (unsigned char wid, int slot_count);
    virtual ~window ();
  
  private:
    int try_add (int index, const slot_item& it, int count);
  
  protected:
    /* 
     * Inserts an add-range to the end of the list.
     * When an attempt to insert an item into the window is made, all
     * add-ranges are tried in order.
     */
    void push_add_range (int start, int end);
  
  public:
    /* 
     * Attempts to insert the specified item into the window.
     * Returns the number of items not added because of insufficient space.
     */
    int add (const slot_item& it);
    
    /* 
     * Changes the contents of the slot at the given index.
     * NOTE: Ownership of the specified value is implicitely passed.
     */
    void set (int index, slot_item *item);
    
    void set (int index, const slot_item& item);
  };
}

#endif


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

#ifndef _hCraft2__SLOT__BLOCKS__H_
#define _hCraft2__SLOT__BLOCKS__H_


namespace hc {
  
  enum block_type
  {
    BT_INVALID = 0xFFFF,
    
    BT_AIR = 0,
    BT_STONE,
    BT_GRASS,
    BT_DIRT,
    BT_COBBLE,
    BT_WOOD,
    BT_SAPLING,
    BT_BEDROCK,
    BT_WATER,
    BT_STILL_WATER,
    BT_LAVA,
    BT_STILL_LAVA,
    BT_SAND,
    BT_GRAVEL,
    BT_GOLD_ORE,
    BT_IRON_ORE,
    BT_COAL_ORE,
    BT_LOG,
    BT_LEAVES,
    BT_SPONGE,
    BT_GLASS,
    BT_LAPIS_ORE,
    BT_LAPIS_BLOCK,
    BT_DISPENSER,
    BT_SANDSTONE,
    BT_NOTE_BLOCK,
    BT_BED,
    BT_POWERED_RAIL,
    BT_DETECTOR_RAIL,
    BT_STICKY_PISTON,
    BT_COBWEB,
    BT_TALL_GRASS,
    BT_DEAD_BUSH,
    BT_PISTON,
    BT_PISTON_HEAD,
    BT_WOOL,
    BT_PISTON_EXTENSION,
    BT_DANDELION,
    BT_POPPY,
    BT_BROWN_MUSHROOM,
    BT_RED_MUSHROOM,
    BT_GOLD_BLOCK,
    BT_IRON_BLOCK,
    BT_DOUBLE_STONE_SLAB,
    BT_STONE_SLAB,
    BT_BRICKS,
    BT_TNT,
    BT_BOOKSHELF,
    BT_MOSSY_COBBLE,
    BT_OBSIDIAN,
    BT_TORCH,
    BT_FIREW,
    BT_MONSTER_SPAWNER,
    BT_OAK_WOOD_STAIRS,
    BT_CHEST,
    BT_REDSTONE_WIRE,
    BT_DIAMOND_ORE,
    BT_DIAMOND_BLOCK,
    BT_WORKBENCH,
    BT_WHEAT,
    BT_FARMLAND,
    BT_FURNACE,
    BT_LIT_FURNACE,
    BT_SIGN,
    BT_OAK_DOOR,
    BT_LADDER,
    BT_RAIL,
    BT_COBBLE_STAIRS,
    BT_WALL_SIGN,
    BT_LEVER,
    BT_STONE_PRESSURE_PLATE,
    BT_IRON_DOOR,
    BT_WOODEN_PRESSURE_PLATE,
  };
  
  
  
  enum block_state
  {
    BS_SOLID,
    BS_FLUID,
  };
  
  
  
  /* 
   * Basic information about a block type.
   */
  struct block_info
  {
    unsigned short id;
    char name[33];
    float blast_resistance;
    char opacity;
    char luminance;
    bool opaque;
    char max_stack;
    block_state state;
    
  public:
    block_info ();
    block_info (unsigned short id, const char *name, float blast_resistance,
      char opacity, char luminance, bool opaque, char max_stack,
      block_state state);
    
  public:
    /* 
     * Returns block information of the block that has the specified ID, or
     * null if no such block is found.
     */
    static block_info* from_id (unsigned short id);
  };
}

#endif


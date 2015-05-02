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

#ifndef _hCraft2__WORLD__CHUNK__H_
#define _hCraft2__WORLD__CHUNK__H_


namespace hc {
  
  /* 
   * A 16x16x16 chunk of blocks.
   * 16 of these stacked on top of each other make up a single 16x256x16 chundk.
   */
  struct sub_chunk
  {
    unsigned short types[4096];  // block ids + meta
    unsigned char sl[2048];   // sky light
    unsigned char bl[2048];   // block light
    
  public:
    sub_chunk ();
  };
  
  
  /* 
   * A 16x256x16 chunk of blocks.
   * Represented as 16 sub-chunks.
   */
  class chunk
  {
    sub_chunk *subs[16];
    unsigned char biomes[256];
    int hmap[256];
    
  public:
    inline sub_chunk* get_sub (int sy) { return this->subs[sy]; }
    inline unsigned char* get_biome_data () { return this->biomes; }
    
    inline int
    get_height (int x, int z)
      { return this->hmap[(z << 4) | x]; }
    
  public:
    chunk ();
    ~chunk ();
    
  private:
    void recalc_height_at (int x, int z, int from = 255);
    
  public:
    void set_id (int x, int y, int z, unsigned short id);
    unsigned short get_id (int x, int y, int z);
    
    void set_meta (int x, int y, int z, unsigned char meta);
    unsigned char get_meta (int x, int y, int z);
    
    void set_sky_light (int x, int y, int z, unsigned char meta);
    unsigned char get_sky_light (int x, int y, int z);
    
    void set_block_light (int x, int y, int z, unsigned char meta);
    unsigned char get_block_light (int x, int y, int z);
    
    void set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta);
  };
}

#endif


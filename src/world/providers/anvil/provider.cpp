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

#include "world/providers/anvil/provider.hpp"
#include "os/fs.hpp"
#include "world/chunk.hpp"
#include "world/world_generator.hpp"
#include "util/binary.hpp"
#include "util/byte_buffer.hpp"
#include "util/nbt.hpp"
#include <sstream>
#include <fstream>
#include <memory>


namespace hc {
  
  std::string
  anvil_provider_spec::path_from_name (const std::string& name)
  {
    return "worlds/" + name;
  }
  
  bool
  anvil_provider_spec::claims (const std::string& path)
  {
    if (!fs::dir_exists (path))
      return false;
    
    if (!fs::dir_exists (path + "/region"))
      return false;
    
    if (!fs::file_exists (path + "/level.dat"))
      return false;
    
    // TODO: strengthen check by ensuring there are no .mcr files in /region/.
    
    return true;
  }
  
  
  
//------------------------------------------------------------------------------
  
  anvil_world_provider::anvil_world_provider ()
    : world_provider (new anvil_provider_spec ())
  {
    this->_open = false;
  }
  
  anvil_world_provider::~anvil_world_provider ()
  {
    this->close ();
  }
  
  
  
  static std::string
  _region_path (const std::string& wpath, int rx, int rz)
  {
    std::ostringstream ss;
    ss << wpath << "/region/r." << rx << "." << rz << ".mca";
    return ss.str ();
  }
  
  
  
//------------------------------------------------------------------------------
  
  static chunk*
  _load_chunk_from_nbt (nbt_tag_compound *root)
  {
    nbt_tag_compound *level = root->get ("Level")->as_compound ();
    if (!level)
      throw world_load_error ("invalid chunk NBT structure");
    
    int cx = level->get ("xPos")->as_int ();
    int cz = level->get ("zPos")->as_int ();
    
    std::unique_ptr<chunk> ch { new chunk (cx, cz) };
    
    nbt_tag_byte_array *biomes = level->get ("Biomes")->as_byte_array ();
    std::memcpy (ch->get_biomes (), biomes->get_data (), biomes->get_length ());
    
    nbt_tag_int_array *hmap = level->get ("HeightMap")->as_int_array ();
    std::memcpy (ch->get_heightmap (), hmap->get_data (), 4 * hmap->get_length ());
    
    nbt_tag_list *sects = level->get ("Sections")->as_list ();
    for (nbt_tag *tag : sects->get_values ())
      {
        nbt_tag_byte_array *bytes;
        nbt_tag_compound *sect = tag->as_compound ();
        int sy = sect->get ("Y")->as_byte ();
        
        ch->set_id (0, sy * 16, 0, 1); // instantiates the sub-chunk
        sub_chunk *sub = ch->get_sub (sy);
        
        bytes = sect->get ("Blocks")->as_byte_array ();
        for (int i = 0; i < 4096; ++i)
          sub->types[i] = (unsigned short)bytes->get_data ()[i] << 4;
        
        bytes = sect->get ("Add")->as_byte_array ();
        if (bytes)
          {
            for (int i = 0; i < 2048; ++i)
              {
                sub->types[2*i] &= 0x0FFF;
                sub->types[2*i] |= ((unsigned short)bytes->get_data ()[i] & 15) << 12;
                
                sub->types[2*i + 1] &= 0x0FFF;
                sub->types[2*i + 1] |= ((unsigned short)bytes->get_data ()[i] >> 4) << 12;
              }
          }
        
        bytes = sect->get ("Data")->as_byte_array ();
        for (int i = 0; i < 2048; ++i)
          {
            sub->types[2*i] &= 0xFFF0;
            sub->types[2*i] |= bytes->get_data ()[i] & 15;
            
            sub->types[2*i + 1] &= 0xFFF0;
            sub->types[2*i + 1] |= bytes->get_data ()[i] >> 4;
          }
        
        bytes = sect->get ("BlockLight")->as_byte_array ();
        std::memcpy (sub->bl, bytes->get_data (), 2048);
        
        bytes = sect->get ("SkyLight")->as_byte_array ();
        std::memcpy (sub->sl, bytes->get_data (), 2048);
      }
    
    return ch.release ();
  }
  
  
  chunk*
  anvil_world_provider::load_chunk (int x, int z)
  {
    int rx = x >> 5;
    int rz = z >> 5;
    
    std::string rpath = _region_path (this->wpath, rx, rz); 
    if (!fs::file_exists (rpath))
      return nullptr;
    
    std::ifstream fs (rpath, std::ios_base::in | std::ios_base::binary);
    
    unsigned char b[4] = { 0 };
    int cindex = (((z & 31) << 5) | (x & 31));
    fs.seekg (cindex * 4);
    fs.read ((char *)b + 1, 3);
    int loc = bin::read_int_be (b);
    int size = fs.get ();
    
    if (loc == 0)
      return nullptr; // chunk not in region file
    
    fs.seekg (loc << 12);
    std::unique_ptr<unsigned char[]> data { new unsigned char [size << 12] };
    fs.read ((char *)data.get (), size << 12);
    int compressed_len = bin::read_int_be (data.get ()) - 1;
    char compression_type = data.get ()[4];
    
    std::unique_ptr<nbt_tag_compound> root;
    try
      {
        nbt_compression_strategy st;
        switch (compression_type)
          {
          case 1: st = NBT_GZIP; break;
          case 2: st = NBT_ZLIB; break;
          
          default:
            throw world_load_error ("invalid compression type");
          }
        
        nbt_reader reader;
        root.reset (reader.read (data.get () + 5, compressed_len, st));
      }
    catch (const nbt_read_error& ex)
      {
        throw world_load_error (std::string ("corrupt NBT (") + ex.what () + ")");
      }
    
    return _load_chunk_from_nbt (root.get ());
  }
  
  
//------------------------------------------------------------------------------
  
  static void
  _make_chunk_data (chunk *ch, unsigned char*& out, int& olen)
  {
    nbt_writer nbt;
    nbt.start_compound ("");
    
    nbt.start_compound ("Level");
    
    nbt.put_int (ch->get_pos ().x, "xPos");
    nbt.put_int (ch->get_pos ().z, "zPos");
    nbt.put_byte_array (ch->get_biomes (), 256, "Biomes");
    nbt.put_int_array (ch->get_heightmap (), 256, "HeightMap");
    
    nbt.start_list (NBT_TAG_COMPOUND, "Sections");
    
    for (int i = 0; i < 16; ++i)
      {
        unsigned char buf[4096];
        sub_chunk *sub = ch->get_sub (i);
        if (!sub)
          continue;
        
        nbt.start_compound ();
        
        nbt.put_byte (i, "Y");
        
        for (int i = 0; i < 4096; ++i)
          buf[i] = (sub->types[i] >> 4) & 0xFF;
        nbt.put_byte_array (buf, 4096, "Blocks");
        
        for (int i = 0; i < 2048; ++i)
          {
            buf[i] = (sub->types[2*i] >> 12) & 15;
            buf[i] |= ((sub->types[2*i + 1] >> 12) & 15) << 4;
          }
        nbt.put_byte_array (buf, 2048, "Add");
        
        for (int i = 0; i < 2048; ++i)
          {
            buf[i] = sub->types[2*i] & 15;
            buf[i] |= (sub->types[2*i + 1] & 15) << 4;
          }
        nbt.put_byte_array (buf, 2048, "Data");
        
        nbt.put_byte_array (sub->bl, 2048, "BlockLight");
        nbt.put_byte_array (sub->sl, 2048, "SkyLight");
        
        nbt.end_compound (); // end of section
      }
    
    nbt.end_list (); // end of Sections
    nbt.end_compound (); // end of Level
    nbt.end_compound (); // end of root
    
    std::stringstream ss;
    nbt.write (ss, NBT_ZLIB);
    
    ss.seekg (0, std::ios_base::end);
    int len = (int)ss.tellg ();
    ss.seekg (0, std::ios_base::beg);
    
    out = new unsigned char [len];
    olen = len;
    ss.read ((char *)out, len);
  }
  
  
  
  static void
  _create_empty_region (const std::string& wpath, int rx, int rz)
  {
    std::ofstream fs (_region_path (wpath, rx, rz),
      std::ios_base::out | std::ios_base::binary);
    
    // 8KB header
    char arr[4096] = { 0 };
    fs.write (arr, 4096);   // locations
    fs.write (arr, 4096);   // timestampts
  }
  
  
  /* 
   * Appends chunk data to the end of the region file and updates the header
   * accordingly.
   */
  static void
  _append_chunk (std::fstream& fs, int cx, int cz, const unsigned char *data,
    unsigned int len)
  {
    int data_len = len + 5;
    int pad = !!(data_len & 4095) * (4096 - (data_len & 4095));
    int final_len = pad + data_len;
    
    fs.seekp (0, std::ios_base::end);
    int fpos = (int)fs.tellp ();
    
    unsigned char buf [4];
    bin::write_int_be (buf, len + 1);
    fs.write ((char *)buf, 4);
    fs.put (2); // compression type: zlib
    
    fs.write ((char *)data, len);
    
    // padding
    char zbuf[4096] = { 0 };
    fs.write (zbuf, pad);
    
    // update header
    unsigned int cindex = ((cz & 31) << 5) | (cx & 31);
    fs.seekp (4 * cindex);
    bin::write_int_be (buf, fpos >> 12);
    fs.write ((char *)buf + 1, 3);
    fs.put (final_len >> 12);
  }
  
  
  namespace {
    
    struct move_data
    {
      int x, z;
      unsigned char *data;
      unsigned int len;
      unsigned int pos;
    };
  }
  
  /* 
   * Writes chunk data to the specified location, overwriting chunks that are
   * in the way. Overwritten chunks are moved to the end of the region file.
   */
  static void
  _overwrite_chunk (std::fstream& fs, int cx, int cz, const unsigned char *data,
    unsigned int len, int location, int prev_size)
  {
    int data_len = len + 5;
    int pad = !!(data_len & 4095) * (4096 - (data_len & 4095));
    int final_len = pad + data_len;
    
    int s_start = location >> 12;
    int s_end   = s_start + (final_len >> 12);
    
    // read header
    unsigned char hdr[4096];
    fs.seekg (0);
    fs.read ((char *)hdr, 4096);
    
    // check header for chunks that are in the way.
    std::vector<move_data> to_move;
    if ((final_len >> 12) > prev_size)
      {
        for (int i = 0; i < 1024; ++i)
          {
            unsigned char b[4] = { 0 };
            std::memcpy (b + 1, hdr + i * 4, 3);
            int loc = bin::read_int_be (b);
            if (loc == (location >> 12))
              continue;
            int size = hdr[i * 4 + 3];
            
            if (loc >= s_start && loc < s_end)
              {
                // chunk will be overwritten
                
                move_data m;
                m.x = i & 31;
                m.z = i >> 5;
                m.data = new unsigned char [size << 12];
                m.len = size << 12;
                
                int ppos = (int)fs.tellg ();
                fs.seekg (loc << 12);
                fs.read ((char *)m.data, m.len);
                fs.seekg (ppos);
                
                to_move.push_back (m);
              }
          }
      }
    
    unsigned char b [4];
    unsigned char zbuf[4096] = { 0 };
    fs.seekp (location);
    bin::write_int_be (b, len + 1);
    fs.write ((char *)b, 4);
    fs.put (2); // compression type: zlib
    fs.write ((char *)data, len);
    fs.write ((char *)zbuf, pad);
    
    // copy overwritten chunks to end
    fs.seekp (0, std::ios_base::end);
    for (move_data& m : to_move)
      {
        m.pos = (unsigned int)fs.tellp ();
        fs.write ((char *)m.data, m.len);
      }
    
    // update header
    unsigned int cindex = ((cz & 31) << 5) | (cx & 31);
    hdr[cindex * 4 + 3] = final_len >> 12;
    for (move_data& m : to_move)
      {
        unsigned char b[4] = { 0 };
        bin::write_int_be (b, m.pos >> 12);
        
        int ind = 4 * (m.z << 5 | m.x);
        std::memcpy (hdr + ind, b + 1, 3);
      }
    fs.seekp (0, std::ios_base::beg);
    fs.write ((char *)hdr, 4096);
    
    fs.seekp (0, std::ios_base::end);
  }
  
  
  void
  anvil_world_provider::save_chunk (chunk *ch)
  {
    int cx = ch->get_pos ().x;
    int cz = ch->get_pos ().z;
    int rx = cx >> 5;
    int rz = cz >> 5;
    
    std::string rpath = _region_path (this->wpath, rx, rz);
    if (!fs::file_exists (rpath))
      {
        _create_empty_region (this->wpath, rx, rz);
      }
    
    std::fstream fs (rpath,
      std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    
    // read location
    unsigned int cindex = ((cz & 31) << 5) | (cx & 31);
    fs.seekg (4 * cindex);
    unsigned char buf[4] = { 0 };
    fs.read ((char *)buf + 1, 3);
    unsigned int loc = bin::read_int_be (buf) << 12;
    int size = fs.get ();
    
    unsigned char *chunk_data_ptr;
    int chunk_data_len;
    _make_chunk_data (ch, chunk_data_ptr, chunk_data_len);
    std::unique_ptr<unsigned char[]> chunk_data (chunk_data_ptr);
    
    if (loc == 0)
      {
        // chunk does not exist, append it to the end of the region file
        _append_chunk (fs, cx, cz, chunk_data.get (), chunk_data_len);
      }
    else
      {
        // chunk exists
        _overwrite_chunk (fs, cx, cz, chunk_data.get (), chunk_data_len, loc, size);
      }
    
    fs.flush ();
  }
  
  
  
//------------------------------------------------------------------------------
  
  static void
  _load_world_data_from_nbt (nbt_tag_compound *root, world_data& wd)
  {
    nbt_tag_compound *data = root->get ("Data")->as_compound ();
    
    wd.name = data->get ("LevelName")->as_string ();
    wd.gen_name = data->get ("generatorName")->as_string ();
    wd.seed = data->get ("RandomSeed")->as_long ();
    wd.spawn_pos = block_pos (
      data->get ("SpawnX")->as_int (),
      data->get ("SpawnY")->as_int (),
      data->get ("SpawnZ")->as_int ());
    
    {
      auto hcs = data->get ("hCraft");
      if (hcs)
        {
          auto hcc = hcs->as_compound ();
          wd.width = hcc->get ("Width")->as_int ();
          wd.depth = hcc->get ("Depth")->as_int ();
        }
      else
        {
          wd.width = -1;
          wd.depth = -1;
        }
    }
  }
  
  void
  anvil_world_provider::load_world_data (world_data& wd)
  {
    std::unique_ptr<nbt_tag_compound> root;
    
    std::ifstream fs (this->wpath + "/level.dat",
      std::ios_base::in | std::ios_base::binary);
    if (!fs)
      throw world_load_error ("level.dat not found");
    
    fs.seekg (0, std::ios_base::end);
    unsigned int size = (unsigned int)fs.tellg ();
    fs.seekg (0, std::ios_base::beg);
    std::unique_ptr<unsigned char []> data { new unsigned char [size] };
    fs.read ((char *)data.get (), size);
    
    try
      {
        nbt_reader reader;
        root.reset (reader.read (data.get (), size, NBT_GZIP));
        data.reset ();
      }
    catch (const nbt_read_error&)
      {
        throw world_load_error ("corrupt level.dat");
      }
    
    _load_world_data_from_nbt (root.get (), wd);
  }
  
  
  
  void
  anvil_world_provider::save_world_data (const world_data& wd)
  {
    nbt_writer writer;
    writer.start_compound ("");
    
    writer.start_compound ("Data");
    
    writer.put_int (19133, "version"); // NBT version
    writer.put_byte (1, "initialized");
    writer.put_string (wd.name, "LevelName");
    writer.put_string (wd.gen_name, "generatorName");
    writer.put_int (0, "generatorVersion");
    writer.put_long (wd.seed, "RandomSeed");
    writer.put_byte (0, "MapFeatures");
    writer.put_int ((int)wd.spawn_pos.x, "SpawnX");
    writer.put_int ((int)wd.spawn_pos.y, "SpawnY");
    writer.put_int ((int)wd.spawn_pos.z, "SpawnZ");
    
    writer.start_compound ("hCraft");
    writer.put_int (wd.width, "Width");
    writer.put_int (wd.depth, "Depth");
    writer.end_compound ();
    
    writer.end_compound (); // end of Data
    
    writer.end_compound (); // end of root
    
    std::ofstream fs (this->wpath + "/level.dat",
      std::ios_base::out | std::ios_base::binary);
    writer.write (fs, NBT_GZIP);
  }
  
  
  
  void
  anvil_world_provider::open (const std::string& path)
  {
    if (this->_open)
      this->close ();
    
    this->wpath = path;
    fs::create_dir (path);
    fs::create_dir (path + "/region");
    
    this->_open = true;
  }
  
  void
  anvil_world_provider::close ()
  {
    if (!this->_open)
      return;
    
    this->_open = false;
  }
}


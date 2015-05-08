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

#include "util/nbt.hpp"
#include "util/binary.hpp"
#include "util/byte_buffer.hpp"
#include <cstring>
#include <ostream>
#include <istream>
#include <zlib.h>
#include <memory>


namespace hc {
  
  unsigned char
  nbt_tag::as_byte () const
    { return (static_cast<const nbt_tag_byte *> (this))->get_value (); }
  
  short
  nbt_tag::as_short () const
    { return (static_cast<const nbt_tag_short *> (this))->get_value (); }
  
  int
  nbt_tag::as_int () const
    { return (static_cast<const nbt_tag_int *> (this))->get_value (); }
  
  long long
  nbt_tag::as_long () const
    { return (static_cast<const nbt_tag_long *> (this))->get_value (); }
  
  float
  nbt_tag::as_float () const
    { return (static_cast<const nbt_tag_float *> (this))->get_value (); }
  
  double
  nbt_tag::as_double () const
    { return (static_cast<const nbt_tag_double *> (this))->get_value (); }
  
  std::string
  nbt_tag::as_string () const
    { return (static_cast<const nbt_tag_string *> (this))->get_value (); }
  
  nbt_tag_byte_array*
  nbt_tag::as_byte_array ()
    { return static_cast<nbt_tag_byte_array *> (this); }
  
  nbt_tag_int_array*
  nbt_tag::as_int_array ()
    { return static_cast<nbt_tag_int_array *> (this); }
  
  nbt_tag_list*
  nbt_tag::as_list ()
    { return static_cast<nbt_tag_list *> (this); }
  
  nbt_tag_compound*
  nbt_tag::as_compound ()
    { return static_cast<nbt_tag_compound *> (this); }
  
  
  
  nbt_tag_byte_array::nbt_tag_byte_array (const unsigned char *data,
    unsigned int len)
  {
    this->data = new unsigned char [len];
    std::memcpy (this->data, data, len);
    this->len = len;
  }
  
  nbt_tag_byte_array::~nbt_tag_byte_array ()
  {
    delete[] this->data;
  }
  
  
  
  nbt_tag_int_array::nbt_tag_int_array (const int *data, unsigned int len)
  {
    this->data = new int [len];
    std::memcpy (this->data, data, len * 4);
    this->len = len;
  }
  
  nbt_tag_int_array::~nbt_tag_int_array ()
  {
    delete[] this->data;
  }
  
  
  
  nbt_tag_list::~nbt_tag_list ()
  {
    for (nbt_tag *t : this->tags)
      delete t;
  }
  
  
  
  nbt_tag_compound::~nbt_tag_compound ()
  {
    for (auto p : this->tags)
      delete p.second;
  }
  
  
  nbt_tag*
  nbt_tag_compound::get (const std::string& name)
  {
    auto itr = this->tags.find (name);
    if (itr == this->tags.end ())
      return nullptr;
    
    return itr->second;
  }
  
  
  
//------------------------------------------------------------------------------
  
  nbt_writer::nbt_writer ()
  {
    frame frm;
    frm.type = NBT_TAG_COMPOUND;
    frm.len = frm.len_pos = 0;
    this->frms.push (frm);
  }
  
  nbt_writer::~nbt_writer ()
  {
    
  }
  
  
  
//------------------------------------------------------------------------------
  
  static void
  _emit_tag_start (byte_buffer& buf, unsigned char id, const char *name)
  {
    int len = (int)std::strlen (name);
    
    buf.put_byte (id);
    buf.put_short_be (len);
    buf.put_bytes ((const unsigned char *)name, len);
  }
  
  
  
  void
  nbt_writer::put_byte (unsigned char val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 1, name);
    
    this->buf.put_byte (val);
  }
  
  void
  nbt_writer::put_short (unsigned short val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 2, name);
    
    this->buf.put_short_be (val);
  }
  
  void
  nbt_writer::put_int (unsigned int val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 3, name);
    
    this->buf.put_int_be (val);
  }
  
  void
  nbt_writer::put_long (unsigned long long val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 4, name);
      
    this->buf.put_long_be (val);
  }
  
  void
  nbt_writer::put_float (float val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 5, name);
      
    this->buf.put_float_be (val);
  }
  
  void
  nbt_writer::put_double (double val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 6, name);
    
    this->buf.put_double_be (val);
  }
  
  void
  nbt_writer::put_byte_array (const unsigned char *arr, int len,
    const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 7, name);
      
    this->buf.put_int_be (len);
    this->buf.put_bytes (arr, len);
  }
  
  void
  nbt_writer::put_int_array (const int *arr, int len, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 11, name);
      
    this->buf.put_int_be (len);
    for (int i = 0; i < len; ++i)
      this->buf.put_int_be (arr[i]);
  }
  
  void
  nbt_writer::put_string (const std::string& val, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 8, name);
    
    this->buf.put_short_be ((unsigned short)val.length ());
    this->buf.put_bytes ((const unsigned char *)val.c_str (),
      (unsigned int)val.length ());
  }
  
  
  void
  nbt_writer::start_list (int tag_type, const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 9, name);
    
    frame frm;
    frm.type = NBT_TAG_LIST;
    frm.len = 0;
    
    
    this->buf.put_byte ((unsigned char)tag_type);
    frm.len_pos = this->buf.get_pos ();
    this->buf.put_int_be (0);
    
    this->frms.push (frm);
  }
  
  void
  nbt_writer::end_list ()
  {
    auto frm = this->frms.top ();
    this->frms.pop ();
    
    auto ppos = this->buf.get_pos ();
    this->buf.seek (frm.len_pos);
    this->buf.put_int_be (frm.len);
    this->buf.seek (ppos);
  }
  
  
  void
  nbt_writer::start_compound (const char *name)
  {
    ++ this->frms.top ().len;
    if (this->frms.top ().type != NBT_TAG_LIST)
      _emit_tag_start (this->buf, 10, name);
    
    frame frm;
    frm.type = NBT_TAG_COMPOUND;
    frm.len = 0;
    frm.len_pos = 0;
    this->frms.push (frm);
  }
  
  void
  nbt_writer::end_compound ()
  {
    this->frms.pop ();
    
    this->buf.put_byte (0);
  }
  
  
  
//------------------------------------------------------------------------------
  
  static void
  _zlib_compress (std::ostream& strm, const unsigned char *data,
    unsigned int len, int window_bits)
  {
#define COMPRESSION_LEVEL 6
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    if (deflateInit2 (&zs, COMPRESSION_LEVEL, Z_DEFLATED, window_bits, 8,
      Z_DEFAULT_STRATEGY) != Z_OK)
      throw nbt_write_error ("zlib compression failed (deflateInit)");
    
    int cap = deflateBound (&zs, len);
    unsigned char *out = new unsigned char [cap];
    zs.next_in = (Bytef *)data;
    zs.avail_in = len;
    zs.next_out = out;
    zs.avail_out = cap;
    if (deflate (&zs, Z_FINISH) != Z_STREAM_END)
      {
        deflateEnd (&zs); 
        throw nbt_write_error ("zlib compression failed (deflate)");
      }
    
    deflateEnd (&zs);
    
    strm.write ((char *)out, zs.total_out);
    delete[] out;
  }
  
  
  /* 
   * Commits the data created in memory to the specified stream, optionally
   * compressing it.
   * 
   * Throws exceptions of type `nbt_write_error' on failure.
   */
  void
  nbt_writer::write (std::ostream& strm, nbt_compression_strategy strategy)
  {
    switch (strategy)
      {
      case NBT_RAW:
        strm.write ((char *)this->buf.get_data (), this->buf.get_length ());
        break;
      
      case NBT_ZLIB:
        _zlib_compress (strm, this->buf.get_data (), this->buf.get_length (), 15);
        break;
      
      case NBT_GZIP:
        _zlib_compress (strm, this->buf.get_data (), this->buf.get_length (), 31);
        break;
      }
  }
  
  
  
//------------------------------------------------------------------------------
  
#define UNEXPECTED_EOF  throw nbt_read_error ("unexpected EOF");
#define ADVANCE(N)      data += (N); avail -= (N);
  
  // forward decs:
  static nbt_tag* _read_tag (const unsigned char*& data, int& len);
  static nbt_tag* _read_payload (const unsigned char*& data, int& avail, int id);
  
  
  nbt_reader::nbt_reader ()
    { }
  
  
  
  static nbt_tag_byte*
  _read_payload_byte (const unsigned char*& data, int& avail)
  {
    if (avail == 0)
      UNEXPECTED_EOF
    
    unsigned char v = *data;
    ADVANCE(1)
    
    return new nbt_tag_byte (v);
  }
  
  static nbt_tag_short*
  _read_payload_short (const unsigned char*& data, int& avail)
  {
    if (avail < 2)
      UNEXPECTED_EOF
    
    short v = bin::read_short_be (data);
    ADVANCE(2)
    
    return new nbt_tag_short (v);
  }
  
  static nbt_tag_int*
  _read_payload_int (const unsigned char*& data, int& avail)
  {
    if (avail < 4)
      UNEXPECTED_EOF
    
    int v = bin::read_int_be (data);
    ADVANCE(4)
    
    return new nbt_tag_int (v);
  }
  
  static nbt_tag_long*
  _read_payload_long (const unsigned char*& data, int& avail)
  {
    if (avail < 8)
      UNEXPECTED_EOF
    
    long long v = bin::read_long_be (data);
    ADVANCE(8)
    
    return new nbt_tag_long (v);
  }
  
  static nbt_tag_float*
  _read_payload_float (const unsigned char*& data, int& avail)
  {
    if (avail < 4)
      UNEXPECTED_EOF
    
    float v = bin::read_float_be (data);
    ADVANCE(4)
    
    return new nbt_tag_float (v);
  }
  
  static nbt_tag_double*
  _read_payload_double (const unsigned char*& data, int& avail)
  {
    if (avail < 8)
      UNEXPECTED_EOF
    
    double v = bin::read_double_be (data);
    ADVANCE(8)
    
    return new nbt_tag_double (v);
  }
  
  static nbt_tag_byte_array*
  _read_payload_byte_array (const unsigned char*& data, int& avail)
  {
    if (avail < 4)
      UNEXPECTED_EOF
    
    int len = bin::read_int_be (data);
    ADVANCE(4)
    
    if (avail < len)
      UNEXPECTED_EOF
    
    nbt_tag_byte_array *tag = new nbt_tag_byte_array (data, len);
    ADVANCE(len)
    return tag;
  }
  
  static nbt_tag_string*
  _read_payload_string (const unsigned char*& data, int& avail)
  {
    if (avail < 2)
      UNEXPECTED_EOF
    
    unsigned short len = bin::read_short_be (data);
    ADVANCE(2)
    
    if (avail < len)
      UNEXPECTED_EOF
    
    char *str = new char [len + 1];
    std::memcpy (str, data, len);
    str[len] = '\0';
    ADVANCE(len)
    
    nbt_tag_string *tag = new nbt_tag_string (str);
    delete[] str;
    return tag;
  }
  
  static nbt_tag_list*
  _read_payload_list (const unsigned char*& data, int& avail)
  {
    if (avail < 5)
      UNEXPECTED_EOF
    
    int tagid = *data;
    ADVANCE(1)
    
    unsigned int count = bin::read_int_be (data);
    ADVANCE(4)
    
    std::unique_ptr<nbt_tag_list> lst { new nbt_tag_list () };
    for (int i = 0; i < (int)count; ++i)
      {
        nbt_tag *tag = _read_payload (data, avail, tagid);
        if (!tag)
          UNEXPECTED_EOF
        
        lst->get_values ().push_back (tag);
      }
    
    return lst.release ();
  }
  
  static nbt_tag_compound*
  _read_payload_compound (const unsigned char*& data, int& avail)
  {
    std::unique_ptr<nbt_tag_compound> comp { new nbt_tag_compound () };
    
    for (;;)
      {
        if (avail == 0)
          UNEXPECTED_EOF
        if (data[0] == 0)
          {
            // consume TAG_End
            ++ data;
            -- avail;
            break;
          }
        
        nbt_tag *tag = _read_tag (data, avail);
        if (!tag)
          UNEXPECTED_EOF
        comp->get_map ()[tag->get_name ()] = tag;
      }
    
    return comp.release ();
  }
  
  static nbt_tag_int_array*
  _read_payload_int_array (const unsigned char*& data, int& avail)
  {
    if (avail < 4)
      UNEXPECTED_EOF
    
    int len = bin::read_int_be (data);
    ADVANCE(4)
    
    if (avail < (len * 4))
      UNEXPECTED_EOF
    
    int *vals = new int [len];
    for (int i = 0; i < len; ++i)
      vals[i] = bin::read_int_be (data + (i * 4));
    ADVANCE(len * 4)
    
    nbt_tag_int_array *tag = new nbt_tag_int_array (vals, len);
    delete[] vals;
    return tag;
  }
  
  
  nbt_tag*
  _read_payload (const unsigned char*& data, int& avail, int id)
  {
    switch (id)
      {
      case 1:   return _read_payload_byte (data, avail);
      case 2:   return _read_payload_short (data, avail);
      case 3:   return _read_payload_int (data, avail);
      case 4:   return _read_payload_long (data, avail);
      case 5:   return _read_payload_float (data, avail);
      case 6:   return _read_payload_double (data, avail);
      case 7:   return _read_payload_byte_array (data, avail);
      case 8:   return _read_payload_string (data, avail);
      case 9:   return _read_payload_list (data, avail);
      case 10:  return _read_payload_compound (data, avail);
      case 11:  return _read_payload_int_array (data, avail);
      
      default:
        throw nbt_read_error ("invalid tag id"); 
      }
  }
  
  
  nbt_tag*
  _read_tag (const unsigned char*& data, int& avail)
  {
    if (avail == 0)
      return nullptr;
    
    // tag id
    int id = *data++;
    -- avail;
    
    // tag name
    if (avail < 2)
      UNEXPECTED_EOF
    short slen = bin::read_short_be (data);
    data += 2;
    avail -= 2;
    std::unique_ptr<char[]> name { new char [slen + 1] };
    if (avail < slen)
      UNEXPECTED_EOF
    std::memcpy (name.get (), data, slen);
    name[slen] = '\0';
    data += slen;
    avail -= slen;
    
    nbt_tag *tag = _read_payload (data, avail, id);
    tag->set_name (name.get ());
    name.reset ();
    
    return tag;
  }
  
  
  
  static nbt_tag_compound*
  _read_raw (const unsigned char *data, int len)
  {
    nbt_tag *tag = _read_tag (data, len);
    nbt_tag_compound *comp = dynamic_cast<nbt_tag_compound *> (tag);
    if (!comp)
      {
        delete tag;
        throw nbt_read_error ("expected NBT compound tag");
      }
    
    return comp;
  }
  
  static nbt_tag_compound*
  _read_compressed (const unsigned char *data, int len)
  {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.next_in = (Bytef *)data;
    zs.avail_in = len;
    if (inflateInit2 (&zs, 47) != Z_OK)
      throw nbt_read_error ("zlib decompresion failed (inflateInit)");
    
    byte_buffer buf;
    for (;;)
      {
        zs.next_out = buf.get_data () + zs.total_out;
        zs.avail_out = buf.get_capacity () - zs.total_out;
        auto ret = inflate (&zs, Z_NO_FLUSH);
        switch (ret)
          {
          case Z_STREAM_END:
            goto done;
          
          case Z_OK:
            break;
          
          case Z_BUF_ERROR:
            buf.extend (buf.get_capacity () * 2);
            break;
            
          default:
            deflateEnd (&zs);
            throw nbt_read_error ("zlib decompresion failed (inflate)");
          }
      }
    
  done:
    int olen = zs.total_out;
    inflateEnd (&zs);
    return _read_raw (buf.get_data (), olen);
  }
  
  /* 
   * Reads and returns an NBT Compound tag from the specified byte array.
   * The returned object must be destroyed by the caller.
   * 
   * Throws exceptions of type `nbt_reader' on failure.
   * Returns null if there are no more tags to read.
   */
  nbt_tag_compound*
  nbt_reader::read (const unsigned char *data, int len,
    nbt_compression_strategy strategy)
  {
    switch (strategy)
      {
      case NBT_RAW:
        return _read_raw (data, len);
      
      case NBT_GZIP:
      case NBT_ZLIB:
        return _read_compressed (data, len);
      }
    
    return nullptr;
  }
}


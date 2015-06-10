// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

/*
Copyright 2008 Pavel Koshevoy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : io_base.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 14 10:41:11 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : IO helpers for file i/o and trail record/replay.

// local includes:
#include "io/io_base.hxx"

// system includes:
#include <string.h>


//----------------------------------------------------------------
// is_open
//
bool
is_open(std::ostream & so)
{
  return so.good();
}


//----------------------------------------------------------------
// is_open
//
bool
is_open(std::istream & si)
{
  return si.good();
}


//----------------------------------------------------------------
// load_address_hex
//
static bool
load_address_hex(const std::string & txt, uint64_t & address)
{
  address = uint64_t(0);
  uint64_t sixteen_to_i = 1;

  size_t digits = txt.size();
  for (size_t i = 0; i < digits; i++)
  {
    size_t pos = digits - 1 - i;
    char c = txt[pos];

    uint64_t d = 0;
    if (c >= '0' && c <= '9')
    {
      d = uint64_t(c) - uint64_t('0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      d = uint64_t(c) - uint64_t('A') + 10;
    }
    else if (c >= 'a' && c <= 'f')
    {
      d = uint64_t(c) - uint64_t('a') + 10;
    }
    else if ((c == 'x' || c == 'X') && (pos == 1) && (txt[0] == '0'))
    {
      // ignore 0x:
      continue;
    }
    else
    {
      return false;
    }

    address += d * sixteen_to_i;
    sixteen_to_i *= 16;
  }

  return true;
}

//----------------------------------------------------------------
// load_address_dec
//
static bool
load_address_dec(const std::string & txt, uint64_t & address)
{
  address = uint64_t(0);
  uint64_t ten_to_i = 1;

  size_t digits = txt.size();
  for (size_t i = 0; i < digits; i++)
  {
    size_t pos = digits - 1 - i;
    char c = txt[pos];

    uint64_t d = 0;
    if (c >= '0' && c <= '9')
    {
      d = uint64_t(c) - uint64_t('0');
    }
    else
    {
      return false;
    }

    address += d * ten_to_i;
    ten_to_i *= 10;
  }

  return true;
}

//----------------------------------------------------------------
// load_address
//
bool
load_address(std::istream & si, uint64_t & address)
{
  std::string txt;
  si >> txt;

  if (txt.size() > 1 && txt[0] == '0' &&
      (txt[1] == 'x' || txt[1] == 'X'))
  {
    return load_address_hex(txt, address);
  }

  return load_address_dec(txt, address);
}

//----------------------------------------------------------------
// save_address_byte
//
inline void
save_address_byte(std::ostream & so, const unsigned char byte)
{
  static const char * hex = "0123456789abcdef";
  unsigned char hi = (byte >> 4) & 0xf;
  unsigned char lo = byte & 0xf;
  so << hex[hi] << hex[lo];
}

//----------------------------------------------------------------
// save_address_big_endian
//
static void
save_address_big_endian(std::ostream & so,
			const unsigned char * bytes,
			unsigned int num_bytes)
{
  so << "0x";
  for (unsigned int i = 0; i < num_bytes; i++)
  {
    const unsigned char byte = bytes[i];
    save_address_byte(so, byte);
  }
}

//----------------------------------------------------------------
// save_address_swap_bytes
//
static void
save_address_swap_bytes(std::ostream & so,
			const unsigned char * bytes,
			unsigned int num_bytes)
{
  so << "0x";
  for (int i = num_bytes - 1; i >= 0; i--)
  {
    const unsigned char byte = bytes[i];
    save_address_byte(so, byte);
  }
}

//----------------------------------------------------------------
// is_little_endian
//
static bool
is_little_endian()
{
  const unsigned char endian[] = { 1, 0 };
  short x = *(const short *)endian;
  return (x == 1);
}

//----------------------------------------------------------------
// save_addr_func_t
//
typedef void(*save_addr_func_t)(std::ostream &,
				const unsigned char *,
				unsigned int);

//----------------------------------------------------------------
// save_addr_func
//
static const save_addr_func_t
save_addr_func = (is_little_endian() ?
		  save_address_swap_bytes :
		  save_address_big_endian);

//----------------------------------------------------------------
// save_address
//
void
save_address(std::ostream & so, const void * address)
{
  save_addr_func(so, (unsigned char *)(&address), sizeof(void *));
}

//----------------------------------------------------------------
// save_address
//
void
save_address(std::ostream & so, uint64_t address)
{
  save_addr_func(so, (unsigned char *)(&address), sizeof(uint64_t));
}


//----------------------------------------------------------------
// encode_special_chars
//
const std::string
encode_special_chars(const std::string & text_plain,
		     const char * special_chars)
{
  static const char escape_char = '\\';

  std::string result;
  size_t text_size = text_plain.size();
  for (size_t i = 0; i < text_size; i++)
  {
    const char c = text_plain[i];
    if (c <= 32 ||
	c >= 127 ||
	c == escape_char ||
	strchr(special_chars, c))
    {
      result += escape_char;
      result += ('0' + char(int(c) / 100));
      result += ('0' + char((int(c) / 10) % 10));
      result += ('0' + char(int(c) % 10));
    }
    else
    {
      result += c;
    }
  }

  return result;
}

//----------------------------------------------------------------
// decode_special_chars
//
const std::string
decode_special_chars(const std::string & text_encoded)
{
  static const char escape_char = '\\';

  std::string result;
  size_t text_size = text_encoded.size();
  for (size_t i = 0; i < text_size; i++)
  {
    char c = text_encoded[i];

    // skip the escape character:
    if (c == escape_char)
    {
      char x = text_encoded[i + 1];
      char y = text_encoded[i + 2];
      char z = text_encoded[i + 3];

      c = char(int(x - '0') * 100 +
	       int(y - '0') * 10 +
	       int(z - '0'));
      i += 3;
    }

    result += c;
  }

  return result;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const long long unsigned int & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, long long unsigned int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const long unsigned int & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, long unsigned int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const unsigned int & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, unsigned int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const int & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, int & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const char & data)
{
  stream << int(data) << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, char & data)
{
  int ch;
  stream >> ch;
  data = char(ch);
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const bool & data)
{
  unsigned int tmp = data;
  stream << tmp << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, bool & data)
{
  unsigned int tmp = 0;
  stream >> tmp;
  data = (tmp != 0);
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const double & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, double & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const float & data)
{
  stream << data << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, float & data)
{
  stream >> data;
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const std::string & data)
{
  stream << data.size() << ' ' << data.data() << ' ';
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, std::string & data)
{
  size_t size = 0;
  stream >> size;
  data.resize(size);

  // eat the whitespace:
  stream.get();

  char * dst = &(data[0]);
  stream.read(dst, size);
  return true;
}


//----------------------------------------------------------------
// save
//
bool
save(std::ostream & stream, const io_base_t & data)
{
  stream << "io_base_t ";
  data.save(stream);
  return true;
}

//----------------------------------------------------------------
// load
//
bool
load(std::istream & stream, io_base_t & data)
{
  std::string magic;
  stream >> magic;
  if (magic != "io_base_t")
  {
    return false;
  }

  stream >> magic;
  bool ok = data.load(stream, magic);
  return ok;
}


//----------------------------------------------------------------
// io_base_t::loaders_
//
std::map<std::string, io_base_t::creator_t> io_base_t::loaders_;

//----------------------------------------------------------------
// io_base_t::disposer
//
void
io_base_t::disposer(io_base_t *& io)
{
  delete io;
  io = NULL;
}

//----------------------------------------------------------------
// io_base_t::load
//
// Load registered objects from a source stream.
//
// Loaded objects are passed to a hanler if one is specified.
// The handler may delete the loaded object, in which case
// the loaded object pointer will be set to NULL.
// If the handler did not NULL-out the loaded object pointer
// the object may be stored in a destination list if one was specified.
// If the destination list is NULL the loaded object is deleted.
bool
io_base_t::load(std::istream & src,
		handler_t handler,
		std::list<io_base_t *> * dst)
{
  if (!is_open(src))
  {
    return false;
  }

  while (true)
  {
    std::string magic;
    src >> magic;
    if (src.eof()) break;

    std::map<std::string, io_base_t::creator_t>::iterator i =
      loaders_.find(magic);
    if (i == loaders_.end())
    {
      return false;
    }

    io_base_t * io = i->second();
    if (!io) return false;

    bool ok = io->load(src, magic);
    if (ok && handler)
    {
      handler(io);
    }

    if (ok && dst && io)
    {
      dst->push_back(io);
    }
    else
    {
      // std::cout << "io_base_t::load -- deleting " << magic << std::endl;
      delete io;
    }

    if (!ok)
    {
      return false;
    }
  }

  return true;
}

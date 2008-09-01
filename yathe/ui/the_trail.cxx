/*
Copyright 2004-2007 University of Utah

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


// File         : the_trail.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003, 2004
// License      : MIT
// Description  : event trail recoring/playback abstract interface,
//                used for regression testing and debugging.

// local includes:
#include "ui/the_trail.hxx"
#include "utils/the_utils.hxx"
#include "utils/the_text.hxx"


// system includes:
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <limits>


//----------------------------------------------------------------
// the_trail_t::trail_
// 
the_trail_t * the_trail_t::trail_ = NULL;

//----------------------------------------------------------------
// the_trail_t::the_trail_t
// 
the_trail_t::the_trail_t(int & argc, char ** argv, bool record_by_default):
  record_by_default_(record_by_default),
  line_num_(0),
  milestone_(0),
  single_step_replay_(false),
  dont_load_events_(false),
  dont_save_events_(false),
  dont_post_events_(false),
  seconds_to_wait_(std::numeric_limits<unsigned int>::max())
{
  // It only makes sence to have single instance of this class,
  // so I will enforce it here:
  assert(trail_ == NULL);
  trail_ = this;
  
  bool given_record_name = false;
  bool given_replay_name = false;
  
  the_text_t trail_replay_name;
  the_text_t trail_record_name(".dont_record.txt");
  
  int argj = 1;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-ask") == 0)
    {
      single_step_replay_ = true;
    }
    else if (strcmp(argv[i], "-replay") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -replay: usage: " << argv[0]
	     << " -replay sample-in.txt" << endl;
	::exit(1);
      }
      
      i++;
      trail_replay_name = argv[i];
      given_replay_name = true;
    }
    else if (strcmp(argv[i], "-record") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -record: usage: " << argv[0]
	     << " -record sample-out.txt" << endl;
	::exit(1);
      }
      
      i++;
      trail_record_name = argv[i];
      given_record_name = true;
    }
    else if (strcmp(argv[i], "-wait") == 0)
    {
      if ((i + 1) >= argc)
      {
	cerr << "ERROR: option -wait: usage: " << argv[0]
	     << " -wait seconds" << endl;
	::exit(1);
      }
      
      i++;
      seconds_to_wait_ = the_text_t(argv[i]).toUInt();
    }
    else
    {
      // remove the arguments that deal with event playback:
      argv[argj] = argv[i];
      argj++;
    }
  }
  
  // update the argument parameter counter:
  argc = argj;
  
  // sanity check:
  if (trail_replay_name == trail_record_name)
  {
    cerr << "ERROR: trail record and replay names can not be the same, "
	 << "aborting..." << endl;
    ::exit(0);
  }
  
  if (given_replay_name)
  {
    replay_stream.open(trail_replay_name, ios::in);
    if (replay_stream.rdbuf()->is_open() == false)
    {
      cerr << "ERROR: could not open "
	   << trail_replay_name << " for playback"<<endl;
      ::exit(1);
    }
    else
    {
      cerr << "NOTE: starting event replay from " << trail_replay_name << endl;
    }
  }
  
  if (given_record_name || record_by_default_)
  {
    record_stream.open(trail_record_name, ios::out);
    if (record_stream.rdbuf()->is_open() == false)
    {
      if (given_record_name)
      {
	cerr << "ERROR: ";
      }
      else
      {
	cerr << "WARNING: ";
      }
      
      cerr << "could not open " << trail_record_name
	   << " trail file for recording"<<endl;
      
      if (given_record_name)
      {
	::exit(1);
      }
    }
  }
}

//----------------------------------------------------------------
// the_trail_t::~the_trail_t
// 
the_trail_t::~the_trail_t()
{
  if (replay_stream.rdbuf()->is_open()) replay_stream.close();
  if (record_stream.rdbuf()->is_open()) record_stream.close();
}

//----------------------------------------------------------------
// the_trail_t::replay_done
// 
void
the_trail_t::replay_done()
{
  if (replay_stream.rdbuf()->is_open())
  {
    replay_stream.close();
    cerr << "NOTE: finished event replay..." << endl;
  }
  
  dont_post_events_ = false;
}

//----------------------------------------------------------------
// the_trail_t::next_milestone_achieved
// 
void
the_trail_t::next_milestone_achieved()
{
  milestone_++;
}

//----------------------------------------------------------------
// load_address_hex
// 
static bool
load_address_hex(const std::string & txt, uint64_t & address)
{
  address = uint64_t(0);
  uint64_t sixteen_to_i = 1;
  
  unsigned int digits = txt.size();
  for (unsigned int i = 0; i < digits; i++)
  {
    unsigned int pos = digits - 1 - i;
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
  
  unsigned int digits = txt.size();
  for (unsigned int i = 0; i < digits; i++)
  {
    unsigned int pos = digits - 1 - i;
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
load_address(istream & si, uint64_t & address)
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
save_address_byte(ostream & so, const unsigned char byte)
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
save_address_big_endian(ostream & so,
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
save_address_swap_bytes(ostream & so,
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
typedef void(*save_addr_func_t)(ostream &,
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
save_address(ostream & so, const void * address)
{
  save_addr_func(so, (unsigned char *)(&address), sizeof(void *));
}

//----------------------------------------------------------------
// save_address
// 
void
save_address(ostream & so, uint64_t address)
{
  save_addr_func(so, (unsigned char *)(&address), sizeof(uint64_t));
}

//----------------------------------------------------------------
// operator >>
// 
istream &
operator >> (istream & si, uint64_t & address)
{
  load_address(si, address);
  return si;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & so, const uint64_t & address)
{
  save_address(so, address);
  return so;
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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
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


// File         : io_base.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 14 10:41:11 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : IO helpers for file i/o and trail record/replay.

#ifndef IO_BASE_HXX_
#define IO_BASE_HXX_

// system includes:
#include <map>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

// uint64_t:
#ifndef uint64_t
#  ifdef WIN32
typedef unsigned __int64 uint64_t;
#  else
#    include <inttypes.h>
#  endif
#endif

// forward declarations:
class io_base_t;


//----------------------------------------------------------------
// is_open
// 
extern bool
is_open(std::ostream & so);

//----------------------------------------------------------------
// is_open
// 
extern bool
is_open(std::istream & si);

//----------------------------------------------------------------
// load_address
// 
extern bool
load_address(std::istream & si, uint64_t & address);

//----------------------------------------------------------------
// save_address
// 
extern void
save_address(std::ostream & so, const void * address);

//----------------------------------------------------------------
// save_address
// 
extern void
save_address(std::ostream & so, uint64_t address);


//----------------------------------------------------------------
// encode_special_chars
// 
extern const std::string
encode_special_chars(const std::string & text_plain,
		     const char * special_chars = " \t\n");

//----------------------------------------------------------------
// decode_special_chars
// 
extern const std::string
decode_special_chars(const std::string & text_encoded);


extern bool save(std::ostream & stream, const long long unsigned int & data);
extern bool load(std::istream & stream, long long unsigned int & data);

extern bool save(std::ostream & stream, const long unsigned int & data);
extern bool load(std::istream & stream, long unsigned int & data);

extern bool save(std::ostream & stream, const unsigned int & data);
extern bool load(std::istream & stream, unsigned int & data);

extern bool save(std::ostream & stream, const int & data);
extern bool load(std::istream & stream, int & data);

extern bool save(std::ostream & stream, const char & data);
extern bool load(std::istream & stream, char & data);

extern bool save(std::ostream & stream, const bool & data);
extern bool load(std::istream & stream, bool & data);

extern bool save(std::ostream & stream, const double & data);
extern bool load(std::istream & stream, double & data);

extern bool save(std::ostream & stream, const float & data);
extern bool load(std::istream & stream, float & data);

extern bool save(std::ostream & stream, const std::string & data);
extern bool load(std::istream & stream, std::string & data);

extern bool save(std::ostream & stream, const io_base_t & data);
extern bool load(std::istream & stream, io_base_t & data);


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream,
     const data_t * data,
     const unsigned int & size)
{
  for (unsigned int i = 0; i < size; i++)
  {
    save(stream, data[i]);
  }
  
  return true;
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, data_t * data, const unsigned int & size)
{
  bool ok = true;
  for (unsigned int i = 0; i < size && ok; i++)
  {
    ok = load(stream, data[i]);
  }
  
  return ok;
}


//----------------------------------------------------------------
// save
// 
template <typename first_t, typename second_t>
bool
save(std::ostream & stream, const std::pair<first_t, second_t> & pair)
{
  bool ok = save(stream, pair.first);
  if (!ok) return false;
  
  return save(stream, pair.second);
}

//----------------------------------------------------------------
// load
// 
template <typename first_t, typename second_t>
bool
load(std::istream & stream, std::pair<first_t, second_t> & pair)
{
  bool ok = load(stream, pair.first);
  if (!ok) return false;
  
  return load(stream, pair.second);
}


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream, const std::list<data_t> & l)
{
  // save the list:
  save(stream, (unsigned int)(l.size()));
  stream << std::endl;
  
  for (typename std::list<data_t>::const_iterator i = l.begin();
       i != l.end();
       ++i)
  {
    save(stream, *i);
    stream << std::endl;
  }
  
  return true;
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, std::list<data_t> & l)
{
  l.clear();
  
  unsigned int size = 0;
  bool ok = load(stream, size);
  for (unsigned int i = 0; i < size && ok; i++)
  {
    data_t data;
    ok = load(stream, data);
    l.push_back(data);
  }
  
  return ok;
}


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream, const std::vector<data_t> & array)
{
  const unsigned int & size = array.size();
  bool ok = save(stream, size);
  
  for (unsigned int i = 0; i < size && ok; i++)
  {
    ok = save(stream, array[i]);
    stream << std::endl;
  }
  
  return ok;
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, std::vector<data_t> & array)
{
  unsigned int size = 0;
  bool ok = load(stream, size);
  array.resize(size);
  
  for (unsigned int i = 0; i < size && ok; i++)
  {
    ok = load(stream, array[i]);
  }
  
  return ok;
}


//----------------------------------------------------------------
// io_base_t
// 
class io_base_t
{
public:
  typedef io_base_t * (*creator_t)();
  
  // file I/O API:
  virtual ~io_base_t() {}
  virtual void save(std::ostream & so) const = 0;
  virtual bool load(std::istream & si, const std::string & magic) = 0;
  
  // typedef for function handling loaded io_base_t objects:
  typedef void (*handler_t)(io_base_t *&);
  
  // default handler for loaded io_base_t object simply deletes them:
  static void disposer(io_base_t *& io);
  
  // Load registered objects from a source stream.
  // Loaded objects are passed to a hanler if one is specified.
  // The handler may delete the loaded object, in which case
  // the loaded object pointer will be set to NULL.
  // If the handler did not NULL-out the loaded object pointer
  // the object may be stored in a destination list if one was specified.
  // If the destination list is NULL the loaded object is deleted.
  static bool load(std::istream & src,
		   handler_t handler = &io_base_t::disposer,
		   std::list<io_base_t *> * dst = NULL);
  
  static std::map<std::string, creator_t> loaders_;
};


#endif // IO_BASE_HXX_

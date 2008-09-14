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
  for (typename std::list<data_t>::const_iterator i = l.begin();
       i != l.end();
       ++i)
  {
    save(stream, *i);
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
  
  // Load registered objects from a source stream, store them in the
  // destination list. If the destination list is NULL the loaded
  // object is deleted instead.
  static bool load(std::istream & src, std::list<io_base_t *> * dst = NULL);
  
  static std::map<std::string, creator_t> loaders_;
};


#endif // IO_BASE_HXX_

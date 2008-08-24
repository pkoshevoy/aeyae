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


// File         : the_file_io.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Jul 27 10:43:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : File IO helper functions for common datatypes.

#ifndef THE_FILE_IO_HXX_
#define THE_FILE_IO_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "utils/the_dynamic_array.hxx"
#include "utils/the_text.hxx"

// system includes:
#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <fstream>


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

extern bool save(std::ostream & stream, const char * data);
extern bool save(std::ostream & stream, const the_text_t & data);
extern bool load(std::istream & stream, the_text_t & data);

class the_knot_point_t;
extern bool save(std::ostream & stream, const the_knot_point_t & d);
extern bool load(std::istream & stream, the_knot_point_t & d);

class the_registry_t;
extern bool save(std::ostream & stream, const the_registry_t & registry);
extern bool load(std::istream & stream, the_registry_t & registry);

class the_id_dispatcher_t;
extern bool save(std::ostream & stream, const the_id_dispatcher_t & d);
extern bool load(std::istream & stream, the_id_dispatcher_t & d);

class the_reference_t;
extern bool save(std::ostream & stream, const the_reference_t * ref);
extern bool load(std::istream & stream, the_reference_t *& ref);

class the_primitive_t;
extern bool save(std::ostream & stream, const the_primitive_t * primitive);
extern bool load(std::istream & stream, the_primitive_t *& primitive);

class the_document_t;
extern bool save(const the_text_t & magic,
		 const the_text_t & filename,
		 const the_document_t * doc);
extern bool load(const the_text_t & magic,
		 const the_text_t & filename,
		 the_document_t *& doc);

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
template <typename data_t>
bool
save(std::ostream & stream, const the_duplet_t<data_t> & duplet)
{
  return save<data_t>(stream, duplet.data(), 2);
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, the_duplet_t<data_t> & duplet)
{
  return load<data_t>(stream, duplet.data(), 2);
}


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream, const the_triplet_t<data_t> & triplet)
{
  return save<data_t>(stream, triplet.data(), 3);
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, the_triplet_t<data_t> & triplet)
{
  return load<data_t>(stream, triplet.data(), 3);
}


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream, const the_quadruplet_t<data_t> & quadruplet)
{
  return save<data_t>(stream, quadruplet.data(), 4);
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, the_quadruplet_t<data_t> & quadruplet)
{
  return load<data_t>(stream, quadruplet.data(), 4);
}


//----------------------------------------------------------------
// save
// 
template <typename data_t>
bool
save(std::ostream & stream, const the_dynamic_array_t<data_t> & array)
{
  const unsigned int & size = array.size();
  save(stream, size);
  
  for (unsigned int i = 0; i < size; i++)
  {
    save(stream, array[i]);
  }
  
  return true;
}

//----------------------------------------------------------------
// load
// 
template <typename data_t>
bool
load(std::istream & stream, the_dynamic_array_t<data_t> & array)
{
  unsigned int size = 0;
  bool ok = load(stream, size);
  
  for (unsigned int i = 0; i < size && ok; i++)
  {
    ok = load(stream, array[i]);
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
// the_loader_t
// 
template <typename data_t>
class the_loader_t
{
public:
  //----------------------------------------------------------------
  // loader_fn_t
  // 
  typedef bool(*loader_fn_t)(std::istream &, data_t *&);
  
  //----------------------------------------------------------------
  // loader_t
  // 
  typedef the_loader_t<data_t> loader_t;
  
  the_loader_t(const the_text_t & id = the_text_t(),
	       loader_fn_t loader = NULL):
    id_(id),
    loader_(loader)
  {}
  
  inline bool operator == (const loader_t & l) const
  { return id_ == l.id_; }
  
  bool load(std::istream & istr, data_t *& data) const
  { return loader_(istr, data); }
  
private:
  the_text_t id_;
  loader_fn_t loader_;
};

//----------------------------------------------------------------
// the_file_io_t
//
template <typename data_t>
class the_file_io_t
{
public:
  //----------------------------------------------------------------
  // loader_t
  // 
  typedef the_loader_t<data_t> loader_t;
  
  // add a file io handler:
  void add(const loader_t & loader)
  {
    unsigned int i = loaders_.index_of(loader);
    
    if (i == ~0u)
    {
      // add a new loader:
      loaders_.append(loader);
    }
    else
    {
      // replace the old loader:
      loaders_[i] = loader;
    }
  }
  
  // load data from a stream:
  bool load(std::istream & stream, data_t *& data) const
  {
    data = NULL;
    
    the_text_t magic_word;
    bool ok = ::load(stream, magic_word);
    if (!ok)
    {
      return false;
    }
    
    if (magic_word == "NULL")
    {
      return true;
    }
    
    std::cout << "loading " << magic_word << endl;
    unsigned int i = loaders_.index_of(loader_t(magic_word, NULL));
    if (i == ~0u)
    {
      return false;
    }
    
    const loader_t & loader = loaders_[i];
    return loader.load(stream, data);
  }
  
private:
  the_dynamic_array_t<loader_t> loaders_;
};

//----------------------------------------------------------------
// the_primitive_file_io
// 
extern the_file_io_t<the_primitive_t> & the_primitive_file_io();

//----------------------------------------------------------------
// the_reference_file_io
// 
extern the_file_io_t<the_reference_t> & the_reference_file_io();


//----------------------------------------------------------------
// the_loader
// 
template <typename base_t, typename data_t>
bool
the_loader(std::istream & stream, base_t *& base)
{
  data_t * data = new data_t();
  bool ok = data->load(stream);
  base = data;
  return ok;
}


#endif // THE_FILE_IO_HXX_

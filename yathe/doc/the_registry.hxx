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


// File         : the_registry.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The registry -- used to keep track of document primitives.

#ifndef THE_REGISTRY_HXX_
#define THE_REGISTRY_HXX_

// system includes:
#include <list>
#include <assert.h>

// local includes:
#include "utils/the_utils.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_dynamic_array.hxx"

// forward declarations:
class the_primitive_t;


//----------------------------------------------------------------
// the_id_dispatcher_t
// 
// This class will be used to distribute ID to the world.
// I hope that none of the models will ever be large enough
// for a rollover:
class the_id_dispatcher_t
{
  friend bool save(std::ostream & stream, const the_id_dispatcher_t & di);
  friend bool load(std::istream & stream, the_id_dispatcher_t & dispatch);
  
public:
  the_id_dispatcher_t(): id_(0) {}
  the_id_dispatcher_t(const the_id_dispatcher_t & dispatcher);
  
  // Use ID's from the reuse list unless empty. If empty
  // then return the current ID counter and postincrement it.
  inline unsigned int dispatch_id()
  {
    if (!reuse_.empty())
    {
      return remove_head(reuse_);
    }
    
    return id_++;
  }
  
  // return the last ID issued:
  inline unsigned int last_id() const
  { return id_ - 1; }
  
  // Insert the ID into the reuse list, the Model Primitive
  // does not need it anymore.
  inline void release(const unsigned int & id)
  {
    reuse_.push_front(id);
#ifndef NDEBUG
    reuse_.sort();
#endif
  }
  
  // Clear the reuse list and reset the ID back to 0.
  inline void reset()
  { reuse_.clear(); id_ = 0; }
  
  // accessors:
  const std::list<unsigned int> & reuse() const
  { return reuse_; }
  
  const unsigned int & id() const
  { return id_; }
  
  // For debugging, dumps this model primitive id dispatcher:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
private:
  // a list of IDs that are no longer used:
  std::list<unsigned int> reuse_;
  
  // next unused ID:
  unsigned int id_;
};

//----------------------------------------------------------------
// the_registry_t
// 
// The master table for all primitives in existence:
class the_registry_t
{
  friend bool save(std::ostream & stream, const the_registry_t & registry);
  friend bool load(std::istream & stream, the_registry_t & registry);
  
public:
  the_registry_t();
  the_registry_t(const the_registry_t & registry);
  ~the_registry_t();
  
  void clear();
  
  inline the_primitive_t * elem(const unsigned int & id) const
  {
#ifndef NDEBUG
    assert(id != UINT_MAX);
    try
    {
      return table_[id];
    }
    catch (...)
    {
      // when compiled in debug mode, fail the assertion rather than
      // throwing an exception:
      assert(false);
      return table_[id];
    }
#else
    return table_[id];
#endif
  }
  
  inline the_primitive_t *& elem(const unsigned int & id)
  {
#ifndef NDEBUG
    assert(id != UINT_MAX);
    try
    {
      return table_[id];
    }
    catch (...)
    {
      // when compiled in debug mode, fail the assertion rather than
      // throwing an exception:
      assert(false);
      return table_[id];
    }
#else
    return table_[id];
#endif
  }
  
  template <class prim_t> prim_t * elem(const unsigned int & id) const
  { return dynamic_cast<prim_t *>(elem(id)); }
  
  inline the_primitive_t * operator[] (const unsigned int & id) const
  { return elem(id); }
  
  inline the_primitive_t *& operator[] (const unsigned int & id)
  { return elem(id); }
  
  // current size of the registry (not actual size):
  inline unsigned int size() const
  { return dispatcher_.last_id() + 1; }
  
  void add(the_primitive_t * prim);
  void del(the_primitive_t * prim);
  
  // create a dependency-sorted list of all primitives in the registry:
  void graph(std::list<unsigned int> & graph) const;
  
  // collect primitives of a certain type into an array:
  template <class prim_t>
  unsigned int
  collect(the_dynamic_array_t<prim_t *> & prims) const
  {
    prims.clear();
    the_dynamic_array_ref_t<prim_t *> array_ref(prims);
    
    const unsigned int registry_size = size();
    for (unsigned int i = 0; i < registry_size; i++)
    {
      prim_t * prim = dynamic_cast<prim_t *>(elem(i));
      if (prim == NULL) continue;
      
      array_ref << prim;
    }
    
    return prims.size();
  }
  
  template <class prim_t>
  unsigned int
  collect(std::list<prim_t *> & prims) const
  {
    prims.clear();
    
    const unsigned int registry_size = size();
    for (unsigned int i = 0; i < registry_size; i++)
    {
      prim_t * prim = dynamic_cast<prim_t *>(elem(i));
      if (prim == NULL) continue;
      
      prims.append(prim);
    }
    
    return prims.size();
  }
  
  // For debugging, dumps this model primitive table:
  void dump(std::ostream & strm, unsigned int indent = 0) const;
  
  // for debugging:
  void assert_sanity() const;
  
private:
  // An instance of an ID dispatcher. This way the table will
  // know when it's time to grow. The IDs are taken from the
  // dispatcher and assigned to the primitives being inserted
  // into the table. The ID is returned when a primitive is
  // removed from the table:
  the_id_dispatcher_t dispatcher_;
  
  // The table itself (it's nothing more than a set of pointers
  // directly referencing the primitives; the id of a primitive
  // is used as the index into the table):
  the_dynamic_array_t<the_primitive_t *> table_;
};


#endif // THE_REGISTRY_HXX_

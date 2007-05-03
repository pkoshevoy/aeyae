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


// File         : the_primitive.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The document primitive object.

#ifndef THE_PRIMITIVE_HXX_
#define THE_PRIMITIVE_HXX_

// system includes:
#include <list>

// local includes:
#include "doc/the_registry.hxx"
#include "utils/the_unique_list.hxx"
#include "utils/the_utils.hxx"
#include "io/the_file_io.hxx"

// forward declarations:
class the_view_volume_t;

#ifndef NOUI
#include "math/the_color.hxx"
class the_pick_data_t;
class the_dl_elem_t;
#endif // NOUI


//----------------------------------------------------------------
// the_primitive_state_t
//
// The state that any model primitive can be in:
typedef enum
{
  THE_REGULAR_STATE_E = 0,
  THE_HILITED_STATE_E,
  THE_SELECTED_STATE_E,
  THE_FAILED_STATE_E,
  THE_NUMBER_OF_STATES_E
} the_primitive_state_t;

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & strm, the_primitive_state_t state);


//----------------------------------------------------------------
// the_primitive_t
// 
// The base class for all model primitives:
class the_primitive_t
{
  friend class the_registry_t;
  friend bool save(std::ostream & stream, const the_primitive_t * primitive);
  friend bool load(std::istream & stream, the_primitive_t *& primitive);
  friend bool load(std::istream & stream, the_registry_t & registry);
  
public:
  the_primitive_t();
  the_primitive_t(const the_primitive_t & primitive);
  virtual ~the_primitive_t();
  
  // every primitive must be able to make a duplicate of itself:
  virtual the_primitive_t * clone() const = 0;
  
  // a human-readable name, should be provided by each instantiable primitive:
  virtual const char * name() const = 0;
  
  // The registry will call these functions whenever this primitive is
  // added to or removed from the registry. These functions should be
  // overriden by the classes that require dependence graph updates:
  virtual void added_to_the_registry(the_registry_t * registry,
				     const unsigned int & id);
  virtual void removed_from_the_registry();
  
  // Our means of identification:
  inline the_registry_t * registry() const
  { return registry_; }
  
  inline const unsigned int & id() const
  { return id_; }
  
  inline the_primitive_t * primitive(const unsigned int & id) const
  { return registry_->elem(id); }
  
  // these can be overridden if necessary:
  virtual void add_supporter(const unsigned int & supporter_id)
  { direct_supporters_.push_back(supporter_id); }
  
  virtual void del_supporter(const unsigned int & supporter_id)
  { direct_supporters_.remove(supporter_id); }
  
  virtual void add_dependent(const unsigned int & dependent_id)
  { direct_dependents_.push_back(dependent_id); }
  
  virtual void del_dependent(const unsigned int & dependent_id)
  { direct_dependents_.remove(dependent_id); }
  
  // accessors to the supporters/dependents of this primitive:
  inline const the::unique_list<unsigned int> & direct_dependents() const
  { return direct_dependents_; }
  
  inline const the::unique_list<unsigned int> & direct_supporters() const
  { return direct_supporters_; }
  
  // check whether this primitive is a direct supporter of a given primitive:
  inline bool direct_supporter_of(const unsigned int & id) const
  { return direct_dependents_.has(id); }
  
  // check whether this primitive is a direct dependent of a given primitive:
  inline bool direct_dependent_of(const unsigned int & id) const
  { return direct_supporters_.has(id); }
  
  // check whether this primitive depends on a given primitive:
  inline bool depends(const unsigned int & id) const
  { return registry_->elem(id)->supports(id_); }
  
  // check whether this primitive supports a given primitive:
  bool supports(const unsigned int & id) const;
  
  // collect all dependents of this primitive:
  void dependents(std::list<unsigned int> & dependents) const;
  
  // collect all supporters of this primitive:
  void supporters(std::list<unsigned int> & supporters) const;
  
  // every primitive should be able to regenerate itself:
  virtual bool regenerate() = 0;
  
  // check whether this primitive is up-to-date:
  inline bool regenerated() const
  { return regenerated_; }
  
  // mark this primitive and all of it's dependents as un-regenerated:
  void request_regeneration();
  
  // current state of this primitive:
  inline the_primitive_state_t current_state() const
  {
    if (current_state_.size() == 0) return THE_REGULAR_STATE_E;
    return current_state_.back();
  }
  
  inline void set_current_state(const the_primitive_state_t & state)
  { current_state_.push_back(state); }
  
  inline void clear_current_state()
  { current_state_.clear(); }
  
  inline bool has_state(const the_primitive_state_t & state) const
  { return has(current_state_, state); }
  
  inline void clear_state(const the_primitive_state_t & state)
  { current_state_.remove(state); }
  
#ifndef NOUI
  // color of the model primitive:
  virtual the_color_t color() const;
  
  // this is used during intersection/proximity testing (for selection):
  virtual bool
  intersect(const the_view_volume_t & /* volume */,
	    std::list<the_pick_data_t> & /* data */) const
  { return false; }
  
  // override this for a generic primitve display mechanism:
  virtual the_dl_elem_t * dl_elem() const
  { return NULL; }
#endif // NOUI
  
  // file io:
  virtual bool save(std::ostream & stream) const;
  virtual bool load(std::istream & stream);
  
  // For debugging, dumps this model primitive table:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;
  
protected:
  // the registry to which this primitive was added:
  the_registry_t * registry_;
  
  // id of this primitive (can be used to lookup this object in the registry):
  unsigned int id_;
  
  // this flag will be set to false to indicate that regeneration
  // is required:
  bool regenerated_;
  
  // a list of states that this primitive is currently in:
  std::list<the_primitive_state_t> current_state_;
  
  // a list of primitives that directly depend on this primitive:
  the::unique_list<unsigned int> direct_dependents_;
  
  // a list of primitives that directly support this primitive:
  the::unique_list<unsigned int> direct_supporters_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & s, const the_primitive_t & p);

//----------------------------------------------------------------
// establish_supporter_dependent
// 
// supporter/dependent graph management functions:
extern void
establish_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id);

//----------------------------------------------------------------
// breakdown_supporter_dependent
// 
extern void
breakdown_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id);


//----------------------------------------------------------------
// nth_dependent
// 
// Lookup the n-th direct dependent of type dependent_t for a
// given primitive.
// 
// NOTE: n is zero based, so to find the first occurence specify n = 0.
// 
template <class dependent_t>
dependent_t *
nth_dependent(const the_primitive_t * primitive, unsigned int n)
{
  assert(primitive != NULL);
  
  the_registry_t * registry = primitive->registry();
  assert(registry != NULL);

  unsigned int index = 0;
  const the::unique_list<unsigned int> & deps = primitive->direct_dependents();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    dependent_t * dep = registry->template elem<dependent_t>(*iter);
    if (dep != NULL)
    {
      if (n == index)
      {
	return dep;
      }
      else
      {
	index++;
      }
    }
  }
  
  return NULL;
}

//----------------------------------------------------------------
// first_dependent
// 
// Lookup the first direct dependent of type dependent_t for a
// given primitive:
// 
template <class dependent_t>
dependent_t *
first_dependent(const the_primitive_t * primitive)
{
  return nth_dependent<dependent_t>(primitive, 0);
}

//----------------------------------------------------------------
// nth_supporter
// 
// Lookup the n-th direct supporter of type supporter_t for a
// given primitive.
// 
// NOTE: n is zero based, so to find the first occurence specify n = 0.
// 
template <class supporter_t>
supporter_t *
nth_supporter(const the_primitive_t * primitive, unsigned int n)
{
  assert(primitive != NULL);
  
  the_registry_t * registry = primitive->registry();
  assert(registry != NULL);
  
  unsigned int index = 0;
  const the::unique_list<unsigned int> & sups = primitive->direct_supporters();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = sups.begin(); iter != sups.end(); ++iter)
  {
    supporter_t * sup = registry->template elem<supporter_t>(*iter);
    if (sup != NULL)
    {
      if (n == index)
      {
	return sup;
      }
      else
      {
	index++;
      }
    }
  }
  
  return NULL;
}


//----------------------------------------------------------------
// first_supporter
// 
// Lookup the first direct supporter of type supporter_t for a
// given primitive:
// 
template <class supporter_t>
supporter_t *
first_supporter(const the_primitive_t * primitive)
{
  return nth_supporter<supporter_t>(primitive, 0);
}

//----------------------------------------------------------------
// direct_dependents
// 
// Lookup the direct dependents of type dependent_t for a
// given primitive, return the number of dependents found:
// 
template <class dependent_t>
unsigned int
direct_dependents(const the_primitive_t * primitive,
		  std::list<dependent_t *> & dependents)
{
  assert(primitive != NULL);
  
  the_registry_t * registry = primitive->registry();
  assert(registry != NULL);
  
  unsigned int num_found = 0;
  
  const the::unique_list<unsigned int> & deps = primitive->direct_dependents();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    dependent_t * dep = registry->template elem<dependent_t>(*iter);
    if (dep == NULL) continue;
    
    dependents.push_back(dep);
    num_found++;
  }
  
  return num_found;
}

//----------------------------------------------------------------
// direct_supporters
// 
// Lookup the direct supporters of type supporter_t for a
// given primitive, return the number of supporters found:
// 
template <class supporter_t>
unsigned int
direct_supporters(const the_primitive_t * primitive,
		  std::list<supporter_t *> & supporters)
{
  assert(primitive != NULL);
  
  the_registry_t * registry = primitive->registry();
  assert(registry != NULL);
  
  unsigned int num_found = 0;
  
  const the::unique_list<unsigned int> & deps = primitive->direct_supporters();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    supporter_t * sup = registry->template elem<supporter_t>(*iter);
    if (sup == NULL) continue;
    
    supporters.push_back(sup);
    num_found++;
  }
  
  return num_found;
}


#endif // THE_PRIMITIVE_HXX_

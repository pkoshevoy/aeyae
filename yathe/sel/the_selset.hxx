// File         : the_selset.hxx
// Author       : Paul A. Koshevoy
// Created      : Sat Jan 22 13:02:00 MDT 2005
// Copyright    : (C) 2005
// License      : GPL.
// Description  : 

#ifndef THE_SELSET_HXX_
#define THE_SELSET_HXX_

// system includes:
#include <list>

// local includes:
#include "doc/the_registry.hxx"
#include "doc/the_primitive.hxx"
#include "utils/the_utils.hxx"

// forward declarations:
class the_graph_t;


//----------------------------------------------------------------
// the_selset_t
// 
class the_selset_t
{
public:
  virtual ~the_selset_t() {}
  
  // each subclass should specify the registry:
  virtual the_registry_t * registry() const = 0;
  
  // shortcut conversion from an ID to a primitive pointer of a given type:
  template <class prim_t> prim_t * prim(const unsigned int & id) const
  { return dynamic_cast<prim_t *>(registry()->elem(id)); }
  
  // check whether the selection set contains a given primitive:
  template <class prim_t> prim_t * has(const unsigned int & id) const
  {
    if (::has(records_, id) == false) return NULL;
    return prim<prim_t>(id);
  }
  
  // return the first/last selected element:
  template <class prim_t> prim_t * head() const
  {
    if (records_.empty()) return NULL;
    return prim<prim_t>(records_.front());
  }
  
  template <class prim_t> prim_t * tail() const
  {
    if (records_.empty()) return NULL;
    return prim<prim_t>(records_.back());
  }
  
  // check whether the selection set is empty:
  inline bool is_empty() const
  { return records_.empty(); }
  
  // check whether a primitive may be successfully added to the selection set:
  bool may_activate(const unsigned int & id) const;
  
  // activate a given primitive and add it to the selection set:
  virtual bool activate(const unsigned int & id);
  
  // deactivate a given primitive and remove it from the selection set:
  virtual bool deactivate(const unsigned int & id);
  
  // deactivate all primitives in the selection set, clear the set:
  void deactivate_all();
  
  // deactivate all primitives, replace the set with the given primitive:
  bool set_active(const unsigned int & id);
  
  // deactivate a primitive if it is active, activate if inactive:
  void toggle_active(const unsigned int & id);
  
  // put together a dependency graph using the selection for the roots:
  void graph(the_graph_t & graph) const;
  
  // accessors:
  inline const std::list<unsigned int> & records() const
  { return records_; }
  
protected:
  // selection records:
  std::list<unsigned int> records_;
};


#endif // THE_SELSET_HXX_

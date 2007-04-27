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

// the includes:
#include <doc/the_registry.hxx>
#include <doc/the_graph.hxx>
#include <utils/the_utils.hxx>


//----------------------------------------------------------------
// the_base_selset_t
// 
template <typename TRecord, typename TTraits>
class the_base_selset_t
{
public:
  typedef TRecord record_t;
  typedef TTraits traits_t;
  typedef the_base_selset_t<TRecord, TTraits> self_t;
  
  virtual ~the_base_selset_t() {}
  
  // check whether the selection set contains a given primitive:
  record_t * has(const record_t & rec)
  {
    typename std::list<record_t>::iterator iter =
      std::find(records_.begin(), records_.end(), rec);
    return (iter == records_.end()) ? NULL : &(*iter);
  }
  
  inline const record_t * has(const record_t & rec) const
  {
    typename std::list<record_t>::const_iterator iter =
      std::find(records_.begin(), records_.end(), rec);
    return (iter == records_.end()) ? NULL : &(*iter);
  }
  
  // return the first/last selected element:
  inline const record_t * head() const
  {
    if (records_.empty()) return NULL;
    return &(records_.front());
  }
  
  inline record_t * head()
  {
    if (records_.empty()) return NULL;
    return &(records_.front());
  }
  
  inline const record_t * tail() const
  {
    if (records_.empty()) return NULL;
    return &(records_.back());
  }
  
  inline record_t * tail()
  {
    if (records_.empty()) return NULL;
    return &(records_.back());
  }
  
  // check whether the selection set is empty:
  inline bool is_empty() const
  { return records_.empty(); }
  
  // check whether a primitive may be successfully added to the selection set:
  bool may_activate(const record_t & rec) const
  {
    if (!traits_.is_valid(rec))
    {
      return false;
    }
    
    return !::has(records_, rec);
  }
  
  // activate a given primitive and add it to the selection set:
  virtual bool activate(const record_t & rec)
  {
    if (!may_activate(rec))
    {
      return false;
    }
    
    records_.push_back(rec);
    traits_.activate(records_.back());
    return true;
  }
  
  // deactivate a given primitive and remove it from the selection set:
  virtual bool deactivate(const record_t & rec)
  {
    record_t * rec_ptr = has(rec);
    if (rec_ptr == NULL)
    {
      return false;
    }
    
    traits_.deactivate(*rec_ptr);
    records_.remove(rec);
    return true;
  }
  
  // deactivate all primitives in the selection set, clear the set:
  void deactivate_all()
  {
    while (!records_.empty())
    {
      const record_t rec = records_.front();
      deactivate(rec);
    }
  }
  
  // deactivate all primitives, replace the set with the given primitive:
  bool set_active(const record_t & rec)
  {
    deactivate_all();
    return activate(rec);
  }
  
  // deactivate a primitive if it is active, activate if inactive:
  void toggle_active(const record_t & rec)
  {
    if (!deactivate(rec))
    {
      activate(rec);
    }
  }
  
  // accessors:
  inline const std::list<record_t> & records() const 
  { return records_; }
  
  inline std::list<record_t> & records()
  { return records_; }
  
protected:
  // selection records:
  std::list<record_t> records_;
  
public:
  // the selection set traits:
  traits_t traits_;
};


//----------------------------------------------------------------
// the_selset_traits_t
// 
class the_selset_traits_t
{
public:
  virtual ~the_selset_traits_t() {}
  
  the_registry_t * registry() const;
  
  virtual bool is_valid(const unsigned int & id) const;
  virtual bool activate(const unsigned int & id) const;
  virtual bool deactivate(const unsigned int & id) const;
};


//----------------------------------------------------------------
// the_selset_t
//
template <typename TTraits>
class the_selset_t : public the_base_selset_t<unsigned int, TTraits>
{
public:
  typedef the_base_selset_t<unsigned int, TTraits> super_t;
  
  // shortcut conversion from an ID to a primitive pointer of a given type:
  template <class prim_t>
  inline prim_t * prim(const unsigned int & id) const
  {
    return super_t::traits_.registry()->template elem<prim_t>(id);
  }
  
  // check whether the selection set contains a given primitive:
  template <class prim_t>
  inline prim_t * has_prim(const unsigned int & id) const
  {
    return
      super_t::has(id) ?
      prim<prim_t>(id) :
      NULL;
  }
  
  // return the first/last selected element:
  template <class prim_t>
  inline prim_t * head_prim() const
  {
    return
      super_t::is_empty() ?
      NULL :
      prim<prim_t>(super_t::records_.front());
  }
  
  template <class prim_t>
  inline prim_t * tail_prim() const
  {
    return
      super_t::is_empty() ?
      NULL :
      prim<prim_t>(super_t::records_.back());
  }
  
  // put together a dependency graph using the selection for the roots:
  inline void graph(the_graph_t & graph) const
  {
    graph.set_roots(super_t::traits_.registry(), super_t::records_);
  }
};


#endif // THE_SELSET_HXX_

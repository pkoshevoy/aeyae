// File         : the_pick_filter.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

#ifndef THE_PICK_FILTER_HXX_
#define THE_PICK_FILTER_HXX_

// local includes:
#include "doc/the_registry.hxx"


//----------------------------------------------------------------
// the_pick_filter_t
//
class the_pick_filter_t
{
public:
  virtual ~the_pick_filter_t() {}
  
  virtual bool allow(const the_registry_t * registry,
		     const unsigned int & id) const
  { return registry->elem(id) != NULL; }
};

//----------------------------------------------------------------
// the_pick_filter_t<prim_t>
// 
template <class prim_t>
class the_pick_filter : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry,
	     const unsigned int & id) const
  { return dynamic_cast<prim_t *>(registry->elem(id)) != NULL; }
};


#endif // THE_PICK_FILTER_HXX_

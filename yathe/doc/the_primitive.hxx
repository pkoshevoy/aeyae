// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
#include "doc/the_graph_node.hxx"
#include "doc/the_registry.hxx"
#include "utils/the_unique_list.hxx"
#include "utils/the_utils.hxx"
#include "io/the_file_io.hxx"
#include "math/the_color.hxx"

// forward declarations:
class the_view_volume_t;
class the_pick_data_t;
class the_dl_elem_t;


//----------------------------------------------------------------
// the_primitive_state_t
//
// The state that any model primitive can be in:
typedef enum
{
  THE_REGULAR_STATE_E = 0,
  THE_HILITED_STATE_E = 1,
  THE_SELECTED_STATE_E = 2,
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
class the_primitive_t : public the_graph_node_t
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

  inline the_primitive_t * primitive(const unsigned int & id) const
  { return registry_->elem<the_primitive_t>(id); }

  // current state of this primitive:
  inline the_primitive_state_t current_state() const
  {
    if (current_state_.size() == 0)
    {
      return THE_REGULAR_STATE_E;
    }

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

  // For debugging, dumps this model primitive table:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  // a list of states that this primitive is currently in:
  std::list<the_primitive_state_t> current_state_;
};


#endif // THE_PRIMITIVE_HXX_

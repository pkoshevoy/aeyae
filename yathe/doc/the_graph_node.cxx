// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_graph_node.cxx
// Author       : Pavel A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Implementation of dependency graph node.

// local includes:
#include "doc/the_graph_node.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_utils.hxx"
#include "utils/debug.hxx"


//----------------------------------------------------------------
// the_graph_node_t::timestamp_
//
unsigned int
the_graph_node_t::timestamp_ = 0;

//----------------------------------------------------------------
// the_graph_node_t::the_graph_node_t
//
the_graph_node_t::the_graph_node_t():
  registry_(NULL),
  id_(UINT_MAX),
  regeneration_state_(the_graph_node_t::REGENERATION_REQUESTED_E),
  timestamp_visited_(0)
{}

//----------------------------------------------------------------
// the_graph_node_t::the_graph_node_t
//
the_graph_node_t::the_graph_node_t(const the_graph_node_t & graph_node):
  registry_(graph_node.registry_),
  id_(graph_node.id_),
  direct_dependents_(graph_node.direct_dependents_),
  direct_supporters_(graph_node.direct_supporters_),
  regeneration_state_(graph_node.regeneration_state_),
  timestamp_visited_(graph_node.timestamp_visited_)
{}

//----------------------------------------------------------------
// the_graph_node_t::~the_graph_node_t
//
the_graph_node_t::~the_graph_node_t()
{}

//----------------------------------------------------------------
// the_graph_node_t::added_to_the_registry
//
void
the_graph_node_t::added_to_the_registry(the_registry_t * registry,
				       const unsigned int & id)
{
  // sanity check:
  assert(id_ == UINT_MAX);
  assert(direct_dependents_.size() == 0);

  registry_ = registry;
  id_ = id;
}

//----------------------------------------------------------------
// the_graph_node_t::removed_from_the_registry
//
void
the_graph_node_t::removed_from_the_registry()
{
  while (direct_supporters_.size() != 0)
  {
    breakdown_supporter_dependent(registry_,
				  direct_supporters_.back(),
				  id_);
  }

  while (direct_dependents_.size() != 0)
  {
    unsigned int dep_id = direct_dependents_.front();
    the_graph_node_t * dep = graph_node(dep_id);
    registry_->del(dep);
    delete dep;
  }

  registry_ = NULL;
  id_ = UINT_MAX;
}

//----------------------------------------------------------------
// the_graph_node_t::supports
//
// check wether this graph_node supports a given graph_node:
bool
the_graph_node_t::supports(const unsigned int & id) const
{
  // obviously, each graph_node supports itself ;-)
  if (id == id_) return true;

  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    const unsigned int & dep_id = *i;
    the_graph_node_t * p = graph_node(dep_id);
    if (p->supports(id)) return true;
  }

  return false;
}

//----------------------------------------------------------------
// the_graph_node_t::dependents
//
void
the_graph_node_t::dependents(std::list<unsigned int> & dependents) const
{
  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    dependents.push_back(*i);
    graph_node(*i)->dependents(dependents);
  }
}

//----------------------------------------------------------------
// the_graph_node_t::supporters
//
void
the_graph_node_t::supporters(std::list<unsigned int> & supporters) const
{
  for (std::list<unsigned int>::const_iterator i = direct_supporters_.begin();
       i != direct_supporters_.end(); ++i)
  {
    supporters.push_back(*i);
    graph_node(*i)->supporters(supporters);
  }
}

//----------------------------------------------------------------
// the_graph_node_t::verify_supporters_regenerated
//
bool
the_graph_node_t::verify_supporters_regenerated() const
{
  for (std::list<unsigned int>::const_iterator i = direct_supporters_.begin();
       i != direct_supporters_.end(); ++i)
  {
    unsigned int sup_id = *i;
    the_graph_node_t * sup = graph_node(sup_id);
    if (!sup->regenerated())
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// the_graph_node_t::verify_supporters_visited
//
bool
the_graph_node_t::verify_supporters_visited() const
{
  for (std::list<unsigned int>::const_iterator i = direct_supporters_.begin();
       i != direct_supporters_.end(); ++i)
  {
    unsigned int sup_id = *i;
    the_graph_node_t * sup = graph_node(sup_id);
    if (sup->timestamp_visited_ < the_graph_node_t::timestamp_)
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// the_graph_node_t::request_regeneration
//
void
the_graph_node_t::request_regeneration(the_graph_node_t * start_here)
{
  the_graph_node_t::timestamp_++;
  start_here->request_regeneration_helper();
}

//----------------------------------------------------------------
// the_graph_node_t::regenerate_recursive
//
bool
the_graph_node_t::regenerate_recursive(the_graph_node_t * start_here)
{
  the_graph_node_t::timestamp_++;
  // std::cout << "------------------- regenerate_recursive " << timestamp_
  //<< " -------------------" << std::endl;
  return start_here->regenerate_recursive_helper();
}

//----------------------------------------------------------------
// the_graph_node_t::regenerate_supporters
//
bool
the_graph_node_t::regenerate_supporters(the_graph_node_t * stop_here)
{
  the_graph_node_t::timestamp_++;
  // std::cout << "------------------- regenerate_supporters " << timestamp_
  //<< " -------------------" << std::endl;
  return stop_here->regenerate_supporters_helper();
}

//----------------------------------------------------------------
// the_graph_node_t::collect_nodes_sorted
//
void
the_graph_node_t::collect_nodes_sorted(the_graph_node_t * start_here,
				       std::list<unsigned int> & nodes)
{
  if (!start_here->direct_supporters().empty())
  {
    // this function only works if the starting node is a root node:
    assert(false);
    return;
  }

  the_graph_node_t::timestamp_++;
  start_here->collect_nodes_sorted_helper(nodes);
}

//----------------------------------------------------------------
// the_graph_node_t::request_regeneration_helper
//
void
the_graph_node_t::request_regeneration_helper()
{
  if (timestamp_visited_ >= the_graph_node_t::timestamp_)
  {
    // we've already visited this node:
    return;
  }

  timestamp_visited_ = the_graph_node_t::timestamp_;
  regeneration_state_ = REGENERATION_REQUESTED_E;

  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    unsigned int dep_id = *i;
    the_graph_node_t * dep = graph_node(dep_id);
    dep->request_regeneration_helper();
  }
}

//----------------------------------------------------------------
// the_graph_node_t::regenerate_recursive_helper
//
bool
the_graph_node_t::regenerate_recursive_helper()
{
  if (timestamp_visited_ >= the_graph_node_t::timestamp_)
  {
    // we've already visited this node:
    return true;
  }

  bool supporters_regenerated = verify_supporters_regenerated();
  if (!supporters_regenerated)
  {
    // can't do anything about this node until its supporters are regenerated:
    return true;
  }

  timestamp_visited_ = the_graph_node_t::timestamp_;
  if (!regenerated())
  {
    // std::cout << "regenerating: " << name() << ", " << id_;

    if (!regenerate())
    {
      // std::cout << ", failed..." << std::endl;

      // node failed to regenerate:
      regeneration_state_ = REGENERATION_FAILED_E;
      return false;
    }

    // std::cout << std::endl;

    // node regenerated successfully:
    regeneration_state_ = REGENERATION_SUCCEEDED_E;
  }

  bool all_ok = true;
  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    unsigned int dep_id = *i;
    the_graph_node_t * dep = graph_node(dep_id);
    bool ok = dep->regenerate_recursive_helper();
    all_ok = all_ok && ok;
  }

  return all_ok;
}

//----------------------------------------------------------------
// the_graph_node_t::regenerate_supporters_helper
//
bool
the_graph_node_t::regenerate_supporters_helper()
{
  if (timestamp_visited_ >= the_graph_node_t::timestamp_)
  {
    // we've already visited this node:
    return true;
  }

  bool all_ok = true;
  for (std::list<unsigned int>::const_iterator i = direct_supporters_.begin();
       i != direct_supporters_.end(); ++i)
  {
    unsigned int node_id = *i;
    the_graph_node_t * node = graph_node(node_id);
    if (!node->regenerated() &&
	!node->regenerate_supporters_helper())
    {
      // if any of the supporters fail to regenerate abort immediately:
      all_ok = false;
      break;
    }
  }

  // update the time stamp, even if the supporters failed to regenerate:
  timestamp_visited_ = the_graph_node_t::timestamp_;

  if (all_ok && !regenerated())
  {
    all_ok = regenerate();
    regeneration_state_ = (all_ok ?
			   REGENERATION_SUCCEEDED_E :
			   REGENERATION_FAILED_E);

    // FIXME:
    // std::cout << "regenerated " << name() << " " << id_ << ", "
    //	      << std::boolalpha << all_ok << endl;
  }

  return all_ok;
}

//----------------------------------------------------------------
// the_graph_node_t::collect_nodes_sorted_helper
//
void
the_graph_node_t::collect_nodes_sorted_helper(std::list<unsigned int> & nodes)
{
  if (timestamp_visited_ >= the_graph_node_t::timestamp_)
  {
    // we've already collected this node:
    return;
  }

  bool supporters_visited = verify_supporters_visited();
  if (!supporters_visited)
  {
    // can't collect this node until all its supporters are visited:
    return;
  }

  timestamp_visited_ = the_graph_node_t::timestamp_;
  nodes.push_back(id_);

  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    unsigned int dep_id = *i;
    the_graph_node_t * dep = graph_node(dep_id);
    dep->collect_nodes_sorted_helper(nodes);
  }
}

//----------------------------------------------------------------
// the_graph_node_t::save
//
bool
the_graph_node_t::save(std::ostream & stream) const
{
  ::save(stream, the_graph_node_t::id_);
  ::save(stream, the_graph_node_t::direct_dependents_);
  ::save(stream, the_graph_node_t::direct_supporters_);
  return true;
}

//----------------------------------------------------------------
// the_graph_node_t::load
//
bool
the_graph_node_t::load(std::istream & stream)
{
  registry_ = NULL;
  timestamp_visited_ = 0;
  regeneration_state_ = REGENERATION_REQUESTED_E;

  ::load(stream, the_graph_node_t::id_);
  ::load(stream, the_graph_node_t::direct_dependents_);
  ::load(stream, the_graph_node_t::direct_supporters_);
  return true;
}

//----------------------------------------------------------------
// the_graph_node_t::dump
//
void
the_graph_node_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_graph_node_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "name() = " << name() << endl
       << INDSTR << "registry_ = " << registry_ << endl
       << INDSTR << "id_ = " << id_ << endl
       << INDSTR << "direct_dependents_ = " << direct_dependents_ << endl
       << INDSTR << "direct_supporters_ = " << direct_supporters_ << endl
       << INDSTR << "timestamp_visited_ = " << timestamp_visited_ << endl
       << INDSTR << "regeneration_state_ = " << regeneration_state_ << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & s, const the_graph_node_t & p)
{
  p.dump(s);
  return s;
}

//----------------------------------------------------------------
// establish_supporter_dependent
//
void
establish_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id)
{
  the_graph_node_t * supporter = registry->elem(supporter_id);
  if (supporter == NULL) return;

  the_graph_node_t * dependent = registry->elem(dependent_id);
  if (dependent == NULL) return;

  supporter->add_dependent(dependent_id);
  dependent->add_supporter(supporter_id);
}

//----------------------------------------------------------------
// breakdown_supporter_dependent
//
void
breakdown_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id)
{
  the_graph_node_t * supporter = registry->elem(supporter_id);
  if (supporter == NULL) return;

  the_graph_node_t * dependent = registry->elem(dependent_id);
  if (dependent == NULL) return;

  supporter->del_dependent(dependent_id);
  dependent->del_supporter(supporter_id);
}

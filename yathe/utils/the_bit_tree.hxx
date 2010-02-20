// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_bit_tree.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2003/01/21 17:10
// Copyright    : (C) 2003
// License      : MIT
// Description  : A structure for order O(log(n)) lookup of
//                arbitrary memory pointers.

#ifndef THE_BIT_TREE_HXX_
#define THE_BIT_TREE_HXX_

// system includes:
#include <list>
#include <vector>
#include <stddef.h>
#include <assert.h>

// local includes:
#include "utils/the_indentation.hxx"

// uint64_t:
#ifdef _WIN32
#ifndef uint64_t
typedef unsigned __int64 uint64_t;
#endif
#else
#include <inttypes.h>
#endif


//----------------------------------------------------------------
// the_bit_tree_node_t
// 
class the_bit_tree_node_t
{
public:
  the_bit_tree_node_t(const bool & is_leaf):
    leaf(is_leaf)
  {}
  
  virtual ~the_bit_tree_node_t()
  {}
  
  // a flag indicating whether this node is a leaf node:
  bool leaf;
  
  // some global constants used when adding and looking up nodes:
  static const unsigned int node_size[];
  static const unsigned int node_mask[];
  
private:
  // disable default constructor:
  the_bit_tree_node_t();
};

//----------------------------------------------------------------
// the_bit_tree_leaf_t
// 
template<class T>
class the_bit_tree_leaf_t : public the_bit_tree_node_t
{
public:
  the_bit_tree_leaf_t():
    the_bit_tree_node_t(true)
  {}
  
  T elem;
};

//----------------------------------------------------------------
// the_bit_tree_branch_t
//
template<class T>
class the_bit_tree_branch_t : public the_bit_tree_node_t
{
public:
  the_bit_tree_branch_t(unsigned int bits_per_node):
    the_bit_tree_node_t(false),
    bits_(node_size[bits_per_node], NULL)
  {}
  
  ~the_bit_tree_branch_t()
  {
    for (unsigned int i = 0; i < bits_.size(); i++)
    {
      delete bits_[i];
      bits_[i] = NULL;
    }
  }
  
  the_bit_tree_leaf_t<T> *
  add(uint64_t addr, unsigned int offset, unsigned int bits_per_node)
  {
    unsigned int z = branch_id(addr, offset, bits_per_node);
    
    if (offset == 0)
    {
      if (bits_[z] == NULL) bits_[z] = new the_bit_tree_leaf_t<T>();
      return leaf(z);
    }
    
    if (bits_[z] == NULL)
    {
      bits_[z] = new the_bit_tree_branch_t<T>(bits_per_node);
    }
    
    return node(z)->add(addr, offset - bits_per_node, bits_per_node);
  }
  
  the_bit_tree_leaf_t<T> *
  get(uint64_t addr, unsigned int offset, unsigned int bits_per_node)
  {
    unsigned int z = branch_id(addr, offset, bits_per_node);
    if (bits_[z] == NULL) return NULL;
    if (offset == 0) return leaf(z);
    return node(z)->get(addr, offset - bits_per_node, bits_per_node);
  }
  
  // store leaf nodes of this tree in a list:
  void
  collect_leaf_nodes(std::list<the_bit_tree_leaf_t<T> *> & leaf_nodes)
  {
    for (unsigned int i = 0; i < bits_.size(); i++)
    {
      the_bit_tree_node_t * base = bits_[i];
      if (base == NULL) continue;
      
      if (base->leaf == false)
      {
	the_bit_tree_branch_t<T> * node =
	  static_cast<the_bit_tree_branch_t<T> *>(base);
	
	node->collect_leaf_nodes(leaf_nodes);
      }
      else
      {
	the_bit_tree_leaf_t<T> * leaf =
	  static_cast<the_bit_tree_leaf_t<T> *>(base);
	
	leaf_nodes.push_back(leaf);
      }
    }
  }
  
  // store leaf node contents of this tree in a list:
  void
  collect_leaf_contents(std::list<T> & leaf_contents) const
  {
    for (unsigned int i = 0; i < bits_.size(); i++)
    {
      const the_bit_tree_node_t * base = bits_[i];
      if (base == NULL) continue;
      
      if (base->leaf == false)
      {
	const the_bit_tree_branch_t<T> * node =
	  static_cast<const the_bit_tree_branch_t<T> *>(base);
	
	node->collect_leaf_contents(leaf_contents);
      }
      else
      {
	const the_bit_tree_leaf_t<T> * leaf =
	  static_cast<const the_bit_tree_leaf_t<T> *>(base);
	
	leaf_contents.push_back(leaf->elem);
      }
    }
  }
  
private:
  // helper function for looking up local branch id:
  inline unsigned int
  branch_id(uint64_t addr,
	    unsigned int offset,
	    unsigned int bits_per_node) const
  {
    uint64_t mask = uint64_t(node_mask[bits_per_node]) << offset;
    uint64_t z = (addr & mask) >> offset;
    return (unsigned int)(z);
  }
  
  // helper functions for distinguishing between tree nodes and leaves:
  inline the_bit_tree_branch_t<T> *
  node(unsigned int branch)
  {
    the_bit_tree_node_t * base = bits_[branch];
    if (base == NULL) return NULL;
    if (base->leaf) return NULL;
    return static_cast<the_bit_tree_branch_t<T> *>(base);
  }
  
  inline the_bit_tree_leaf_t<T> *
  leaf(unsigned int branch)
  {
    the_bit_tree_node_t * base = bits_[branch];
    if (base == NULL) return NULL;
    if (!base->leaf) return NULL;
    return static_cast<the_bit_tree_leaf_t<T> *>(base);
  }
  
  // datamembers:
  std::vector<the_bit_tree_node_t *> bits_;
};


//----------------------------------------------------------------
// the_bit_tree_t
// 
template<class T>
class the_bit_tree_t
{
public:
  the_bit_tree_t(unsigned int bits_per_addr = sizeof(uint64_t) * 8,
		 unsigned int bits_per_node = 4):
    root_(NULL),
    bits_per_addr_(bits_per_addr),
    bits_per_node_(bits_per_node),
    offset_(bits_per_addr - bits_per_node)
  { root_ = new the_bit_tree_branch_t<T>(bits_per_node_); }
  
  ~the_bit_tree_t()
  {
    delete root_;
    root_ = NULL;
  }
  
  // lookup a given address in the tree, return NULL if not found,
  // otherwise return the leaf node corresponding to that address:
  inline const the_bit_tree_leaf_t<T> * get(const void * addr) const
  { return get(size_t(addr)); }
  
  inline the_bit_tree_leaf_t<T> * get(const void * addr)
  { return get(size_t(addr)); }
  
  inline const the_bit_tree_leaf_t<T> * get(uint64_t addr) const 
  { return root_->get((uint64_t)addr, offset_, bits_per_node_); }
  
  inline the_bit_tree_leaf_t<T> * get(uint64_t addr)
  { return root_->get(addr, offset_, bits_per_node_); }
  
  // create a leaf node for a given address:
  inline the_bit_tree_leaf_t<T> * add(const void * addr)
  { return add(size_t(addr)); }
  
  inline the_bit_tree_leaf_t<T> * add(uint64_t addr)
  { return root_->add(addr, offset_, bits_per_node_); }
  
  // store leaf nodes of this tree in a list:
  inline void collect_leaf_nodes(std::list<the_bit_tree_leaf_t<T> *> & nodes)
  { root_->collect_leaf_nodes(nodes); }
  
  inline void collect_leaf_contents(std::list<T> & leaf_contents) const
  { root_->collect_leaf_contents(leaf_contents); }
  
private:
  the_bit_tree_branch_t<T> * root_;
  
  unsigned int bits_per_addr_;
  unsigned int bits_per_node_;
  unsigned int offset_;
};


#endif // THE_BIT_TREE_HXX_

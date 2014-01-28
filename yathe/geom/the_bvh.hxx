// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_bvh.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jun  7 20:41:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A bounding volume hierarchy class -- a raytracing
//                acceleration datastructure.

#ifndef THE_BVH_HXX_
#define THE_BVH_HXX_

// local includes:
#include "math/the_aa_bbox.hxx"
#include "math/v3x1p3x1.hxx"
#include "utils/the_utils.hxx"

// system includes:
#include <algorithm>
#include <list>


// helper function used to precalculate some constants to speed up
// repeated intersection calcualtions between a constant ray and
// various axis aligned bounding boxes:
inline void setup_fast_ray(const v3x1_t & d,
			   float inv_d[3],
			   unsigned int e[3])
{
  inv_d[0] = 1.0 / d[0];
  e[0] = inv_d[0] < 0.0;

  inv_d[1] = 1.0 / d[1];
  e[1] = inv_d[1] < 0.0;

  inv_d[2] = 1.0 / d[2];
  e[2] = inv_d[2] < 0.0;
}

//----------------------------------------------------------------
// the_bvh_node_t
//
// Base class for the Bounding Volume Hierarchy nodes.
//
template <class geom_t>
class the_bvh_node_t
{
public:
  the_bvh_node_t(const the_aa_bbox_t & bbox):
    bbox_(bbox)
  {}

  virtual ~the_bvh_node_t()
  {}

  // make a duplicate of this node:
  virtual the_bvh_node_t<geom_t> * clone() const = 0;

  // find the intersection between a given (fast) ray and the bounding
  // box of this node, return true if intersection exists, return
  // false otherwise:
  bool fast_bbox_ray_intersection(const float ray_o[3],
				  const float inv_d[3],
				  const unsigned int e[3],
				  float & t_min,
				  float & t_max) const
  {
    float t[3][2];

    t[0][e[0]]     = (bbox_.min_[0] - ray_o[0]) * inv_d[0];
    t[0][1 - e[0]] = (bbox_.max_[0] - ray_o[0]) * inv_d[0];

    t[1][e[1]]     = (bbox_.min_[1] - ray_o[1]) * inv_d[1];
    t[1][1 - e[1]] = (bbox_.max_[1] - ray_o[1]) * inv_d[1];

    t[2][e[2]]     = (bbox_.min_[2] - ray_o[2]) * inv_d[2];
    t[2][1 - e[2]] = (bbox_.max_[2] - ray_o[2]) * inv_d[2];

    t_min = std::max(t[0][0], std::max(t[1][0], t[2][0]));
    t_max = std::min(t[0][1], std::min(t[1][1], t[2][1]));

    return t_min <= t_max;
  }

  // find the intersection between a given (fast) ray and the bounding
  // box of this node, return true if intersection exists, return
  // false otherwise:
  inline bool fast_bbox_ray_intersection(const float ray_o[3],
					 const float inv_d[3],
					 const unsigned int e[3]) const
  {
    float t_min;
    float t_max;
    return fast_bbox_ray_intersection(ray_o, inv_d, e, t_min, t_max);
  }

  // build a list of geometry that may potentially be intersected
  // by the given ray:
  virtual void hit(const float ray_o[3],
		   const float inv_d[3],
		   const unsigned int e[3],
		   std::list<geom_t> & geom) const = 0;

  // collect all elements stored in the leaf nodes:
  virtual void elements(std::list<geom_t> & geom_list) const = 0;

  // accessor:
  inline const the_aa_bbox_t & bbox() const
  { return bbox_; }

private:
  // disable default member functions:
  the_bvh_node_t();

protected:
  // axis-aligned bounding box of the geometry contained within this node:
  const the_aa_bbox_t bbox_;
};


//----------------------------------------------------------------
// the_leaf_bvh_node_t
//
// A leaf node may contain more than one piece of geometry.
//
template <class geom_t>
class the_leaf_bvh_node_t : public the_bvh_node_t<geom_t>
{
public:
  the_leaf_bvh_node_t(const the_aa_bbox_t & bbox, const geom_t & geom):
    the_bvh_node_t<geom_t>(bbox),
    geom_(geom)
  {}

  // virtual:
  the_bvh_node_t<geom_t> * clone() const
  { return new the_leaf_bvh_node_t<geom_t>(*this); }

  // virtual:
  void hit(const float ray_o[3],
	   const float inv_d[3],
	   const unsigned int e[3],
	   std::list<geom_t> & geom_list) const
  {
    if (this->fast_bbox_ray_intersection(ray_o, inv_d, e))
    {
      geom_list.push_back(geom_);
    }
  }

  // virtual:
  void elements(std::list<geom_t> & geom_list) const
  { geom_list.push_back(geom_); }

private:
  // disable default member functions:
  the_leaf_bvh_node_t();
  the_leaf_bvh_node_t<geom_t> &
  operator = (const the_leaf_bvh_node_t<geom_t> & leaf);

  // the geometry contained within the bounding box of this node:
  geom_t geom_;
};


//----------------------------------------------------------------
// the_internal_bvh_node_t
//
template <class geom_t>
class the_internal_bvh_node_t : public the_bvh_node_t<geom_t>
{
public:
  the_internal_bvh_node_t(const the_aa_bbox_t & bbox,
			  std::list<geom_t> & geom_list):
    the_bvh_node_t<geom_t>(bbox)
  {
    node_[0] = NULL;
    node_[1] = NULL;

    std::list<geom_t> geom_left;
    std::list<geom_t> geom_right;
    the_aa_bbox_t bbox_left;
    the_aa_bbox_t bbox_right;

    split(geom_list,
	  geom_left,
	  geom_right,
	  bbox_left,
	  bbox_right);

    assert(!(bbox_left.is_empty() || bbox_right.is_empty()));

    if (is_size_one(geom_left))
    {
      node_[0] = new the_leaf_bvh_node_t<geom_t>(bbox_left,
						 *(geom_left.begin()));
    }
    else
    {
      node_[0] = new the_internal_bvh_node_t<geom_t>(bbox_left,
						     geom_left);
    }

    if (is_size_one(geom_right))
    {
      node_[1] =
	new the_leaf_bvh_node_t<geom_t>(bbox_right,
					*(geom_right.begin()));
    }
    else
    {
      node_[1] = new the_internal_bvh_node_t<geom_t>(bbox_right,
						     geom_right);
    }
  }

  the_internal_bvh_node_t(const the_internal_bvh_node_t<geom_t> & node):
    the_bvh_node_t<geom_t>(node)
  {
    node_[0] = node.node_[0]->clone();
    node_[1] = node.node_[1]->clone();
  }

  ~the_internal_bvh_node_t()
  {
    delete node_[0];
    node_[0] = NULL;

    delete node_[1];
    node_[1] = NULL;
  }

  // virtual:
  the_bvh_node_t<geom_t> * clone() const
  { return new the_internal_bvh_node_t<geom_t>(*this); }

  // virtual:
  void hit(const float ray_o[3],
	   const float inv_d[3],
	   const unsigned int e[3],
	   std::list<geom_t> & geom) const
  {
    if (this->fast_bbox_ray_intersection(ray_o, inv_d, e))
    {
      node_[0]->hit(ray_o, inv_d, e, geom);
      node_[1]->hit(ray_o, inv_d, e, geom);
    }
  }

  // virtual:
  void elements(std::list<geom_t> & geom_list) const
  {
    node_[0]->elements(geom_list);
    node_[1]->elements(geom_list);
  }

private:
  // disable default member functions:
  the_internal_bvh_node_t();
  the_internal_bvh_node_t<geom_t> &
  operator = (const the_internal_bvh_node_t<geom_t> & node);

  // split the geometry list into left/right:
  void split(std::list<geom_t> & geom_list,
	     std::list<geom_t> & geom_left,
	     std::list<geom_t> & geom_right,
	     the_aa_bbox_t & bbox_left,
	     the_aa_bbox_t & bbox_right)
  {
    // generate a list of bounding boxes of the geometry,
    // and calculate the bounding box of the centers of the geometry
    // bounding boxes:
    std::list<the_aa_bbox_t> bbox_list;
    the_aa_bbox_t bbox_of_bbox_centers;

    for (typename std::list<geom_t>::const_iterator i = geom_list.begin();
	 i != geom_list.end(); ++i)
    {
      the_aa_bbox_t bbox;
      calc_bbox(*i, bbox);

      bbox_list.push_back(bbox);
      bbox_of_bbox_centers << bbox.center();
    }

    if (bbox_of_bbox_centers.is_singular())
    {
      // the list can NOT be split optimally, simply move the first element
      // to the left, and the rest to the right:
      bbox_left = remove_head(bbox_list);
      geom_left.push_back(remove_head(geom_list));

      while (!geom_list.empty())
      {
	geom_right.push_back(remove_head(geom_list));
	bbox_right += remove_head(bbox_list);
      }

      return;
    }

    // the list can be split optimally:
    unsigned int axis_id = bbox_of_bbox_centers.largest_dimension();
    const float & min = bbox_of_bbox_centers.min_[axis_id];
    const float & max = bbox_of_bbox_centers.max_[axis_id];

    while (!geom_list.empty())
    {
      geom_t geom(remove_head(geom_list));
      the_aa_bbox_t bbox(remove_head(bbox_list));

      float d_min = bbox.center()[axis_id] - min;
      float d_max = max - bbox.center()[axis_id];
      if (d_min < d_max)
      {
	geom_left.push_back(geom);
	bbox_left += bbox;
      }
      else
      {
	geom_right.push_back(geom);
	bbox_right += bbox;
      }
    }
  }

  // pointers to sub-nodes contained within the volume of this node:
  the_bvh_node_t<geom_t> * node_[2];
};

//----------------------------------------------------------------
// the_bvh_t
//
template <class geom_t>
class the_bvh_t
{
public:
  the_bvh_t():
    root_(NULL)
  {}

  the_bvh_t(const the_aa_bbox_t & bbox,
	    const std::list<geom_t> & geom_list):
    root_(NULL)
  { setup(bbox, geom_list); }

  the_bvh_t(const the_bvh_t & bvh):
    root_(NULL)
  { *this = bvh; }

  ~the_bvh_t()
  { clear(); }

  the_bvh_t & operator = (const the_bvh_t & bvh)
  {
    if (&bvh != this)
    {
      clear();

      if (bvh.root_ != NULL)
      {
	root_ = bvh.root_->clone();
      }
    }

    return *this;
  }

  void clear()
  {
    delete root_;
    root_ = NULL;
  }

  // setup the hierarchy:
  void setup(const the_aa_bbox_t & bbox,
	     std::list<geom_t> & geom_list)
  {
    clear();

    if (is_size_one(geom_list))
    {
      root_ = new the_leaf_bvh_node_t<geom_t>(bbox, *(geom_list.begin()));
    }
    else if (!geom_list.empty())
    {
      root_ = new the_internal_bvh_node_t<geom_t>(bbox, geom_list);
    }
  }

  // build a list of geometry that may potentially be intersected
  // by the given ray:
  bool hit(const p3x1_t & o,
	   const v3x1_t & d,
	   std::list<geom_t> & geom_list) const
  {
    if (root_ == NULL) return false;

    float ray_o[3] = { o[0], o[1], o[2] };
    float inv_d[3];
    unsigned int e[3];
    setup_fast_ray(d, inv_d, e);

    root_->hit(ray_o, inv_d, e, geom_list);
    return !(geom_list.empty());
  }

  // collect all elements stored in the leaf nodes of this tree:
  inline void elements(std::list<geom_t> & geom_list) const
  {
    if (root_ == NULL) return;
    root_->elements(geom_list);
  }

  // accessor:
  inline const the_bvh_node_t<geom_t> * root() const
  { return root_; }

private:
  // the root of the bounding volume hierarchy:
  the_bvh_node_t<geom_t> * root_;
};


#endif // THE_BVH_HXX_

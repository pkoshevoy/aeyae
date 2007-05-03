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


// File         : the_bbox.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A bounding box aligned to a local coordinate system.

#ifndef THE_BBOX_HXX_
#define THE_BBOX_HXX_

// system includes:
#include <vector>

// local includes:
#include "math/the_aa_bbox.hxx"
#include "math/the_coord_sys.hxx"


// bounding box layout:
// 
//        C1-------E9------C5
//        /|              /|
//       / |             / |          Z
//     E0  |   F0      E4  |          |
//     /   e1      f1  /   |          |
//    /    |          /   E5          | reference coordinate system
//  C0-------E8------C4    |          |
//   |     |         |  F5 |          + - - - - Y
//   | f4  c2----e10-|-----C6        /
//   |    /          |    /         /
//  E3   /  F3       E7  /         X
//   |  e2      f2   |  E6
//   | /             | /   
//   |/              |/
//  C3------E11------C7
//  
//  Fi - face id,   fi - hidden face id.
//  Ei - edge id,   ei - hidden edge id.
//  Ci - corner id, ci - hidden corner id.
//  
//  The bounding box faces correspond to the fiew point orientation as follows:
//  F0 = top face
//  f1 = back face
//  f2 = bottom face
//  F3 = front face
//  f4 = right face
//  F5 = left face

//----------------------------------------------------------------
// the_bbox_face_id_t
// 
typedef enum
{
  THE_TOP_FACE_E,    // F0
  THE_BACK_FACE_E,   // f1
  THE_BOTTOM_FACE_E, // f2
  THE_FRONT_FACE_E,  // F3
  THE_RIGHT_FACE_E,  // f4
  THE_LEFT_FACE_E    // F5
} the_bbox_face_id_t;

//----------------------------------------------------------------
// the_bbox_t
// 
class the_bbox_t
{
public:
  the_bbox_t()
  { clear(); }
  
  the_bbox_t(const the_coord_sys_t & ref_cs):
    ref_cs_(ref_cs)
  { clear(); }
  
  // reset the bounding box to be empty:
  inline void clear()
  { aa_.clear(); }
  
  // expansion operator:
  inline the_bbox_t & operator << (const p3x1_t & wcs_pt)
  {
    p3x1_t lcs_pt = ref_cs_.to_lcs(wcs_pt);
    aa_ << lcs_pt;
    return *this;
  }
  
  // addition/expansion operators:
  the_bbox_t & operator += (const the_bbox_t & bbox);
  
  inline the_bbox_t operator + (const the_bbox_t & bbox) const
  {
    the_bbox_t ret_val(*this);
    return ret_val += bbox;
  }
  
  // scale operators:
  inline the_bbox_t & operator *= (const float & s)
  {
    aa_ *= s;
    return *this;
  }
  
  inline the_bbox_t operator * (const float & s) const
  {
    the_bbox_t result(*this);
    result *= s;
    return result;
  }
  
  // uniformly advance/retreat every face of the bounding box by value r:
  inline the_bbox_t & operator += (const float & r)
  {
    aa_ += r;
    return *this;
  }
  
  inline the_bbox_t operator + (const float & r) const
  {
    the_bbox_t result(*this);
    result += r;
    return result;
  }
  
  inline the_bbox_t operator - (const float & r) const
  { return (*this + (-r)); }
  
  inline the_bbox_t & operator -= (const float & r)
  { return (*this += (-r)); }
  
  // equality test operator:
  inline bool operator == (const the_bbox_t & bbox) const
  { return ((ref_cs_ == bbox.ref_cs_) && (aa_ == bbox.aa_)); }
  
  // return true if the volume of this bounding box is smaller than
  // the volume of the given bounding box:
  inline bool operator < (const the_bbox_t & bbox) const
  { return (wcs_volume() < bbox.wcs_volume()); }
  
  // calculate the volume of this bounding box:
  inline float wcs_volume() const
  {
    if (is_empty()) return 0.0;

    p3x1_t min = wcs_min();
    p3x1_t max = wcs_max();
    return (max[0] - min[0]) * (max[1] - min[1]) * (max[2] - min[2]);
  }
  
  // accessors to the internal axis aligned bounding box:
  inline const the_aa_bbox_t & aa() const { return aa_; }
  inline       the_aa_bbox_t & aa()       { return aa_; }
  
  // accessors to the local reference coordinate system:
  inline const the_coord_sys_t & ref_cs() const { return ref_cs_; }
  inline       the_coord_sys_t & ref_cs()       { return ref_cs_; }
  
  // get min/max points (expressed in the world coordinate system - wcs):
  inline const p3x1_t wcs_min() const { return ref_cs_.to_wcs(aa_.min_); }
  inline const p3x1_t wcs_max() const { return ref_cs_.to_wcs(aa_.max_); }
  
  // min/max points (expressed in local reference coordinate system - lcs):
  inline const p3x1_t & lcs_min() const { return aa_.min_; }
  inline const p3x1_t & lcs_max() const { return aa_.max_; }
  
  // lookup projection of minimum and maximum points onto a given axis:
  void wcs_min_max(unsigned int axis, p3x1_t & min, p3x1_t & max) const;
  
  // length of the bounding box edge along a given coordinate system axis:
  float wcs_length(unsigned int axis) const;
  
  // convert min/max into 8 bounding box corners (expressed in wcs),
  // the caller has to make sure that corner_array is of size 8:
  void wcs_corners(p3x1_t * wcs_corner_array) const;
  
  // bounding box validity tests:
  inline bool is_empty() const
  { return aa_.is_empty(); }
  
  inline bool is_singular() const
  { return aa_.is_singular(); }
  
  inline bool is_linear() const
  { return aa_.is_linear(); }
  
  inline bool is_planar() const
  { return aa_.is_planar(); }
  
  inline bool is_spacial() const
  { return aa_.is_spacial(); }
  
  // calculate the center of the bounding box:
  inline const p3x1_t wcs_center() const
  { return ref_cs_.to_wcs(aa_.center()); }
  
  // calculate the radius of the bounding box (sphere):
  float wcs_radius(const p3x1_t & wcs_center) const;
  
  inline float wcs_radius() const
  { return wcs_radius(wcs_center()); }
  
  // calculate the radius of the bounding box (cylinder):
  float wcs_radius(const p3x1_t & wcs_center, unsigned int axis_w_id) const;
  
  inline float wcs_radius(unsigned int axis_w_id) const
  { return wcs_radius(wcs_center(), axis_w_id); }
  
  // check whether a given point is contained between the faces of the
  // bounding box normal to the x, y, and z axis of the reference coordinate
  // system - if the point is contained within all three, the point is
  // inside the bounding box:
  inline void contains(const p3x1_t & wcs_pt,
		       bool & contained_in_x,
		       bool & contained_in_y,
		       bool & contained_in_z) const
  {
    p3x1_t lcs_pt = ref_cs_.to_lcs(wcs_pt);
    aa_.contains(lcs_pt,  contained_in_x, contained_in_y, contained_in_z);
  }
  
  // check whether the bounding box contains a given point:
  inline bool contains(const p3x1_t & wcs_pt) const
  {
    bool contained_in_x = false;
    bool contained_in_y = false;
    bool contained_in_z = false;
    contains(wcs_pt, contained_in_x, contained_in_y, contained_in_z);
    return (contained_in_x && contained_in_y && contained_in_z);
  }
  
  // check whether the bounding box contains another bounding box:
  inline bool contains(const the_bbox_t & bbox) const
  { return contains(bbox.wcs_min()) && contains(bbox.wcs_max()); }
  
  // check whether the bounding boxes intersect:
  bool intersects(const the_bbox_t & bbox) const;
  
  // For debugging, dumps this bounding box into a stream (indented):
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // bounding box expressed in the local (reference) coordinate system:
  the_aa_bbox_t aa_;
  
  // the coordinate system with respect to which the bounding
  // box will be computed:
  the_coord_sys_t ref_cs_;
};

//----------------------------------------------------------------
// operator *
// 
inline the_bbox_t
operator * (float s, const the_bbox_t & bbox)
{ return bbox * s; }

//----------------------------------------------------------------
// operator <<
// 
inline ostream &
operator << (ostream & strm, const the_bbox_t & bbox)
{
  bbox.dump(strm);
  return strm;
}


//----------------------------------------------------------------
// the_bbox_face_corner_id_t
// 
typedef enum
{
  THE_UL_CORNER_E,
  THE_UR_CORNER_E,
  THE_LR_CORNER_E,
  THE_LL_CORNER_E
} the_bbox_face_corner_id_t;

//----------------------------------------------------------------
// the_bbox_face_t
// 
class the_bbox_face_t
{
public:
  the_bbox_face_t(the_bbox_face_id_t face_id, const the_bbox_t & bbox);
  ~the_bbox_face_t() {}
  
  // accessor:
  inline const p3x1_t & operator [] (the_bbox_face_corner_id_t corner_id) const
  { return corner_[corner_id]; }
  
private:
  // disable default constructor:
  the_bbox_face_t();
  
  // corners of the bounding box face expressed in the world coordinate system:
  std::vector<p3x1_t> corner_;
};


#endif // THE_BBOX_HXX_

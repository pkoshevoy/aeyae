// File         : the_view_volume.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Dec 7 16:44:17 MDT 2003
// Copyright    : (C) 2003
// License      : GPL.
// Description  : Helper class used to evaluate the view volume.

#ifndef THE_VIEW_VOLUME_HXX_
#define THE_VIEW_VOLUME_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_ray.hxx"
#include "math/the_coord_sys.hxx"


/*
  View Volume diagram:
  
	(0, 0, 1)+---------------------+
	        /:                   / |
	       / :                 /   |
	     W/axis        far   /     |
	     /   :             /       |
     Origin /    :           /         |
  (0, 0, 0)+-----U-axis----+(1, 0, 0)  +
	   |    :          |         /
	   |   :           |       /
	  V|axis  NEAR     |     /
	   | :             |   /
	   |:              | /
  (0, 1, 0)+---------------+
  
  U axis: the width of the viewport, parameterized [0, 1].
  V axis: the height of the viewport, parameterized [0, 1].
  W axis: the vector from the upper left corner of the NEAR clipping
          face of the view volume, to the upper left corner of the FAR
	  clipping face, parameterized [0, 1].
  Origin: the upper left corner of the near clipping face.
  
  U and V axes can be assumed to be always orthogonal to each other, however
  the W axis will not be orthogonal to U or V during perspective viewing.
  
  View Volume near/far clipping face conventions:

  UL, 0           UR, 1
    +----U-axis-----+
    |               |
    |               |
   V|axis           |
    |               |
    |               |
    +---------------+
  LL, 3           LR, 2
  
  Origin = UL.
  U axis = UR - UL, parameterized [0, 1].
  V axis = LL - UL, parameterized [0, 1].
*/

//----------------------------------------------------------------
// the_view_volume_t
// 
class the_view_volume_t
{
public:
  // default constructor (unit cube):
  the_view_volume_t();
  
  // it is up to the caller to ensure that the near/far face corners are sane:
  the_view_volume_t(const p3x1_t * near_face, const p3x1_t * far_face)
  { setup(near_face, far_face); }
  
  // the simplest way to setup a viewing volume:
  the_view_volume_t(// look-from point, look-at point, and up vector
		    // are expressed in the world coordinate system,
		    // up vector is a unit vector:
		    const p3x1_t & wcs_lf,
		    const p3x1_t & wcs_la,
		    const v3x1_t & wcs_up,
		    
		    // near and far are parameters along unit vector
		    // from look-from point to look-at point:
		    const float & near_plane,
		    const float & far_plane,
		    
		    // dimensions of the near clipping face
		    // (expressed in the world coordinate system):
		    const float & wcs_width,
		    const float & wcs_height,
		    
		    // a flag indicating wether the viewing volume is
		    // orthogonal or perpsecive:
		    const bool & perspective);
  
  // copy constructor:
  the_view_volume_t(const the_view_volume_t & view_volume)
  { *this = view_volume; }
  
  // assignment operator:
  the_view_volume_t & operator = (const the_view_volume_t & view_volume);
  
  // it is up to the caller to ensure that the near/far face corners are sane:
  void setup(const p3x1_t * near_face, const p3x1_t * far_face);
  
  // an intuitive way to set up a view volume:
  void setup(// look-from point, look-at point, and up vector
	     // are expressed in the world coordinate system,
	     // up vector is a unit vector:
	     const p3x1_t & wcs_lf,
	     const p3x1_t & wcs_la,
	     const v3x1_t & wcs_up,
	     
	     // near and far are parameters along unit vector
	     // from look-from point to look-at point:
	     const float & near_plane,
	     const float & far_plane,
	     
	     // dimensions of the near clipping face
	     // (expressed in the world coordinate system):
	     const float & wcs_width,
	     const float & wcs_height,
	     
	     // a flag indicating wether the viewing volume is
	     // orthogonal or perpsecive:
	     const bool & perspective);
  
  // configure a given volume as a sub-volume of this volume (the boundaries
  // of the sub-volume are expressed as min/max UV bounding box corners):
  void sub_volume(const p2x1_t & scs_min,
		  const p2x1_t & scs_max,
		  the_view_volume_t & volume) const;
  
  // return a view volume representing a portion of this volume:
  inline const the_view_volume_t
  sub_volume(const p2x1_t & scs_min, const p2x1_t & scs_max) const
  {
    the_view_volume_t volume;
    sub_volume(scs_min, scs_max, volume);
    return volume;
  }
  
  // find the depth of a given point expressed in the world coordinate system:
  inline float depth_of_wcs_pt(const p3x1_t & wcs_pt) const
  {
    float w = ((w_axis_orthogonal_ * (wcs_pt - origin_[0])) /
	       (w_axis_orthogonal_ * w_axis_orthogonal_));
    return w;
  }
  
  // find the U,V coordinate system frame at a given depth:
  inline void uv_frame_at_depth(const float & depth, the_uv_csys_t & cs) const
  {
    cs.u_axis() = u_axis_[0] + depth * (u_axis_[1] - u_axis_[0]);
    cs.v_axis() = v_axis_[0] + depth * (v_axis_[1] - v_axis_[0]);
    cs.origin() = origin_[0] + depth * (origin_[1] - origin_[0]);
  }
  
  // transform a point expressed in the view volume coordinate
  // system to the world coordinate system:
  inline void uvw_to_wcs(const p3x1_t & uvw_pt, p3x1_t & wcs_pt) const
  {
    the_ray_t w_ray = to_ray(p2x1_t(uvw_pt.x(), uvw_pt.y()));
    wcs_pt = w_ray * uvw_pt.z();
  }
  
  // transform a point expressed in the world coordinate system
  // system to the view volume coordinate:
  inline void wcs_to_uvw(const p3x1_t & wcs_pt, p3x1_t & uvw_pt) const
  {
    // shortcuts:
    float & w = uvw_pt.z();
    float & u = uvw_pt.x();
    float & v = uvw_pt.y();
    
    w = depth_of_wcs_pt(wcs_pt);
    
    the_uv_csys_t uv_frame;
    uv_frame_at_depth(w, uv_frame);
    
    uv_frame.wcs_to_lcs(wcs_pt, u, v);
  }
  
  // transform a point expressed in the local cylindrical coordinate system
  // system to the world coordinate system:
  inline void cyl_to_wcs(const p3x1_t & cyl_pt, p3x1_t & wcs_pt) const
  {
    p3x1_t uvw_pt;
    the_coord_sys_t::cyl_to_xyz(cyl_pt, uvw_pt);
    
    // express the point in the world coordinate system:
    uvw_to_wcs(uvw_pt, wcs_pt);
  }
  
  // transform a point expressed in the world coordinate system
  // system to the view volume coordinate:
  inline void wcs_to_cyl(const p3x1_t & wcs_pt, p3x1_t & cyl_pt) const
  {
    p3x1_t uvw_pt;
    wcs_to_uvw(wcs_pt, uvw_pt);
    
    // express the point in the cylindrical coordinate system:
    the_coord_sys_t::xyz_to_cyl(uvw_pt, cyl_pt);
  }
  
  // express a view volume point in the world coordinate system:
  inline const p3x1_t to_wcs(const p3x1_t & uvw_pt) const
  {
    p3x1_t wcs_pt;
    uvw_to_wcs(uvw_pt, wcs_pt);
    return wcs_pt;
  }
  
  // express a world point in the view volume coordinate system:
  inline const p3x1_t to_uvw(const p3x1_t & wcs_pt) const
  {
    p3x1_t uvw_pt;
    wcs_to_uvw(wcs_pt, uvw_pt);
    return uvw_pt;
  }
  
  // transform a point expressed in the screen (near clipping face)
  // coordinate system to the world coordinate system:
  inline void scs_to_wcs(const p2x1_t & scs_pt, p3x1_t & wcs_pt) const
  {
    wcs_pt = (origin_[0] +
	      u_axis_[0] * scs_pt.x() +
	      v_axis_[0] * scs_pt.y());
  }
  
  // transform a point expressed in the world coordinate system
  // system to the screen (near clipping plane) coordinate system:
  inline void wcs_to_scs(const p3x1_t & wcs_pt, p2x1_t & scs_pt) const
  {
    p3x1_t uvw_pt;
    wcs_to_uvw(wcs_pt, uvw_pt);
    scs_pt.assign(uvw_pt.x(), uvw_pt.y());
  }
  
  // express a near clipping face point in the world coordinate system:
  inline const p3x1_t to_wcs(const p2x1_t & scs_pt) const
  {
    p3x1_t wcs_pt;
    scs_to_wcs(scs_pt, wcs_pt);
    return wcs_pt;
  }
  
  // express a world point in the near clipping face coordinate system:
  inline const p2x1_t to_scs(const p3x1_t & wcs_pt) const
  {
    p2x1_t scs_pt;
    wcs_to_scs(wcs_pt, scs_pt);
    return scs_pt;
  }
  
  // calculate the ray originating from some screen coordinate into the view
  // volume; the ray is parameterized from [0, 1], where 0 corresponds to the
  // near clipping face and 1 corrensponds to the far clipping face:
  void scs_pt_to_ray(const p2x1_t & scs_pt, the_ray_t & ray) const;
  
  inline const the_ray_t to_ray(const p2x1_t & scs_pt) const
  {
    the_ray_t ray;
    scs_pt_to_ray(scs_pt, ray);
    return ray;
  }
  
  // check whether a given point expressed in the view volume coordinate system
  // is contained by this view volume (within some tolerance):
  inline bool
  contains_uvw_pt(const p3x1_t & uvw_pt, float tolerance = 0.0) const
  {
    // shortcuts:
    const float & u = uvw_pt.x();
    const float & v = uvw_pt.y();
    const float & w = uvw_pt.z();
    
    return !((u < -tolerance) || ((u - 1.0) > tolerance) ||
	     (v < -tolerance) || ((v - 1.0) > tolerance) ||
	     (w < -tolerance) || ((w - 1.0) > tolerance));
  }
  
  // check whether a given point expressed in the world coordinate system
  // is contained by this view volume (within some tolerance):
  inline bool
  contains_wcs_pt(const p3x1_t & wcs_pt, float tolerance = 0.0) const
  { return contains_uvw_pt(to_uvw(wcs_pt), tolerance); }
  
  // For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // accessors:
  inline const p3x1_t * origin() const { return origin_; }
  inline const v3x1_t * u_axis() const { return u_axis_; }
  inline const v3x1_t * v_axis() const { return v_axis_; }
  
  inline const v3x1_t & w_axis_orthogonal() const
  { return w_axis_orthogonal_; }
  
protected:
  // origin and U,V axes of the near/far clipping faces (0 == near, 1 == far):
  p3x1_t origin_[2];
  v3x1_t u_axis_[2];
  v3x1_t v_axis_[2];
  
  // the projection of the W axis vector onto U,V axes cross product vector;
  // this vector is used to determine the depth of a given WCS point:
  v3x1_t w_axis_orthogonal_;
};

inline ostream &
operator << (ostream & stream, const the_view_volume_t & view_volume)
{
  view_volume.dump(stream);
  return stream;
}


#endif // THE_VIEW_VOLUME_HXX_

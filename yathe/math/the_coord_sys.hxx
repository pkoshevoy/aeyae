// File         : the_coord_sys.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  : 

#ifndef THE_COORD_SYS_HXX_
#define THE_COORD_SYS_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "utils/the_indentation.hxx"

// system includes:
#include <assert.h>

// forward declarations:
class m4x4_t;
class the_transform_t;

// shorthand, undefined at the end of the file:
#define x_axis_ axis_[0]
#define y_axis_ axis_[1]
#define z_axis_ axis_[2]


//----------------------------------------------------------------
// normalize
// 
inline float
normalize(const float & value, const float & norm)
{
  if (norm == 0.0) return 0.0;
  return value / norm;
}

//----------------------------------------------------------------
// the_uv_csys_t
// 
// 2D coordinate system:
// 
class the_uv_csys_t
{
public:
  // default constructor:
  the_uv_csys_t();
  
  // explicit constructor:
  the_uv_csys_t(const v3x1_t & u_axis,
		const v3x1_t & v_axis,
		const p3x1_t & origin);
  
  // destructor:
  virtual ~the_uv_csys_t() {}
  
  // accessors:
  inline const v3x1_t & u_axis() const { return u_axis_; }
  inline const v3x1_t & v_axis() const { return v_axis_; }
  inline const p3x1_t & origin() const { return origin_; }
  
  inline v3x1_t & u_axis() { return u_axis_; }
  inline v3x1_t & v_axis() { return v_axis_; }
  inline p3x1_t & origin() { return origin_; }
  
  // convert a point/vector expressed in local/world coordinate system 
  // to world/local coordinate system:
  virtual void lcs_to_wcs(const float & u,
			  const float & v,
			  p3x1_t & wcs_pt) const;
  
  virtual void wcs_to_lcs(const p3x1_t & wcs_pt,
			  float & u,
			  float & v) const;
  
  // transform a point expressed in the polar coordinate system
  // system to the cartesian coordinate system:
  inline static void polar_to_cartesian(const float & radius,
					const float & angle,
					float & u,
					float & v)
  {
    u = radius * cos(angle);
    v = radius * sin(angle);
  }
  
  // transform a point expressed in the cartesian coordinate system
  // system to the cylindrical coordinate system:
  inline static void cartesian_to_polar(const float & u,
					const float & v,
					float & radius,
					float & angle)
  {
    if (u == 0.0 && v == 0.0)
    {
      radius = 0.0;
      angle  = 0.0;
    }
    else
    {
      radius = sqrt(u * u + v * v);
      angle  = atan2(v, u);
    }
  }
  
  // For debugginge:
  void dump(ostream & stream, unsigned int indent = 0) const;
  
protected:
  v3x1_t u_axis_;
  v3x1_t v_axis_;
  p3x1_t origin_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & stream, const the_uv_csys_t & cs);


//----------------------------------------------------------------
// the_cyl_uv_csys_t
// 
// A cylindrical coordinate system.
// 
// NOTE: points expressed in the cylindrical coordinate system are
// triplets of radius, angle and height data in that order: (r, a, h)
// 
class the_cyl_uv_csys_t : public the_uv_csys_t
{
public:
  the_cyl_uv_csys_t();
  
  the_cyl_uv_csys_t(const v3x1_t & u_axis,
		    const v3x1_t & v_axis,
		    const p3x1_t & origin);
  
  // virtual: convert a point/vector expressed in local/world coordinate
  // system to world/local coordinate system:
  void lcs_to_wcs(const float & radius,
		  const float & angle,
		  p3x1_t & wcs_pt) const;
  
  void wcs_to_lcs(const p3x1_t & wcs_pt,
		  float & radius,
		  float & angle) const;
};


//----------------------------------------------------------------
// the_coord_sys_t
// 
class the_coord_sys_t
{
public:
  // default constructor:
  the_coord_sys_t();
  
  // explicit constructor:
  the_coord_sys_t(const v3x1_t & x_axis,
		  const v3x1_t & y_axis,
		  const v3x1_t & z_axis,
		  const p3x1_t & origin);
  
  // construct a coordinate system such that Z == normal:
  the_coord_sys_t(const p3x1_t & origin,
		  const v3x1_t & normal);
  
  // construct a coordinate system such that Z == normal and X == vup:
  the_coord_sys_t(const p3x1_t & origin,
		  const v3x1_t & normal,
		  const v3x1_t & vup);
  
  // construct a coordinate system such that Z is normal to the
  // plane defined by three points, origin will be at the first point:
  the_coord_sys_t(const p3x1_t & a,
		  const p3x1_t & b,
		  const p3x1_t & c);
  
  // construct a forward-transformed coordinate system:
  the_coord_sys_t(const the_coord_sys_t & ref_cs,
		  const the_transform_t & transform);
  
  virtual ~the_coord_sys_t() {}
  
  // equality test operator:
  bool operator == (const the_coord_sys_t & cs) const;
  
  // scale the coordinate system axis:
  the_coord_sys_t & operator *= (float scale);
  
  // return a scaled version of this coordinate system:
  const the_coord_sys_t operator * (float scale) const;
  
  // translate the coordinate system origin by a given offset vector:
  the_coord_sys_t & operator += (const v3x1_t & offset);
  the_coord_sys_t & operator -= (const v3x1_t & offset);
  
  // return a copy of this coordinate system translated by a given offset:
  const the_coord_sys_t operator + (const v3x1_t & offset) const;
  const the_coord_sys_t operator - (const v3x1_t & offset) const;
  
  // change this coordiante system by rotating it around a given axis
  // by a specified angle:
  void rotate(const v3x1_t & axis, const float & angle);
  
  // const accessors:
  inline const v3x1_t & x_axis() const { return x_axis_; }
  inline const v3x1_t & y_axis() const { return y_axis_; }
  inline const v3x1_t & z_axis() const { return z_axis_; }
  inline const p3x1_t & origin() const { return origin_; }
  
  // const accessor to the axis by axis index (0 == X, 1 == Y, 2 == Z):
  inline const v3x1_t & operator [] (const unsigned int & axis_idx) const
  { return axis_[axis_idx]; }
  
  // non-const accessors:
  inline v3x1_t & x_axis() { return x_axis_; }
  inline v3x1_t & y_axis() { return y_axis_; }
  inline v3x1_t & z_axis() { return z_axis_; }
  inline p3x1_t & origin() { return origin_; }
  
  // non-const accessor to the axis by axis index (0 == X, 1 == Y, 2 == Z):
  inline v3x1_t & operator [] (const unsigned int & axis_idx)
  { return axis_[axis_idx]; }
  
  // convert a point/vector expressed in local/world coordinate system 
  // to world/local coordinate system:
  virtual void lcs_to_wcs(const p3x1_t & lcs_pt, p3x1_t & wcs_pt) const;
  virtual void wcs_to_lcs(const p3x1_t & wcs_pt, p3x1_t & lcs_pt) const;
  
  void lcs_to_wcs(const v3x1_t & lcs_vec, v3x1_t & wcs_vec) const;
  void wcs_to_lcs(const v3x1_t & wcs_vec, v3x1_t & lcs_vec) const;
  
  // transform a point expressed in the cylindrical coordinate system
  // system to the cartesian coordinate system:
  template <class pv3x1_t>
  inline static void cyl_to_xyz(const pv3x1_t & cyl, pv3x1_t & xyz)
  {
    // shortcuts:
    const float & radius = cyl.x();
    const float & angle  = cyl.y();
    const float & height = cyl.z();
    
    float & x = xyz.x();
    float & y = xyz.y();
    float & z = xyz.z();
    
    the_uv_csys_t::polar_to_cartesian(radius, angle, x, y);
    z = height;
  }
  
  // transform a point expressed in the cartesian coordinate system
  // system to the cylindrical coordinate system:
  template <class pv3x1_t>
  inline static void xyz_to_cyl(const pv3x1_t & xyz, pv3x1_t & cyl)
  {
    // shortcuts:
    const float & x = xyz.x();
    const float & y = xyz.y();
    const float & z = xyz.z();
    
    float & radius = cyl.x();
    float & angle  = cyl.y();
    float & height = cyl.z();
    
    the_uv_csys_t::cartesian_to_polar(x, y, radius, angle);
    height = z;
  }
  
  // transform a point expressed in the spherical coordinate system
  // system to the cartesian coordinate system:
  template <class pv3x1_t>
  inline static void sph_to_xyz(const pv3x1_t & sph, pv3x1_t & xyz)
  {
    // shortcuts:
    const float & radius  = sph.x();
    const float & azimuth = sph.y(); // longitude
    const float & polar   = sph.z(); // colatitude
    
    float & x = xyz.x();
    float & y = xyz.y();
    float & z = xyz.z();
    
    x = radius * sin(polar) * cos(azimuth);
    y = radius * sin(polar) * sin(azimuth);
    z = radius * cos(polar);
  }
  
  // transform a point expressed in the cartesian coordinate system
  // system to the spherical coordinate system:
  template <class pv3x1_t>
  inline static void xyz_to_sph(const pv3x1_t & xyz, pv3x1_t & sph)
  {
    // shortcuts:
    const float & x = xyz.x();
    const float & y = xyz.y();
    const float & z = xyz.z();
    
    float & radius  = sph.x();
    float & azimuth = sph.y(); // longitude
    float & polar   = sph.z(); // colatitude
    
    radius  = sqrt(x * x + y * y + z * z);
    azimuth = atan2(y, x);
    polar   = atan2(sqrt(x * x + y * y), z);
  }
  
  // convenience functions:
  inline const p3x1_t to_wcs(const p3x1_t & lcs_pt) const
  { p3x1_t wcs_pt; lcs_to_wcs(lcs_pt, wcs_pt); return wcs_pt; }
  
  inline const p3x1_t to_lcs(const p3x1_t & wcs_pt) const
  { p3x1_t lcs_pt; wcs_to_lcs(wcs_pt, lcs_pt); return lcs_pt; }
  
  inline const v3x1_t to_wcs(const v3x1_t & lcs_vec) const
  { v3x1_t wcs_vec; lcs_to_wcs(lcs_vec, wcs_vec); return wcs_vec; }
  
  inline const v3x1_t to_lcs(const v3x1_t & wcs_vec) const
  { v3x1_t lcs_vec; wcs_to_lcs(wcs_vec, lcs_vec); return lcs_vec; }
  
  void get_transform(// the axis scales:
		     float scale[3],
		     
		     // the translation vector:
		     float origin[3],
		     
		     // cylindrical coordinates of the rotation axis:
		     float & azimuth, // longitude
		     float & polar,   // colatitude
		     
		     // the rotation angle:
		     float & alpha) const;
  
  void get_transform(float & x_scale,
		     float & y_scale,
		     float & z_scale,
		     float x_unit[3],
		     float y_unit[3],
		     float z_unit[3],
		     float origin[3]) const;
  
  // setup a matrix that would transform points and vectors expressed
  // in the world coordinate system to the local coordinate system:
  void wcs_to_lcs(m4x4_t & transform) const;
  
  // setup a matrix that would transform points and vectors expressed
  // in the local coordinate system to the world coordinate system:
  void lcs_to_wcs(m4x4_t & transform) const;
  
  // This coordinate system can also be viewed as a transform,
  // these functions are here to help put the transform information
  // into concatenated matrices (mother-of-all-matrices) format.
  // The mom matrix will transform from local coordinate system
  // to the world coordinate system.
  // The mom_inverse will transfrom from world coordinate system
  // to the local coordinate system.
  void get_mom(m4x4_t & mom, m4x4_t & mom_inverse) const;
  
  // For debugginge:
  void dump(ostream & stream, unsigned int indent = 0) const;
  
protected:
  // the x,y,z axes of this coordinate system:
  v3x1_t axis_[3];
  
  // origin of this coordinate system:
  p3x1_t origin_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & stream, const the_coord_sys_t & cs);


//----------------------------------------------------------------
// the_cyl_coord_sys_t
// 
// A cylindrical coordinate system.
// 
// NOTE: points expressed in the cylindrical coordinate system are
// triplets of radius, angle and height data in that order: (r, a, h)
// 
class the_cyl_coord_sys_t : public the_coord_sys_t
{
public:
  the_cyl_coord_sys_t();
  
  the_cyl_coord_sys_t(const v3x1_t & x_axis,
		      const v3x1_t & y_axis,
		      const v3x1_t & z_axis,
		      const p3x1_t & origin);
  
  // virtual: convert a point/vector expressed in local/world coordinate
  // system to world/local coordinate system:
  void lcs_to_wcs(const p3x1_t & cyl_pt, p3x1_t & wcs_pt) const;
  void wcs_to_lcs(const p3x1_t & wcs_pt, p3x1_t & cyl_pt) const;
};

// undefine shorthand:
#undef x_axis_
#undef y_axis_
#undef z_axis_


#endif // THE_COORD_SYS_HXX_

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


// File         : the_coord_sys.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  6 12:10:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Several coordinate system classes.

// local includes:
#include "math/the_coord_sys.hxx"
#include "math/m4x4.hxx"
#include "math/the_transform.hxx"

// system includes:
#include <algorithm>

// shorthand, undefined at the end of the file:
#define x_axis_ axis_[0]
#define y_axis_ axis_[1]
#define z_axis_ axis_[2]


//----------------------------------------------------------------
// the_uv_csys_t::the_uv_csys_t
// 
the_uv_csys_t::the_uv_csys_t():
  u_axis_(1.0, 0.0, 0.0),
  v_axis_(0.0, 1.0, 0.0),
  origin_(0.0, 0.0, 0.0)
{}

//----------------------------------------------------------------
// the_uv_csys_t::the_uv_csys_t
// 
the_uv_csys_t::the_uv_csys_t(const v3x1_t & u_axis,
			     const v3x1_t & v_axis,
			     const p3x1_t & origin):
  u_axis_(u_axis),
  v_axis_(v_axis),
  origin_(origin)
{}

//----------------------------------------------------------------
// the_uv_csys_t::lcs_to_wcs
// 
void
the_uv_csys_t::lcs_to_wcs(const float & u,
			  const float & v,
			  p3x1_t & wcs_pt) const
{
  // express the point in the world coordinate system:
  wcs_pt = origin_ + u_axis_ * u + v_axis_ * v;
}

//----------------------------------------------------------------
// the_uv_csys_t::wcs_to_lcs
// 
void
the_uv_csys_t::wcs_to_lcs(const p3x1_t & wcs_pt,
			  float & u,
			  float & v) const
{
  // express the point in the local coordinate system:
  v3x1_t wcs_vec = wcs_pt - origin_;
  u = (u_axis_ * wcs_vec) / (u_axis_ * u_axis_);
  v = (v_axis_ * wcs_vec) / (v_axis_ * v_axis_);
}

//----------------------------------------------------------------
// the_uv_csys_t::dump
// 
void
the_uv_csys_t::dump(ostream & stream, unsigned int indent) const
{
  stream << INDSCP << "the_uv_csys_t(" << (void *)this << ")" << endl
	 << INDSCP << "{" << endl
	 << INDSTR << "u_axis_ = " << u_axis_ << endl
	 << INDSTR << "v_axis_ = " << v_axis_ << endl
	 << INDSTR << "origin_ = " << origin_ << endl
	 << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & stream, const the_uv_csys_t & cs)
{
  cs.dump(stream);
  return stream;
}


//----------------------------------------------------------------
// the_cyl_uv_csys_t::the_cyl_uv_csys_t
// 
the_cyl_uv_csys_t::the_cyl_uv_csys_t():
  the_uv_csys_t()
{}

//----------------------------------------------------------------
// the_cyl_uv_csys_t::the_cyl_uv_csys_t
// 
the_cyl_uv_csys_t::the_cyl_uv_csys_t(const v3x1_t & u_axis,
				     const v3x1_t & v_axis,
				     const p3x1_t & origin):
  the_uv_csys_t(u_axis, v_axis, origin)
{}

//----------------------------------------------------------------
// the_cyl_uv_csys_t::lcs_to_wcs
// 
void
the_cyl_uv_csys_t::lcs_to_wcs(const float & radius,
			      const float & angle,
			      p3x1_t & wcs_pt) const
{
  // express the point in the local coordinate system:
  float u;
  float v;
  the_uv_csys_t::polar_to_cartesian(radius, angle, u, v);
  
  // express the point in the world coordinate system:
  the_uv_csys_t::lcs_to_wcs(u, v, wcs_pt);
}

//----------------------------------------------------------------
// the_cyl_uv_csys_t::wcs_to_lcs
// 
void
the_cyl_uv_csys_t::wcs_to_lcs(const p3x1_t & wcs_pt,
			      float & radius,
			      float & angle) const
{
  // express the point in the local coordinate system:
  float u;
  float v;
  the_uv_csys_t::wcs_to_lcs(wcs_pt, u, v);
  
  // express the point in the cylindrical coordinate system:
  the_uv_csys_t::cartesian_to_polar(u, v, radius, angle);
}


//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t():
  origin_(0.0, 0.0, 0.0)
{
  x_axis_.assign(1.0, 0.0, 0.0);
  y_axis_.assign(0.0, 1.0, 0.0);
  z_axis_.assign(0.0, 0.0, 1.0);
}

//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t(const v3x1_t & x_axis,
				 const v3x1_t & y_axis,
				 const v3x1_t & z_axis,
				 const p3x1_t & origin):
  origin_(origin)
{
  x_axis_ = x_axis;
  y_axis_ = y_axis;
  z_axis_ = z_axis;
}

//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t(const p3x1_t & origin,
				 const v3x1_t & normal):
  origin_(origin)
{
  z_axis_ = normal;
  
  v3x1_t r_vec = normal.normal();
  x_axis_ = !(z_axis_ % r_vec);
  y_axis_ = !(z_axis_ % x_axis_);
}

//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t(const p3x1_t & origin,
				 const v3x1_t & normal,
				 const v3x1_t & vup):
  origin_(origin)
{
  z_axis_ = !normal;
  
  y_axis_ = !(z_axis_ % !vup);
  x_axis_ = !(y_axis_ % z_axis_);
}

//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t(const p3x1_t & a,
				 const p3x1_t & b,
				 const p3x1_t & c):
  origin_(a)
{
  x_axis_ = !(b - a);
  y_axis_ = !(c - a);
  z_axis_ = !(x_axis_ % y_axis_);
  y_axis_ = !(z_axis_ % x_axis_);
}

//----------------------------------------------------------------
// the_coord_sys_t::the_coord_sys_t
// 
the_coord_sys_t::the_coord_sys_t(const the_coord_sys_t & ref_cs,
				 const the_transform_t & transform)
{
  transform.eval(ref_cs, *this);
}

//----------------------------------------------------------------
// the_coord_sys_t::operator ==
// 
bool
the_coord_sys_t::operator == (const the_coord_sys_t & cs) const
{
  return ((x_axis_ == cs.x_axis_) &&
	  (y_axis_ == cs.y_axis_) &&
	  (z_axis_ == cs.z_axis_) &&
	  (origin_ == cs.origin_));
}

//----------------------------------------------------------------
// the_coord_sys_t::operator *=
// 
the_coord_sys_t &
the_coord_sys_t::operator *= (float scale)
{
  x_axis_ *= scale;
  y_axis_ *= scale;
  z_axis_ *= scale;
  return *this;
}

//----------------------------------------------------------------
// the_coord_sys_t::operator *
// 
const the_coord_sys_t
the_coord_sys_t::operator * (float scale) const
{
  the_coord_sys_t scaled_cs(*this);
  scaled_cs *= scale;
  return scaled_cs;
}

//----------------------------------------------------------------
// the_coord_sys_t::operator +=
// 
the_coord_sys_t &
the_coord_sys_t::operator += (const v3x1_t & offset)
{
  origin_ += offset;
  return *this;
}

//----------------------------------------------------------------
// the_coord_sys_t::operator -=
// 
the_coord_sys_t &
the_coord_sys_t::operator -= (const v3x1_t & offset)
{
  origin_ -= offset;
  return *this;
}

//----------------------------------------------------------------
// the_coord_sys_t::operator +
// 
const the_coord_sys_t
the_coord_sys_t::operator + (const v3x1_t & offset) const
{
  the_coord_sys_t offset_cs(*this);
  offset_cs += offset;
  return offset_cs;
}

//----------------------------------------------------------------
// the_coord_sys_t::operator -
// 
const the_coord_sys_t
the_coord_sys_t::operator - (const v3x1_t & offset) const
{
  the_coord_sys_t offset_cs(*this);
  offset_cs -= offset;
  return offset_cs;
}

//----------------------------------------------------------------
// the_coord_sys_t::rotate
// 
void
the_coord_sys_t::rotate(const v3x1_t & axis, const float & angle)
{
  const m4x4_t r(!axis, angle);
  
  x_axis_ = r * x_axis_;
  y_axis_ = r * y_axis_;
  z_axis_ = r * z_axis_;
}

//----------------------------------------------------------------
// the_coord_sys_t::lcs_to_wcs
// 
void
the_coord_sys_t::lcs_to_wcs(const p3x1_t & lcs_pt, p3x1_t & wcs_pt) const
{
  // shortcuts:
  const float & x = lcs_pt.x();
  const float & y = lcs_pt.y();
  const float & z = lcs_pt.z();
  
  // express the point in the world coordinate system:
  wcs_pt = (origin_ + x_axis_ * x + y_axis_ * y + z_axis_ * z);
}

//----------------------------------------------------------------
// the_coord_sys_t::wcs_to_lcs
// 
void
the_coord_sys_t::wcs_to_lcs(const p3x1_t & wcs_pt, p3x1_t & lcs_pt) const
{
  // shortcuts:
  float & x = lcs_pt.x();
  float & y = lcs_pt.y();
  float & z = lcs_pt.z();
  
  // express the point in the local coordinate system:
  v3x1_t wcs_vec = wcs_pt - origin_;
  x = (x_axis_ * wcs_vec) / (x_axis_ * x_axis_);
  y = (y_axis_ * wcs_vec) / (y_axis_ * y_axis_);
  z = (z_axis_ * wcs_vec) / (z_axis_ * z_axis_);
}

//----------------------------------------------------------------
// the_coord_sys_t::lcs_to_wcs
// 
void
the_coord_sys_t::lcs_to_wcs(const v3x1_t & lcs_vec, v3x1_t & wcs_vec) const
{
  p3x1_t lcs_pt(lcs_vec.data());
  p3x1_t wcs_pt;
  lcs_to_wcs(lcs_pt, wcs_pt);
  wcs_vec = wcs_pt - origin();
}

//----------------------------------------------------------------
// the_coord_sys_t::wcs_to_lcs
// 
void
the_coord_sys_t::wcs_to_lcs(const v3x1_t & wcs_vec, v3x1_t & lcs_vec) const
{
  p3x1_t wcs_pt = origin() + wcs_vec;
  p3x1_t lcs_pt;
  wcs_to_lcs(wcs_pt, lcs_pt);
  lcs_vec.assign(lcs_pt.data());
}

//----------------------------------------------------------------
// the_coord_sys_t::get_transform
// 
void
the_coord_sys_t::get_transform(// the axis scales:
			       float scale[3],
			       
			       // the translation vector:
			       float origin[3],
			       
			       // spherical coordinates of the rotation axis:
			       float & azimuth, // longitude
			       float & polar,   // colatitude
			       
			       // the rotation angle:
			       float & alpha) const
{
  static const float angle_eps = 1e-7;
  
  scale[0] = x_axis_.norm();
  scale[1] = y_axis_.norm();
  scale[2] = z_axis_.norm();
  
  origin[0] = origin_.x();
  origin[1] = origin_.y();
  origin[2] = origin_.z();
  
  float r[3][3] =
  {
    {
      normalize(x_axis_.x(), scale[0]),
      normalize(y_axis_.x(), scale[1]),
      normalize(z_axis_.x(), scale[2])
    },
    
    {
      normalize(x_axis_.y(), scale[0]),
      normalize(y_axis_.y(), scale[1]),
      normalize(z_axis_.y(), scale[2])
    },
    
    {
      normalize(x_axis_.z(), scale[0]),
      normalize(y_axis_.z(), scale[1]),
      normalize(z_axis_.z(), scale[2])
    }
  };
  
  float trace_r = r[0][0] + r[1][1] + r[2][2];
  float cos_alpha = (trace_r - 1.0) / 2.0;
  
  float r3223 = r[2][1] - r[1][2];
  float r1331 = r[0][2] - r[2][0];
  float r2112 = r[1][0] - r[0][1];
  float sin_alpha = 0.5 * sqrt(r3223 * r3223 + r1331 * r1331 + r2112 * r2112);
  
  alpha = atan2(sin_alpha, cos_alpha);
  
  if (fabs(alpha) < angle_eps)
  {
    alpha = 0.0;
    azimuth = 0.0;
    polar = 0.0;
  }
  else
  {
    v3x1_t rot_axis(r3223, r1331, r2112);
    rot_axis /= (2.0 * sin_alpha);
    
    v3x1_t sph_axis;
    xyz_to_sph(rot_axis, sph_axis);
    
    polar = std::min(M_PI, std::max(0.0, double(sph_axis.z())));
    if (polar < angle_eps || fabs(polar - M_PI) < angle_eps)
    {
      azimuth = 0.0;
    }
    else
    {
      azimuth = sph_axis.y();
    }
  }
}

//----------------------------------------------------------------
// normalize
// 
static float
normalize(const float a[3], float b[3])
{
  b[0] = a[0];
  b[1] = a[1];
  b[2] = a[2];
  
  float norm = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  if (norm != 0.0)
  {
    float scale = 1.0 / norm;
    b[0] *= scale;
    b[1] *= scale;
    b[2] *= scale;
  }
  
  return norm;
}

//----------------------------------------------------------------
// the_coord_sys_t::get_transform
// 
void
the_coord_sys_t::get_transform(float & x_scale,
			       float & y_scale,
			       float & z_scale,
			       float x_unit[3],
			       float y_unit[3],
			       float z_unit[3],
			       float origin[3]) const
{
  x_scale = normalize(x_axis_.data(), x_unit);
  y_scale = normalize(y_axis_.data(), y_unit);
  z_scale = normalize(z_axis_.data(), z_unit);
  
  origin[0] = origin_.x();
  origin[1] = origin_.y();
  origin[2] = origin_.z();
}

//----------------------------------------------------------------
// the_coord_sys_t::wcs_to_lcs
// 
void
the_coord_sys_t::wcs_to_lcs(m4x4_t & transform) const
{
  float x_scale;
  float y_scale;
  float z_scale;
  float x_unit[3];
  float y_unit[3];
  float z_unit[3];
  float origin[3];
  get_transform(x_scale, y_scale, z_scale, x_unit, y_unit, z_unit, origin);
  
  transform  = m4x4_t(normalize(1.0, x_scale),
		      normalize(1.0, y_scale),
		      normalize(1.0, z_scale));
  transform *= m4x4_t(x_unit[0], x_unit[1], x_unit[2],
		      y_unit[0], y_unit[1], y_unit[2],
		      z_unit[0], z_unit[1], z_unit[2]);
  transform *= m4x4_t(v3x1_t(-origin[0], -origin[1], -origin[2]));
}

//----------------------------------------------------------------
// the_coord_sys_t::lcs_to_wcs
// 
void
the_coord_sys_t::lcs_to_wcs(m4x4_t & transform) const
{
  float x_scale;
  float y_scale;
  float z_scale;
  float x_unit[3];
  float y_unit[3];
  float z_unit[3];
  float origin[3];
  get_transform(x_scale, y_scale, z_scale, x_unit, y_unit, z_unit, origin);
  
  transform  = m4x4_t(v3x1_t(origin[0], origin[1], origin[2]));
  transform *= m4x4_t(x_unit[0], y_unit[0], z_unit[0],
		      x_unit[1], y_unit[1], z_unit[1],
		      x_unit[2], y_unit[2], z_unit[2]);
  transform *= m4x4_t(x_scale, y_scale, z_scale);
}

//----------------------------------------------------------------
// the_coord_sys_t::get_mom
// 
void
the_coord_sys_t::get_mom(m4x4_t & mom, m4x4_t & mom_inverse) const
{
  float x_unit[3];
  float y_unit[3];
  float z_unit[3];
  
  float x_scale = normalize(x_axis_.data(), x_unit);
  float y_scale = normalize(y_axis_.data(), y_unit);
  float z_scale = normalize(z_axis_.data(), z_unit);
  
  m4x4_t m_scale(x_scale,
		 y_scale,
		 z_scale);
  m4x4_t m_s_inv(normalize(1.0, x_scale),
		 normalize(1.0, y_scale),
		 normalize(1.0, z_scale));
  
  m4x4_t m_orientation(x_unit[0], y_unit[0], z_unit[0],
		       x_unit[1], y_unit[1], z_unit[1],
		       x_unit[2], y_unit[2], z_unit[2]);
  m4x4_t m_orienta_inv(x_unit[0], x_unit[1], x_unit[2],
		       y_unit[0], y_unit[1], y_unit[2],
		       z_unit[0], z_unit[1], z_unit[2]);
  
  m4x4_t m_translation(v3x1_t(origin_.x(),
			      origin_.y(),
			      origin_.z()));
  m4x4_t m_transla_inv(v3x1_t(-origin_.x(),
			      -origin_.y(),
			      -origin_.z()));
  
  mom         = m_translation * m_orientation * m_scale;
  mom_inverse = m_s_inv * m_orienta_inv * m_transla_inv;
}

//----------------------------------------------------------------
// the_coord_sys_t::dump
// 
void
the_coord_sys_t::dump(ostream & stream, unsigned int indent) const
{
  stream << INDSCP << "the_coord_sys_t(" << (void *)this << ")" << endl
	 << INDSCP << "{" << endl
	 << INDSTR << "x_axis_ = " << x_axis_ << endl
	 << INDSTR << "y_axis_ = " << y_axis_ << endl
	 << INDSTR << "z_axis_ = " << z_axis_ << endl
	 << INDSTR << "origin_ = " << origin_ << endl
	 << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & stream, const the_coord_sys_t & cs)
{
  cs.dump(stream);
  return stream;
}


//----------------------------------------------------------------
// the_cyl_coord_sys_t::the_cyl_coord_sys_t
// 
the_cyl_coord_sys_t::the_cyl_coord_sys_t():
  the_coord_sys_t()
{}

//----------------------------------------------------------------
// the_cyl_coord_sys_t::the_cyl_coord_sys_t
// 
the_cyl_coord_sys_t::the_cyl_coord_sys_t(const v3x1_t & x_axis,
					 const v3x1_t & y_axis,
					 const v3x1_t & z_axis,
					 const p3x1_t & origin):
  the_coord_sys_t(x_axis, y_axis, z_axis, origin)
{}

//----------------------------------------------------------------
// the_cyl_coord_sys_t::lcs_to_wcs
// 
void
the_cyl_coord_sys_t::lcs_to_wcs(const p3x1_t & cyl_pt, p3x1_t & wcs_pt) const
{
  // express the point in the local coordinate system:
  p3x1_t lcs_pt;
  the_coord_sys_t::cyl_to_xyz(cyl_pt, lcs_pt);
  
  // express the point in the world coordinate system:
  the_coord_sys_t::lcs_to_wcs(lcs_pt, wcs_pt);
}

//----------------------------------------------------------------
// the_cyl_coord_sys_t::wcs_to_lcs
// 
void
the_cyl_coord_sys_t::wcs_to_lcs(const p3x1_t & wcs_pt, p3x1_t & cyl_pt) const
{
  // express the point in the local coordinate system:
  p3x1_t lcs_pt;
  the_coord_sys_t::wcs_to_lcs(wcs_pt, lcs_pt);
  
  // express the point in the cylindrical coordinate system:
  the_coord_sys_t::xyz_to_cyl(lcs_pt, cyl_pt);
}

// undefine shorthand:
#undef x_axis_
#undef y_axis_
#undef z_axis_

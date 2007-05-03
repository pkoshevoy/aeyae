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


// File         : m4x4.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 4x4 matrix class.

// local includes:
#include "math/m4x4.hxx"


// shorthand, undefined at the end of the file:
#define X_ data_[0]
#define Y_ data_[1]
#define Z_ data_[2]
#define W_ data_[3]

#if 0
#define M00_ data_[0]
#define M01_ data_[1]
#define M02_ data_[2]
#define M03_ data_[3]
#define M10_ data_[4]
#define M11_ data_[5]
#define M12_ data_[6]
#define M13_ data_[7]
#define M20_ data_[8]
#define M21_ data_[9]
#define M22_ data_[10]
#define M23_ data_[11]
#define M30_ data_[12]
#define M31_ data_[13]
#define M32_ data_[14]
#define M33_ data_[15]
#else
#define M00_ data_[0]
#define M01_ data_[4]
#define M02_ data_[8]
#define M03_ data_[12]
#define M10_ data_[1]
#define M11_ data_[5]
#define M12_ data_[9]
#define M13_ data_[13]
#define M20_ data_[2]
#define M21_ data_[6]
#define M22_ data_[10]
#define M23_ data_[14]
#define M30_ data_[3]
#define M31_ data_[7]
#define M32_ data_[11]
#define M33_ data_[15]
#endif


//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
m4x4_t::m4x4_t()
{
  M00_ = 0.0;
  M01_ = 0.0;
  M02_ = 0.0;
  M03_ = 0.0;
  
  M10_ = 0.0;
  M11_ = 0.0;
  M12_ = 0.0;
  M13_ = 0.0;
  
  M20_ = 0.0;
  M21_ = 0.0;
  M22_ = 0.0;
  M23_ = 0.0;
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 0.0;
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
m4x4_t::m4x4_t(const v3x1_t & axis, const float & radians)
{
  const float & x = axis.x();
  const float & y = axis.y();
  const float & z = axis.z();
  
  const float ct = cos(radians);
  const float st = sin(radians);
  const float vt = 1.0 - ct; // versine of theta
  
  assign(x * x * vt + ct,     x * y * vt - z * st, x * z * vt + y * st, 0.0,
	 x * y * vt + z * st, y * y * vt + ct,     y * z * vt - x * st, 0.0,
	 x * z * vt - y * st, y * z * vt + x * st, z * z * vt + ct,     0.0,
	 0.0,                 0.0,                 0.0,                 1.0);
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
m4x4_t::m4x4_t(const float & m11, const float & m12,
	       const float & m13, const float & m14,
	       
	       const float & m21, const float & m22,
	       const float & m23, const float & m24,
	       
	       const float & m31, const float & m32,
	       const float & m33, const float & m34,
	       
	       const float & m41, const float & m42,
	       const float & m43, const float & m44)
{
  M00_ = m11;
  M01_ = m12;
  M02_ = m13;
  M03_ = m14;
  
  M10_ = m21;
  M11_ = m22;
  M12_ = m23;
  M13_ = m24;
  
  M20_ = m31;
  M21_ = m32;
  M22_ = m33;
  M23_ = m34;
  
  M30_ = m41;
  M31_ = m42;
  M32_ = m43;
  M33_ = m44;
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
m4x4_t::m4x4_t(const float & m11, const float & m12, const float & m13,
	       const float & m21, const float & m22, const float & m23,
	       const float & m31, const float & m32, const float & m33)
{
  M00_ = m11;
  M01_ = m12;
  M02_ = m13;
  M03_ = 0.0;
  
  M10_ = m21;
  M11_ = m22;
  M12_ = m23;
  M13_ = 0.0;
  
  M20_ = m31;
  M21_ = m32;
  M22_ = m33;
  M23_ = 0.0;
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 1.0;
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
// construct a scale matrix:
// 
m4x4_t::m4x4_t(const float & scale_x,
	       const float & scale_y,
	       const float & scale_z)
{
  M00_ = scale_x;
  M01_ = 0.0;
  M02_ = 0.0;
  M03_ = 0.0;
  
  M10_ = 0.0;
  M11_ = scale_y;
  M12_ = 0.0;
  M13_ = 0.0;
  
  M20_ = 0.0;
  M21_ = 0.0;
  M22_ = scale_z;
  M23_ = 0.0;
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 1.0;
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
// construct a uniform scale matrix:
// 
m4x4_t::m4x4_t(const float & scale)
{
  M00_ = scale;
  M01_ = 0.0;
  M02_ = 0.0;
  M03_ = 0.0;
  
  M10_ = 0.0;
  M11_ = scale;
  M12_ = 0.0;
  M13_ = 0.0;
  
  M20_ = 0.0;
  M21_ = 0.0;
  M22_ = scale;
  M23_ = 0.0;
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 1.0;
}

//----------------------------------------------------------------
// m4x4_t::m4x4_t
// 
// construct a translation matrix:
// 
m4x4_t::m4x4_t(const v3x1_t & translate)
{
  M00_ = 1.0;
  M01_ = 0.0;
  M02_ = 0.0;
  M03_ = translate.x();
  
  M10_ = 0.0;
  M11_ = 1.0;
  M12_ = 0.0;
  M13_ = translate.y();
  
  M20_ = 0.0;
  M21_ = 0.0;
  M22_ = 1.0;
  M23_ = translate.z();
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 1.0;
}

//----------------------------------------------------------------
// m4x4_t::assign
// 
void
m4x4_t::assign(const float & m11, const float & m12,
	       const float & m13, const float & m14,
	       
	       const float & m21, const float & m22,
	       const float & m23, const float & m24,
	       
	       const float & m31, const float & m32,
	       const float & m33, const float & m34,
	       
	       const float & m41, const float & m42,
	       const float & m43, const float & m44)
{
  M00_ = m11;
  M01_ = m12;
  M02_ = m13;
  M03_ = m14;
  
  M10_ = m21;
  M11_ = m22;
  M12_ = m23;
  M13_ = m24;
  
  M20_ = m31;
  M21_ = m32;
  M22_ = m33;
  M23_ = m34;
  
  M30_ = m41;
  M31_ = m42;
  M32_ = m43;
  M33_ = m44;
}

//----------------------------------------------------------------
// m4x4_t::assign
// 
void
m4x4_t::assign(const float & m11, const float & m12, const float & m13,
	       const float & m21, const float & m22, const float & m23,
	       const float & m31, const float & m32, const float & m33)
{
  M00_ = m11;
  M01_ = m12;
  M02_ = m13;
  M03_ = 0.0;
  
  M10_ = m21;
  M11_ = m22;
  M12_ = m23;
  M13_ = 0.0;
  
  M20_ = m31;
  M21_ = m32;
  M22_ = m33;
  M23_ = 0.0;
  
  M30_ = 0.0;
  M31_ = 0.0;
  M32_ = 0.0;
  M33_ = 1.0;
}

//----------------------------------------------------------------
// m4x4_t::multiply
// 
void
m4x4_t::multiply(const m4x4_t & a, m4x4_t & b) const
{
  b.M00_ = (M00_ * a.M00_ + M01_ * a.M10_ + M02_ * a.M20_ + M03_ * a.M30_);
  b.M01_ = (M00_ * a.M01_ + M01_ * a.M11_ + M02_ * a.M21_ + M03_ * a.M31_);
  b.M02_ = (M00_ * a.M02_ + M01_ * a.M12_ + M02_ * a.M22_ + M03_ * a.M32_);
  b.M03_ = (M00_ * a.M03_ + M01_ * a.M13_ + M02_ * a.M23_ + M03_ * a.M33_);
  
  b.M10_ = (M10_ * a.M00_ + M11_ * a.M10_ + M12_ * a.M20_ + M13_ * a.M30_);
  b.M11_ = (M10_ * a.M01_ + M11_ * a.M11_ + M12_ * a.M21_ + M13_ * a.M31_);
  b.M12_ = (M10_ * a.M02_ + M11_ * a.M12_ + M12_ * a.M22_ + M13_ * a.M32_);
  b.M13_ = (M10_ * a.M03_ + M11_ * a.M13_ + M12_ * a.M23_ + M13_ * a.M33_);
  
  b.M20_ = (M20_ * a.M00_ + M21_ * a.M10_ + M22_ * a.M20_ + M23_ * a.M30_);
  b.M21_ = (M20_ * a.M01_ + M21_ * a.M11_ + M22_ * a.M21_ + M23_ * a.M31_);
  b.M22_ = (M20_ * a.M02_ + M21_ * a.M12_ + M22_ * a.M22_ + M23_ * a.M32_);
  b.M23_ = (M20_ * a.M03_ + M21_ * a.M13_ + M22_ * a.M23_ + M23_ * a.M33_);
  
  b.M30_ = (M30_ * a.M00_ + M31_ * a.M10_ + M32_ * a.M20_ + M33_ * a.M30_);
  b.M31_ = (M30_ * a.M01_ + M31_ * a.M11_ + M32_ * a.M21_ + M33_ * a.M31_);
  b.M32_ = (M30_ * a.M02_ + M31_ * a.M12_ + M32_ * a.M22_ + M33_ * a.M32_);
  b.M33_ = (M30_ * a.M03_ + M31_ * a.M13_ + M32_ * a.M23_ + M33_ * a.M33_);
}

//----------------------------------------------------------------
// m4x4_t::multiply
// 
void
m4x4_t::multiply(const float & a, m4x4_t & b) const
{
  b.M00_ = M00_ * a;
  b.M01_ = M01_ * a;
  b.M02_ = M02_ * a;
  b.M03_ = M03_ * a;
  
  b.M10_ = M10_ * a;
  b.M11_ = M11_ * a;
  b.M12_ = M12_ * a;
  b.M13_ = M13_ * a;
  
  b.M20_ = M20_ * a;
  b.M21_ = M21_ * a;
  b.M22_ = M22_ * a;
  b.M23_ = M23_ * a;
  
  b.M30_ = M30_ * a;
  b.M31_ = M31_ * a;
  b.M32_ = M32_ * a;
  b.M33_ = M33_ * a;
}

//----------------------------------------------------------------
// m4x4_t::multiply
// 
void
m4x4_t::multiply(const p4x1_t & a, p4x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M01_ * a.Y_ + M02_ * a.Z_ + M03_ * a.W_;
  b.Y_ = M10_ * a.X_ + M11_ * a.Y_ + M12_ * a.Z_ + M13_ * a.W_;
  b.Z_ = M20_ * a.X_ + M21_ * a.Y_ + M22_ * a.Z_ + M23_ * a.W_;
  b.W_ = M30_ * a.X_ + M31_ * a.Y_ + M32_ * a.Z_ + M33_ * a.W_;
}

//----------------------------------------------------------------
// m4x4_t::multiply
// 
void
m4x4_t::multiply(const p3x1_t & a, p3x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M01_ * a.Y_ + M02_ * a.Z_ + M03_;
  b.Y_ = M10_ * a.X_ + M11_ * a.Y_ + M12_ * a.Z_ + M13_;
  b.Z_ = M20_ * a.X_ + M21_ * a.Y_ + M22_ * a.Z_ + M23_;
  float w = M30_ * a.X_ + M31_ * a.Y_ + M32_ * a.Z_ + M33_;
  b *= (1.0 / w);
}

//----------------------------------------------------------------
// m4x4_t::multiply
// 
void
m4x4_t::multiply(const v3x1_t & a, v3x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M01_ * a.Y_ + M02_ * a.Z_;
  b.Y_ = M10_ * a.X_ + M11_ * a.Y_ + M12_ * a.Z_;
  b.Z_ = M20_ * a.X_ + M21_ * a.Y_ + M22_ * a.Z_;
}

//----------------------------------------------------------------
// m4x4_t::transpose_multiply
// 
void
m4x4_t::transpose_multiply(const p4x1_t & a, p4x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M10_ * a.Y_ + M20_ * a.Z_ + M30_ * a.W_;
  b.Y_ = M01_ * a.X_ + M11_ * a.Y_ + M21_ * a.Z_ + M31_ * a.W_;
  b.Z_ = M02_ * a.X_ + M12_ * a.Y_ + M22_ * a.Z_ + M32_ * a.W_;
  b.W_ = M03_ * a.X_ + M13_ * a.Y_ + M23_ * a.Z_ + M33_ * a.W_;
}

//----------------------------------------------------------------
// m4x4_t::transpose_multiply
// 
void
m4x4_t::transpose_multiply(const p3x1_t & a, p3x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M10_ * a.Y_ + M20_ * a.Z_ + M30_;
  b.Y_ = M01_ * a.X_ + M11_ * a.Y_ + M21_ * a.Z_ + M31_;
  b.Z_ = M02_ * a.X_ + M12_ * a.Y_ + M22_ * a.Z_ + M32_;
  float w = M03_ * a.X_ + M13_ * a.Y_ + M23_ * a.Z_ + M33_;
  b *= (1.0 / w);
}

//----------------------------------------------------------------
// m4x4_t::transpose_multiply
// 
void
m4x4_t::transpose_multiply(const v3x1_t & a, v3x1_t & b) const
{
  b.X_ = M00_ * a.X_ + M10_ * a.Y_ + M20_ * a.Z_;
  b.Y_ = M01_ * a.X_ + M11_ * a.Y_ + M21_ * a.Z_;
  b.Z_ = M02_ * a.X_ + M12_ * a.Y_ + M22_ * a.Z_;
}

//----------------------------------------------------------------
// m4x4_t::identity
// 
const m4x4_t 
m4x4_t::identity()
{
  m4x4_t i;
  i.M00_ = 1.0;
  i.M11_ = 1.0;
  i.M22_ = 1.0;
  i.M33_ = 1.0;
  return i;
}

//----------------------------------------------------------------
// m4x4_t::screen
// 
const m4x4_t 
m4x4_t::screen(const float & nx,
	       const float & ny,
	       const float & xmin,
	       const float & ymin,
	       const float & xmax,
	       const float & ymax)
{
  m4x4_t translate_1 = m4x4_t::translate((nx - 1.0) / 2.0,
					 (ny - 1.0) / 2.0,
					 0.0);
  float sx = -nx / (xmax - xmin);
  float sy = -ny / (ymax - ymin);
  float s_max = (sy > sx) ? sy : sx;
  m4x4_t scale = m4x4_t::scale(s_max, s_max, 1.0);
  
  m4x4_t translate_2 = m4x4_t::translate(-(xmax + xmin) / 2.0,
					 -(ymax + ymin) / 2.0,
					 0.0);
  
  return (translate_1 * scale * translate_2);
}

//----------------------------------------------------------------
// m4x4_t::screen_inverse
// 
const m4x4_t 
m4x4_t::screen_inverse(const float & nx,
		       const float & ny,
		       const float & xmin,
		       const float & ymin,
		       const float & xmax,
		       const float & ymax)
{
  m4x4_t translate_1_inv = m4x4_t::translate((1.0 - nx) / 2.0,
					     (1.0 - ny) / 2.0,
					     0.0);
  float sx = (xmin - xmax) / nx;
  float sy = (ymin - ymax) / ny;
  float s_min = (sy < sx) ? sy : sx;
  m4x4_t scale_inv = m4x4_t::scale(s_min, s_min, 1.0);
  
  m4x4_t translate_2_inv = m4x4_t::translate((xmax + xmin) / 2.0,
					     (ymax + ymin) / 2.0,
					     0.0);
  
  return (translate_2_inv * scale_inv * translate_1_inv);
}

//----------------------------------------------------------------
// m4x4_t::perspective
// 
const m4x4_t 
m4x4_t::perspective(const float & near_plane, const float & far_plane)
{
  m4x4_t m;
  m.M00_ = 1.0;
  m.M11_ = 1.0;
  m.M22_ = (near_plane + far_plane) / near_plane;
  m.M23_ = -far_plane;
  m.M32_ = 1.0 / near_plane;
  return m;
}

//----------------------------------------------------------------
// m4x4_t::view
// 
const m4x4_t
m4x4_t::view(const p3x1_t & lf, const p3x1_t & la, const v3x1_t & v_up)
{
  v3x1_t w = !(la - lf);  // w = |la - lf|
  v3x1_t u = !(v_up % w); // u = |v_up x w|
  v3x1_t v = w % u;       // v = w x u
  
  m4x4_t r;
  
  r.M00_ = u.X_;
  r.M01_ = u.Y_;
  r.M02_ = u.Z_;
  
  r.M10_ = v.X_;
  r.M11_ = v.Y_;
  r.M12_ = v.Z_;
  
  r.M20_ = w.X_;
  r.M21_ = w.Y_;
  r.M22_ = w.Z_;
  
  r.M33_ = 1.0;
  
  m4x4_t t = m4x4_t::translate(-lf.X_, -lf.Y_, -lf.Z_);
  return (r * t);
}

//----------------------------------------------------------------
// m4x4_t::view_inverse
// 
const m4x4_t
m4x4_t::view_inverse(const p3x1_t & lf, const p3x1_t & la, const v3x1_t & v_up)
{
  v3x1_t w = !(la - lf);  // w = |la - lf|
  v3x1_t u = !(v_up % w); // u = |v_up x w|
  v3x1_t v = w % u;       // v = w x u
  
  m4x4_t r_inv;
  
  r_inv.M00_ = u.X_;
  r_inv.M01_ = v.X_;
  r_inv.M02_ = w.X_;
  
  r_inv.M10_ = u.Y_;
  r_inv.M11_ = v.Y_;
  r_inv.M12_ = w.Y_;
  
  r_inv.M20_ = u.Z_;
  r_inv.M21_ = v.Z_;
  r_inv.M22_ = w.Z_;
  
  r_inv.M33_ = 1.0;
  
  m4x4_t t_inv = m4x4_t::translate(lf.X_, lf.Y_, lf.Z_);
  
  return (t_inv * r_inv);
}

//----------------------------------------------------------------
// m4x4_t::translate
// 
const m4x4_t
m4x4_t::translate(const float & x, const float & y, const float & z)
{
  m4x4_t r;
  r.M00_ = 1.0;
  r.M11_ = 1.0;
  r.M22_ = 1.0;
  r.M33_ = 1.0;
  r.M03_ = x;
  r.M13_ = y;
  r.M23_ = z;
  return r;
}

//----------------------------------------------------------------
// m4x4_t::translate_inverse
// 
const m4x4_t
m4x4_t::translate_inverse(const float & x, const float & y, const float & z)
{
  m4x4_t r;
  r.M00_ = 1.0;
  r.M11_ = 1.0;
  r.M22_ = 1.0;
  r.M33_ = 1.0;
  r.M03_ = -x;
  r.M13_ = -y;
  r.M23_ = -z;
  return r;
}

//----------------------------------------------------------------
// m4x4_t::scale
// 
const m4x4_t
m4x4_t::scale(const float & sx, const float & sy, const float & sz)
{
  m4x4_t r;
  r.M00_ = sx;
  r.M11_ = sy;
  r.M22_ = sz;
  r.M33_ = 1.0;
  return r;
}

//----------------------------------------------------------------
// m4x4_t::scale_inverse
// 
const m4x4_t
m4x4_t::scale_inverse(const float & sx, const float & sy, const float & sz)
{
  m4x4_t r;
  r.M00_ = 1.0 / sx;
  r.M11_ = 1.0 / sy;
  r.M22_ = 1.0 / sz;
  r.M33_ = 1.0;
  return r;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & strm, const m4x4_t & m)
{
  return strm << (void *)(&m) << " = " << endl
	      << "[ "
	      << setw(11) << m[0][0] << ' '
	      << setw(11) << m[0][1] << ' '
	      << setw(11) << m[0][2] << ' '
	      << setw(11) << m[0][3] << " ]" << endl
	      << "[ "
	      << setw(11) << m[1][0] << ' '
	      << setw(11) << m[1][1] << ' '
	      << setw(11) << m[1][2] << ' '
	      << setw(11) << m[1][3] << " ]" << endl
	      << "[ "
	      << setw(11) << m[2][0] << ' '
	      << setw(11) << m[2][1] << ' '
	      << setw(11) << m[2][2] << ' '
	      << setw(11) << m[2][3] << " ]" << endl
	      << "[ "
	      << setw(11) << m[3][0] << ' '
	      << setw(11) << m[3][1] << ' '
	      << setw(11) << m[3][2] << ' '
	      << setw(11) << m[3][3] << " ]" << endl;
}


// undefine shorthand:
#undef X_
#undef Y_
#undef Z_
#undef W_
#undef M00_
#undef M01_
#undef M02_
#undef M03_
#undef M10_
#undef M11_
#undef M12_
#undef M13_
#undef M20_
#undef M21_
#undef M22_
#undef M23_
#undef M30_
#undef M31_
#undef M32_
#undef M33_

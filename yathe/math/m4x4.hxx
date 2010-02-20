// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : m4x4.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 4x4 matrix class.

#ifndef M4X4_HXX_
#define M4X4_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"


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
// m4x4_t
//
class m4x4_t
{
public:
  // ZERO matrix:
  m4x4_t();
  
  // rotation matrix by angle theta around vector k:
  m4x4_t(const v3x1_t & axis_of_rotation, const float & radians);
  
  m4x4_t(const float & m11, const float & m12,
	 const float & m13, const float & m14,
	 
	 const float & m21, const float & m22,
	 const float & m23, const float & m24,
	 
	 const float & m31, const float & m32,
	 const float & m33, const float & m34,
	 
	 const float & m41, const float & m42,
	 const float & m43, const float & m44);
  
  // same as above, except only sets up the upper-left 3x3 portion
  // of the matrix, and sets the lower-right corner to 1 and everything
  // else to zero:
  m4x4_t(const float & m11, const float & m12, const float & m13,
	 const float & m21, const float & m22, const float & m23,
	 const float & m31, const float & m32, const float & m33);
  
  // construct a scale matrix:
  m4x4_t(const float & scale_x,
	 const float & scale_y,
	 const float & scale_z);
  
  // construct a uniform scale matrix (by default - identity):
  m4x4_t(const float & scale);
  
  // construct a translation matrix:
  m4x4_t(const v3x1_t & translate);
  
  void assign(const float & m11, const float & m12,
	      const float & m13, const float & m14,
	      
	      const float & m21, const float & m22,
	      const float & m23, const float & m24,
	      
	      const float & m31, const float & m32,
	      const float & m33, const float & m34,
	      
	      const float & m41, const float & m42,
	      const float & m43, const float & m44);
  
  // same as above, except only sets up the upper-left 3x3 portion
  // of the matrix, and sets the lower-right corner to 1 and everything
  // else to zero:
  void assign(const float & m11, const float & m12, const float & m13,
	      const float & m21, const float & m22, const float & m23,
	      const float & m31, const float & m32, const float & m33);
  
  void multiply(const m4x4_t & a, m4x4_t & b) const;
  void multiply(const float & f, m4x4_t & b) const;
  
  void multiply(const p4x1_t & a, p4x1_t & b) const;
  void multiply(const p3x1_t & a, p3x1_t & b) const;
  void multiply(const v3x1_t & a, v3x1_t & b) const;
  
  void transpose_multiply(const p4x1_t & a, p4x1_t & b) const;
  void transpose_multiply(const p3x1_t & a, p3x1_t & b) const;
  void transpose_multiply(const v3x1_t & a, v3x1_t & b) const;
  
  // matrix times matrix, in place:
  inline m4x4_t & operator *= (const m4x4_t & m)
  {
    m4x4_t r;
    multiply(m, r);
    *this = r;
    return *this;
  }
  
  // matrix times scalar, in place:
  inline const m4x4_t operator *= (const float & f)
  {
    m4x4_t r;
    multiply(f, r);
    *this = r;
    return *this;
  }
  
  // matrix times matrix:
  inline const m4x4_t operator * (const m4x4_t & m) const
  {
    m4x4_t r;
    multiply(m, r);
    return r;
  }
  
  // matrix times scalar:
  inline const m4x4_t operator * (const float & f) const
  {
    m4x4_t r;
    multiply(f, r);
    return r;
  }
  
  // matrix times point:
  inline const p3x1_t operator * (const p3x1_t & p) const
  {
    p3x1_t r;
    multiply(p, r);
    return r;
  }
  
  // matrix times point:
  inline const p4x1_t operator * (const p4x1_t & p) const
  {
    p4x1_t r;
    multiply(p, r);
    return r;
  }
  
  // matrix times vector:
  inline const v3x1_t operator * (const v3x1_t & v) const
  {
    v3x1_t r;
    multiply(v, r);
    return r;
  }
  
  // Identity matrix:
  static const m4x4_t identity();
  
  // Screen transform/inverse matricies:
  static const m4x4_t screen(const float & nx,
			     const float & ny,
			     const float & xmin,
			     const float & ymin,
			     const float & xmax,
			     const float & ymax);
  static const m4x4_t screen_inverse(const float & nx,
				     const float & ny,
				     const float & xmin,
				     const float & ymin,
				     const float & xmax,
				     const float & ymax);
  
  // Perspective projection matrix:
  static const m4x4_t perspective(const float & near, const float & far);
  
  // View transform matrix:
  static const m4x4_t view(const p3x1_t & lf,
			   const p3x1_t & la,
			   const v3x1_t & Vup);
  
  // View transform inverse matrix:
  static const m4x4_t view_inverse(const p3x1_t & lf,
				   const p3x1_t & la,
				   const v3x1_t & Vup);
  
  // Translation/inverse matricies:
  static const m4x4_t translate(const float & x,
				const float & y,
				const float & z);
  
  static const m4x4_t translate_inverse(const float & x,
					const float & y,
					const float & z);
  
  static const m4x4_t translate(const v3x1_t & v)
  { return translate(v.X_, v.Y_, v.Z_); }
  
  static const m4x4_t translate_inverse(const v3x1_t & v)
  { return translate_inverse(v.X_, v.Y_, v.Z_); }
  
  // Scale/inverse matricies:
  static const m4x4_t scale(const float & sx,
			    const float & sy,
			    const float & sz);
  
  static const m4x4_t scale_inverse(const float & sx,
				    const float & sy,
				    const float & sz);
  
  static const m4x4_t scale(const v3x1_t & v)
  { return scale(v.X_, v.Y_, v.Z_); }
  
  static const m4x4_t scale_inverse(const v3x1_t & v)
  { return scale_inverse(v.X_, v.Y_, v.Z_); }
  
  // datamember accessors:
  inline const float * operator[] (unsigned int row) const
  { return &(data_[row * 4]); }
  
  inline float * data()
  { return data_; }
  
  inline const float * data() const
  { return data_; }
  
protected:
  float data_[16];
};

extern ostream &
operator << (ostream & stream, const m4x4_t & m); // output


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


#endif // M4X4_HXX_

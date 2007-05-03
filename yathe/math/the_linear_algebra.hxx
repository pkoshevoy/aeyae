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


// File         : the_linear_algebra.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sat Oct  2 12:12:30 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Common linear algebra functions.

#ifndef THE_LINEAR_ALGEBRA_HXX_
#define THE_LINEAR_ALGEBRA_HXX_


//----------------------------------------------------------------
// the_determinant_2x2
// 
// Calculate the determinant of a 2x2 matrix.
//
#define the_determinant_2x2( a11, a12, \
                             a21, a22 ) \
  ( a11 * a22 - a12 * a21 )


//----------------------------------------------------------------
// the_cramers_rule_2x2
// 
// Solve a 2x2 linear system using the Cramer's Rule.
// 
template <class T>
bool
the_cramers_rule_2x2(const T & a11, const T & a12,
		     const T & a21, const T & a22,
		     const T & b1,
		     const T & b2,
		     T & x1,
		     T & x2,
		     T determinant_tolerance = 1e-6)
{
  T d = the_determinant_2x2(a11, a12,
			    a21, a22);
  if (fabs(d) <= determinant_tolerance || d != d) return false;
  
  T d1 = the_determinant_2x2(b1, a12,
			     b2, a22);
  
  T d2 = the_determinant_2x2(a11, b1,
			     a21, b2);
  
  x1 = d1 / d;
  x2 = d2 / d;
  
  return true;
}


//----------------------------------------------------------------
// the_determinant_3x3
// 
// Calculate the determinant of a 3x3 matrix.
//
#define the_determinant_3x3( a11, a12, a13, \
                             a21, a22, a23, \
			     a31, a32, a33 )\
  ( a11 * a22 * a33 - a11 * a32 * a23 + \
    a21 * a32 * a13 - a21 * a12 * a33 + \
    a31 * a12 * a23 - a31 * a22 * a13 )


// shorthand, undefined at the end of the file:
#define a11 A[0]
#define a12 A[1]
#define a13 A[2]
#define a21 A[3]
#define a22 A[4]
#define a23 A[5]
#define a31 A[6]
#define a32 A[7]
#define a33 A[8]
#define x1  x[0]
#define x2  x[1]
#define x3  x[2]
#define b1  b[0]
#define b2  b[1]
#define b3  b[2]

//----------------------------------------------------------------
// the_cramers_rule_3x3
// 
// Solve a 3x3 linear system using the Cramer's Rule.
// 
template <class T>
bool
the_cramers_rule_3x3(const T * A,
		     const T * b,
		     T * x,
		     T determinant_tolerance = 1e-6)
{
  T d = the_determinant_3x3(a11, a12, a13,
			    a21, a22, a23,
			    a31, a32, a33);
  if (fabs(d) <= determinant_tolerance || d != d) return false;
  
  T d1 = the_determinant_3x3(b1, a12, a13,
			     b2, a22, a23,
			     b3, a32, a33);
  
  T d2 = the_determinant_3x3(a11, b1, a13,
			     a21, b2, a23,
			     a31, b3, a33);
  
  T d3 = the_determinant_3x3(a11, a12, b1,
			     a21, a22, b2,
			     a31, a32, b3);
  
  x1 = d1 / d;
  x2 = d2 / d;
  x3 = d3 / d;
  
  return true;
}

// undefine shorthand:
#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33
#undef x1
#undef x2
#undef x3
#undef b1
#undef b2
#undef b3


#endif // THE_LINEAR_ALGEBRA_HXX_

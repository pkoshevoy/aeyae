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


// File         : the_camera.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu May 27 11:21:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : The camera class used for raytracing.

// local includes:
#include "math/the_camera.hxx"

// system includes:
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// namespace access:
using std::cout;
using std::endl;


//----------------------------------------------------------------
// the_camera_t::the_camera_t
// 
the_camera_t::the_camera_t(const p3x1_t & eye,
			   const v3x1_t & gaze,
			   const v3x1_t & up,
			   const float & focus,
			   const float & image_w,
			   const float & image_h,
			   const unsigned int & pixels_u,
			   const unsigned int & pixels_v):
  eye_(eye),
  gaze_(gaze),
  up_(!up),
  focus_(focus),
  image_w_(image_w),
  image_h_(image_h),
  pixels_u_(pixels_u),
  pixels_v_(pixels_v),
  sampling_method_(NULL)
{
#ifndef NDEBUG
  float dot = fabs(gaze_ * up_);
  assert(dot < 0.0001);
#endif
  
  v3x1_t u = !(gaze_ % up_);
  v3x1_t v = -up;
  
  image_cs_.u_axis() = u * (image_w_ / float(pixels_u));
  image_cs_.v_axis() = v * (image_h_ / float(pixels_v));
  image_cs_.origin() = eye_ + gaze_ * focus_ - 0.5 * (image_w_ * u +
						      image_h_ * v);
}


//----------------------------------------------------------------
// the_pinhole_camera_t::rays
// 
void
the_pinhole_camera_t::rays(const the_mersenne_twister_t & rng,
			   const unsigned int & pixel_u,
			   const unsigned int & pixel_v,
			   std::vector<the_ray_t> & rays) const
{
  // minor optimization:
  p3x1_t wcs_pt;
  unsigned int index;
  
  const unsigned int & nu = samples_u();
  const unsigned int & nv = samples_v();
  
  for (unsigned int i = 0; i < nu; i++)
  {
    for (unsigned int j = 0; j < nv; j++)
    {
      image_cs_.lcs_to_wcs(float(pixel_u) + sample_u(rng, i),
			   float(pixel_v) + sample_v(rng, j),
			   wcs_pt);
      
      index = i + j * nu;
      rays[index].p() = eye_;
      rays[index].v() = wcs_pt - eye_;
    }
  }
}


//----------------------------------------------------------------
// the_thinlens_camera_t::set_sampling_method
// 
void
the_thinlens_camera_t::
set_sampling_method(const the_sampling_method_t & sampling_method)
{
  the_camera_t::set_sampling_method(sampling_method);
  
  const unsigned int & nu = sampling_method.samples_u();
  const unsigned int & nv = sampling_method.samples_v();
  
  permutation_u_.resize(nu);
  permutation_v_.resize(nv);
  
  for (unsigned int i = 0; i < nu; i++) permutation_u_[i] = i;
  for (unsigned int i = 0; i < nv; i++) permutation_v_[i] = i;
  
  for (unsigned int i = 0; i < nu - 1; i++)
  {
    unsigned int index =
      i + 1 + (unsigned int)(float(nu - i - 1) * float(rand()) /
			     (float(RAND_MAX) + 1.0));
    std::swap(permutation_u_[i], permutation_u_[index]);
  }
  
  for (unsigned int i = 0; i < nv - 1; i++)
  {
    unsigned int index =
      i + 1 + (unsigned int)(float(nv - i - 1) * float(rand()) /
			     (float(RAND_MAX) + 1.0));
    std::swap(permutation_v_[i], permutation_v_[index]);
  }
}

//----------------------------------------------------------------
// the_thinlens_camera_t::rays
// 
void
the_thinlens_camera_t::rays(const the_mersenne_twister_t & rng,
			    const unsigned int & pixel_u,
			    const unsigned int & pixel_v,
			    std::vector<the_ray_t> & rays) const
{
  // minor optimization:
  p3x1_t wcs_pt;
  p3x1_t eye_pt;
  unsigned int index;
  
  const unsigned int & nu = samples_u();
  const unsigned int & nv = samples_v();
  
  for (unsigned int i = 0; i < nu; i++)
  {
    for (unsigned int j = 0; j < nv; j++)
    {
      image_cs_.lcs_to_wcs(float(pixel_u) + sample_u(rng, i),
			   float(pixel_v) + sample_v(rng, j),
			   wcs_pt);
#if 0
      const unsigned int & pi = permutation_u_[i];
      const unsigned int & pj = permutation_v_[j];
      
      const float su = 0.5 + sample_u(rng, pi);
      const float sv = 0.5 + sample_v(rng, pj);
      lens_cs_.lcs_to_wcs(sqrt(su), 2.0 * M_PI * sv, eye_pt);
#else
      const unsigned int pi = (unsigned int)(rng.genrand_real2() * float(nu));
      const unsigned int pj = (unsigned int)(rng.genrand_real2() * float(nv));
      
      /*
	This code is based on work by Peter Shirley and Kenneth Chiu:
	
	This transforms points on [-0.5, 0.5]^2 to points on unit disk
	centered at origin. Each "pie-slice" quadrant of square is handled
	as a seperate case. The bad floating point cases are all handled
	appropriately.
	
	The regions for (a, b) are:
	
		     0.5 Pi
	       +-------|-------+         Y |
	       | \           / |           |
	       |   \   2   /   |           |
	       |     \   /     |           |
	1.0 Pi -  3    x   1   - 0.0 Pi    +------- X
	       |     /   \     |
	       |   /   4   \   |
	       | /           \ |
	       +-------|-------+
		     1.5 Pi
      */
      
      // (a, b) on [-1, 1] x [-1, 1]:
      const float a = 2.0 * sample_u(rng, pi);
      const float b = 2.0 * sample_v(rng, pj);
      
      float phi;
      float r;
      
      if (a > -b)
      {
	// region 1 or 2
	if (a > b)
	{
	  // region 1, also |a| > |b|
	  r = a;
	  phi = (M_PI / 4) * (b / a);
	}
	else
	{
	  // region 2, also |b| > |a|
	  r = b;
	  phi = (M_PI / 4) * (2 - (a / b));
	}
      }
      else
      {
	// region 3 or 4
	if (a < b)
	{
	  // region 3, also |a| >= |b|, a != 0
	  r = -a;
	  phi = (M_PI / 4) * (4 + (b / a));
	}
	else
	{
	  // region 4, |b| >= |a|, but a==0 and b==0 could occur.
	  r = -b;
	  if (b != 0)
	  {
	    phi = (M_PI / 4) * (6 - (a / b));
	  }
	  else
	  {
	    phi = 0;
	  }
	}
      }
      
      lens_cs_.lcs_to_wcs(r, phi, eye_pt);
#endif
      
      index = i + j * nu;
      rays[index].p() = eye_pt;
      rays[index].v() = wcs_pt - eye_pt;
    }
  }
}

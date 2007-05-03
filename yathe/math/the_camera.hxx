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


// File         : the_camera.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu May 27 11:12:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : The camera class used for raytracing.

#ifndef THE_CAMERA_HXX_
#define THE_CAMERA_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_sampling_method.hxx"
#include "math/the_ray.hxx"
#include "math/the_coord_sys.hxx"
#include "math/the_mersenne_twister.hxx"

// system includes:
#include <vector>


//----------------------------------------------------------------
// the_camera_type_t
// 
typedef enum
{
  THE_PINHOLE_CAMERA_E,
  THE_THINLENS_CAMERA_E,
  THE_UNKNOWN_CAMERA_E
} the_camera_type_t;

//----------------------------------------------------------------
// the_camera_type_to_str
// 
inline const char *
the_camera_type_to_str(const the_camera_type_t & ct)
{
  switch (ct)
  {
    case THE_PINHOLE_CAMERA_E:	return "pinhole";
    case THE_THINLENS_CAMERA_E:	return "thinlens";
    default:			break;
  }
  
  return NULL;
}

//----------------------------------------------------------------
// the_str_to_camera_type
// 
inline the_camera_type_t
the_str_to_camera_type(const char * str)
{
  if (strcmp(str, "pinhole") == 0)	return THE_PINHOLE_CAMERA_E;
  if (strcmp(str, "thinlens") == 0)	return THE_THINLENS_CAMERA_E;
  return THE_UNKNOWN_CAMERA_E;
}


//----------------------------------------------------------------
// the_camera_t
// 
class the_camera_t
{
public:
  the_camera_t(const p3x1_t & eye,
	       const v3x1_t & gaze,
	       const v3x1_t & up,
	       const float & focus,
	       const float & image_w,
	       const float & image_h,
	       const unsigned int & pixels_u,
	       const unsigned int & pixels_v);
  
  virtual ~the_camera_t() {}
  
  // generate a set of rays (could be just one) for a given pixel coordinate:
  virtual void rays(const the_mersenne_twister_t & rng,
		    const unsigned int & pixel_u,
		    const unsigned int & pixel_v,
		    std::vector<the_ray_t> & rays) const = 0;
  
  // WARNING: the camera does not own the sampling method:
  virtual void
  set_sampling_method(const the_sampling_method_t & sampling_method)
  { sampling_method_ = &sampling_method; }
  
  inline const unsigned int & samples_u() const
  { return sampling_method_->samples_u(); }
  
  inline const unsigned int & samples_v() const
  { return sampling_method_->samples_v(); }
  
  inline float sample_u(const the_mersenne_twister_t & rng,
			 const unsigned int & u_index) const
  { return sampling_method_->sample_u(rng, u_index); }
  
  inline float sample_v(const the_mersenne_twister_t & rng,
			 const unsigned int & v_index) const
  { return sampling_method_->sample_v(rng, v_index); }
  
protected:
  // the position of the eye:
  p3x1_t eye_;
  
  // the direction in which the eye is gazing:
  v3x1_t gaze_;
  
  // the vector which defines "up" (points to the sky), must be normal to
  // the gaze direction:
  v3x1_t up_;
  
  // the depth of the image plane along the gaze vector, at which
  // objects appear in focus (objects are always in focus for the
  // pinhole camera):
  float focus_;
  
  // the dimensions of the image:
  float image_w_;
  float image_h_;
  
  // the discretization of the image plane:
  unsigned int pixels_u_;
  unsigned int pixels_v_;
  
  // the coordinate system of the image plane:
  the_uv_csys_t image_cs_;
  
  // the sampling method:
  const the_sampling_method_t * sampling_method_;
};

//----------------------------------------------------------------
// the_pinhole_camera_t
// 
class the_pinhole_camera_t : public the_camera_t
{
public:
  the_pinhole_camera_t(const p3x1_t & eye,
		       const v3x1_t & gaze,
		       const v3x1_t & up,
		       const float & focus,
		       const float & image_w,
		       const float & image_h,
		       const unsigned int & pixels_u,
		       const unsigned int & pixels_v):
    the_camera_t(eye, gaze, up, focus, image_w, image_h, pixels_u, pixels_v)
  {}
  
  // virtual:
  void rays(const the_mersenne_twister_t & rng,
	    const unsigned int & pixel_u,
	    const unsigned int & pixel_v,
	    std::vector<the_ray_t> & rays) const;
};

//----------------------------------------------------------------
// the_thinlens_camera_t
// 
class the_thinlens_camera_t : public the_camera_t
{
public:
  the_thinlens_camera_t(const p3x1_t & eye,
			const v3x1_t & gaze,
			const v3x1_t & up,
			const float & focus,
			const float & image_w,
			const float & image_h,
			const unsigned int & pixels_u,
			const unsigned int & pixels_v,
			const float & lens_radius):
    the_camera_t(eye, gaze, up, focus, image_w, image_h, pixels_u, pixels_v),
    lens_radius_(lens_radius)
  {
    lens_cs_ = the_cyl_uv_csys_t(lens_radius_ * (!image_cs_.u_axis()),
				 lens_radius_ * (!image_cs_.v_axis()),
				 eye_);
  }
  
  // virtual:
  void set_sampling_method(const the_sampling_method_t & sampling_method);
  
  // virtual:
  void rays(const the_mersenne_twister_t & rng,
	    const unsigned int & pixel_u,
	    const unsigned int & pixel_v,
	    std::vector<the_ray_t> & rays) const;
  
private:
  std::vector<unsigned int> permutation_u_;
  std::vector<unsigned int> permutation_v_;
  
  the_cyl_uv_csys_t lens_cs_;
  const float lens_radius_;
};


#endif // THE_CAMERA_HXX_

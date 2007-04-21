// File         : the_sampling_method.hxx
// Author       : Paul A. Koshevoy
// Created      : Thu May 27 11:12:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  :

#ifndef THE_SAMPLING_METHOD_HXX_
#define THE_SAMPLING_METHOD_HXX_

// local includes:
#include "math/the_mersenne_twister.hxx"

// system includes:
#include <assert.h>
#include <string.h>


//----------------------------------------------------------------
// the_sampling_method_type_t
// 
typedef enum
{
  THE_REGULAR_SAMPLING_METHOD_E,
  THE_RANDOM_SAMPLING_METHOD_E,
  THE_JITTERED_SAMPLING_METHOD_E,
  THE_BSPLINE_SAMPLING_METHOD_E,
  THE_STRATIFIED_BSPLINE_SAMPLING_METHOD_E,
  THE_UNKNOWN_SAMPLING_METHOD_E
} the_sampling_method_type_t;

//----------------------------------------------------------------
// the_sampling_method_type_to_str
// 
inline const char *
the_sampling_method_type_to_str(const the_sampling_method_type_t & smt)
{
  switch (smt)
  {
    case THE_REGULAR_SAMPLING_METHOD_E:
      return "regular";
      
    case THE_RANDOM_SAMPLING_METHOD_E:
      return "random";
      
    case THE_JITTERED_SAMPLING_METHOD_E:
      return "jittered";
      
    case THE_BSPLINE_SAMPLING_METHOD_E:
      return "bspline";
      
    case THE_STRATIFIED_BSPLINE_SAMPLING_METHOD_E:
      return "stratified_bspline";
      
    default:
      break;
  }
  
  assert(false);
  return NULL;
}

//----------------------------------------------------------------
// the_str_to_sampling_method_type
// 
inline const the_sampling_method_type_t
the_str_to_sampling_method_type(const char * str)
{
  if (strcmp(str, "regular") == 0)
    return THE_REGULAR_SAMPLING_METHOD_E;
  
  if (strcmp(str, "random") == 0)
    return THE_RANDOM_SAMPLING_METHOD_E;
  
  if (strcmp(str, "jittered") == 0)
    return THE_JITTERED_SAMPLING_METHOD_E;
  
  if (strcmp(str, "bspline") == 0)
    return THE_BSPLINE_SAMPLING_METHOD_E;
  
  if (strcmp(str, "stratified_bspline") == 0)
    return THE_STRATIFIED_BSPLINE_SAMPLING_METHOD_E;
  
  return THE_UNKNOWN_SAMPLING_METHOD_E;
}


//----------------------------------------------------------------
// the_sampling_method_t
// 
class the_sampling_method_t
{
public:
  the_sampling_method_t(const unsigned int & samples_u,
			const unsigned int & samples_v):
    samples_u_(samples_u),
    samples_v_(samples_v)
  {}
  
  virtual ~the_sampling_method_t()
  {}
  
  virtual float sample_u(const the_mersenne_twister_t & rng,
			 const unsigned int & u_index) const = 0;
  virtual float sample_v(const the_mersenne_twister_t & rng,
			 const unsigned int & v_index) const = 0;
  
  // accessors:
  inline const unsigned int & samples_u() const
  { return samples_u_; }
  
  inline const unsigned int & samples_v() const
  { return samples_v_; }
  
protected:
  const unsigned int samples_u_;
  const unsigned int samples_v_;
};

//----------------------------------------------------------------
// the_regular_sampling_method_t
// 
class the_regular_sampling_method_t : public the_sampling_method_t
{
public:
  the_regular_sampling_method_t(const unsigned int & samples_u,
				const unsigned int & samples_v):
    the_sampling_method_t(samples_u, samples_v)
  {}
  
  // virtual:
  float sample_u(const the_mersenne_twister_t & /* rng */,
		 const unsigned int & u_index) const
  { return (0.5 + float(u_index)) / float(samples_u_); }
  
  float sample_v(const the_mersenne_twister_t & /* rng */,
		 const unsigned int & v_index) const
  { return (0.5 + float(v_index)) / float(samples_v_); }
};

//----------------------------------------------------------------
// the_jittered_sampling_method_t
// 
class the_jittered_sampling_method_t : public the_sampling_method_t
{
public:
  the_jittered_sampling_method_t(const unsigned int & samples_u,
				 const unsigned int & samples_v):
    the_sampling_method_t(samples_u, samples_v)
  {}
  
  // virtual:
  float sample_u(const the_mersenne_twister_t & rng,
		 const unsigned int & u_index) const
  { return (rng.genrand_real2() + float(u_index)) / float(samples_u_); }
  
  float sample_v(const the_mersenne_twister_t & rng,
		 const unsigned int & v_index) const
  { return (rng.genrand_real2() + float(v_index)) / float(samples_v_); }
};

//----------------------------------------------------------------
// the_random_sampling_method_t
// 
class the_random_sampling_method_t : public the_sampling_method_t
{
public:
  the_random_sampling_method_t(const unsigned int & samples_u,
			       const unsigned int & samples_v):
    the_sampling_method_t(samples_u, samples_v)
  {}
  
  // virtual:
  float sample_u(const the_mersenne_twister_t & rng,
		 const unsigned int & /* u_index */) const
  { return rng.genrand_real2(); }
  
  float sample_v(const the_mersenne_twister_t & rng,
		 const unsigned int & /* v_index */) const
  { return rng.genrand_real2(); }
};

//----------------------------------------------------------------
// the_bspline_sampling_method_t
// 
class the_bspline_sampling_method_t : public the_sampling_method_t
{
public:
  the_bspline_sampling_method_t(const unsigned int & samples_u,
				const unsigned int & samples_v):
    the_sampling_method_t(samples_u, samples_v)
  {}
  
  // virtual:
  float sample_u(const the_mersenne_twister_t & rng,
		 const unsigned int & /* u_index */) const
  { return 0.5 + rnd_B3_PDF(rng); }
  
  float sample_v(const the_mersenne_twister_t & rng,
		 const unsigned int & /* v_index */) const
  { return 0.5 + rnd_B3_PDF(rng); }
  
private:
  // generate a random number with unstratified probability density function
  // (PDF) matching cubic b-spline basis function:
  inline float rnd_B3_PDF(const the_mersenne_twister_t & rng) const
  {
    return (-3.0 / 2.0 +
	    rng.genrand_real2() +
	    rng.genrand_real2() +
	    rng.genrand_real2() +
	    rng.genrand_real2());
  }
};

//----------------------------------------------------------------
// the_stratified_bspline_sampling_method_t
// 
class the_stratified_bspline_sampling_method_t : public the_sampling_method_t
{
public:
  the_stratified_bspline_sampling_method_t(const unsigned int & samples_u,
					   const unsigned int & samples_v):
    the_sampling_method_t(samples_u, samples_v)
  {}
  
  // virtual:
  float sample_u(const the_mersenne_twister_t & rng,
		 const unsigned int & /* u_index */) const
  { return 0.5 + rnd_B3_PDF_stratified(rng); }
  
  float sample_v(const the_mersenne_twister_t & rng,
		 const unsigned int & /* v_index */) const
  { return 0.5 + rnd_B3_PDF_stratified(rng); }
  
private:
  // helper:
  static float distb1(const float & r)
  {
    float u = r;
    for (unsigned int i = 0; i < 5; i++)
    {
      u = ((11.0 * r + u * u * (6.0 + u * (8.0 - 9.0 * u))) /
	   (4.0 + 12.0 * u * (1.0 + u * (1.0 - u))));
    }
    
    return u;
  }
  
  // helper:
  inline float D(const float & r) const
  {
    static const float r1 = 1.0 / 24.0;
    static const float r2 = 1.0 - r1;
    static const float s = 24.0 / 11.0;
    
    if (r <  r1) return (pow(24.0 * r, 0.25) - 2.0);
    if (r >= r2) return (2.0 - pow(24.0 * (1.0 - r), 0.25));
    if (r < 0.5) return (distb1(s * (r - r1)) - 1.0);
    return (1.0 - distb1(s * (r2 - r)));
  }
  
  // generate a random number with stratified probability density function
  // (PDF) matching cubic b-spline basis function:
  inline float rnd_B3_PDF_stratified(const the_mersenne_twister_t & rng) const
  { return D(rng.genrand_real2()); }
};


#endif // THE_SAMPLING_METHOD_HXX_

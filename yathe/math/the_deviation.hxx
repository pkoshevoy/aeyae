// File         : the_deviation.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jun 14 11:46:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : Helper classes used to find closest points between objects.

#ifndef THE_DEVIATION_HXX_
#define THE_DEVIATION_HXX_

// system includes:
#include <list>
#include <math.h>

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_view_volume.hxx"
#include "math/the_ray.hxx"


//----------------------------------------------------------------
// the_slope_sign_t
// 
typedef the_duplet_t<float> the_slope_sign_t;


//----------------------------------------------------------------
// the_deviation_min_t
// 
// This class represents a local minimum of the deviation curve,
// where s is the parameter (abscissa, the x coordinate),
// and r is the deviation (ordinate, the y coordinate).
// 
class the_deviation_min_t
{
public:
  the_deviation_min_t(const float & s = FLT_MAX,
		      const float & r = FLT_MAX):
    s_(s),
    r_(r)
  {}
  
  // comparison operator:
  inline bool operator == (const the_deviation_min_t & deviation) const
  { return ((s_ == deviation.s_) && (r_ == deviation.r_)); }
  
  inline bool operator < (const the_deviation_min_t & deviation) const
  { return r_ < deviation.r_; }
  
  // datamembers:
  float s_; // abscissa, x-coordinate of the deviation curve
  float r_; // ordinate, y-coordinate, the deviation
};


//----------------------------------------------------------------
// the_deviation_t
// 
// Base class for a parametric 2D curve representing deviation/proximity
// between some 3D objects. Finding the minima of this curve is equivalent
// to finding the closest points between the 3D objects.
// 
// FIXME: this class should be rewritten -- 2006/04/30
// 
class the_deviation_t
{
public:
  virtual ~the_deviation_t() {}
  
  // the derived class will have a better idea where to sample the curve
  // initialy, therefore this function has to be implemented for each
  // derived class; the function returns the number of segments in the
  // domain of the deviation curve that may contain the curve minima;
  // s0 and s1 are the start/end parameters of the curve:
  virtual unsigned int
  init_slope_signs(const unsigned int & steps_per_segment,
		   std::list<the_slope_sign_t> & ss,
		   float & s0,
		   float & s1) const = 0;
  
  // Z(s) - distance squared between the objects,
  // dZ(s)/ds - derivative of Z(s):
  virtual void eval_Z(const float & s, float & Z, float & dZ) const = 0;
  
  // R(s) - distance between the objects:
  // dR(s)/ds - derivative of R(s):
  inline void eval_R(const float & s, float & R, float & dR) const
  {
    eval_Z(s, R, dR);
    R = sqrt(R);
    dR /= (2.0 * sqrt(R));
  }
  
  inline float R(const float & s) const
  {
    float Z;
    float dZ;
    eval_Z(s, Z, dZ);
    return sqrt(Z);
  }
  
  // find all minima points of the volume-curve deviation function and
  // put them into a list as pairs "s, r(s)":
  bool
  find_local_minima(std::list<the_deviation_min_t> & solution,
		    const unsigned int & steps_per_segment = 10) const;
  
  void
  store_slope_sign(std::list<the_slope_sign_t> & slope_signs,
		   const float & s) const;
  
  bool
  isolate_minima(the_slope_sign_t & a,
		 the_slope_sign_t & b,
		 the_slope_sign_t & c,
		 const float & tolerance) const;
};

//----------------------------------------------------------------
// the_volume_ray_deviation_t
// 
// R(s): the deviation/proximity curve of a given ray P(s) to the
//       axis of the given volume Q(t), expressed within the coordinate
//       system of the volume.
// 
// P(s):    parametric ray, s is an independent variable on [0, 1] domain.
// Q(t(s)): parametric volume, t is a function of s.
// 
class the_volume_ray_deviation_t : public the_deviation_t
{
private:
  // disable default constructor:
  the_volume_ray_deviation_t();
  
public:
  the_volume_ray_deviation_t(const the_view_volume_t & Q,
			     const the_ray_t & P):
    P_(P),
    Q_(Q)
  {}
  
  // virtual:
  unsigned int init_slope_signs(const unsigned int & /* steps_per_segment */,
				std::list<the_slope_sign_t> & ss,
				float & s0,
				float & s1) const;
  
  // P(s), dP(s)/ds:
  inline void eval_P(const float & s, p3x1_t & P, v3x1_t & dP) const
  { P_.position_and_derivative(s, P, dP); }
  
  // virtual:
  void eval_Z(const float & s, float & Z, float & dZ) const
  {
    p3x1_t P;
    v3x1_t dP;
    eval_P(s, P, dP);

    float t;
    float dt;
    {
      const v3x1_t & w_axis = Q_.w_axis_orthogonal();
      const p3x1_t & origin = Q_.origin()[0];
      float w_axis_norm_sqrd = w_axis * w_axis;
      
      t = (w_axis * (P - origin)) / w_axis_norm_sqrd;
      dt = (w_axis * dP) / w_axis_norm_sqrd;
    }
    
    v3x1_t U;
    v3x1_t dU;
    {
      const v3x1_t * u = Q_.u_axis();
      v3x1_t u10 = u[1] - u[0];
      
      U = u[0] + t * u10;
      dU = dt * u10;
    }
    
    v3x1_t V;
    v3x1_t dV;
    {
      const v3x1_t * v = Q_.v_axis();
      v3x1_t v10 = v[1] - v[0];
      
      V = v[0] + t * v10;
      dV = dt * v10;
    }
    
    v3x1_t D;
    v3x1_t dD;
    {
      const p3x1_t * origin = Q_.origin();
      v3x1_t o10 = origin[1] - origin[0];
      
      p3x1_t O = origin[0] + t * o10;
      v3x1_t dO = dt * o10;
      
      D = P - O;
      dD = dP - dO;
    }
    
    float E = D * U;
    float F = D * V;
    float dE = D * dU + dD * U;
    float dF = D * dV + dD * V;
    
    float G = U * U;
    float H = V * V;
    float dG = 2.0 * U * dU;
    float dH = 2.0 * V * dV;
    
    float X = E / G;
    float Y = F / H;
    float dX = (G * dE - dG * E) / (G * G);
    float dY = (H * dF - dH * F) / (H * H);
    
    Z = X * X + Y * Y;
    dZ = 2.0 * (X * dX + Y * dY);
  }
  
protected:
  // data members:
  const the_ray_t &         P_;
  const the_view_volume_t & Q_;
};


#endif // THE_DEVIATION_HXX_

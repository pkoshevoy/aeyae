// File         : the_curve.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Oct 31 16:33:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

#ifndef THE_CURVE_HXX_
#define THE_CURVE_HXX_

// system includes:
#include <list>
#include <assert.h>

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_view_volume.hxx"
#include "math/the_color.hxx"
#include "math/the_deviation.hxx"
#include "geom/the_point.hxx"
#include "opengl/the_disp_list.hxx"
#include "opengl/the_appearance.hxx"
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "sel/the_pick_filter.hxx"
#include "utils/the_utils.hxx"

// forward declarations:
class the_bbox_t;
class the_curve_deviation_t;


//----------------------------------------------------------------
// the_curve_geom_t
// 
// Abstract base class for paramentic curves (Bezier, BSpline, etc...)
// 
class the_curve_geom_t
{
public:
  virtual ~the_curve_geom_t() {}
  
  // evaluate this curve properties at a given parameter:
  virtual bool eval(const float & t,
		    p3x1_t & P0, // position
		    v3x1_t & P1, // first derivative
		    v3x1_t & P2, // second derivative
		    float & curvature,
		    float & torsion) const = 0;
  
  // override this for optimization - write a stripped down version of eval:
  virtual bool position(const float & t, p3x1_t & position) const
  {
    v3x1_t P1; // first derivative
    v3x1_t P2; // second derivative
    float curvature;
    float torsion;
    return eval(t, position, P1, P2, curvature, torsion);
  }
  
  // override this for optimization - write a stripped down version of eval:
  virtual bool derivative(const float & t, v3x1_t & derivative) const
  {
    p3x1_t P0; // position
    v3x1_t P2; // second derivative
    float curvature;
    float torsion;
    return eval(t, P0, derivative, P2, curvature, torsion);
  }
  
  // override this for optimization - write a stripped down version of eval:
  virtual bool position_and_derivative(const float & t,
				       p3x1_t & position,
				       v3x1_t & derivative) const
  {
    v3x1_t P2; // second derivative
    float curvature;
    float torsion;
    return eval(t, position, derivative, P2, curvature, torsion);
  }
  
  // returns number of segments:
  virtual unsigned int
  init_slope_signs(const the_curve_deviation_t & deviation,
		   const unsigned int & steps_per_segment,
		   std::list<the_slope_sign_t> & ss,
		   float & s0,
		   float & s1) const = 0;
  
  virtual void calc_bbox(the_bbox_t & bbox) const = 0;
  
  // return the parameter range [t_min, t_min] on which this curve can be
  // evaluated:
  virtual float t_min() const = 0;
  virtual float t_max() const = 0;
  
  // find the parameters of closest points on this curve to a given point:
  virtual bool intersect(const p3x1_t & point,
			 std::list<the_deviation_min_t> & s_rs) const;
  
  // find the parameters of closest points on this curve to the axis of the
  // given volume:
  virtual bool intersect(const the_view_volume_t & volume,
			 std::list<the_deviation_min_t> & s_rs) const;
};


//----------------------------------------------------------------
// the_curve_geom_dl_elem_t
// 
class the_curve_geom_dl_elem_t : public the_dl_elem_t
{
public:
  the_curve_geom_dl_elem_t(const the_curve_geom_t & geom,
			   const the_color_t & color,
			   const unsigned int & segments = 100);
  
  the_curve_geom_dl_elem_t(const the_curve_geom_t & geom,
			   const the_color_t & zebra_a,
			   const the_color_t & zebra_b,
			   const unsigned int & segments = 100);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  // disable default constructor:
  the_curve_geom_dl_elem_t();
  
  // a reference to the curve being drawn:
  const the_curve_geom_t & geom_;
  
  // number of segments approximating the curve:
  unsigned int segments_;
  
  // the segment colors:
  the_color_t zebra_[2];
};


//----------------------------------------------------------------
// the_curve_deviation_t
// 
class the_curve_deviation_t : public the_deviation_t
{
private:
  // disable default constructor:
  the_curve_deviation_t();
  
public:
  the_curve_deviation_t(const the_curve_geom_t & P): P_(P) {}
  
  inline void eval_P(const float & s, p3x1_t & P, v3x1_t & dP) const
  {
#ifndef NDEBUG
    bool ok =
#endif
      P_.position_and_derivative(s, P, dP);
    assert(ok);
  }
  
  // virtual:
  unsigned int init_slope_signs(const unsigned int & steps_per_segment,
				std::list<the_slope_sign_t> & ss,
				float & s0,
				float & s1) const
  { return P_.init_slope_signs(*this, steps_per_segment, ss, s0, s1); }
  
protected:
  // data members:
  const the_curve_geom_t & P_;
};

//----------------------------------------------------------------
// the_point_curve_deviation_t
// 
class the_point_curve_deviation_t : public the_curve_deviation_t
{
private:  
  // disable default constructor:
  the_point_curve_deviation_t();
  
public:
  the_point_curve_deviation_t(const p3x1_t & Q,
			      const the_curve_geom_t & P):
    the_curve_deviation_t(P),
    Q_(Q)
  {}
  
  // virtual:
  void eval_Z(const float & s, float & Z, float & dZ) const
  {
    p3x1_t P;
    v3x1_t dP;
    eval_P(s, P, dP);
    
    v3x1_t D = P - Q_;
    const v3x1_t & dD = dP;
    
    Z = D * D;
    dZ = 2.0 * D * dD;
  }
  
protected:
  // data members:
  const p3x1_t & Q_;
};

//----------------------------------------------------------------
// the_volume_curve_deviation_t
// 
// R(s): the deviation/proximity curve of a given curve P(s) to the
//       axis of the given volume Q(t), expressed within the coordinate
//       system of the volume.
// 
// P(s):    parametric curve, s is an independent variable on [0, 1] domain.
// Q(t(s)): parametric volume, t is a function of s.
// 
class the_volume_curve_deviation_t : public the_curve_deviation_t
{
private:  
  // disable default constructor:
  the_volume_curve_deviation_t();
  
public:
  the_volume_curve_deviation_t(const the_view_volume_t & Q,
			       const the_curve_geom_t & P):
    the_curve_deviation_t(P),
    Q_(Q)
  {}
  
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
      const p3x1_t * o = Q_.origin();
      v3x1_t o10 = o[1] - o[0];
      
      p3x1_t O = o[0] + t * o10;
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
  const the_view_volume_t & Q_;
};


//----------------------------------------------------------------
// the_curve_t
// 
class the_curve_t : public the_primitive_t
{
public:
  virtual const the_curve_geom_t & geom() const = 0;
  
  // virtual: this is used during intersection/proximity testing:
  bool intersect(const the_view_volume_t & volume,
		 std::list<the_pick_data_t> & data) const;
  
  // virtual: display a piecewise linear approximation of the curve:
  the_dl_elem_t * dl_elem() const
  { return new the_curve_geom_dl_elem_t(geom(), color()); }
  
  // virtual: color of the model primitive:
  the_color_t color() const
  { return THE_APPEARANCE.palette().curve()[current_state()]; }
};


//----------------------------------------------------------------
// the_curve_ref_t
// 
// Reference to a Rational Bezier curve at a specified parameter value:
// 
class the_curve_ref_t : public the_reference_t
{
public:
  the_curve_ref_t(const unsigned int & id, const float & param);
  
  // virtual: a method for cloning references (potential memory leak):
  the_curve_ref_t * clone() const
  { return new the_curve_ref_t(*this); }
  
  // virtual:
  const char * name() const
  { return "the_curve_ref_t"; }
  
  // virtual: calculate the 3D value of this reference:
  bool eval(the_registry_t * registry, p3x1_t & wcs_pt) const;
  
  // virtual: if possible, adjust this reference:
  bool move(the_registry_t * registry,
	    const the_view_mgr_t & view_mgr,
	    const p3x1_t & wcs_pt);
  
  // virtual: Equality test:
  bool equal(const the_reference_t * ref) const;
  
  // virtual:
  the_point_symbol_id_t symbol() const
  { return THE_CIRCLE_SYMBOL_E; }
  
  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);
  
  // virtual: For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // accessors:
  inline const float & param() const
  { return param_; }
  
  inline void set_param(const float & new_param)
  { param_ = new_param; }
  
protected:
  the_curve_ref_t();
  
  // the parameter at which the curve is being referenced
  float param_;
};


//----------------------------------------------------------------
// the_curve_pick_filter_t
// 
class the_curve_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_primitive_t * primitive = registry->elem(id);
    the_curve_t * curve = dynamic_cast<the_curve_t *>(primitive);
    
    return (curve != NULL);
  }
};


//----------------------------------------------------------------
// the_knot_point_t
// 
// Knot point, break point, interpolation point:
// 
class the_knot_point_t
{
public:
  the_knot_point_t(const unsigned int & id = UINT_MAX,
		   const float & param = FLT_MAX):
    id_(id),
    param_(param)
  {}
  
  inline bool operator == (const the_knot_point_t & kp) const
  { return ((param_ == kp.param_) || (id_ == kp.id_)); }
  
  inline bool operator < (const the_knot_point_t & kp) const
  { return (param_ < kp.param_); }
  
  // id of the point primitive:
  unsigned int id_;
  
  // parameter associated with the point primitive:
  float param_;
};

inline ostream &
operator << (ostream & s, const the_knot_point_t & kp)
{
  s << kp.id_ << ':' << kp.param_;
  return s;
}


//----------------------------------------------------------------
// the_intcurve_t
// 
// Interpolation curve
// 
class the_intcurve_t : public the_curve_t
{
public:
  // virtual:
  const char * name() const
  { return "interpolation curve"; }
  
  // insert a point in the list (find the closest point on the curve
  // and decide where it belongs among the other points):
  bool insert(const unsigned int & id);
  
  // add a point to the head of the list:
  bool add_head(const unsigned int & id);
  
  // add a point to the tail of the list:
  bool add_tail(const unsigned int & id);
  
  // add a point to the list (pick the closest end of the curve
  // and add either to the tail or the head):
  bool add(const unsigned int & id);
  
  // delete a point from this curve:
  bool del(const unsigned int & id);
  
  // adjacent points that are too close to each other will be thinned
  // out by throwing out the duplicates until only one remains.
  // Too close means that the squared distance between the points is
  // less then or equal to the threshold:
  void remove_duplicates(const float & threshold = THE_EPSILON);
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // accessors:
  inline const std::list<the_knot_point_t> & pts() const { return pts_; }
  inline       std::list<the_knot_point_t> & pts()       { return pts_; }
  
  // helpers:
  inline the_point_t * point(const unsigned int & id) const
  { return dynamic_cast<the_point_t *>(primitive(id)); }
  
  inline bool has_at_least_two_points() const
  { return is_size_two_or_larger(pts_); }
  
protected:
  // update the parameters associated with the interpolation points;
  // the default implementation will setup a normalized chord-length
  // parameterization:
  virtual void setup_parameterization();
  
  // lookup the segment corresponding to a given parameter,
  // return UINT_MAX on failure:
  unsigned int segment(const float & param) const;
  
  // get the 3D point values for each point primitive:
  void point_values(std::vector<p3x1_t> & wcs_pts,
		    std::vector<float> & weights,
		    std::vector<float> & params) const;
  
  // check whether this curve already has a point at the given location:
  bool has_point_value(const p3x1_t & wcs_pt) const;
  
  // list of interpolation points defining this curve:
  std::list<the_knot_point_t> pts_;
};


//----------------------------------------------------------------
// the_intcurve_pick_filter_t
// 
class the_intcurve_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_primitive_t * primitive = registry->elem(id);
    the_intcurve_t * curve = dynamic_cast<the_intcurve_t *>(primitive);
    
    return (curve != NULL);
  }
};


#endif // THE_CURVE_HXX_

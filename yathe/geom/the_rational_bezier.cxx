// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_rational_bezier.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sat Oct 30 16:20:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A rational Bezier curve.

// local includes:
#include "geom/the_rational_bezier.hxx"
#include "geom/the_polyline.hxx"
#include "geom/the_point.hxx"
#include "opengl/the_appearance.hxx"
#include "opengl/the_view_mgr.hxx"
#include "doc/the_reference.hxx"
#include "math/the_bbox.hxx"
#include "utils/the_fifo.hxx"

// system includes:
#include <assert.h>


//----------------------------------------------------------------
// the_rational_bezier_geom_t::reset
//
void
the_rational_bezier_geom_t::reset(const std::vector<p3x1_t> & pts,
				  const std::vector<float> & wts)
{
  pt_ = pts;
  wt_ = wts;
}

//----------------------------------------------------------------
// the_rational_bezier_geom_t::eval
//
// This implementation is based on the work by M. S. Floater
// "Derivatives of rational Bezier curves", published in
// Computer Aided Geometric Design 9 (1992) pages 161-174.
//
bool
the_rational_bezier_geom_t::eval(const float & t,
				 p3x1_t & P0, // position
				 v3x1_t & P1, // first derivative
				 v3x1_t & P2, // second derivative
				 float & curvature,
				 float & torsion) const
{
  // shortcut:
  const size_t & num_pts = pt_.size();
  if (num_pts == 0) return false;

  const float n = float(num_pts);
  const float t1 = 1 - t;

  the_fifo_t< std::vector<p3x1_t> > p(4);
  the_fifo_t< std::vector<float> > w(4);

  // initialize the first element in the fifo:
  p.shift();
  w.shift();
  p[0] = pt_;
  w[0] = wt_;

  for (size_t i = 1; i < num_pts; i++)
  {
    // shift the fifo (1st becomes 2nd):
    p.shift();
    w.shift();

    const std::vector<p3x1_t> & p0 = p[1];
    const std::vector<float> & w0 = w[1];

    std::vector<p3x1_t> & p1 = p[0];
    std::vector<float> & w1 = w[0];

    size_t n1 = p0.size() - 1;
    p1.resize(n1);
    w1.resize(n1);

    for (size_t i = 0; i < n1; i++)
    {
      // de Casteljau algorithm for Rational Bezier:
      float wa = w0[i] * t1;
      float wb = w0[i + 1] * t;
      w1[i] = wa + wb;
      p1[i] = (wa * p0[i] + wb * p0[i + 1]) / w1[i];
    }
  }

  // calculate the position:
  const float & w00 = w[0][0];
  const p3x1_t & p00 = p[0][0];

  P0 = p00;

  if (num_pts == 1)
  {
    P1.assign(0, 0, 0);
    P2.assign(0, 0, 0);
    curvature = 0;
    torsion = 0;
    return true;
  }

  // calculate the tangent:
  const float & w01 = w[1][0];
  const float & w11 = w[1][1];

  const p3x1_t & p01 = p[1][0];
  const p3x1_t & p11 = p[1][1];

  const v3x1_t p11_p01 = p11 - p01;

  P1 = (n * ((w01 * w11) / (w00 * w00))) * p11_p01;

  if (num_pts == 2)
  {
    P2.assign(0, 0, 0);
    curvature = 0;
    torsion = 0;
    return true;
  }

  // calculate the normal:
  const float & w02 = w[2][0];
  const float & w12 = w[2][1];
  const float & w22 = w[2][2];

  const p3x1_t & p02 = p[2][0];
  const p3x1_t & p12 = p[2][1];
  const p3x1_t & p22 = p[2][2];

  const v3x1_t p22_p12 = p22 - p12;
  const v3x1_t p12_p02 = p12 - p02;

  const float n1 = n - 1;

  P2 =
    (n / (w00 * w00 * w00)) *
    ((w22 * (2 * w01 * (n * w01 - w00) - n1 * w02 * w00)) * p22_p12 -
     (w02 * (2 * w11 * (n * w11 - w00) - n1 * w22 * w00)) * p12_p02);

  // calculate the curvature:
  const v3x1_t x1202_2212(p12_p02 % p22_p12);
  const float d1101_1101 = p11_p01 * p11_p01;
  const float d1202x2212_1202x2212 = x1202_2212 * x1202_2212;

  const float R1 =
    (w02 * w12 * w22 * w00 * w00 * w00) /
    (w01 * w01 * w01 * w11 * w11 * w11);

  curvature =
    R1 * ((n1 * sqrt(d1202x2212_1202x2212)) /
	  (n * d1101_1101 * sqrt(d1101_1101)));

  if (num_pts == 3)
  {
    torsion = 0;
    return true;
  }

  // calculate the torsion:
  const float & w03 = w[3][0];
  const float & w13 = w[3][1];
  const float & w23 = w[3][2];
  const float & w33 = w[3][3];

  const p3x1_t & p03 = p[3][0];
  const p3x1_t & p13 = p[3][1];
  const p3x1_t & p23 = p[3][2];
  const p3x1_t & p33 = p[3][3];

  const float n2 = n - 2;

  const float R2 =
    (w03 * w13 * w23 * w33 * w00 * w00) /
    (w02 * w02 * w12 * w12 * w22 * w22);

  torsion =
    R2 * ((n2 * (((p13 - p03) % (p23 - p13)) * (p33 - p23))) /
	  (n * d1202x2212_1202x2212));

  return true;
}


//----------------------------------------------------------------
// the_rational_bezier_geom_t::init_slope_signs
//
size_t
the_rational_bezier_geom_t::
init_slope_signs(const the_curve_deviation_t & deviation,
		 const size_t & steps_per_segment,
		 std::list<the_slope_sign_t> & slope_signs,
		 float & s0,
		 float & s1) const
{
  if (pt_.size() < 2) return 0;

  // There can be at most 2 * degree minima:
  s0 = 0.0;
  s1 = 1.0;

  deviation.store_slope_sign(slope_signs, s0);

  size_t segments = 2 * (pt_.size() - 1);
  for (size_t i = 0; i < segments; i++)
  {
    const float k[] =
    {
      float(i) / float(segments),
      float(i + 1) / float(segments)
    };

    for (size_t j = 0; j < steps_per_segment; j++)
    {
      float s =
	k[0] + (((0.5f + float(j)) / float(steps_per_segment)) *
		(k[1] - k[0]));
      deviation.store_slope_sign(slope_signs, s);
    }
  }

  deviation.store_slope_sign(slope_signs, s1);
  return segments;
}

//----------------------------------------------------------------
// the_rational_bezier_geom_t::calc_bbox
//
void
the_rational_bezier_geom_t::calc_bbox(the_bbox_t & bbox) const
{
  const size_t & num_pts = pt_.size();
  for (size_t i = 0; i < num_pts; i++)
  {
    bbox << pt_[i];
  }
}


//----------------------------------------------------------------
// the_rational_bezier_t::regenerate
//
bool
the_rational_bezier_t::regenerate()
{
  const the_polyline_t * p = polyline();
  if (p == NULL) return false;

  const the_polyline_geom_t & geom = p->geom();
  if (geom.pt().size() < 2) return false;

  geom_.reset(geom.pt(), geom.wt());
  return true;
}

//----------------------------------------------------------------
// the_rational_bezier_t::dump
//
void
the_rational_bezier_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_rational_bezier_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_primitive_t::dump(strm, INDNXT);
  strm << INDSTR << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_rational_bezier_t::polyline
//
the_polyline_t *
the_rational_bezier_t::polyline() const
{
  if (direct_supporters().empty()) return NULL;
  if (registry() == NULL) return NULL;

  const unsigned int & polyline_id = *(direct_supporters().rbegin());
  the_polyline_t * p =
    dynamic_cast<the_polyline_t *>(registry()->elem(polyline_id));

  return p;
}

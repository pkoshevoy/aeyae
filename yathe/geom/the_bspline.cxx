// File         : the_bspline.cxx
// Author       : Paul A. Koshevoy
// Created      : Wed Nov 17 17:50:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

// local includes:
#include "geom/the_bspline.hxx"
#include "geom/the_polyline.hxx"
#include "geom/the_point.hxx"
#include "doc/the_reference.hxx"
#include "math/the_bbox.hxx"
#include "opengl/the_appearance.hxx"
#include "opengl/the_view_mgr.hxx"
#include "opengl/OpenGLCapabilities.h"
#include "utils/the_fifo.hxx"
#include "utils/the_domain_array.hxx"
#include "utils/the_utils.hxx"

// system includes:
#include <assert.h>


//----------------------------------------------------------------
// the_same
// 
// Check whether n consequtive values in a given array
// are the same:
// 
static bool
the_same(const float * a, const unsigned int & n, const float & tol = 1e-6)
{
  for (unsigned int i = 1; i < n; i++)
  {
    float d = fabs(a[i - 1] - a[i]);
    if (d > tol) return false;
  }
  
  return true;
}

//----------------------------------------------------------------
// copy
// 
inline static void
copy(const std::vector<float> & a, // array to copy from
     const unsigned int & ai,       // where to copy from in array a
     std::vector<float> & b,       // array to copy to
     const unsigned int & bi,       // where to copy to in array b
     const unsigned int & n)        // how many items should be copied
{ for (unsigned int i = 0; i < n; i++) b[bi + i] = a[ai + i]; }

//----------------------------------------------------------------
// uniform
// 
inline static void
uniform(std::vector<float> & T,
	const float & a,
	const unsigned int & ai,
	const float & b,
	const unsigned int & bi)
{
  float n = float(bi - ai);
  
  for (unsigned int i = ai; i <= bi; i++)
  {
    float t = a + (b - a) * (float(i - ai) / n);
    T[i] = t;
  }
}

//----------------------------------------------------------------
// fill
// 
inline static void
fill(std::vector<float> & T,
     const float & a,
     const unsigned int & ai,
     const unsigned int & n)
{
  for (unsigned int i = 0; i < n; i++) T[ai + i] = a;
}


//----------------------------------------------------------------
// the_bspline_geom_t::reset
// 
void
the_bspline_geom_t::reset(const std::vector<p3x1_t> & pts,
			  const std::vector<float> & kts)
{
  pt_ = pts;
  kt_ = kts;
}

//----------------------------------------------------------------
// the_bspline_geom_t::eval
// 
bool
the_bspline_geom_t::eval(const float & t,
			 p3x1_t & P0, // position
			 v3x1_t & P1, // first derivative
			 v3x1_t & P2, // second derivative
			 float & curvature,
			 float & torsion) const
{
  const unsigned int J = find_segment_index(t);
  if (J == UINT_MAX) return false;
  
  unsigned int K = degree();
  assert(K <= J);
  
  // initialize Bi,k:
  the_fifo_t< std::vector<float> > B(K + 1);
  const float * T = &(kt_[0]);
  
  // B,J,0:
  B.shift();
  B[0].resize(1);
  B[0][0] = 1.0;
  
  // k = 1, ... K:
  for (unsigned int k = 1; k <= K; k++)
  {
    // shift the fifo (1st becomes 2nd):
    B.shift();
    const std::vector<float> & B0 = B[1]; // Bi,k-1
    std::vector<float> &       Bk = B[0]; // Bi,k
    Bk.resize(k + 1);
    
    // NOTE: page 184, Elaine Cohen "Geometric Modeling with Splines":
    // if (Ti >= Ti+1+k) then Bi,k = 0.0;
    
    // B,J-k,k:
    if (T[J - k + 1] < T[J + 1])
    {
      Bk[0] = B0[0] * ((T[J + 1] - t) /
		       (T[J + 1] - T[J - k + 1]));
    }
    else
    {
      Bk[0] = 0.0;
    }
    
    // B,J,k:
    if (T[J] < T[J + k])
    {
      Bk[k] = B0[k - 1] * ((t - T[J]) /
			   (T[J + k] - T[J]));
    }
    else
    {
      Bk[k] = 0.0;
    }
    
    for (unsigned int i = 1; i < k; i++)
    {
      // B,J-k+i,k:
      Bk[i] = 0.0;
      
      if (T[J - k + i] < T[J + i])
      {
	Bk[i] = B0[i - 1] * ((t - T[J - k + i]) /
			     (T[J + i] - T[J - k + i]));
      }
      
      if (T[J - k + i + 1] < T[J + i + 1])
      {
	Bk[i] += B0[i] * ((T[J + i + 1] - t) /
			 (T[J + i + 1] - T[J - k + i + 1]));
      }
    }
  }
  
  // evaluate the curve:
  const std::vector<float> & Bk = B[0]; // Bi,k
  const p3x1_t * P = &(pt_[J - K]);
  
  // FIXME: unsigned int offset = J - K;
  K = std::min(K, (unsigned int)(pt_.size() - J + K - 1));
  
  P0.assign(0.0, 0.0, 0.0);
  for (unsigned int m = 0; m <= K; m++)
  {
    P0 += P[m] * Bk[m];
  }
  
  // evaluate the first 3 derivatives:
  v3x1_t derivative[] =
    {
      v3x1_t(0.0, 0.0, 0.0),
      v3x1_t(0.0, 0.0, 0.0),
      v3x1_t(0.0, 0.0, 0.0)
    };
  
  for (unsigned int d = 1; d <= 3 && d <= K; d++)
  {
    the_fifo_t< std::vector<v3x1_t> > Q(2);
    Q.shift();
    
    Q[0].resize(K + 1);
    for (unsigned int m = 0; m <= K; m++)
    {
      Q[0][m].assign(P[m].data());
    }
    
    for (unsigned int j = 1; j <= d; j++)
    {
      Q.shift();
      Q[0].resize(K + 1 - j);
      
      const float * S = &(T[J - K + j]);
      for (unsigned int m = 0; m <= K - j; m++)
      {
	Q[0][m] = (Q[1][m + 1] - Q[1][m]) / (S[m + K - j + 1] - S[m]);
      }
    }
    
    for (unsigned int m = 0; m <= K - d; m++)
    {
      derivative[d - 1] += Q[0][m] * B[d][m];
    }
    
    float scale = float(K);
    for (unsigned int j = 2; j <= d; j++)
    {
      scale *= float(K - j + 1);
    }
    
    derivative[d - 1] *= scale;
  }
  
  if (K < 1) P1.assign(1.0, 0.0, 0.0);
  else       P1 = derivative[0];
  
  if (K < 2) P2 = P1.normal();
  else       P2 = derivative[1];
  
  if (K >= 2)
  {
    const v3x1_t & b1 = derivative[0];
    const v3x1_t & b2 = derivative[1];
    
    v3x1_t b1xb2 = b1 % b2;
    float b1_n2 = b1.norm_sqrd();
    curvature = b1xb2.norm() / (b1_n2 * sqrt(b1_n2));
    
    if (K >= 3)
    {
      const v3x1_t & b3 = derivative[2];
      torsion = (b3 * b1xb2) / b1xb2.norm_sqrd();
    }
  }
  
  return true;
}

//----------------------------------------------------------------
// the_bspline_geom_t::position
// 
bool
the_bspline_geom_t::position(const float & t, p3x1_t & P0) const
{
  const unsigned int J = find_segment_index(t);
  if (J == UINT_MAX) return false;
  
  unsigned int K = degree();
  assert(K <= J);
  
  // initialize Bi,k:
  the_fifo_t< std::vector<float> > B(K + 1);
  const float * T = &(kt_[0]);
  
  // B,J,0:
  B.shift();
  B[0].resize(1);
  B[0][0] = 1.0;
  
  // k = 1, ... K:
  for (unsigned int k = 1; k <= K; k++)
  {
    // shift the fifo (1st becomes 2nd):
    B.shift();
    const std::vector<float> & B0 = B[1]; // Bi,k-1
    std::vector<float> &       Bk = B[0]; // Bi,k
    Bk.resize(k + 1);
    
    // NOTE: page 184, Elaine Cohen "Geometric Modeling with Splines":
    // if (Ti >= Ti+1+k) then Bi,k = 0.0;
    
    // B,J-k,k:
    if (T[J - k + 1] < T[J + 1])
    {
      Bk[0] = B0[0] * ((T[J + 1] - t) /
		       (T[J + 1] - T[J - k + 1]));
    }
    else
    {
      Bk[0] = 0.0;
    }
    
    // B,J,k:
    if (T[J] < T[J + k])
    {
      Bk[k] = B0[k - 1] * ((t - T[J]) /
			   (T[J + k] - T[J]));
    }
    else
    {
      Bk[k] = 0.0;
    }
    
    for (unsigned int i = 1; i < k; i++)
    {
      // B,J-k+i,k:
      Bk[i] = 0.0;
      
      if (T[J - k + i] < T[J + i])
      {
	Bk[i] = B0[i - 1] * ((t - T[J - k + i]) /
			     (T[J + i] - T[J - k + i]));
      }
      
      if (T[J - k + i + 1] < T[J + i + 1])
      {
	Bk[i] += B0[i] * ((T[J + i + 1] - t) /
			 (T[J + i + 1] - T[J - k + i + 1]));
      }
    }
  }
  
  // evaluate the curve:
  const std::vector<float> & Bk = B[0]; // Bi,k
  const p3x1_t * P = &(pt_[J - K]);
  
  // FIXME: unsigned int offset = J - K;
  K = std::min(K, (unsigned int)(pt_.size() - J + K - 1));
  
  P0.assign(0.0, 0.0, 0.0);
  for (unsigned int m = 0; m <= K; m++)
  {
    P0 += P[m] * Bk[m];
  }
  
  return true;
}

//----------------------------------------------------------------
// the_bspline_geom_t::derivative
// 
bool
the_bspline_geom_t::derivative(const float & t, v3x1_t & derivative) const
{
  const unsigned int J = find_segment_index(t);
  if (J == UINT_MAX) return false;
  
  unsigned int K = degree();
  assert(K <= J);
  
  // initialize Bi,k:
  the_fifo_t< std::vector<float> > B(K + 1);
  const float * T = &(kt_[0]);
  
  // B,J,0:
  B.shift();
  B[0].resize(1);
  B[0][0] = 1.0;
  
  // k = 1, ... K:
  for (unsigned int k = 1; k <= K; k++)
  {
    // shift the fifo (1st becomes 2nd):
    B.shift();
    const std::vector<float> & B0 = B[1]; // Bi,k-1
    std::vector<float> &       Bk = B[0]; // Bi,k
    Bk.resize(k + 1);
    
    // NOTE: page 184, Elaine Cohen "Geometric Modeling with Splines":
    // if (Ti >= Ti+1+k) then Bi,k = 0.0;
    
    // B,J-k,k:
    if (T[J - k + 1] < T[J + 1])
    {
      Bk[0] = B0[0] * ((T[J + 1] - t) /
		       (T[J + 1] - T[J - k + 1]));
    }
    else
    {
      Bk[0] = 0.0;
    }
    
    // B,J,k:
    if (T[J] < T[J + k])
    {
      Bk[k] = B0[k - 1] * ((t - T[J]) /
			   (T[J + k] - T[J]));
    }
    else
    {
      Bk[k] = 0.0;
    }
    
    for (unsigned int i = 1; i < k; i++)
    {
      // B,J-k+i,k:
      Bk[i] = 0.0;
      
      if (T[J - k + i] < T[J + i])
      {
	Bk[i] = B0[i - 1] * ((t - T[J - k + i]) /
			     (T[J + i] - T[J - k + i]));
      }
      
      if (T[J - k + i + 1] < T[J + i + 1])
      {
	Bk[i] += B0[i] * ((T[J + i + 1] - t) /
			 (T[J + i + 1] - T[J - k + i + 1]));
      }
    }
  }
  
  // FIXME: unsigned int offset = J - K;
  K = std::min(K, (unsigned int)(pt_.size() - J + K - 1));
  
  // evaluate the first derivative:
  if (K < 1)
  {
    derivative.assign(1.0, 0.0, 0.0);
    return true;
  }
  
  const p3x1_t * P = &(pt_[J - K]);
  
  the_fifo_t< std::vector<v3x1_t> > Q(2);
  Q.shift();
  
  Q[0].resize(K + 1);
  for (unsigned int m = 0; m <= K; m++)
  {
    Q[0][m].assign(P[m].data());
  }
  
  Q.shift();
  Q[0].resize(K);
  
  const float * S = &(T[J - K + 1]);
  for (unsigned int m = 0; m < K; m++)
  {
    Q[0][m] = (Q[1][m + 1] - Q[1][m]) / (S[m + K] - S[m]);
  }
  
  derivative.assign(0.0, 0.0, 0.0);
  for (unsigned int m = 0; m < K; m++)
  {
    derivative += Q[0][m] * B[1][m];
  }
  
  float scale = float(K);
  derivative *= scale;
  
  return true;
}


//----------------------------------------------------------------
// the_bspline_geom_t::position_and_derivative
// 
bool
the_bspline_geom_t::position_and_derivative(const float & t,
					    p3x1_t & position,
					    v3x1_t & derivative) const
{
  const unsigned int J = find_segment_index(t);
  if (J == UINT_MAX) return false;
  
  unsigned int K = degree();
  assert(K <= J);
  
  // initialize Bi,k:
  the_fifo_t< std::vector<float> > B(K + 1);
  const float * T = &(kt_[0]);
  
  // B,J,0:
  B.shift();
  B[0].resize(1);
  B[0][0] = 1.0;
  
  // k = 1, ... K:
  for (unsigned int k = 1; k <= K; k++)
  {
    // shift the fifo (1st becomes 2nd):
    B.shift();
    const std::vector<float> & B0 = B[1]; // Bi,k-1
    std::vector<float> &       Bk = B[0]; // Bi,k
    Bk.resize(k + 1);
    
    // NOTE: page 184, Elaine Cohen "Geometric Modeling with Splines":
    // if (Ti >= Ti+1+k) then Bi,k = 0.0;
    
    // B,J-k,k:
    if (T[J - k + 1] < T[J + 1])
    {
      Bk[0] = B0[0] * ((T[J + 1] - t) /
		       (T[J + 1] - T[J - k + 1]));
    }
    else
    {
      Bk[0] = 0.0;
    }
    
    // B,J,k:
    if (T[J] < T[J + k])
    {
      Bk[k] = B0[k - 1] * ((t - T[J]) /
			   (T[J + k] - T[J]));
    }
    else
    {
      Bk[k] = 0.0;
    }
    
    for (unsigned int i = 1; i < k; i++)
    {
      // B,J-k+i,k:
      Bk[i] = 0.0;
      
      if (T[J - k + i] < T[J + i])
      {
	Bk[i] = B0[i - 1] * ((t - T[J - k + i]) /
			     (T[J + i] - T[J - k + i]));
      }
      
      if (T[J - k + i + 1] < T[J + i + 1])
      {
	Bk[i] += B0[i] * ((T[J + i + 1] - t) /
			 (T[J + i + 1] - T[J - k + i + 1]));
      }
    }
  }
  
  // evaluate the curve:
  const std::vector<float> & Bk = B[0]; // Bi,k
  const p3x1_t * P = &(pt_[J - K]);
  
  // FIXME: unsigned int offset = J - K;
  K = std::min(K, (unsigned int)(pt_.size() - J + K - 1));
  
  // evaluate the position:
  position.assign(0.0, 0.0, 0.0);
  for (unsigned int m = 0; m <= K; m++)
  {
    position += P[m] * Bk[m];
  }
  
  // evaluate the derivative:
  if (K < 1)
  {
    derivative.assign(1.0, 0.0, 0.0);
    return true;
  }
  
  the_fifo_t< std::vector<v3x1_t> > Q(2);
  Q.shift();
  
  Q[0].resize(K + 1);
  for (unsigned int m = 0; m <= K; m++)
  {
    Q[0][m].assign(P[m].data());
  }
  
  Q.shift();
  Q[0].resize(K);
  
  const float * S = &(T[J - K + 1]);
  for (unsigned int m = 0; m < K; m++)
  {
    Q[0][m] = (Q[1][m + 1] - Q[1][m]) / (S[m + K] - S[m]);
  }
  
  derivative.assign(0.0, 0.0, 0.0);
  for (unsigned int m = 0; m < K; m++)
  {
    derivative += Q[0][m] * B[1][m];
  }
  
  float scale = float(K);
  derivative *= scale;
  
  return true;
}

//----------------------------------------------------------------
// the_bspline_geom_t::init_slope_signs
// 
unsigned int
the_bspline_geom_t::
init_slope_signs(const the_curve_deviation_t & deviation,
		 const unsigned int & steps_per_segment,
		 std::list<the_slope_sign_t> & slope_signs,
		 float & s0,
		 float & s1) const
{
  if (pt_.size() < 2) return 0;
  
  s0 = t_min();
  s1 = t_max();
  float ds = s1 - s0;
  
  deviation.store_slope_sign(slope_signs, s0);
  
  // There can be at most 2 * degree minima:
  unsigned int segments = 2 * degree() + 1;
  for (unsigned int i = 0; i < segments; i++)
  {
    const float k[] =
      {
	s0 + ds * (float(i) / float(segments)),
	s0 + ds * (float(i + 1) / float(segments))
      };
    
    for (unsigned int j = 0; j < steps_per_segment; j++)
    {
      float s =
	k[0] + (((0.5 + float(j)) / float(steps_per_segment)) *
		(k[1] - k[0]));
      deviation.store_slope_sign(slope_signs, s);
    }
  }
  
  deviation.store_slope_sign(slope_signs, s1);
  return segments;
}

//----------------------------------------------------------------
// the_bspline_geom_t::calc_bbox
// 
void
the_bspline_geom_t::calc_bbox(the_bbox_t & bbox) const
{
  const unsigned int & num_pts = pt_.size();
  for (unsigned int i = 0; i < num_pts; i++)
  {
    bbox << pt_[i];
  }
}

//----------------------------------------------------------------
// the_bspline_geom_t::t_min
// 
float
the_bspline_geom_t::t_min() const
{
  const unsigned int k = degree();
  return kt_[k];
}

//----------------------------------------------------------------
// the_bspline_geom_t::t_max
// 
float
the_bspline_geom_t::t_max() const
{
  const unsigned int k = degree();
  const unsigned int m = kt_.size();
  const unsigned int n = m - k - 2;

  float t0 = kt_[k];
  float t1 = kt_[n + 1];
  float d = fabs(kt_[m - 1] - t1);
  float e = (THE_EPSILON * (t1 - t0)) / 2.0;
  if (d <= e) t1 -= e;
  
  return t1;
}

//----------------------------------------------------------------
// the_bspline_geom_t::find_segment_index
// 
unsigned int
the_bspline_geom_t::find_segment_index(const float & t) const
{
  const unsigned int & m = kt_.size();
  
#if 0
  for (unsigned int i = 1; i < m; i++)
  {
    if (kt_[i - 1] <= t && t < kt_[i]) return i - 1;
  }
  
  return UINT_MAX;
#else
  // binary search:
  unsigned int a = 0;
  unsigned int b = m - 1;
  
  // check for out of bounds:
  if (kt_[a] > t) return UINT_MAX;
  if (kt_[b] < t) return UINT_MAX;
  
  // perform a binary search:
  while (b - a > 1)
  {
    unsigned int c = (a + b) / 2;
    if (kt_[c] <= t) a = c;
    else b = c;
  }
  assert(a != b);
  
  return a;
#endif
}


//----------------------------------------------------------------
// the_bspline_geom_dl_elem_t::the_bspline_geom_dl_elem_t
// 
the_bspline_geom_dl_elem_t::
the_bspline_geom_dl_elem_t(const the_bspline_geom_t & curve,
			   const the_color_t & color):
  geom_(curve),
  color_(color)
{}

//----------------------------------------------------------------
// the_bspline_geom_dl_elem_t::draw
// 
void
the_bspline_geom_dl_elem_t::draw() const
{
  GLUnurbsObj * glu_nurbs_obj = gluNewNurbsRenderer();
  if (glu_nurbs_obj == NULL) return;
  
  glPushAttrib(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    glColor4fv(color_.rgba());
    
    gluNurbsProperty(glu_nurbs_obj, GLU_AUTO_LOAD_MATRIX, GLU_TRUE);
    gluNurbsProperty(glu_nurbs_obj, GLU_CULLING, GLU_FALSE);
    gluNurbsProperty(glu_nurbs_obj, GLU_SAMPLING_METHOD, GLU_PATH_LENGTH);
    gluNurbsProperty(glu_nurbs_obj, GLU_SAMPLING_TOLERANCE, 16.0);
    
    gluBeginCurve(glu_nurbs_obj);
    gluNurbsCurve(glu_nurbs_obj,
		  geom_.kt().size(),
		  (float *)(&(geom_.kt()[0])),
		  sizeof(p3x1_t) / sizeof(float),
		  (float *)(&(geom_.pt()[0])),
		  geom_.order(),
		  GL_MAP1_VERTEX_3);
    gluEndCurve(glu_nurbs_obj);
  }
  glPopAttrib();
  
  gluDeleteNurbsRenderer(glu_nurbs_obj);
}

//----------------------------------------------------------------
// the_bspline_geom_dl_elem_t::update_bbox
// 
void
the_bspline_geom_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  geom_.calc_bbox(bbox);
}


//----------------------------------------------------------------
// the_knot_vector_t::init
// 
bool
the_knot_vector_t::init(const unsigned int & degree,
			const std::vector<float> & knots)
{
  const unsigned int k = degree;
  const unsigned int m = knots.size();
  const unsigned int n = m - 2 - k;
  
  if (k > n) return false;
  
  degree_ = k;
  knots_ = knots;
  request_regeneration();
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::init
// 
bool
the_knot_vector_t::init(const unsigned int & degree,
			const unsigned int & num_pt,
			const float & t0,
			const float & t1,
			const bool & a_floating,
			const bool & b_floating)
{
  const unsigned int k = degree;
  const unsigned int n = num_pt - 1;
  const unsigned int m = n + 2 + k;
  
  if (k > n) return false;
  
  const float dt = (t1 - t0) / float(n + 1 - k);
  knots_.resize(m);
  
  if (a_floating && !b_floating)
  {
    // make the head floating and tail open:
    uniform(knots_, t0 - dt * float(k), 0, t1, m - k - 1);
    fill(knots_, t1, m - k, k);
  }
  else if (b_floating && !a_floating)
  {
    // make the tail floating and head open:
    fill(knots_, t0, 0, k);
    uniform(knots_, t0, k, t1 + dt * float(k), m - 1);
  }
  else if (a_floating && b_floating)
  {
    // make both ends floating:
    uniform(knots_, t0, 0, t1, m - 1);
  }
  else
  {
    // make both ends open:
    fill(knots_, t0, 0, k);
    uniform(knots_, t0, k, t1, m - k - 1);
    if (k > 0) fill(knots_, t1, m - k, k - 1);
    knots_[m - 1] = t1 + THE_EPSILON * (t1 - t0);
  }
  
  degree_ = k;
  request_regeneration();
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::set_target_degree
// 
void
the_knot_vector_t::set_target_degree(const unsigned int & target_degree)
{
  target_degree_ = std::max(1u, target_degree);
  request_regeneration();
}

//----------------------------------------------------------------
// the_knot_vector_t::regenerate
// 
bool
the_knot_vector_t::regenerate()
{
  regenerated_ = false;
  
  const the_polyline_t * p = polyline();
  if (p == NULL) return false;
  if (!p->regenerated()) return false;
  
  regenerated_ = update(p->geom().pt().size());
  return regenerated_;
}

//----------------------------------------------------------------
// the_knot_vector_t::update
// 
bool
the_knot_vector_t::update(const unsigned int & polyline_pts)
{
  // adjust the knot vector in case the number of points in the
  // polyline has changed:
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  const unsigned int n = m - 2 - k;
  
  int dn = (n + 1) - polyline_pts;
  for (int i = dn; i < 0; i++)
  {
    if (!insert_point()) return false;
  }
  
  for (int i = 0; i < dn; i++)
  {
    if (!remove_point())
    {
      lower_degree();
      remove_point();
    }
  }
  
  // try to match the degree to the target:
  int dk = degree_ - target_degree_;
  for (int i = dk; i < 0; i++) raise_degree();
  for (int i = 0; i < dk; i++) lower_degree();
  
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::polyline
// 
the_polyline_t *
the_knot_vector_t::polyline() const
{
  if (direct_supporters().empty()) return NULL;
  if (registry() == NULL) return NULL;
  
  const unsigned int & polyline_id = direct_supporters().back();
  the_polyline_t * p =
    dynamic_cast<the_polyline_t *>(registry()->elem(polyline_id));
  
  return p;
}

//----------------------------------------------------------------
// the_knot_vector_t::dump
// 
void
the_knot_vector_t::dump(ostream & strm, unsigned int /* indent */) const
{
#if 0
  strm << INDSCP << "the_knot_vector_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_primitive_t::dump(strm, INDNXT);
  strm << INDSTR << "degree_: " << degree_ << endl
       << INDSTR << "knots_ : " << knots_ << endl
       << INDSCP << "}" << endl << endl;
#else
  
  // ios::fmtflags prev_flags = strm.flags();
  // strm.setf(ios::fixed);
  
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  const unsigned int n = m - 2 - k;
  strm << "id " << id() << endl
       << "k: " << k << endl
       << "m: " << m << endl
       << "n: " << n << endl;
  
  int prev_precision = strm.precision(3);
  int width = strm.precision() + 3;
  
  for (unsigned int i = 0; i < m; i++)
  {
    strm << setw(width) << knots_[i] << ' ';
  }
  strm << endl << endl;
  strm.precision(prev_precision);
#endif
}

//----------------------------------------------------------------
// the_knot_vector_t::raise_degree
// 
bool
the_knot_vector_t::raise_degree()
{
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  const unsigned int n = m - 2 - k;
  if (k >= n) return false;
  
  bool a_floating = !the_same(&(knots_[0]), k + 1);
  bool b_floating = !the_same(&(knots_[m - k - 1]), k);
  
  std::vector<float> T(m + 1);
  
  if (a_floating && !b_floating)
  {
    // keep the head floating and tail open:
    copy(knots_, 0, T, 0, m);
    T[m] = T[m - 1];
    T[m - 1] = T[m - 2];
  }
  else if (b_floating && !a_floating)
  {
    // keep the tail floating and head open:
    copy(knots_, 0, T, 1, m);
    T[0] = T[1];
  }
  else if (a_floating && b_floating)
  {
    // keep both ends floating:
    copy(knots_, 0, T, 0, m);
    T[m] = T[m - 1] + (T[m - 1] - T[m - 2]);
  }
  else
  {
    // keep both ends open:
    copy(knots_, 0, T, 0, k + 1);
    copy(knots_, m - (k + 1), T, m - k, k + 1);
    uniform(T, T[k], k + 1, T[m - k], m - (k + 1));
  }
  
  degree_++;
  knots_ = T;
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::lower_degree
// 
bool
the_knot_vector_t::lower_degree()
{
  if (degree_ == 0) return false;
  
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  
  bool a_floating = !the_same(&(knots_[0]), k + 1);
  bool b_floating = !the_same(&(knots_[m - k - 1]), k);
  
  std::vector<float> T(m - 1);
  
  if (a_floating && !b_floating)
  {
    // keep the head floating and tail open:
    copy(knots_, 0, T, 0, m - k - 1);
    copy(knots_, m - k, T, m - k - 1, k);
  }
  else if (b_floating && !a_floating)
  {
    // keep the tail floating and head open:
    copy(knots_, 1, T, 0, m - 1);
  }
  else if (a_floating && b_floating)
  {
    // keep both ends floating:
    copy(knots_, 0, T, 0, m - 1);
  }
  else
  {
    // keep both ends open:
    copy(knots_, 1, T, 0, k - 1);
    copy(knots_, m - (k - 1), T, m - k, k - 1);
    uniform(T, knots_[k], k - 1, knots_[m - k], m - k - 1);
  }
  
  degree_--;
  knots_ = T;
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::insert_point
// 
bool
the_knot_vector_t::insert_point()
{
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  
  bool a_floating = !the_same(&(knots_[0]), k + 1);
  bool b_floating = !the_same(&(knots_[m - k - 1]), k);
  
  std::vector<float> T(m + 1);
  
  if (a_floating)
  {
    // keep the head floating:
    copy(knots_, 0, T, 1, m);
    T[0] = T[1] + (T[1] - T[2]);
  }
  else if (b_floating)
  {
    // keep the tail floating:
    copy(knots_, 0, T, 0, m);
    T[m] = T[m - 1] + (T[m - 1] - T[m - 2]);
  }
  else
  {
    // keep both ends open:
    copy(knots_, 0, T, 0, k);
    copy(knots_, m - k, T, m - k + 1, k);
    uniform(T, knots_[k], k, knots_[m - k - 1], m - k);
  }
  
  knots_ = T;
  return true;
}

//----------------------------------------------------------------
// the_knot_vector_t::remove_point
// 
bool
the_knot_vector_t::remove_point()
{
  const unsigned int k = degree_;
  const unsigned int m = knots_.size();
  const unsigned int n = m - 2 - k;
  
  if (k >= n) return false;
  
  bool a_floating = !the_same(&(knots_[0]), k + 1);
  bool b_floating = !the_same(&(knots_[m - k - 1]), k);
  
  std::vector<float> T(m - 1);
  
  if (a_floating)
  {
    // keep the head floating:
    copy(knots_, 1, T, 0, m - 1);
  }
  else if (b_floating)
  {
    // keep the tail floating:
    copy(knots_, 0, T, 0, m - 1);
  }
  else
  {
    // keep both ends open:
    copy(knots_, 0, T, 0, k);
    copy(knots_, m - k, T, m - k - 1, k);
    uniform(T, knots_[k], k, knots_[m - k - 1], m - k - 2);
  }
  
  knots_ = T;
  return true;
}


//----------------------------------------------------------------
// the_bspline_t::regenerate
// 
bool
the_bspline_t::regenerate()
{
  regenerated_ = false;
  
  const the_knot_vector_t * v = knot_vector();
  if (v == NULL) return false;
  if (!v->regenerated()) return false;
  
  const the_polyline_t * p = v->polyline();
  if (p == NULL) return false;
  if (!p->regenerated()) return false;
  
  const the_polyline_geom_t & geom = p->geom();
  if (geom.pt().size() < 2) return false;
  
  geom_.reset(geom.pt(), v->knots());
  
  regenerated_ = true;
  return true;
}

//----------------------------------------------------------------
// the_bspline_t::dump
// 
void
the_bspline_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_bspline_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_primitive_t::dump(strm, INDNXT);
  strm << INDSTR << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_bspline_t::knot_vector
// 
the_knot_vector_t *
the_bspline_t::knot_vector() const
{
  if (direct_supporters().empty()) return NULL;
  if (registry() == NULL) return NULL;
  
  const unsigned int & knot_vector_id = direct_supporters().back();
  the_knot_vector_t * v =
    dynamic_cast<the_knot_vector_t *>(registry()->elem(knot_vector_id));
  
  return v;
}


//----------------------------------------------------------------
// the_interpolation_bspline_t::color
// 
the_color_t
the_interpolation_bspline_t::color() const
{
  return THE_APPEARANCE.palette().curve()[current_state()];
}

//----------------------------------------------------------------
// delta
// 
inline static float
delta(const the_domain_array_t<float> & knot, const int & i)
{ return (knot[i + 1] - knot[i]); }

//----------------------------------------------------------------
// the_interpolation_bspline_t::regenerate
// 
bool
the_interpolation_bspline_t::regenerate()
{
  regenerated_ = false;
  
  setup_parameterization();
  
  std::vector<p3x1_t> pts(pts_.size());
  std::vector<float> wts(pts_.size());
  std::vector<float> kts(pts_.size());
  point_values(pts, wts, kts);
  
  regenerated_ = update_geom(pts, wts, kts);
  return regenerated_;
}

//----------------------------------------------------------------
// the_interpolation_bspline_t::update_geom
// 
bool
the_interpolation_bspline_t::update_geom(const std::vector<p3x1_t> & pts,
					 const std::vector<float> & wts,
					 const std::vector<float> & kts)
{
  // the curve must have at least two distinct points to regenerate properly:
  if (kts.size() < 2) return false;
  
  // number of segments:
  unsigned int num_seg = kts.size() - 1;
  
  // solve for tangent control points:
  p3x1_t head_tangent_pt = bessel_pt(pts, kts, THE_HEAD_TANGENT_E);
  p3x1_t tail_tangent_pt = bessel_pt(pts, kts, THE_TAIL_TANGENT_E);
  
  // setup the end knots:
  the_domain_array_t<float> knot;
  knot.set_domain(-3, num_seg + 3);
  knot[-3] = kts[0];
  knot[-2] = kts[0];
  knot[-1] = kts[0];
  knot[num_seg + 1] = kts[num_seg];
  knot[num_seg + 2] = kts[num_seg];
  knot[num_seg + 3] = kts[num_seg] + THE_EPSILON; // FIXME: 2006/03/16
  
  // calculate the knot vector (centripetal parametrization):
  for (unsigned int i = 0; i <= num_seg; i++) knot[i] = kts[i];
  
  // build the tridagonal matrix:
  std::vector<float> ai(num_seg + 1);
  std::vector<float> bi(num_seg + 1);
  std::vector<float> gi(num_seg + 1);
  
  // initialize matrix boundaries:
  ai[0] = ai[num_seg] = 0.0;
  bi[0] = bi[num_seg] = 1.0;
  gi[0] = gi[num_seg] = 0.0;
  
  // setup right-hand-side, initialize boundaries:
  the_domain_array_t<p3x1_t> ri(-1, num_seg + 1);
  ri[-1] = pts[0];
  ri[0] = head_tangent_pt;
  ri[num_seg] = tail_tangent_pt;
  ri[num_seg + 1] = pts[num_seg];
  
  for (unsigned int i = 1; i < num_seg; i++)
  {
    float delta_im2 = delta(knot, i - 2);
    float delta_im1 = delta(knot, i - 1);
    float delta_i   = delta(knot, i);
    float delta_ip1 = delta(knot, i + 1);
    float delta_im1_plus_delta_i = delta_im1 + delta_i;
    
    ai[i] = (((delta_i * delta_i) / (delta_im2 + delta_im1 + delta_i)) /
	     delta_im1_plus_delta_i);
    bi[i] = (((delta_i * (delta_im2 + delta_im1)) /
	      (delta_im2 + delta_im1 + delta_i) +
	      (delta_im1 * (delta_i + delta_ip1)) /
	      (delta_im1 + delta_i + delta_ip1)) /
	     delta_im1_plus_delta_i);
    gi[i] = (((delta_im1 * delta_im1) / (delta_im1 + delta_i + delta_ip1)) /
	     delta_im1_plus_delta_i);
    
    ri[i] = pts[i];
  }
  
  // perform LU-factorization:
  std::vector<float> LU[2];
  LU[0].resize(num_seg + 1);
  LU[1].resize(num_seg + 1);
  
  LU[1][0] = bi[0];
  
  for (unsigned int i = 1; i < num_seg + 1; i++)
  {
    LU[0][i] = ai[i] / LU[1][i - 1];
    LU[1][i] = bi[i] - LU[0][i] * gi[i - 1];
  }
  
  // solve the system:
  std::vector<p3x1_t> tmp(num_seg + 1);
  
  // forward substitution:
  tmp[0] = ri[0];
  for (unsigned int i = 1; i < num_seg + 1; i++)
  {
    v3x1_t v = ri[i] - LU[0][i] * tmp[i - 1];
    tmp[i].assign(v.data());
  }
  
  // backward substitution:
  the_domain_array_t<p3x1_t> ctl_pt;
  ctl_pt.set_domain(-1, num_seg + 1);
  ctl_pt[-1] = ri[-1];
  ctl_pt[0] = ri[0];
  ctl_pt[num_seg] = tmp[num_seg] / LU[1][num_seg];
  for (int i = (int)num_seg - 1; i > 0; i--)
  {
    v3x1_t v = (tmp[i] - gi[i] * ctl_pt[i + 1]) / LU[1][i];
    ctl_pt[i].assign(v.data());
  }
  ctl_pt[num_seg + 1] = ri[num_seg + 1];
  
  // setup the bspline geom:
  geom_.reset(ctl_pt.data(), knot.data());
  
  return true;
}

//----------------------------------------------------------------
// the_interpolation_bspline_t::dump
// 
void
the_interpolation_bspline_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_interpolation_bspline_t(" << (void *)this << ")"
       << endl
       << INDSCP << "{" << endl;
  the_intcurve_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_interpolation_bspline_t::setup_parameterization
// 
void
the_interpolation_bspline_t::setup_parameterization()
{
  if (pts_.size() < 2) return;
  float length = 0.0;
  
  std::list<the_knot_point_t>::iterator ia = pts_.begin();
  p3x1_t pa = point((*ia).id_)->value();
  (*ia).param_ = 0.0;
  
  for (unsigned int i = 1; i < pts_.size(); i++)
  {
    std::list<the_knot_point_t>::iterator ib = next(ia);
    p3x1_t pb = point((*ib).id_)->value();
    length += sqrt((pb - pa).norm());
    (*ib).param_ = length;
    
    pa = pb;
    ia = ib;
  }
  
  for (std::list<the_knot_point_t>::iterator i = ++(pts_.begin());
       i != pts_.end(); ++i)
  {
    (*i).param_ /= length;
  }
}

//----------------------------------------------------------------
// bessel_pt
// 
static const p3x1_t
bessel_pt(const p3x1_t & x0,
	  const p3x1_t & x1,
	  const p3x1_t & x2,
	  float u0,
	  float u1,
	  float u2)
{
  float d0 = u1 - u0;
  float d1 = u2 - u1;
  float d0_p_d1 = d0 + d1;
  v3x1_t dx0 = x1 - x0;
  v3x1_t dx1 = x2 - x1;
  
  return (x0 + (2.0 * dx0 - d1 * dx0 / d0_p_d1 -
		d0 * d0 * dx1 / (d1 * d0_p_d1)) / 3.0);
}

//----------------------------------------------------------------
// the_interpolation_bspline_t::bessel_pt
// 
const p3x1_t
the_interpolation_bspline_t::bessel_pt(const std::vector<p3x1_t> & pts,
				       const std::vector<float> & kts,
				       const the_tangent_id_t & tan_id) const
{
  unsigned int num_seg = pts.size() - 1;
  if (num_seg == 0) return pts[0];
  
  if (num_seg == 1)
  {
    if (tan_id == THE_HEAD_TANGENT_E)
    {
      return pts[0] + (pts[1] - pts[0]) / 3.0;
    }
    
    return pts[1] + (pts[0] - pts[1]) / 3.0;
  }
  
  if (tan_id == THE_HEAD_TANGENT_E)
  {
    return ::bessel_pt(pts[0],
		       pts[1],
		       pts[2],
		       kts[0],
		       kts[1],
		       kts[2]);
  }
  
  return ::bessel_pt(pts[num_seg],
		     pts[num_seg - 1],
		     pts[num_seg - 2],
		     kts[num_seg],
		     kts[num_seg - 1],
		     kts[num_seg - 2]);
}

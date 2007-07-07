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


// File         : the_triangle_mesh.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun  6 12:00:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A triangle mesh object, used for raytracing.

// local includes:
#include "geom/the_triangle_mesh.hxx"
#include "math/the_ray.hxx"
#include "math/the_aa_bbox.hxx"
#include "utils/the_dloop.hxx"
#include "utils/the_utils.hxx"
#include "utils/the_dynamic_array.hxx"

// system includes:
#include <iostream>
#include <fstream>

// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// duplicate
// 
the_vertex_ids_t
duplicate(const the_vertex_ids_t & in, the_triangle_mesh_t & mesh)
{
  the_vertex_ids_t dup(UINT_MAX, UINT_MAX, UINT_MAX);
  
  // duplicate the vertex:
  if (in.vx != UINT_MAX)
  {
    dup.vx = mesh.vx_.size();
    mesh.vx_.append(mesh.vx_[in.vx]);
  }
  
  // duplicate the texture coordinate:
  if (in.vt != UINT_MAX)
  {
    dup.vt = mesh.vt_.size();
    mesh.vt_.append(mesh.vt_[in.vt]);
  }
  
  // duplicate the vertex normal:
  if (in.vn != UINT_MAX)
  {
    dup.vn = mesh.vn_.size();
    mesh.vn_.append(mesh.vn_[in.vn]);
  }
  
  return dup;
}


//----------------------------------------------------------------
// is_convex_vertex
// 
static bool
is_convex_vertex(const v3x1_t & normal, const v3x1_t & z)
{ return (z * normal) > float(0); }

//----------------------------------------------------------------
// is_convex_vertex
// 
static bool
is_convex_vertex(const v3x1_t & normal,
		 the_triangle_mesh_t & mesh,
		 const std::vector<the_vertex_ids_t> & ids,
		 const unsigned int & a,
		 const unsigned int & b,
		 const unsigned int & c)
{
  const the_dynamic_array_t<p3x1_t> & vx = mesh.vx_;
  
  v3x1_t x = !(vx[ids[b].vx] - vx[ids[a].vx]);
  v3x1_t y = !(vx[ids[c].vx] - vx[ids[b].vx]);
  v3x1_t z = !(x % y);
  
  return is_convex_vertex(normal, z);
}

//----------------------------------------------------------------
// is_ear_vertex
// 
static bool
is_ear_vertex(const v3x1_t & normal,
	      the_triangle_mesh_t & mesh,
	      const std::vector<the_vertex_ids_t> & ids,
	      const unsigned int & a,
	      const unsigned int & b,
	      const unsigned int & c,
	      const std::list<unsigned int> & reflex)
{
  const the_dynamic_array_t<p3x1_t> & vx = mesh.vx_;
  
  // build the test triangle:
  the_mesh_triangle_t test(&mesh, ids[a].vx, ids[b].vx, ids[c].vx);
  
  for (std::list<unsigned int>::const_iterator i = reflex.begin();
       i != reflex.end(); ++i)
  {
    const unsigned int k = *i;
    
    // skip the vertices of the triangle being tested:
    if (k == a || k == b || k == c) continue;
    
    // check whether this vertex lays inside the triangle being tested:
    the_ray_t ray(vx[ids[k].vx], normal);
    
    float t;
    float v;
    float w;
    if (test.intersect(ray, t, v, w))
    {
      if (v > float(0) && w > float(0) && (v + w) < float(1))
      {
	// vertex lays inside the triangle:
	return false;
      }
    }
  }
  
  return true;
}


//----------------------------------------------------------------
// the_face_info_t::the_face_info_t
// 
the_face_info_t::the_face_info_t():
  face_type(THE_VERTEX_TEXTURE_NORMAL_FACE_E)
{}

//----------------------------------------------------------------
// the_face_info_t::the_face_info_t
// 
the_face_info_t::the_face_info_t(const the_face_info_t & face)
{
  *this = face;
}

//----------------------------------------------------------------
// the_face_info_t::operator =
// 
the_face_info_t &
the_face_info_t::operator = (const the_face_info_t & face)
{
  if (this == &face) return *this;
  
  face_type = face.face_type;
  vertices = face.vertices;
  return *this;
}

//----------------------------------------------------------------
// the_face_info_t::add
// 
void
the_face_info_t::add(const unsigned int & vx,
		     const unsigned int & vn,
		     const unsigned int & vt)
{
  switch (face_type)
  {
    case THE_VERTEX_TEXTURE_NORMAL_FACE_E:
      if (vt == UINT_MAX)
      {
	face_type = ((vn == UINT_MAX) ?
		     THE_VERTEX_FACE_E :
		     THE_VERTEX_NORMAL_FACE_E);
      }
      else if (vn == UINT_MAX)
      {
	face_type = THE_VERTEX_TEXTURE_FACE_E;
      }
      break;
      
    case THE_VERTEX_TEXTURE_FACE_E:
      if (vt == UINT_MAX)
      {
	face_type = THE_VERTEX_FACE_E;
      }
      break;
      
    case THE_VERTEX_NORMAL_FACE_E:
      if (vn == UINT_MAX)
      {
	face_type = THE_VERTEX_FACE_E;
      }
      break;
      
    case THE_VERTEX_FACE_E:
      break;
  }
  
  vertices.append(the_vertex_ids_t(vx, vt, vn));
}

//----------------------------------------------------------------
// the_face_info_t::add_hole
// 
// this method will modify the mesh by adding a hole to an existing face:
void
the_face_info_t::add_hole(const std::list<the_vertex_ids_t> & hole,
			  the_triangle_mesh_t & mesh)
{
  // setup a random access method for the face vertices:
  std::vector<the_vertex_ids_t> fv;
  copy_a_to_b(vertices, fv);
  
  // setup a random access method for the hole vertices:
  std::vector<the_vertex_ids_t> hv(hole.begin(), hole.end());
  
  // shortcuts:
  unsigned int fn = fv.size();
  unsigned int hn = hv.size();
  
  // assemble the outline edges:
  std::vector<the_ray_t> fe(fn);
  for (unsigned int fi = 0; fi < fn; fi++)
  {
    unsigned int ia = fv[fi].vx;
    unsigned int ib = fv[(fi + 1) % fn].vx;
    fe[fi] = the_ray_t(mesh.vx_[ia], mesh.vx_[ib] - mesh.vx_[ia]);
  }
  
  // assemble the hole edges:
  std::vector<the_ray_t> he(hn);
  for (unsigned int hi = 0; hi < hn; hi++)
  {
    unsigned int ia = hv[hi].vx;
    unsigned int ib = hv[(hi + 1) % hn].vx;
    he[hi] = the_ray_t(mesh.vx_[ia], mesh.vx_[ib] - mesh.vx_[ia]);
  }
  
  // find the cut edge:
  unsigned int best_fi = ~0u;
  unsigned int best_hi = ~0u;
  float best_norm_sqr = FLT_MAX;
  
  for (unsigned int fi = 0; fi < fn; fi++)
  {
    unsigned int ia = fv[fi].vx;
    unsigned int fi_prev = ((fi + fn) - 1) % fn;
    
    for (unsigned int hi = 0; hi < hn; hi++)
    {
      unsigned int ib = hv[hi].vx;
      unsigned int hi_prev = ((hi + hn) - 1) % hn;
      
      the_ray_t cut(mesh.vx_[ib], mesh.vx_[ia] - mesh.vx_[ib]);
      float norm_sqr = cut.v().norm_sqrd();
      
      // we are looking for the shortest cut edge:
      if (norm_sqr >= best_norm_sqr) continue;
      
      // make sure the cut edge does not intersect any of the outline:
      bool safe = true;
      for (unsigned int i = 0; i < fn; i++)
      {
	// skip edges adjacent to the cut edge:
	if (i == fi || i == fi_prev) continue;
	
	float tc; // parameter along the cut edge
	float to; // parameter along the outline edge
	float co; // distance between the edges
	if (!cut.intersect(fe[i], tc, to, co)) continue;
	if (tc < -THE_EPSILON || (tc - float(1)) > THE_EPSILON ||
	    to < -THE_EPSILON || (to - float(1)) > THE_EPSILON) continue;
	
	safe = false;
	break;
      }
      if (!safe) continue;
      
      // make sure the cut edge does not intersect the hole:
      for (unsigned int i = 0; i < hn; i++)
      {
	// skip edges adjacent to the cut edge:
	if (i == hi || i == hi_prev) continue;
	
	float tc; // parameter along the cut edge
	float th; // parameter along the hole edge
	float ch; // distance between the edges
	if (!cut.intersect(he[i], tc, th, ch)) continue;
	if (tc < -THE_EPSILON || (tc - float(1)) > THE_EPSILON ||
	    th < -THE_EPSILON || (th - float(1)) > THE_EPSILON) continue;
	
	safe = false;
	break;
      }
      if (!safe) continue;
      
      // this is the best cut edge so far:
      best_fi = fi;
      best_hi = hi;
      best_norm_sqr = norm_sqr;
    }
  }
  
  // duplicate the cut edge vertices:
  the_vertex_ids_t f_dup = duplicate(fv[best_fi], mesh);
  the_vertex_ids_t h_dup = duplicate(hv[best_hi], mesh);
  
  // assemble the new vertex list:
  std::list<the_vertex_ids_t> new_verts;
  for (unsigned int fi = 0; fi < fn; fi++)
  {
    new_verts.push_back(fv[fi]);
    if (fi == best_fi)
    {
      for (unsigned int hi = 0; hi < hn; hi++)
      {
	new_verts.push_back(hv[(hi + best_hi) % hn]);
      }
      
      // append the duplicates:
      new_verts.push_back(h_dup);
      new_verts.push_back(f_dup);
    }
  }
  
  // rebuild the face:
  face_type = THE_VERTEX_TEXTURE_NORMAL_FACE_E;
  vertices.clear();
  
  while (!new_verts.empty())
  {
    the_vertex_ids_t ids = remove_head(new_verts);
    add(ids.vx, ids.vn, ids.vt);
  }
}

//----------------------------------------------------------------
// the_face_info_t::tesselate
// 
bool
the_face_info_t::tesselate(std::list<the_mesh_triangle_t> & triangles,
			   the_triangle_mesh_t & mesh) const
{
  // setup a random access method for the face vertices:
  std::vector<the_vertex_ids_t> fv;
  copy_a_to_b(vertices, fv);
  
  unsigned int num_vertices = fv.size();
  if (num_vertices < 3)
  {
    cerr << "face with less then 3 vertices, skipping..." << endl;
    return false;
  }
  
  // setup a quick-access vertex list:
  std::vector<the_vertex_ids_t> ids;
  copy_a_to_b(vertices, ids);
  
  // setup a double-linked cyclic list of vertices:
  the_dloop_t<unsigned int> vertex;
  for (unsigned int j = 0; j < num_vertices; j++)
  {
    vertex.append(j);
  }
  
  // bootstrap the convex/reflex vertex labels:
  std::vector<bool> convex(num_vertices);
  std::list<unsigned int> reflex;
  v3x1_t normal;
  {
    std::vector<v3x1_t> z(num_vertices);
    for (unsigned int j = 0; j < num_vertices; j++)
    {
      // calculate the vertex normals:
      unsigned int left = ((j + num_vertices) - 1) % num_vertices;
      unsigned int right = (j + 1) % num_vertices;
      
      v3x1_t x = !(mesh.vx_[ids[j].vx] - mesh.vx_[ids[left].vx]);
      v3x1_t y = !(mesh.vx_[ids[right].vx] - mesh.vx_[ids[j].vx]);
      z[j] = !(x % y);
    }
    
    // use the first non-zero length normal as the polygon normal:
    for (unsigned int j = 0; j < num_vertices; j++)
    {
      if (z[j] == v3x1_t(float(0), float(0), float(0))) continue;
      
      normal = z[j];
      break;
    }
      
    // apply the reflex/convex test:
    unsigned int num_reflex = 0;
    for (unsigned int j = 0; j < num_vertices; j++)
    {
      convex[j] = is_convex_vertex(normal, z[j]);
      if (!convex[j])
      {
	reflex.push_back(j);
	num_reflex++;
      }
    }
    
    // The number of convex vertices should be greater than the number of
    // reflex vertices. If that is not the case - reverse the polygon normal
    // and re-evaluate the convex/reflex attributes (or you could just invert
    // them, but the vertices with zero-length normals would be classified as
    // convex and as a result the ear-clipping triangulation algorithm might
    // produce degenerate triangles with co-linear vertices):
    if (num_reflex > num_vertices - num_reflex)
    {
      // reverse the plane normal:
      normal = -normal;
      
      reflex.clear();
      for (unsigned int j = 0; j < num_vertices; j++)
      {
	convex[j] = is_convex_vertex(normal, z[j]);
	if (!convex[j]) reflex.push_back(j);
      }
    }
  }
  
  // bootstrap the cyclic list of ears:
  the_dloop_t<unsigned int> ear;
  
  the_dlink_t<unsigned int> * index = vertex.head;
  for (unsigned int j = 0; j < vertex.size; j++, index = index->next)
  {
    // check for ears:
    unsigned int b = index->elem;
    
    // skip non-convex vertices:
    if (!convex[b]) continue;
      
    unsigned int a = ((b + num_vertices) - 1) % num_vertices;
    unsigned int c = (b + 1) % num_vertices;
    if (!is_ear_vertex(normal, mesh, ids, a, b, c, reflex)) continue;
    
    ear.append(b); // add to the ear list
  }
  
  // triangulate:
  while (!ear.is_empty() && vertex.size > 2)
  {
    unsigned int b = ear.remove_head();
    
    // find the ear neighbors:
    the_dlink_t<unsigned int> * lb = vertex.link(b);
    the_dlink_t<unsigned int> * la = lb->prev;
    the_dlink_t<unsigned int> * lc = lb->next;
    vertex.remove(lb);
    
    // update the ear/convex/reflex lists:
    unsigned int a = la->elem;
    unsigned int c = lc->elem;
    
    if (!convex[a])
    {
      // if vertex a was reflex, see if it became convex:
      convex[a] = is_convex_vertex(normal, mesh, ids, la->prev->elem, a, c);
      if (convex[a]) reflex.remove(a);
    }
    
    if (convex[a] && !ear.has(a))
    {
      // if vertex a is convex see if it became an ear vertex:
      if (is_ear_vertex(normal, mesh, ids, la->prev->elem, a, c, reflex))
      {
	// add to the ear list:
	ear.append(a);
      }
    }
    
    if (!convex[c])
    {
      // if vertex a was reflex, see if it became convex:
      convex[c] = is_convex_vertex(normal, mesh, ids, a, c, lc->next->elem);
      if (convex[c]) reflex.remove(c);
    }
    
    if (convex[c] && !ear.has(c))
    {
      // if vertex c is convex see if it became an ear vertex:
      if (is_ear_vertex(normal, mesh, ids, a, c, lc->next->elem, reflex))
      {
	// add to the ear list:
	ear.append(c);
      }
    }
    
    // assemble the triangle:
    the_mesh_triangle_t triangle(&mesh,
				 ids[a].vx,
				 ids[b].vx,
				 ids[c].vx,
				 ids[a].vn,
				 ids[b].vn,
				 ids[c].vn,
				 ids[a].vt,
				 ids[b].vt,
				 ids[c].vt);
    
    // check for degeneracies:
    v3x1_t normal = triangle.calc_normal();
    if (normal == v3x1_t(0.0, 0.0, 0.0))
    {
      cerr << "WARNING: degenerate triangle, adding anyway..." << endl;
    }
    
    triangles.push_back(triangle);
  }
  
  return true;
}


//----------------------------------------------------------------
// the_mesh_triangle_t::get_vx
// 
p3x1_t &
the_mesh_triangle_t::get_vx(const unsigned int & index) const
{
  return mesh_->vx_[vx_[index]];
}

//----------------------------------------------------------------
// the_mesh_triangle_t::get_vn
// 
v3x1_t &
the_mesh_triangle_t::get_vn(const unsigned int & index) const
{
  return mesh_->vn_[vn_[index]];
}

//----------------------------------------------------------------
// the_mesh_triangle_t::get_vt
// 
p2x1_t &
the_mesh_triangle_t::get_vt(const unsigned int & index) const
{
  return mesh_->vt_[vt_[index]];
}

//----------------------------------------------------------------
// the_mesh_triangle_t::calc_vx
// 
p3x1_t
the_mesh_triangle_t::calc_vx(const p3x1_t & bar_pt) const
{
  return (bar_pt[0] * get_vx(0) +
	  bar_pt[1] * get_vx(1) +
	  bar_pt[2] * get_vx(2));
}

//----------------------------------------------------------------
// the_mesh_triangle_t::calc_vn
// 
v3x1_t
the_mesh_triangle_t::calc_vn(const p3x1_t & bar_pt) const
{
  return !(bar_pt[0] * get_vn(0) +
	   bar_pt[1] * get_vn(1) +
	   bar_pt[2] * get_vn(2));
}

//----------------------------------------------------------------
// the_mesh_triangle_t::calc_vt
// 
p2x1_t
the_mesh_triangle_t::calc_vt(const p3x1_t & bar_pt) const
{
  return (bar_pt[0] * get_vt(0) +
	  bar_pt[1] * get_vt(1) +
	  bar_pt[2] * get_vt(2));
}

//----------------------------------------------------------------
// the_mesh_triangle_t::calc_normal
// 
v3x1_t
the_mesh_triangle_t::calc_normal() const
{
  return !((get_vx(1) - get_vx(0)) % (get_vx(2) - get_vx(1)));
}

//----------------------------------------------------------------
// the_mesh_triangle_t::calc_area
// 
float
the_mesh_triangle_t::calc_area() const
{
  v3x1_t ab = get_vx(1) - get_vx(0);
  v3x1_t bc = get_vx(2) - get_vx(1);
  float twice_area_abc = (ab % bc).norm();
  return 0.5 * twice_area_abc;
}

//----------------------------------------------------------------
// the_mesh_triangle_t::intersect
// 
// This algorithm is from REAL-TIME RENDERING, 2nd edition
// by Tomas Akenine-Moller and Eric Haines
// 
bool
the_mesh_triangle_t::intersect(const the_ray_t & ray,
			       float & t,
			       float & v,
			       float & w) const
{
  static const float eps = 1e-7;
  
  const p3x1_t & o = ray.p();
  const v3x1_t & d = ray.v();
  
  const p3x1_t & v0 = get_vx(0);
  const p3x1_t & v1 = get_vx(1);
  const p3x1_t & v2 = get_vx(2);
  
  v3x1_t e1 = v1 - v0;
  v3x1_t e2 = v2 - v0;
  
  v3x1_t p = d % e2;
  float a = e1 * p;
  if (a > -eps && a < eps) return false;
  
  float f = 1.0 / a;
  
  v3x1_t s = o - v0;
  v = f * (s * p);
  if (v < 0.0 || v > 1.0) return false;
  
  v3x1_t q = s % e1;
  w = f * (d * q);
  if (w < 0.0 || (v + w) > 1.0) return false;
  
  t = f * (e2 * q);
  return true;
}


//----------------------------------------------------------------
// the_triangle_mesh_t::the_triangle_mesh_t
// 
the_triangle_mesh_t::the_triangle_mesh_t():
  vx_(4096),
  vn_(4096),
  vt_(4096)
{}

//----------------------------------------------------------------
// the_triangle_mesh_t::the_triangle_mesh_t
// 
the_triangle_mesh_t::the_triangle_mesh_t(const the_triangle_mesh_t & mesh):
  triangles_(mesh.triangles_),
  vx_(mesh.vx_),
  vn_(mesh.vn_),
  vt_(mesh.vt_)
{
  const unsigned int & num_triangles = triangles_.size();
  for (unsigned int i = 0; i < num_triangles; i++)
  {
    the_mesh_triangle_t & ti = triangles_[i];
    ti.mesh_ = this;
  }
}

//----------------------------------------------------------------
// the_triangle_mesh_t::load
// 
bool
the_triangle_mesh_t::load(const the_text_t & filename,
			  const bool & remove_duplicates,
			  const bool & calculate_normals,
			  const bool & discard_existing_normals,
			  const unsigned int & maximum_angle_between_normals)
{
  if (filename.match_tail(".m", true))
  {
    if (!load_m(filename, remove_duplicates))
    {
      return false;
    }
  }
  else if (filename.match_tail(".obj", true))
  {
    if (!load_obj(filename, remove_duplicates))
    {
      return false;
    }
  }
  else
  {
    // default:
    return false;
  }
  
  // move all vertices so that the center of the bounding box
  // coincides with the origin:
  shift_to_center();
  
  // fix up the normal vectors wherever they are missing:
  if (calculate_normals)
  {
    float minimum_normal_dot_product =
      cos(float(maximum_angle_between_normals) / M_PI);
    
    calc_vertex_normals(discard_existing_normals,
			minimum_normal_dot_product);
  }
  else
  {
    calc_face_normals(false);
  }
  
  // make sure we have texture coordinates:
  calc_texture_coords(false);
  
  return true;
}

//----------------------------------------------------------------
// the_triangle_mesh_t::calc_area
// 
float
the_triangle_mesh_t::calc_area() const
{
  // shortcut:
  const unsigned int & num_triangles = triangles_.size();
  float area = float(0);
  
  for (unsigned int i = 0; i < num_triangles; i++)
  {
    const the_mesh_triangle_t & ti = triangles_[i];
    area += ti.calc_area();
  }
  
  return area;
}

//----------------------------------------------------------------
// the_triangle_mesh_t::calc_vertex_normals
// 
void
the_triangle_mesh_t::
calc_vertex_normals(const bool & discard_current_normals,
		    const float & minimum_normal_dot_product)
{
  // shortcut:
  const unsigned int & num_triangles = triangles_.size();
  
  if (discard_current_normals)
  {
    for (unsigned int i = 0; i < num_triangles; i++)
    {
      the_mesh_triangle_t & ti = triangles_[i];
      ti.vn_[0] = UINT_MAX;
      ti.vn_[1] = UINT_MAX;
      ti.vn_[2] = UINT_MAX;
    }
    
    vn_.clear();
  }
  
  // iterate over the triangles, and calculate the vertex normals as necessary:
  for (unsigned int i = 0; i < num_triangles; i++)
  {
    the_mesh_triangle_t & ti = triangles_[i];
    v3x1_t ni = ti.calc_normal();
    
    for (unsigned int j = 0; j < 3; j++)
    {
      if (ti.vn_[j] == UINT_MAX)
      {
	v3x1_t normal(0.0, 0.0, 0.0);
	
	unsigned int num_good_neighbors = 0;
	the_dynamic_array_t<unsigned int> good_neighbors(0, 6, UINT_MAX);
	the_dynamic_array_t<unsigned int> good_neighbors_v(0, 6, UINT_MAX);
	
	// find all triangles that share this vertex, and calculate their
	// normal vector:
	for (unsigned int k = 0; k < num_triangles; k++)
	{
	  the_mesh_triangle_t & tk = triangles_[k];
	  for (unsigned int l = 0; l < 3; l++)
	  {
	    if (ti.vx_[j] != tk.vx_[l]) continue;
	    if (tk.vn_[l] != UINT_MAX) continue;
	    
	    v3x1_t nk = tk.calc_normal();
	    float dot_jk = ni * nk;
	    if (dot_jk <= minimum_normal_dot_product) continue;
	    
	    normal += nk;
	    good_neighbors[num_good_neighbors] = k;
	    good_neighbors_v[num_good_neighbors] = l;
	    num_good_neighbors++;
	  }
	}
	
	normal.normalize();
	
	// before adding a normal, make sure that this face does not already
	// have this normal vector associated with some other vertex:
	unsigned int normal_id = UINT_MAX;
	for (unsigned int l = 0; l < 3; l++)
	{
	  if (ti.vn_[l] == UINT_MAX) continue;
	  if (ti.get_vn(l) != normal) continue;
	  normal_id = ti.vn_[l];
	}
	
	if (normal_id == UINT_MAX)
	{
	  // add a new normal to the table:
	  vn_.resize(vn_.size() + 1);
	  vn_.end_elem(1) = normal;
	  normal_id = vn_.size() - 1;
	}
	
	// make sure the good neigbors that share this vertex also
	// share the normal:
	for (unsigned int k = 0; k < num_good_neighbors; k++)
	{
	  the_mesh_triangle_t & tk = triangles_[good_neighbors[k]];
	  tk.vn_[good_neighbors_v[k]] = normal_id;
	}
      }
      
      assert(ti.vn_[j] != UINT_MAX);
    }
  }
}

//----------------------------------------------------------------
// the_triangle_mesh_t::calc_face_normals
// 
void
the_triangle_mesh_t::calc_face_normals(const bool & discard_current_normals)
{
  // shortcut:
  const unsigned int & num_triangles = triangles_.size();
  
  if (discard_current_normals)
  {
    for (unsigned int i = 0; i < num_triangles; i++)
    {
      the_mesh_triangle_t & ti = triangles_[i];
      ti.vn_[0] = UINT_MAX;
      ti.vn_[1] = UINT_MAX;
      ti.vn_[2] = UINT_MAX;
    }
    
    vn_.clear();
  }
  
  // iterate over the triangles, and calculate the face normals as necessary:
  for (unsigned int i = 0; i < num_triangles; i++)
  {
    the_mesh_triangle_t & ti = triangles_[i];
    
    for (unsigned int j = 0; j < 3; j++)
    {
      if (ti.vn_[j] == UINT_MAX)
      {
	// add a new normal to the table:
	unsigned int normal_id = vn_.size();
	vn_.resize(vn_.size() + 1);
	vn_.end_elem(1) = !ti.calc_normal();
	
	for (unsigned int k = j; k < 3; k++)
	{
	  if (ti.vn_[k] == UINT_MAX)
	  {
	    ti.vn_[k] = normal_id;
	  }
	}
	
	break;
      }
    }
  }
}

//----------------------------------------------------------------
// the_triangle_mesh_t::calc_texture_coords
// 
void
the_triangle_mesh_t::calc_texture_coords(const bool & discard_current_coords)
{
  // FIXME: this is a very poor test to see whether the all vertices
  // have been assigned a texture map coordinate:
  if ((vt_.size() == vx_.size()) && !discard_current_coords)
  {
    // texture coordinates already exist:
    return;
  }
  
  const unsigned int & num_vx = vx_.size();
  vt_.resize(num_vx);
  
  // iterate over the vertices, and calculate corresponding texture coordinate:
  float scale = 1.0 / M_PI;
  for (unsigned int i = 0; i < num_vx; i++)
  {
    p3x1_t sph_pt;
    // the_coord_sys_t::xyz_to_sph(vx_[i], sph_pt);
    assert(0);
    vt_[i].assign(sph_pt.y() * scale, sph_pt.z() * scale);
  }
  
  // iterate over the triangles and setup texture coordinates:
  const unsigned int & num_triangles = triangles_.size();
  for (unsigned int i = 0; i < num_triangles; i++)
  {
    the_mesh_triangle_t & ti = triangles_[i];
    ti.vt_[0] = ti.vx_[0];
    ti.vt_[1] = ti.vx_[1];
    ti.vt_[2] = ti.vx_[2];
  }
}

//----------------------------------------------------------------
// the_triangle_mesh_t::shift_to_center
// 
v3x1_t
the_triangle_mesh_t::shift_to_center()
{
  the_aa_bbox_t lcs_bbox;
  calc_bbox(lcs_bbox);
  
  v3x1_t shift(lcs_bbox.center().data());
  const unsigned int & num_vertices = vx_.size();
  for (unsigned int i = 0; i < num_vertices; i++)
  {
    vx_[i] -= shift;
  }
  
  return shift;
}

//----------------------------------------------------------------
// find_first_match
// 
static unsigned int
find_first_match(const the_dynamic_array_t<p3x1_t> & vx_array,
		 const p3x1_t & vx)
{
  const unsigned int & size = vx_array.size();
  for (unsigned int i = 0; i < size; i++)
  {
    if (vx_array[i] == vx) return i;
  }
  
  return UINT_MAX;
}

//----------------------------------------------------------------
// the_load_m_state_t
// 
typedef enum
{
  THE_LOAD_M_HAS_NOTHING_E, // initial state
  THE_LOAD_M_HAS_COMMENT_E, // comment (starts with # character)
  THE_LOAD_M_HAS_Vertex_E,  // vertex data
  THE_LOAD_M_HAS_Face_E     // face data
} the_load_m_state_t;

//----------------------------------------------------------------
// the_triangle_mesh_t::load_m
// 
bool
the_triangle_mesh_t::load_m(const the_text_t & filepath,
			    const bool & remove_duplicates)
{
  // open the file:
  std::ifstream file;
  file.open(filepath, ios::in);
  if (!file.is_open()) return false;
  
  // reinitialize the vertex arrays:
  vx_.clear();
  vn_.clear();
  vt_.clear();
  
  // array of faces:
  the_dynamic_array_t<the_face_info_t> f_array(4096);
  
  // a map from old vertex id to new vertex id:
  the_dynamic_array_t<unsigned int> vx_map(4096);
  
  // a map from old face id to new face id:
  the_dynamic_array_t<unsigned int> fc_map(4096);
  
  // setup the initial state of the parser:
  the_load_m_state_t state = THE_LOAD_M_HAS_NOTHING_E;
  
  // parse the file:
  while (file.eof() == false)
  {
    the_text_t token_line;
    getline(file, token_line);
    token_line = token_line.simplify_ws();
    
    // parse the line:
    std::istringstream stream(token_line.text());
    while (stream.eof() == false)
    {
      the_text_t token;
      
      // process the token:
      if (state == THE_LOAD_M_HAS_NOTHING_E)
      {
	stream >> token;
	
	if (token[0] == '#')
	{
	  state = THE_LOAD_M_HAS_COMMENT_E;
	}
	else if (token == "Vertex")
	{
	  state = THE_LOAD_M_HAS_Vertex_E;
	}
	else if (token == "Face")
	{
	  // create room for the new face:
	  f_array.resize(f_array.size() + 1);
	  state = THE_LOAD_M_HAS_Face_E;
	}
      }
      else if (state == THE_LOAD_M_HAS_COMMENT_E)
      {
	// comment, skip the rest of the line:
	state = THE_LOAD_M_HAS_NOTHING_E;
	break;
      }
      else if (state == THE_LOAD_M_HAS_Vertex_E)
      {
	// load vertex index:
	stream >> token;
	bool token_ok = true;
	unsigned int vertex_index = token.toUInt(&token_ok);
	if (token_ok == false)
	{
	  cerr << "warning: bad vertex ID: " << token << endl
	       << "line: " << token_line << endl << endl;
	  continue;
	}
	
	// load three floats:
	p3x1_t vertex;
	for (unsigned int i = 0; i < 3; i++)
	{
	  stream >> token;
	  bool token_ok = true;
	  vertex[i] = token.toFloat(&token_ok);
	  if (token_ok == false)
	  {
	    cerr << "warning: bad vertex coordinate: " << token << endl
		 << "line: " << token_line << endl << endl;
	  }
	}
	
	// setup the last vertex in the array:
	unsigned int id =
	  remove_duplicates ?
	  find_first_match(vx_, vertex) :
	  UINT_MAX;
	
	if (id == UINT_MAX)
	{
	  vx_map[vertex_index] = vx_.size();
	  vx_.resize(vx_.size() + 1);
	  vx_.end_elem(1) = vertex;
	}
	else
	{
	  cerr << "found duplicate point, remapping..." << endl;
	  vx_map[vertex_index] = id;
	}
	
	state = THE_LOAD_M_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_M_HAS_Face_E)
      {
	// consume the face index toke:
	stream >> token;
	bool token_ok = true;
	token.toUInt(&token_ok);
	if (token_ok == false)
	{
	  cerr << "warning: bad face ID: " << token << endl
	       << "line: " << token_line << endl << endl;
	  continue;
	}
	
	// load three vertex IDs:
	for (unsigned int i = 0; i < 3; i++)
	{
	  stream >> token;
	  bool token_ok = true;
	  unsigned int vertex_index = token.toUInt(&token_ok);
	  if (token_ok == false)
	  {
	    cerr << "warning: bad vertex ID: " << token << endl
		 << "line: " << token_line << endl << endl;
	  }
	  
	  // append the vertex index to the face:
	  f_array.end_elem(1).add(vertex_index);
	}
	
	state = THE_LOAD_M_HAS_NOTHING_E;
      }
    }
    
    // end of line, reset the state:
    state = THE_LOAD_M_HAS_NOTHING_E;
  }
  
  file.close();
  
  // put everything into a list for now:
  std::list<the_mesh_triangle_t> triangles;
  
  const unsigned int & num_faces = f_array.size();
  for (unsigned int i = 0; i < num_faces; i++)
  {
    const the_face_info_t & face = f_array[i];
    const unsigned int & num_vertices = face.vertices.size();
    
    if (num_vertices < 3)
    {
      cerr << "face with less then 3 vertices, skipping..." << endl;
      continue;
    }
    
    std::vector<the_vertex_ids_t> vertices;
    copy_a_to_b(face.vertices, vertices);
    
    for (unsigned int j = 2; j < num_vertices; j++)
    {
      const the_vertex_ids_t & a = vertices[0];
      const the_vertex_ids_t & b = vertices[j - 1];
      const the_vertex_ids_t & c = vertices[j];
      
      the_mesh_triangle_t triangle(this,
				   vx_map[a.vx],
				   vx_map[b.vx],
				   vx_map[c.vx],
				   a.vn,
				   b.vn,
				   c.vn,
				   a.vt,
				   b.vt,
				   c.vt);
      
      // check for degeneracies:
      v3x1_t normal = triangle.calc_normal();
      if (normal == v3x1_t(0.0, 0.0, 0.0))
      {
	cerr << "degenerate triangle, skipping..." << endl;
	continue;
      }
      
      triangles.push_back(triangle);
    }
  }
  
  copy_a_to_b(triangles, triangles_);
  
  return true;
}

//----------------------------------------------------------------
// the_load_obj_state_t
// 
typedef enum
{
  THE_LOAD_OBJ_HAS_NOTHING_E, // initial state
  THE_LOAD_OBJ_HAS_COMMENT_E, // comment (starts with # character)
  THE_LOAD_OBJ_HAS_mtllib_E,  // name of the material library used
  THE_LOAD_OBJ_HAS_v_E,       // vertex data
  THE_LOAD_OBJ_HAS_vt_E,      // texture coordinate data
  THE_LOAD_OBJ_HAS_vn_E,      // normal data
  THE_LOAD_OBJ_HAS_g_E,       // group name
  THE_LOAD_OBJ_HAS_usemtl_E,  // name of the material used
  THE_LOAD_OBJ_HAS_f_E        // face data
} the_load_obj_state_t;

//----------------------------------------------------------------
// the_triangle_mesh_t::load_obj
// 
bool
the_triangle_mesh_t::load_obj(const the_text_t & filepath,
			      const bool & remove_duplicates)
{
  // open the file:
  std::ifstream file;
  file.open(filepath, ios::in);
  if (!file.is_open()) return false;
  
  // reinitialize the vertex arrays:
  vx_.clear();
  vn_.clear();
  vt_.clear();
  
  // array of faces:
  the_dynamic_array_t<the_face_info_t> f_array(4096);
  
  // a map from old vertex id to new vertex id:
  the_dynamic_array_t<unsigned int> vx_map(4096);
  
  // setup the initial state of the parser:
  the_load_obj_state_t state = THE_LOAD_OBJ_HAS_NOTHING_E;
  
  // parse the file:
  while (file.eof() == false)
  {
    the_text_t token_line;
    getline(file, token_line);
    token_line = token_line.simplify_ws();
    
    // parse the line:
    std::istringstream stream(token_line.text());
    while (stream.eof() == false)
    {
      the_text_t token;
      
      // process the token:
      if (state == THE_LOAD_OBJ_HAS_NOTHING_E)
      {
	stream >> token;
	
	if (token[0] == '#')
	{
	  state = THE_LOAD_OBJ_HAS_COMMENT_E;
	}
	else if (token == "mtllib")
	{
	  state = THE_LOAD_OBJ_HAS_mtllib_E;
	}
	else if (token == "v")
	{
	  vx_map.resize(vx_map.size() + 1);
	  state = THE_LOAD_OBJ_HAS_v_E;
	}
	else if (token == "vt")
	{
	  vt_.resize(vt_.size() + 1);
	  state = THE_LOAD_OBJ_HAS_vt_E;
	}
	else if (token == "vn")
	{
	  vn_.resize(vn_.size() + 1);
	  state = THE_LOAD_OBJ_HAS_vn_E;
	}
	else if (token == "g")
	{
	  state = THE_LOAD_OBJ_HAS_g_E;
	}
	else if (token == "usemtl")
	{
	  state = THE_LOAD_OBJ_HAS_usemtl_E;
	}
	else if (token == "f")
	{
	  // create room for the new face:
	  f_array.resize(f_array.size() + 1);
	  state = THE_LOAD_OBJ_HAS_f_E;
	}
      }
      else if (state == THE_LOAD_OBJ_HAS_COMMENT_E)
      {
	// comment, skip the rest of the line:
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
	break;
      }
      else if (state == THE_LOAD_OBJ_HAS_mtllib_E)
      {
	// FIXME: pkoshevoy: load the material library:
	stream >> token;
	
	if (load_mtl(token.text()) == false)
	{
	  cerr << "warning: could not load material: " << token << endl
	       << "line: " << token_line << endl << endl;
	}
	
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_v_E)
      {
	// load three floats:
	p3x1_t vertex;
	for (unsigned int i = 0; i < 3; i++)
	{
	  stream >> token;
	  bool token_ok = true;
	  vertex[i] = token.toFloat(&token_ok);
	  if (token_ok == false)
	  {
	    cerr << "warning: bad vertex coordinate: " << token << endl
		 << "line: " << token_line << endl << endl;
	  }
	}
	
	// setup the last vertex in the array:
	unsigned int id =
	  remove_duplicates ?
	  find_first_match(vx_, vertex) :
	  UINT_MAX;
	
	if (id == UINT_MAX)
	{
	  vx_map.end_elem(1) = vx_.size();
	  vx_.resize(vx_.size() + 1);
	  vx_.end_elem(1) = vertex;
	}
	else
	{
	  cerr << "found duplicate point, remapping..." << endl;
	  vx_map.end_elem(1) = id;
	}
	
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_vt_E)
      {
	// load two floats:
	p2x1_t uv;
	for (unsigned int i = 0; i < 2; i++)
	{
	  stream >> token;
	  bool token_ok = true;
	  uv[i] = token.toFloat(&token_ok);
	  if (token_ok == false)
	  {
	    cerr << "warning: bad texture coordinate: " << token << endl
		 << "line: " << token_line << endl << endl;
	  }
	}
	
	// setup the last texture coordinate in the array:
	vt_.end_elem(1) = uv;
	
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_vn_E)
      {
	// load three floats:
	v3x1_t normal;
	for (unsigned int i = 0; i < 3; i++)
	{
	  stream >> token;
	  bool token_ok = true;
	  normal[i] = token.toFloat(&token_ok);
	  if (token_ok == false)
	  {
	    cerr << "warning: bad vertex normal component: " << token << endl
		 << "line: " << token_line << endl << endl;
	  }
	}
	
	// setup the last vertex normal in the array:
	vn_.end_elem(1) = normal;
	
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_g_E)
      {
	// FIXME: pkoshevoy: start a new group:
	stream >> token;
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_usemtl_E)
      {
	// FIXME: pkoshevoy: setup current material:
	stream >> token;
	state = THE_LOAD_OBJ_HAS_NOTHING_E;
      }
      else if (state == THE_LOAD_OBJ_HAS_f_E)
      {
	// load an unsigned integer (or three unsigned integers
	// separated by two '/' character):
	stream >> token;
	
	unsigned int array_index[] =
	{ UINT_MAX, UINT_MAX, UINT_MAX };
	unsigned int num_separators = token.contains('/');
	
	if (num_separators == 0)
	{
	  bool token_ok = true;
	  array_index[0] = token.toUInt(&token_ok) - 1;
	  if (token_ok == false)
	  {
	    cerr << "warning: bad vertex ID: " << token << endl
		 << "line: " << token_line << endl << endl;
	    continue;
	  }
	}
	else if (num_separators == 2)
	{
	  std::vector<the_text_t> token_list;
	  if (token.split(token_list, '/', true) != 3)
	  {
	    cerr << "warning: wrong number of vertex IDs: " << token << endl
		 << "line: " << token_line << endl << endl;
	    continue;
	  }
	  
	  bool token_ok = true;
	  for (unsigned int i = 0; i < 3; i++)
	  {
	    array_index[i] = token_list[i].toUInt(&token_ok) - 1;
	    if (token_ok == false)
	    {
	      if (i == 0)
	      {
		cerr << "warning: bad vertex IDs: " << token << endl
		     << "line: " << token_line << endl << endl;
		break;
	      }
	      
	      array_index[i] = UINT_MAX;
	    }
	  }
	  
	  if (token_ok == false)
	  {
	    continue;
	  }
	}
	else
	{
	  cerr << "warning: wrong number of vertex ID separators: "
	       << token << endl
	       << "line: " << token_line << endl << endl;
	  continue;
	}
	
	// append the vertex index to the last face in the array:
	f_array.end_elem(1).add(array_index[0],
				array_index[2],
				array_index[1]);
      }
    }
    
    // end of line, reset the state:
    state = THE_LOAD_OBJ_HAS_NOTHING_E;
  }
  
  file.close();
  
  // put everything into a list for now:
  std::list<the_mesh_triangle_t> triangles;
  
  const unsigned int & num_faces = f_array.size();
  for (unsigned int i = 0; i < num_faces; i++)
  {
    const the_face_info_t & face = f_array[i];
    const unsigned int & num_vertices = face.vertices.size();
    
    if (num_vertices < 3)
    {
      cerr << "face with less then 3 vertices, skipping..." << endl;
      continue;
    }
    
    std::vector<the_vertex_ids_t> vertices;
    copy_a_to_b(face.vertices, vertices);
    
    for (unsigned int j = 2; j < num_vertices; j++)
    {
      const the_vertex_ids_t & a = vertices[0];
      const the_vertex_ids_t & b = vertices[j - 1];
      const the_vertex_ids_t & c = vertices[j];
      
      the_mesh_triangle_t triangle(this,
				   vx_map[a.vx],
				   vx_map[b.vx],
				   vx_map[c.vx],
				   a.vn,
				   b.vn,
				   c.vn,
				   a.vt,
				   b.vt,
				   c.vt);
      
      // check for degeneracies:
      v3x1_t normal = triangle.calc_normal();
      if (normal == v3x1_t(0.0, 0.0, 0.0))
      {
	cerr << "degenerate triangle, skipping..." << endl;
	continue;
      }
      
      triangles.push_back(triangle);
    }
  }
  
  copy_a_to_b(triangles, triangles_);
  
  return true;
}

//----------------------------------------------------------------
// the_load_mtl_state_t
// 
typedef enum
{
  THE_LOAD_MTL_HAS_NOTHING_E, // initial state
  THE_LOAD_MTL_HAS_newmtl_E,  // name of the material
  THE_LOAD_MTL_HAS_Ka_E,      // ambient color
  THE_LOAD_MTL_HAS_Kd_E,      // diffuse color
  THE_LOAD_MTL_HAS_Ks_E,      // specular color
  THE_LOAD_MTL_HAS_Ns_E,      // specular highlight color
  THE_LOAD_MTL_HAS_Tr_E,      // transparenct, d is sometimes used
  THE_LOAD_MTL_HAS_illum_E    // illumination model
} the_load_mtl_state_t;

//----------------------------------------------------------------
// the_triangle_mesh_t::load_mtl
// 
bool
the_triangle_mesh_t::load_mtl(const the_text_t & /* filepath */)
{
  return false;
}

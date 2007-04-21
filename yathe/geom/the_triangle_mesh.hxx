// File         : the_triangle_mesh.hxx
// Author       : Paul A. Koshevoy
// Created      : Sat Jun  5 22:11:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

#ifndef THE_TRIANGLE_MESH_HXX_
#define THE_TRIANGLE_MESH_HXX_

// system includes:
#include <list>
#include <vector>
#include <float.h>
#include <limits.h>
#include <sys/types.h>

// local includes:
#include "math/v3x1p3x1.hxx"
#include "utils/the_array.hxx"
#include "utils/the_dynamic_array.hxx"

// forward declarations:
class the_triangle_mesh_t;
class the_ray_t;


//----------------------------------------------------------------
// the_vertex_ids_t
// 
class the_vertex_ids_t
{
public:
  the_vertex_ids_t():
    vx(UINT_MAX),
    vt(UINT_MAX),
    vn(UINT_MAX)
  {}
  
  the_vertex_ids_t(const unsigned int & vertex,
		   const unsigned int & vertex_texture_point,
		   const unsigned int & vertex_normal):
    vx(vertex),
    vt(vertex_texture_point),
    vn(vertex_normal)
  {}
  
  inline bool operator == (const the_vertex_ids_t & id) const
  { return vx == id.vx && vt == id.vt && vn == id.vn; }
  
  // data members:
  unsigned int vx; // vertex id
  unsigned int vt; // vertex texture point id
  unsigned int vn; // vertex normal id
};


//----------------------------------------------------------------
// duplicate
// 
extern the_vertex_ids_t
duplicate(const the_vertex_ids_t & in, the_triangle_mesh_t & mesh);

//----------------------------------------------------------------
// the_mesh_triangle_t
// 
class the_mesh_triangle_t
{
public:
  the_mesh_triangle_t(the_triangle_mesh_t * parent = NULL,
		      const unsigned int & vertex_a = UINT_MAX,
		      const unsigned int & vertex_b = UINT_MAX,
		      const unsigned int & vertex_c = UINT_MAX,
		      const unsigned int & normal_a = UINT_MAX,
		      const unsigned int & normal_b = UINT_MAX,
		      const unsigned int & normal_c = UINT_MAX,
		      const unsigned int & texture_a = UINT_MAX,
		      const unsigned int & texture_b = UINT_MAX,
		      const unsigned int & texture_c = UINT_MAX):
    mesh_(parent)
  {
    vx_[0] = vertex_a;
    vx_[1] = vertex_b;
    vx_[2] = vertex_c;
    
    vn_[0] = normal_a;
    vn_[1] = normal_b;
    vn_[2] = normal_c;
    
    vt_[0] = texture_a;
    vt_[1] = texture_b;
    vt_[2] = texture_c;
  }
  
  // equality test:
  inline bool operator == (const the_mesh_triangle_t & triangle) const
  {
    return ((vx_[0] == triangle.vx_[0]) &&
	    (vx_[1] == triangle.vx_[1]) &&
	    (vx_[2] == triangle.vx_[2]));
  }
  
  // comparison test (used for sorting):
  inline bool operator < (const the_mesh_triangle_t & triangle) const
  { return get_vx(0) < triangle.get_vx(0); }
  
  // accessors to the vertex, normal, and texture coordinates:
  p3x1_t & get_vx(const unsigned int & index) const;
  v3x1_t & get_vn(const unsigned int & index) const;
  p2x1_t & get_vt(const unsigned int & index) const;
  
  // calculate the surface point on the triangle at the
  // specified barycentric coordinates:
  p3x1_t calc_vx(const p3x1_t & bar_pt) const;
  
  // calculate the surface normal of the point on the triangle at the
  // specified barycentric coordinates:
  v3x1_t calc_vn(const p3x1_t & bar_pt) const;
  
  // calculate the surface point texture coordinate on the triangle at the
  // specified barycentric coordinates:
  p2x1_t calc_vt(const p3x1_t & bar_pt) const;
  
  // calculate the surface normal of the triangle, defined as the cross
  // product of two edges of the triangle:
  v3x1_t calc_normal() const;
  
  // calculate the area of the triangle:
  float calc_area() const;
  
  // calculate the intersection point between a ray and the triangle.
  // If the intersection does not exist, return false. Otherwise store the
  // result as a barycentric point (u, v, w), where u = 1 - v - w.
  // u == 1 corresponds to vertex 0,
  // v == 1 corresponds to vertex 1,
  // w == 1 corresponds to vertex 2.
  // The barycentric point lies within the triangle if u + v + w == 1.
  bool intersect(const the_ray_t & ray,
		 float & t,
		 float & v,
		 float & w) const;
  
  // calculate the bounding box of the triangle:
  template <class bbox_t>
  void calc_bbox(bbox_t & bbox) const
  {
    bbox << get_vx(0);
    bbox << get_vx(1);
    bbox << get_vx(2);
  }
  
  // pointer to the mesh that owns this triangle:
  the_triangle_mesh_t * mesh_;
  
  // IDs of the mesh vertices defining this triangle:
  unsigned int vx_[3];
  
  // IDs of vertex normals of this triangle:
  unsigned int vn_[3];
  
  // IDs of vertex texture coordinates of this triangle:
  unsigned int vt_[3];
};


//----------------------------------------------------------------
// the_face_type_t
// 
typedef enum
{
  THE_VERTEX_FACE_E,
  THE_VERTEX_NORMAL_FACE_E,
  THE_VERTEX_TEXTURE_FACE_E,
  THE_VERTEX_TEXTURE_NORMAL_FACE_E
} the_face_type_t;

//----------------------------------------------------------------
// the_face_info_t
// 
class the_face_info_t
{
public:
  the_face_info_t();
  the_face_info_t(const the_face_info_t & face);
  
  the_face_info_t & operator = (const the_face_info_t & face); 
  void add(const unsigned int & vx,
	   const unsigned int & vn = UINT_MAX,
	   const unsigned int & vt = UINT_MAX);
  
  // this method will modify the mesh by adding a hole to an existing face:
  void add_hole(const std::list<the_vertex_ids_t> & hole,
		the_triangle_mesh_t & mesh);
  
  // tesselate this face into a set of triangles:
  bool tesselate(std::list<the_mesh_triangle_t> & triangles,
		 the_triangle_mesh_t & mesh) const;
  
  // data members:
  the_face_type_t face_type;
  the_dynamic_array_t<the_vertex_ids_t> vertices;
};


//----------------------------------------------------------------
// the_triangle_mesh_t
//
class the_triangle_mesh_t
{
  friend class the_mesh_triangle_t;
  
public:
  the_triangle_mesh_t();
  the_triangle_mesh_t(const the_triangle_mesh_t & mesh);
  
  // accessors:
  inline const the_dynamic_array_t<p3x1_t> & vx() const { return vx_; }
  inline const the_dynamic_array_t<v3x1_t> & vn() const { return vn_; }
  inline const the_dynamic_array_t<p2x1_t> & vt() const { return vt_; }
  
  inline const the_array_t<the_mesh_triangle_t> & triangles() const
  { return triangles_; }
  
  // calculate the bounding box of the mesh:
  template <class bbox_t>
  void calc_bbox(bbox_t & bbox) const
  {
    const unsigned int & num_vertices = vx_.size();
    for (unsigned int i = 0; i < num_vertices; i++)
    {
      bbox << vx_[i];
    }
  }
  
  // calculate the area of the mesh:
  float calc_area() const;
  
  // calculate vertex normals:
  void calc_vertex_normals(const bool & discard_current_normals = false,
			   const float & minimum_normal_dot_product = 0.3);
  
  // set vertex normals to face normals:
  void calc_face_normals(const bool & discard_current_normals = false);
  
  // calculate texture coordinates on the fly (sphere map) whenever
  // the number of vertices in the vertex table does not equal the
  // number of texture coordinates, or when forced:
  void calc_texture_coords(const bool & discard_current_coords = false);
  
  // this function will transform the vertices so that the center of their
  // bounding box would coincide with the origin:
  v3x1_t shift_to_center();
  
  // setup a closed surface mesh based on a set of contours:
  template <typename point_t>
  void setup(const std::vector<point_t> & points,
	     unsigned int points_per_contour)
  {
    unsigned int num_points = points.size();
    vx_.resize(num_points);
    for (unsigned int i = 0; i < num_points; i++)
    {
      const point_t & xyz = points[i];
      vx_[i] = p3x1_t(xyz[0], xyz[1], xyz[2]);
    }
    
    unsigned int num_contours = points.size() / points_per_contour;
    
    unsigned int num_triangles =
      (num_contours - 1) * points_per_contour * 2 +
      (points_per_contour - 2) * 2;
    
    triangles_.resize(num_triangles);
    for (unsigned int i = 1; i < num_contours; i++)
    {
      unsigned int offset = (i - 1) * points_per_contour;
      for (unsigned int j = 0; j < points_per_contour; j++)
      {
	the_mesh_triangle_t & tri_a = triangles_[(offset + j) * 2];
	the_mesh_triangle_t & tri_b = triangles_[(offset + j) * 2 + 1];
	tri_a.mesh_ = this;
	tri_b.mesh_ = this;
	
	tri_a.vx_[0] = offset + j;
	tri_a.vx_[2] = offset + j + points_per_contour;
	tri_a.vx_[1] =
	  offset + points_per_contour + (j + 1) % points_per_contour;
	
	tri_b.vx_[0] = offset + j;
	tri_b.vx_[1] = offset + (j + 1) % points_per_contour;
	tri_b.vx_[2] =
	  offset + points_per_contour + (j + 1) % points_per_contour;
      }
    }
    
    // cap the surface -- setup 2 cap faces from the boundary contours
    // and tesselate them:
    {
      the_face_info_t cap[2];
      unsigned int offset = num_points - points_per_contour;
      for (unsigned int i = 0; i < points_per_contour; i++)
      {
	cap[0].add(i);
	cap[1].add(i + offset);
      }
      
      for (unsigned int i = 0; i < 2; i++)
      {
	std::list<the_mesh_triangle_t> tri;
	cap[i].tesselate(tri, *this);
	
	unsigned int j = 0;
	for (std::list<the_mesh_triangle_t>::const_iterator iter =
	       tri.begin(); iter != tri.end(); ++iter, j++)
	{
	  unsigned int k =
	    (num_contours - 1) * points_per_contour * 2 +
	    (points_per_contour - 2) * i + j;
	  triangles_[k] = *iter;
	}
      }
    }
    
    for (unsigned int i = 0; i < num_triangles; i++)
    {
      const the_mesh_triangle_t & tri = triangles_[i];
      assert(tri.mesh_);
      // cerr << tri.vx_[0] << ' ' << tri.vx_[1] << ' ' << tri.vx_[2] << endl;
    }
    
    // this is optional:
    // calc_face_normals(true);
    // calc_texture_coords(true);
  }
  
  // array of mesh triangles:
  the_array_t<the_mesh_triangle_t> triangles_;
  
  // array of mesh vertices:
  the_dynamic_array_t<p3x1_t> vx_;
  the_dynamic_array_t<v3x1_t> vn_;
  the_dynamic_array_t<p2x1_t> vt_;
};
  

#endif // THE_TRIANGLE_MESH_HXX_

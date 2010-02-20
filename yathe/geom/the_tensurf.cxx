// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_tensurf.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Dec 28 13:36:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A tensor product surface.

// local includes:
#include "geom/the_tensurf.hxx"
#include "geom/the_grid.hxx"

// globals:
extern bool DRAW_POLYLINE;
extern bool DRAW_SURFACE;


//----------------------------------------------------------------
// the_tensurf_t::regenerate
// 
bool
the_tensurf_t::regenerate()
{
  const the_grid_t * g = grid();
  if (g == NULL) return false;
  if (!g->regenerated()) return false;
  if (g->is_singular()) return false;
  
  /*
  the_aa_bbox_t bbox;
  g->calc_bbox(bbox);
  cerr << bbox << endl;
  if (bbox.is_empty() || bbox.is_singular()) return false;
  */
  
  const size_t & rows = g->v().size();
  const size_t & cols = g->u().size();
  
  // update the knot vectors:
  knot_vector_u_.set_target_degree(3);
  knot_vector_u_.init(1, cols);
  if (!knot_vector_u_.update(cols)) return false;
  
  knot_vector_v_.set_target_degree(3);
  knot_vector_v_.init(1, rows);
  if (!knot_vector_v_.update(rows)) return false;
  
  size_t quads_u = std::min<size_t>(rows * 10, 100);
  size_t quads_v = std::min<size_t>(cols * 10, 100);
  
  // calculate the mesh:
  {
    // shorthand:
    std::vector<std::vector<the_vertex_t> > & pt = tri_mesh_;
    const std::vector<std::vector<p3x1_t> > & mesh = g->grid();
    
    resize(tri_mesh_, quads_u + 1, quads_v + 1);
    std::vector<the_bspline_geom_t> geom_row(rows);
    
    for (size_t i = 0; i < rows; i++)
    {
      geom_row[i].reset(mesh[i], knot_vector_u_.knots());
    }
    
    std::vector<p3x1_t> temp_pt(rows);
    the_bspline_geom_t geom_col;
    
    // sample along the u direction:
    for (size_t i = 0; i <= quads_u; i++)
    {
      const float u = float(i) / float(quads_u);
      
      // construct a bspline on the fly:
      for (size_t j = 0; j < rows; j++)
      {
	bool ok = geom_row[j].position(u, temp_pt[j]);
	if (!ok) assert(false);
      }
      
      geom_col.reset(temp_pt, knot_vector_v_.knots());
      
      // sample along the v direction:
      for (size_t j = 0; j <= quads_v; j++)
      {
	const float v = float(j) / float(quads_v);
	bool ok = geom_col.position(v, pt[i][j].vx);
	if (!ok) assert(false);
      }
    }
    
    // try to approximate the surface normals at the vertices:
    std::vector< std::vector<v3x1_t> > qn;
    resize(qn, quads_u, quads_v);
    
    for (size_t i = 0; i < quads_u; i++)
    {
      for (size_t j = 0; j < quads_v; j++)
      {
	qn[i][j] = !((pt[i + 1][j + 1].vx - pt[i][j].vx) %
		     (pt[i][j + 1].vx - pt[i + 1][j].vx));
      }
    }
    
    // assign the normals on the inside:
    for (size_t i = 1; i < quads_u; i++)
    {
      for (size_t j = 1; j < quads_v; j++)
      {
	pt[i][j].vn =
	  !(qn[i - 1][j - 1] + qn[i - 1][j] + qn[i][j - 1] + qn[i][j]);
      }
    }
    
    // assign the normals along the boundaries (excluding the corners):
    for (size_t i = 1; i < quads_u; i++)
    {
      float d2 = (pt[i][0].vx - pt[i][quads_v].vx).norm_sqrd();
      if (d2 <= THE_EPSILON)
      {
	// points are coincident:
	v3x1_t n =
	  !(qn[i - 1][0] + qn[i - 1][quads_v - 1] +
	    qn[i][0] + qn[i][quads_v - 1]);
	pt[i][0].vn = n;
	pt[i][quads_v].vn = n;
      }
      else
      {
	pt[i][0].vn = !(qn[i - 1][0] + qn[i][0]);
 	pt[i][quads_v].vn = !(qn[i - 1][quads_v - 1] + qn[i][quads_v - 1]);
      }
    }
    
    for (size_t j = 1; j < quads_v; j++)
    {
      float d2 = (pt[0][j].vx - pt[quads_u][j].vx).norm_sqrd();
      if (d2 <= THE_EPSILON)
      {
	// points are coincident:
	v3x1_t n =
	  !(qn[0][j - 1] + qn[quads_u - 1][j - 1] +
	    qn[0][j] + qn[quads_u - 1][j]);
	
	pt[0][j].vn = n;
	pt[quads_u][j].vn = n;
      }
      else
      {
	pt[0][j].vn = !(qn[0][j - 1] + qn[0][j]);
	pt[quads_u][j].vn = !(qn[quads_u - 1][j - 1] + qn[quads_u - 1][j]);
      }
    }
    
    // assign the normals at the corners:
    {
      float d0001 = (pt[0][0].vx - pt[0][quads_v].vx).norm_sqrd();
      float d1011 = (pt[quads_u][0].vx - pt[quads_u][quads_v].vx).norm_sqrd();
      float d0010 = (pt[0][0].vx - pt[quads_u][0].vx).norm_sqrd();
      float d0111 = (pt[0][quads_v].vx - pt[quads_u][quads_v].vx).norm_sqrd();
      
      pt[0][0].vn.assign(0.0, 0.0, 0.0);
      pt[0][quads_v].vn.assign(0.0, 0.0, 0.0);
      pt[quads_u][0].vn.assign(0.0, 0.0, 0.0);
      pt[quads_u][quads_v].vn.assign(0.0, 0.0, 0.0);
      
      if (d0001 <= THE_EPSILON)
      {
	v3x1_t n = qn[0][0] + qn[0][quads_v - 1];
	pt[0][0].vn += n;
	pt[0][quads_v].vn += n;
      }
      
      if (d1011 <= THE_EPSILON)
      {
	v3x1_t n = qn[quads_u - 1][0] + qn[quads_u - 1][quads_v - 1];
	pt[quads_u][0].vn += n;
	pt[quads_u][quads_v].vn += n;
      }
      
      if (d0010 <= THE_EPSILON)
      {
	v3x1_t n = qn[0][0] + qn[quads_u - 1][0];
	pt[0][0].vn += n;
	pt[quads_u][0].vn += n;
      }
      
      if (d0111 <= THE_EPSILON)
      {
	v3x1_t n = qn[0][quads_v - 1] + qn[quads_u - 1][quads_v - 1];
	pt[0][quads_v].vn += n;
	pt[quads_u][quads_v].vn += n;
      }
      
      if (!((d0010 <= THE_EPSILON) || (d0001 <= THE_EPSILON)))
      {
	pt[0][0].vn +=
	  pt[1][0].vn +
	  pt[0][1].vn +
	  pt[1][1].vn;
      }
      
      if (!((d0010 <= THE_EPSILON) || (d1011 <= THE_EPSILON)))
      {
	pt[quads_u][0].vn +=
	  pt[quads_u - 1][0].vn +
	  pt[quads_u - 1][1].vn +
	  pt[quads_u][1].vn;
      }
      
      if (!((d0001 <= THE_EPSILON) || (d0111 <= THE_EPSILON)))
      {
	pt[0][quads_v].vn +=
	  pt[0][quads_v - 1].vn +
	  pt[1][quads_v].vn +
	  pt[1][quads_v - 1].vn;
      }
      
      if (!((d1011 <= THE_EPSILON) || (d0111 <= THE_EPSILON)))
      {
	pt[quads_u][quads_v].vn +=
	  pt[quads_u][quads_v - 1].vn +
	  pt[quads_u - 1][quads_v - 1].vn +
	  pt[quads_u - 1][quads_v - 1].vn;
      }
      
      pt[0][0].vn.normalize();
      pt[0][quads_v].vn.normalize();
      pt[quads_u][0].vn.normalize();
      pt[quads_u][quads_v].vn.normalize();
    }
  }
  
  return true;
}

//----------------------------------------------------------------
// the_tensurf_dl_elem_t
// 
class the_tensurf_dl_elem_t : public the_dl_elem_t
{
public:
  the_tensurf_dl_elem_t(const std::vector< std::vector<the_vertex_t> > &
			tri_mesh,
			const size_t & iso_skip = 4,
			const the_color_t & tri_color = the_color_t::WHITE,
			const the_color_t & iso_color = the_color_t::BLACK,
			const the_color_t & crv_color = the_color_t::RED):
    tri_mesh_(tri_mesh),
    iso_skip_(iso_skip),
    tri_color_(tri_color),
    iso_color_(iso_color),
    crv_color_(crv_color)
  {}
  
  // virtual:
  void draw() const
  {
    size_t rows = tri_mesh_.size();
    if (rows == 0) return;
    
    size_t cols = tri_mesh_[0].size();
    /*
    const std::vector<std::vector<the_vertex_t> > & pt = tri_mesh_;
    const the_vertex_t & a = pt[0][0];
    const the_vertex_t & b = pt[0][cols - 1];
    const the_vertex_t & c = pt[rows - 1][cols - 1];
    const the_vertex_t & d = pt[rows - 1][0];
    glBegin(GL_QUADS);
    glNormal3fv(a.vn.data());
    glVertex3fv(a.vx.data());
    glNormal3fv(b.vn.data());
    glVertex3fv(b.vx.data());
    glNormal3fv(c.vn.data());
    glVertex3fv(c.vx.data());
    glNormal3fv(d.vn.data());
    glVertex3fv(d.vx.data());
    glEnd();
    return;
    */
    
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    {
      if (DRAW_SURFACE)
      {
	/*
	glDisable(GL_LIGHTING);
	glColor4f(1, 1, 1, 1);
	*/
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPolygonOffset(1.0, 1.0);
	
	glEnable(GL_COLOR_MATERIAL);
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
	glColor4fv(tri_color_.mul3(0.75).rgba());
	
	the_color_t bgcolor =
	  0.3f * the_color_t::AMPAD_DARK +
	  0.7f * the_color_t::AMPAD_LIGHT;
	
	the_color_t diffuse =
	  0.6f * tri_color_.mul3(bgcolor) +
	  0.4f * bgcolor;
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glColor4fv(diffuse.rgba());
	
	the_color_t specular =
	  0.36f * bgcolor +
	  0.64f * the_color_t::WHITE;
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);
	glColor4fv(specular.rgba());
	
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 7e+1);
	for (size_t i = 1; i < rows; i++)
	{
	  glBegin(GL_QUAD_STRIP);
	  for (size_t j = 0; j < cols; j++)
	  {
	    const the_vertex_t & a = tri_mesh_[i - 1][j];
	    const the_vertex_t & b = tri_mesh_[i][j];
	    
	    glNormal3fv(a.vn.data());
	    glVertex3fv(a.vx.data());
	    
	    glNormal3fv(b.vn.data());
	    glVertex3fv(b.vx.data());
	  }
	  glEnd();
	}
	
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_POLYGON_OFFSET_FILL);
      }
    }
    glDisable(GL_LIGHTING);
    glColor4fv(iso_color_.rgba());
    
    size_t iso_incr = iso_skip_ + 1;
    
    // FIXME: 20051024: if (!DRAW_POLYLINE)
    {
      for (size_t j = 0; j < cols; j += iso_incr)
      {
	glBegin(GL_LINE_STRIP);
	for (size_t i = 0; i < rows; i++)
	{
	  const the_vertex_t & a = tri_mesh_[i][j];
	  glNormal3fv(a.vn.data());
	  glVertex3fv(a.vx.data());
	}
	glEnd();
      }
      
      for (size_t i = 0; i < rows; i += iso_incr)
      {
	glBegin(GL_LINE_STRIP);
	for (size_t j = 0; j < cols; j++)
	{
	  const the_vertex_t & a = tri_mesh_[i][j];
	  glNormal3fv(a.vn.data());
	  glVertex3fv(a.vx.data());
	}
	glEnd();
      }
    }
    
    glPopAttrib();
  }
  
  void update_bbox(the_bbox_t & bbox) const
  {
    size_t rows = tri_mesh_.size();
    if (rows == 0) return;
    
    size_t cols = tri_mesh_[0].size();
    for (size_t i = 0; i < rows; i++)
    {
      for (size_t j = 0; j < cols; j++)
      {
	bbox << tri_mesh_[i][j].vx;
      }
    }
  }
  
private:
  // disable default constructor:
  the_tensurf_dl_elem_t();
  
protected:
  // references to the external data:
  const std::vector< std::vector<the_vertex_t> > & tri_mesh_;
  
  // how many rows/columns to skip between isolines:
  const size_t iso_skip_;
  
  // surface colors (going from low point color to high point color):
  const the_color_t tri_color_;
  const the_color_t iso_color_;
  const the_color_t crv_color_;
};

//----------------------------------------------------------------
// the_tensurf_t::dl_elem
// 
the_dl_elem_t *
the_tensurf_t::dl_elem() const
{
  return new the_tensurf_dl_elem_t(tri_mesh_,
				   4,
#if 0
				   the_color_t::RED,
				   the_color_t::ORANGE,
#else
				   the_color_t::WHITE,
				   the_color_t::BLACK,
#endif
				   the_color_t::RED);
}

//----------------------------------------------------------------
// the_tensurf_t::grid
// 
the_grid_t *
the_tensurf_t::grid() const
{
  if (direct_supporters().empty()) return NULL;
  if (registry() == NULL) return NULL;
  
  return registry()->elem<the_grid_t>(*(direct_supporters().rbegin()));
}

//----------------------------------------------------------------
// the_tensurf_t::dump
// 
void
the_tensurf_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_tensurf_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_primitive_t::dump(strm, INDNXT);
  strm << INDSTR << endl
       << INDSCP << "}" << endl << endl;
}

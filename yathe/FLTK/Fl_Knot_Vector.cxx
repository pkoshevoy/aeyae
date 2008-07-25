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


// File         : Fl_Knot_Vector.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2004/11/24 16:23
// Copyright    : (C) 2004
// License      : MIT
// Description  : FLTK widget for editing a bspline curve knot vector.

// local includes:
#include "FLTK/Fl_Knot_Vector.H"
#include "FLTK/the_fltk_std_widgets.hxx"

// FLTK includes:
#include <FL/fl_draw.H>
#include <FL/Fl.H>

// system includes:
#include <stdio.h>
#include <algorithm>


//----------------------------------------------------------------
// Fl_Knot_Vector::Fl_Knot_Vector
// 
Fl_Knot_Vector::Fl_Knot_Vector(int x,
			       int y,
			       int w,
			       int h,
			       const char * label):
  Fl_Widget(x, y, w, h, label),
  active_duck_(NULL)
{}

//----------------------------------------------------------------
// margin
// 
// calculate the left/right margins:
static float margin()
{
  int lw = 0;
  int lh = 0;
  fl_measure("-1.0000", lw, lh);
  return 0.5 * float(lw + 2);
}

//----------------------------------------------------------------
// Fl_Knot_Vector::handle
// 
int
Fl_Knot_Vector::handle(int event)
{
  static int ox = 0;
  
  int mx = Fl::event_x();
  int my = Fl::event_y();
  
  switch (event)
  {
    case FL_PUSH:
    {
      set_anchor();
      
      // activate a duck:
      float d_min = FLT_MAX;
      for (std::list<duck_t>::const_iterator i = anchor_.begin();
	   i != anchor_.end(); ++i)
      {
	const duck_t & duck = *i;
	int dx = abs(mx - duck.x);
	int dy = abs(my - duck.y);
	
	float d = sqrt(float(dx * dx) + float(dy * dy));
	if (d < d_min)
	{
	  d_min = d;
	  active_duck_ = &duck;
	  ox = mx - duck.x;
	}
      }
      
      if (d_min > 7.0) active_duck_ = NULL;
    }
    break;
    
    case FL_RELEASE:
    {
      // deactivate the duck:
      active_duck_ = NULL;
    }
    break;
    
    case FL_DRAG:
      if (active_duck_ != NULL)
      {
	const float t0 = parameter_.front();
	const float t1 = parameter_.back();
	
	float M = margin();
	float X = float(x()) + M;
	float W = float(w()) - 2.0 * M;
	
	float tx = float(mx - ox);
	float t = t0 + (t1 - t0) * (tx - X) / W;
	
	float l = knots_[active_duck_->i - 1];
	float r = knots_[active_duck_->i + 1];
	t = std::max(l, std::min(r, t));
	
	knots_[active_duck_->i] = t;
	do_callback();
      }
      break;
      
    default:
      return Fl_Widget::handle(event);
  }
  
  redraw();
  return 1;
}

//----------------------------------------------------------------
// arrow_t
// 
typedef enum { LEFT_ARROW_E, RIGHT_ARROW_E, UP_ARROW_E } arrow_t;

//----------------------------------------------------------------
// draw_arrow
// 
static void
draw_arrow(int x, int y, arrow_t a)
{
  if (a == LEFT_ARROW_E)
  {
    fl_xyline(x + 1, y - 2, x + 2);
    fl_xyline(x - 1, y - 1, x + 2);
    fl_xyline(x - 3, y,     x + 2);
    fl_xyline(x - 1, y + 1, x + 2);
    fl_xyline(x + 1, y + 2, x + 2);
  }
  else if (a == RIGHT_ARROW_E)
  {
    fl_xyline(x - 2, y - 2, x - 1);
    fl_xyline(x - 2, y - 1, x + 1);
    fl_xyline(x - 2, y,     x + 3);
    fl_xyline(x - 2, y + 1, x + 1);
    fl_xyline(x - 2, y + 2, x - 1);
  }
  else if (a == UP_ARROW_E)
  {
    fl_yxline(x - 2, y + 2, y + 1);
    fl_yxline(x - 1, y + 2, y - 1);
    fl_yxline(x,     y + 2, y - 3);
    fl_yxline(x + 1, y + 2, y - 1);
    fl_yxline(x + 2, y + 2, y + 1);
  }
}

#define SET_FL_COLOR( R, G, B ) \
fl_color(active_r() ? fl_rgb_color( R , G , B ) : \
fl_inactive(fl_rgb_color( R , G , B )))

//----------------------------------------------------------------
// Fl_Knot_Vector::draw
// 
void
Fl_Knot_Vector::draw()
{
  Fl_Color saved_cr = fl_color();
  int saved_ff = fl_font();
  int saved_fs = fl_size();
  // fl_font(FL_SCREEN_BOLD, 10);
  // fl_font(FL_SCREEN, 10);
  fl_font(FL_SCREEN, 8);
  
  float M = margin();
  float F = float(fl_size());
  float X = float(x()) + M;
  float Y = float(y() + PH - 2);
  float W = float(w()) - 2.0 * M;
  
  // clear the widget:
  fl_draw_box(FL_FLAT_BOX, x(), y(), w(), h(), color());
  
  // draw the ruler:
  {
    float y_ref = Y + F;

    SET_FL_COLOR(0, 0, 0);
    fl_xyline(int(X), int(y_ref), int(X + W));
    
    unsigned int n_notches = (w() / 100 + 1) * 10;
    for (unsigned int i = 0; i <= n_notches; i++)
    {
      float t = float(i) / float(n_notches);
      float tx = t * W + X;
      
      float df = 0.25 * F + 0.25 * F * float(i % 5 == 0);
      
      fl_yxline(int(tx), int(y_ref), int(y_ref + df));
    }
  }
  
  // draw the knots:
  if (knots_.size() > 1)
  {
    ducks_.clear();
    multiplicity_.clear();
    parameter_.clear();
    
    // first classify knots according to their multiplicity:
    multiplicity_.push_back(1);
    parameter_.push_back(knots_[0]);
    
    for (unsigned int i = 1; i < knots_.size(); i++)
    {
      float d = fabs(knots_[i] - knots_[i - 1]);
      if (d < THE_EPSILON)
      {
	(*(multiplicity_.rbegin()))++;
      }
      else
      {
	multiplicity_.push_back(1);
	parameter_.push_back(knots_[i]);
      }
    }
    
    float y_ref = Y + 0.75 * F;
    float df = 0.75 * F;
    
    std::list<unsigned int>::const_iterator mi = multiplicity_.begin();
    std::list<float>::const_iterator pi = parameter_.begin();
    
    unsigned int knot_index = 0;
    
    const float t0 = parameter_.front();
    const float t1 = parameter_.back();
    
    while (mi != multiplicity_.end())
    {
      const unsigned int & m = *mi;
      const float & t = *pi;
      
      float tx = ((t - t0) / (t1 - t0)) * W + X;
      
      SET_FL_COLOR(0, 0, 0);
      if (active_duck_ != NULL)
      {
	if (knot_index == active_duck_->i ||
	    (knot_index + m - 1) == active_duck_->i)
	{
	  SET_FL_COLOR(255, 0, 0);
	  fl_yxline(int(tx), int(y_ref), int(y_ref + df));
	  fl_yxline(int(tx) - 1, int(y_ref), int(y_ref + df));
	  fl_yxline(int(tx) + 1, int(y_ref), int(y_ref + df));
	}
      }
      
      char label[16];
      if (m != 1 || ((active_duck_ != NULL) &&
		     (knot_index == active_duck_->i ||
		      (knot_index + m - 1) == active_duck_->i)))
      {
	sprintf(label, "%.2f", t);
	
	int lw = 0;
	int lh = 0;
	fl_measure(label, lw, lh);
	fl_draw(label, int(tx) - lw / 2, int(y_ref - 2));
      }
      
      if (m == 1)
      {
	// up arrow:
	int dx = int(tx);
	int dy = int(Y + 2.0 * F);
	
	if (pi == parameter_.begin() ||
	    &(*pi) == &(*(parameter_.rbegin())))
	{
	  fl_color(fl_inactive(fl_rgb_color(0, 0, 0)));
	}
	else
	{
	  SET_FL_COLOR(255, 0, 0);
	  ducks_.push_back(duck_t(dx, dy, knot_index));
	}
	draw_arrow(dx, dy, UP_ARROW_E);
	
	SET_FL_COLOR(0, 0, 0);
	sprintf(label, "%i", knot_index);
	fl_draw(label, dx + 4, dy + 3);
      }
      else
      {
	// right arrow:
	int dx = int(tx);
	int dy = int(Y + 2.0 * F);
	
	if (&(*pi) == &(*(parameter_.rbegin())))
	{
	  fl_color(fl_inactive(fl_rgb_color(0, 0, 0)));
	}
	else
	{
	  SET_FL_COLOR(255, 0, 0);
	  ducks_.push_back(duck_t(dx, dy, knot_index + m - 1));
	}
	draw_arrow(dx, dy, RIGHT_ARROW_E);
	
	SET_FL_COLOR(0, 0, 0);
	sprintf(label, "%i", knot_index + m - 1);
	fl_draw(label, dx + 4, dy + 3);
	
	// left arrow:
	dy = int(Y + 3.0 * F);
	
	if (pi == parameter_.begin())
	{
	  fl_color(fl_inactive(fl_rgb_color(0, 0, 0)));
	}
	else
	{
	  SET_FL_COLOR(255, 0, 0);
	  ducks_.push_back(duck_t(dx, dy, knot_index));
	}
	draw_arrow(dx, dy, LEFT_ARROW_E);
	
	SET_FL_COLOR(0, 0, 0);
	sprintf(label, "%i", knot_index);
	fl_draw(label, dx + 4, dy + 3);
      }
      
      // move on to the next parameter:
      knot_index += m;
      ++mi;
      ++pi;
    }
  }
  
  fl_color(saved_cr);
  fl_font(saved_ff, saved_fs);
}

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


// File         : Fl_Knot_Vector.H
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2004/11/24 16:23
// Copyright    : (C) 2004
// License      : MIT
// Description  : FLTK widget for editing a bspline curve knot vector.

#ifndef Fl_Knot_Vector_H
#define Fl_Knot_Vector_H

// system includes:
#include <vector>
#include <list>

// FLTK includes:
#include <FL/Fl_Widget.H>

// local includes:
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// duck_t
// 
class duck_t
{
public:
  duck_t(int X = -1,
	 int Y = -1,
	 unsigned int I = UINT_MAX):
    x(X),
    y(Y),
    i(I)
  {}
  
  inline bool operator == (const duck_t & d) const
  { return i == d.i && x == d.x && y == d.y; }
  
  int x;
  int y;
  unsigned int i;
};

//----------------------------------------------------------------
// Fl_Knot_Vector
// 
class FL_EXPORT Fl_Knot_Vector : public Fl_Widget
{
public:
  Fl_Knot_Vector(int x, int y, int w, int h, const char * label = 0);
  
  // virtual:
  int handle(int event);
  
  // accessors:
  inline const std::vector<float> & value() const
  { return knots_; }
  
  inline void value(const std::vector<float> & knots)
  {
    knots_ = knots;
    redraw();
  }
  
protected:
  // virtual:
  void draw();
  
private:
  inline void set_anchor()
  { anchor_ = ducks_; }
  
  std::vector<float> knots_;
  
  std::list<unsigned int> multiplicity_;
  std::list<float> parameter_;
  std::list<duck_t> ducks_;
  std::list<duck_t> anchor_;
  
  const duck_t * active_duck_;
};


#endif // Fl_Knot_Vector_H

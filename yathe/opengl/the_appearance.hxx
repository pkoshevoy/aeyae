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


// File         : the_appearance.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The appearance class in combination with the palette class
//                provides basic theme/skin support for the OpenGL view widget.

#ifndef THE_APPEARANCE_HXX_
#define THE_APPEARANCE_HXX_

// local includes:
#include "opengl/the_palette.hxx"
#include "opengl/the_ep_grid.hxx"

// forward declarations:
class the_view_t;


//----------------------------------------------------------------
// the_appearance_t
//
// base class for various view appearances (original, ampad, etc..)
class the_appearance_t
{
public:
  the_appearance_t(the_palette_id_t id): palette_(id) {}
  virtual ~the_appearance_t() {}
  
  // palette accessor:
  inline const the_palette_t & palette() const
  { return palette_; }
  
  virtual void draw_background(the_view_t & view) const;
  
  virtual void draw_edit_plane(the_view_t & view) const = 0;
  
  virtual void draw_coordinate_system(the_view_t & view) const;
  
  virtual void draw_view_label(the_view_t & view) const;
  
protected:
  // color palette specific to the appearance:
  the_palette_t palette_;
};


//----------------------------------------------------------------
// the_original_appearance_t
// 
class the_original_appearance_t : public the_appearance_t
{
public:
  the_original_appearance_t(): the_appearance_t(THE_ORIGINAL_PALETTE_E) {}
  
  // virtual:
  void draw_edit_plane(the_view_t & view) const;
};


//----------------------------------------------------------------
// the_ampad_appearance_t
// 
class the_ampad_appearance_t : public the_appearance_t
{
public:
  the_ampad_appearance_t(): the_appearance_t(THE_AMPAD_PALETTE_E) {}
  
  // virtual:
  void draw_edit_plane(the_view_t & view) const;
};


//----------------------------------------------------------------
// the_generic_appearance_t
// 
class the_generic_appearance_t : public the_appearance_t
{
public:
  the_generic_appearance_t(the_palette_id_t id = THE_NORCOM_PALETTE_E):
    the_appearance_t(id) {}
  
  // virtual:
  void draw_edit_plane(the_view_t & view) const;
};


//----------------------------------------------------------------
// THE_APPEARANCE
// 
extern const the_appearance_t & THE_APPEARANCE;


#endif // THE_APPEARANCE_HXX_

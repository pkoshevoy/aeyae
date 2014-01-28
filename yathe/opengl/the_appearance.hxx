// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
namespace yathe
{
  extern void set_default_appearance(const the_appearance_t * a);
  extern const the_appearance_t * get_default_appearance();
}

//----------------------------------------------------------------
// THE_APPEARANCE
//
#define THE_APPEARANCE (*yathe::get_default_appearance())


#endif // THE_APPEARANCE_HXX_

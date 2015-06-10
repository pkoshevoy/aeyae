// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_font.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : An OpenGL bitmap font class.

#ifndef THE_FONT_HXX_
#define THE_FONT_HXX_

// local includes:
#include "opengl/the_symbols.hxx"


//----------------------------------------------------------------
// the_font_t
//
class the_font_t : public the_symbols_t
{
public:
  the_font_t(const the_text_t & n): the_symbols_t(n) {}

  // virtual: the number of display lists to be compiled:
  unsigned int size() const { return 128; }

  void print(const the_text_t & str,
	     const p3x1_t & pos,
	     const the_color_t & color) const;

  void print(const the_text_t & str,
	     const p3x1_t & pos,
	     const the_color_t & font_color,
	     const the_color_t & mask_color) const;

  void print_mask(const the_text_t & str,
		  const p3x1_t & pos,
		  const the_color_t & color) const;

protected:
  // disable default constructor:
  the_font_t();
};


#endif // THE_FONT_HXX_

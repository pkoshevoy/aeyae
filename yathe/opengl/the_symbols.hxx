// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_symbols.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : OpenGL 2D point symbols bitmap font.

#ifndef THE_SYMBOLS_HXX_
#define THE_SYMBOLS_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "utils/the_text.hxx"

// forward declarations:
class the_color_t;


//----------------------------------------------------------------
// the_symbols_t
// 
class the_symbols_t
{
public:
  the_symbols_t(const the_text_t & name):
    name_(name),
    dl_offset_(0)
  {}
  
  virtual ~the_symbols_t() {}
  
  // the name of this symbol set:
  inline const the_text_t & name() const
  { return name_; }
  
  // draw a symbol at a specified raster position:
  void draw(const the_color_t & color,
	    const p3x1_t & pos,
	    const unsigned int & symbol_id) const;
  
  void draw(const unsigned int & symbol_id,
	    const p3x1_t & pos,
	    const the_color_t & color,
	    const the_color_t & mask_color) const;
  
  // draw a symbol at the current raster position:
  void draw(const unsigned int & symbol_id) const;
  
  // change the current raster position:
  void move(const p3x1_t & pos) const;
  
  // dimensions of symbols of this set:
  virtual unsigned int width() const = 0;
  virtual unsigned int height() const = 0;
  
  // origin of a symbol:
  virtual float x_origin() const = 0;
  virtual float y_origin() const = 0;
  
  // how much the raster position will be advanced once a symbol is drawn:
  virtual float x_step() const = 0;
  virtual float y_step() const = 0;
  
  // symbol bitmap accessors:
  virtual unsigned char *
  bitmap(const unsigned int & symbol_id) const = 0;
  
  virtual unsigned char *
  bitmap_mask(const unsigned int & /* symbol_id */) const
  { return NULL; }
  
  // the number of display lists to be compiled:
  virtual unsigned int size() const = 0;
  
  // compile/decompile the OpenGL display lists:
  void compile();
  void decompile();
  
protected:
  // disable default constructor:
  the_symbols_t();
  
  // low level bitmap drawing helper functions:
  void draw_bitmap(const unsigned int & id,
		   const int & offset_x = 0,
		   const int & offset_y = 0) const;
  void draw_bitmap_mask(const unsigned int & id) const;
  
  // name of this display list collection:
  the_text_t name_;
  
  // starting display list id offset:
  unsigned int dl_offset_;
};

  
#endif // THE_SYMBOLS_HXX_

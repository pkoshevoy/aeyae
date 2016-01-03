// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_GRID_VIEW_STYLE_H_
#define YAE_GRID_VIEW_STYLE_H_

// local interfaces:
#include "yaePlaylistView.h"
#include "yaePlaylistViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // GridViewStyle
  //
  struct GridViewStyle : public PlaylistViewStyle
  {
    GridViewStyle(const char * id, PlaylistView & playlist);
  };

}


#endif // YAE_GRID_VIEW_STYLE_H_

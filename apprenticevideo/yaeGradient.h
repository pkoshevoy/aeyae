// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_GRADIENT_H_
#define YAE_GRADIENT_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // Gradient
  //
  struct Gradient : public Item
  {
    enum Orientation { kHorizontal, kVertical };

    Gradient(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    ItemRef opacity_;
    TGradientRef color_;
    Orientation orientation_;
  };

}


#endif // YAE_GRADIENT_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_RECTANGLE_H_
#define YAE_RECTANGLE_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // Rectangle
  //
  struct Rectangle : public Item
  {
    Rectangle(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // border width:
    ItemRef border_;

    ItemRef opacity_;
    ColorRef color_;
    ColorRef colorBorder_;
  };

}


#endif // YAE_RECTANGLE_H_

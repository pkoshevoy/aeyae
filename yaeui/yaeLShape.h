// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 21:32:22 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LSHAPE_H_
#define YAE_LSHAPE_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // LShape
  //
  class YAEUI_API LShape : public Item
  {
    LShape(const LShape &);
    LShape & operator = (const LShape &);

  public:
    LShape(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    bool visible() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // the thickness of the L shape
    ItemRef weight_;
    ItemRef opacity_;
    ColorRef color_;
  };

}


#endif // YAE_LSHAPE_H_

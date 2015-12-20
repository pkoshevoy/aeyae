// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ROUND_RECT_H_
#define YAE_ROUND_RECT_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // RoundRect
  //
  class RoundRect : public Item
  {
    RoundRect(const RoundRect &);
    RoundRect & operator = (const RoundRect &);

  public:
    RoundRect(const char * id);
    ~RoundRect();

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // keep implementation details private:
    struct TPrivate;
    TPrivate * p_;

    // corner radius:
    ItemRef radius_;

    // border width:
    ItemRef border_;

    ColorRef color_;
    ColorRef colorBorder_;
    ColorRef background_;
  };

}


#endif // YAE_ROUND_RECT_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan  1 17:16:07 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_IMAGE_LIVE_H_
#define YAE_IMAGE_LIVE_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // ImageLive
  //
  struct ImageLive : public Item
  {
    ImageLive(const char * id);

    // virtual:
    void uncache();

    // virtual:
    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const;

    // virtual:
    void paintContent() const;

    mutable Canvas * canvas_;
    ItemRef opacity_;
  };

}


#endif // YAE_IMAGE_LIVE_H_

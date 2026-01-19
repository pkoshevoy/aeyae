// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Feb  7 09:07:35 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yaeui:
#include "yaePlayerStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerStyle::PlayerStyle
  //
  PlayerStyle::PlayerStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view)
  {
    font_.setFamily(QString::fromUtf8("Lucida Grande"));

    unit_size_.set(new UnitSize(view));
  }

  //----------------------------------------------------------------
  // PlayerStyle::uncache
  //
  void
  PlayerStyle::uncache()
  {
    ItemViewStyle::uncache();
  }

}

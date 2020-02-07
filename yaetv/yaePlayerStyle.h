// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Feb  7 09:07:35 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_STYLE_H_
#define YAE_PLAYER_STYLE_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"

// local:
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerStyle
  //
  struct PlayerStyle : public ItemViewStyle
  {
    PlayerStyle(const char * id, const ItemView & view);

    // virtual:
    void uncache();
  };
}


#endif // YAE_PLAYER_STYLE_H_

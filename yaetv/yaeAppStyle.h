// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Feb  7 08:50:19 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APP_STYLE_H_
#define YAE_APP_STYLE_H_

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
  // AppStyle
  //
  struct AppStyle : public ItemViewStyle
  {
    AppStyle(const char * id, const ItemView & view);

    virtual void uncache();

    ColorRef bg_sidebar_;
    ColorRef bg_splitter_;
    ColorRef bg_epg_;
    ColorRef fg_epg_;
    ColorRef fg_epg_chan_;
    ColorRef bg_epg_tile_;
    ColorRef bg_epg_scrollbar_;
    ColorRef fg_epg_scrollbar_;
    ColorRef bg_epg_cancelled_;
    ColorRef bg_epg_rec_;

    TGradientPtr bg_epg_header_;
    TGradientPtr bg_epg_shadow_;
    TGradientPtr bg_epg_channel_;

    TTexturePtr collapsed_;
    TTexturePtr expanded_;
  };

}


#endif // YAE_APP_STYLE_H_

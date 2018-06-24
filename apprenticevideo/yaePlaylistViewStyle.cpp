// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <algorithm>
#include <cmath>

// local interfaces:
#include "yaePlaylistViewStyle.h"
#include "yaeText.h"
#include "yaeTexture.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistViewStyle::PlaylistViewStyle
  //
  PlaylistViewStyle::PlaylistViewStyle(const char * id,
                                       PlaylistView & playlist):
    ItemViewStyle(id, playlist),
    playlist_(playlist),
    now_playing_(Item::addNewHidden<Text>("now_playing")),
    eyetv_badge_(Item::addNewHidden<Text>("eyetv_badge"))
  {
    xbutton_ = Item::addHidden<Texture>
      (new Texture("xbutton", QImage())).sharedPtr<Texture>();

    collapsed_ = Item::addHidden<Texture>
      (new Texture("collapsed", QImage())).sharedPtr<Texture>();

    expanded_ = Item::addHidden<Texture>
      (new Texture("expanded", QImage())).sharedPtr<Texture>();
  }

  //----------------------------------------------------------------
  // PlaylistViewStyle::uncache
  //
  void
  PlaylistViewStyle::uncache()
  {
    bg_xbutton_.uncache();
    fg_xbutton_.uncache();

    bg_hint_.uncache();
    fg_hint_.uncache();

    bg_badge_.uncache();
    fg_badge_.uncache();

    bg_label_.uncache();
    fg_label_.uncache();

    bg_label_selected_.uncache();
    fg_label_selected_.uncache();

    bg_group_.uncache();
    fg_group_.uncache();

    bg_item_.uncache();
    bg_item_playing_.uncache();
    bg_item_selected_.uncache();

    ItemViewStyle::uncache();
  }

}

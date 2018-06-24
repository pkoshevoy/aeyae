// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>

// local interfaces:
#include "yaeListViewStyle.h"
#include "yaeText.h"
#include "yaeTexture.h"


namespace yae
{

  //----------------------------------------------------------------
  // ListViewStyle::ListViewStyle
  //
  ListViewStyle::ListViewStyle(const char * id, PlaylistView & playlist):
    PlaylistViewStyle(id, playlist)
  {
    // filter shadow gradient:
    filter_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *filter_shadow_;
      gradient[0.000] = Color(0x1f1f1f, 1.0);
      gradient[0.519] = Color(0x1f1f1f, 0.9);
      gradient[0.520] = Color(0x000000, 0.5);
      gradient[0.570] = Color(0x000000, 0.0);
      gradient[1.000] = Color(0x000000, 0.0);
    }

    // timeline shadow gradient:
    timeline_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *timeline_shadow_;
      gradient[0.000] = Color(0x000000, 0.0);
      gradient[0.580] = Color(0x000000, 0.0);
      gradient[0.634] = Color(0x000000, 0.5);
      gradient[0.635] = Color(0x000000, 0.9);
      gradient[1.000] = Color(0x000000, 1.0);
    }

    // color palette:
    bg_ = ColorRef::constant(Color(0x1f1f1f, 0.87));
    fg_ = ColorRef::constant(Color(0xffffff, 1.0));

    border_ = ColorRef::constant(Color(0x7f7f7f, 1.0));
    cursor_ = ColorRef::constant(Color(0xf12b24, 1.0));
    scrollbar_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    separator_ = scrollbar_;
    underline_ = cursor_;

    bg_xbutton_ = ColorRef::constant(Color(0x000000, 0.0));
    fg_xbutton_ = ColorRef::constant(Color(0xffffff, 0.5));

    bg_focus_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    fg_focus_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_edit_selected_ = ColorRef::constant(Color(0xffffff, 1.0));
    fg_edit_selected_ = ColorRef::constant(Color(0x000000, 1.0));

    bg_timecode_ = ColorRef::constant(Color(0x7f7f7f, 0.25));
    fg_timecode_ = ColorRef::constant(Color(0xFFFFFF, 0.5));

    bg_controls_ = bg_timecode_;
    fg_controls_ = ColorRef::constant(fg_timecode_.get().opaque(0.75));

    bg_hint_ = ColorRef::constant(Color(0x1f1f1f, 0.0));
    fg_hint_ = ColorRef::constant(Color(0xffffff, 0.5));

    bg_badge_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    fg_badge_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_group_ = ColorRef::constant(Color(0x7f7f7f, 0.75));
    fg_group_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_item_ = ColorRef::constant(Color(0x000000, 0.0));
    bg_item_playing_ = ColorRef::constant(Color(0x7f7f7f, 0.1));
    bg_item_selected_ = ColorRef::constant(Color(0x7f7f7f, 0.2));

    bg_label_ = ColorRef::constant(Color(0x3f3f3f, 0.5));
    fg_label_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_label_selected_ = ColorRef::constant(bg_item_selected_.get().
                                            transparent());
    fg_label_selected_ = ColorRef::constant(Color(0xffffff, 1.0));

    timeline_played_ = cursor_;
    timeline_included_ = ColorRef::constant(Color(0xFFFFFF, 0.5));
    timeline_excluded_ = ColorRef::constant(Color(0xFFFFFF, 0.2));

    // generate an x-button texture:
    {
      QImage img = xbuttonImage(128,
                                fg_xbutton_.get(),
                                fg_xbutton_.get().transparent(),
                                0.1);
      xbutton_->setImage(img);
    }

    // generate collapsed group button texture:
    {
      QImage img = triangleImage(128,
                                 fg_group_.get(),
                                 bg_group_.get().transparent(),
                                 90.0);
      collapsed_->setImage(img);
    }

    // generate expanded group button texture:
    {
      QImage img = triangleImage(128,
                                 fg_group_.get(),
                                 bg_group_.get().transparent(),
                                 180.0);
      expanded_->setImage(img);
    }

    // generate pause button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_.get(),
                             bg_controls_.get().transparent(),
                             2, 0.8);
      pause_->setImage(img);
    }

    // generate play button texture:
    {
      QImage img = triangleImage(128,
                                 fg_controls_.get(),
                                 bg_controls_.get().transparent(),
                                 90.0);
      play_->setImage(img);
    }

    // generate playlist grid on button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_.get(),
                             bg_controls_.get().transparent(),
                             3, 0.7, 90);
      grid_on_->setImage(img);
    }

    // generate playlist grid off button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_.get().a_scaled(0.3),
                             bg_controls_.get().transparent(),
                             3, 0.7, 90);
      grid_off_->setImage(img);
    }

    // configure common style attributes:
    cell_width_ = addExpr(new GetScrollviewWidth(playlist_), 1.0, -2.0);
    cell_height_ = ItemRef::reference(title_height_, 0.84);

    font_size_ = ItemRef::reference(title_height_, 0.46);

    now_playing_.anchors_.top_ = ItemRef::constant(0.0);
    now_playing_.anchors_.left_ = ItemRef::constant(0.0);
    now_playing_.text_ = TVarRef::constant(TVar(QObject::tr("now playing")));
    now_playing_.font_ = font_small_;
    now_playing_.font_.setBold(true);
    now_playing_.fontSize_ = ItemRef::reference(font_size_, 0.7 * kDpiScale);

    eyetv_badge_.font_ = font_large_;
    eyetv_badge_.anchors_.top_ = ItemRef::constant(0.0);
    eyetv_badge_.anchors_.left_ = ItemRef::constant(0.0);
    eyetv_badge_.text_ = TVarRef::constant(TVar(QObject::tr("eyetv")));
    eyetv_badge_.font_.setBold(false);
    eyetv_badge_.fontSize_ = ItemRef::reference(font_size_, 1.0 * kDpiScale);

    layout_root_.reset(new GroupListLayout());
    layout_group_.reset(new ItemListLayout());
    layout_item_.reset(new ItemListRowLayout());
  }

}

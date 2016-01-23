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
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_small_.setHintingPreference(QFont::PreferFullHinting);
#endif
    font_small_.setStyleHint(QFont::SansSerif);
    font_small_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
#if 0 // (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
                                  QFont::NoSubpixelAntialias |
#endif
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    // main font:
    font_ = font_small_;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    font_.setWeight(78);
#endif

    font_large_ = font_small_;

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_large_.setFamily("impact");
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_large_.setStretch(QFont::Condensed);
      font_large_.setWeight(QFont::Black);
    }

    // filter shadow gradient:
    filter_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *filter_shadow_;
      gradient[0.000] = Color(0xdfdfdf, 1.0);
      gradient[0.519] = Color(0xdfdfdf, 0.9);
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
      gradient[0.635] = Color(0xdfdfdf, 0.9);
      gradient[1.000] = Color(0xdfdfdf, 1.0);
    }

    // color palette:
    bg_ = Color(0xffffff, 0.87);
    fg_ = Color(0x000000, 1.0);

    border_ = Color(0xafafaf, 1.0);
    cursor_ = Color(0xff7f00, 1.0);
    scrollbar_ = Color(0xafafaf, 0.75);
    separator_ = Color(0x7f7f7f, 0.25);
    underline_ = Color(0x3f7fff, 1.0);

    bg_xbutton_ = Color(0x000000, 0.0);
    fg_xbutton_ = Color(0x000000, 0.25);

    bg_focus_ = Color(0xffffff, 0.5);
    fg_focus_ = Color(0x000000, 0.5);

    bg_edit_selected_ = underline_;
    fg_edit_selected_ = Color(0xffffff, 1.0);

    bg_timecode_ = Color(0xffffff, 0.5);
    fg_timecode_ = Color(0x3f3f3f, 0.5);

    bg_hint_ = Color(0xffffff, 0.0);
    fg_hint_ = Color(0x1f1f1f, 0.5);

    bg_badge_ = Color(0xff7f00, 1.0);
    fg_badge_ = Color(0xffffff, 0.5);

    bg_label_ = Color(0xffffff, 0.0);
    fg_label_ = Color(0x000000, 1.0);

    bg_group_ = Color(0xafafaf, 0.75);
    fg_group_ = Color(0xffffff, 1.0);

    bg_item_ = Color(0xffffff, 0.0);
    bg_item_playing_ = cursor_.scale_a(0.75);
    bg_item_selected_ = underline_.scale_a(0.75);

    bg_label_selected_ = bg_item_selected_.transparent();
    fg_label_selected_ = Color(0xffffff, 1.0);

    timeline_played_ = underline_;
    timeline_included_ = timeline_played_.scale_a(0.5);
    timeline_excluded_ = Color(0x7f7f7f, 0.5);

    // configure common style attributes:
    title_height_.height_ =
      title_height_.addExpr(new CalcTitleHeight(playlist_, 24.0));

    // generate an x-button texture:
    {
      QImage img =
        xbuttonImage(128, fg_xbutton_, bg_xbutton_.transparent(), 0.1);
      xbutton_->setImage(img);
    }

    // generate collapsed group button texture:
    {
      QImage img =
        triangleImage(128, fg_xbutton_, bg_xbutton_.transparent(), 90.0);
      collapsed_->setImage(img);
    }

    // generate expanded group button texture:
    {
      QImage img =
        triangleImage(128, fg_xbutton_, bg_xbutton_.transparent(), 180.0);
      expanded_->setImage(img);
    }

    cell_width_.width_ = cell_width_.
      addExpr(new GetScrollviewWidth(playlist_), 1.0, -2.0);

    cell_height_.height_ =
      ItemRef::scale(title_height_, kPropertyHeight, 1.0);

    font_size_.height_ =
      ItemRef::scale(title_height_, kPropertyHeight, 0.52);

    now_playing_.anchors_.top_ = ItemRef::constant(0.0);
    now_playing_.anchors_.left_ = ItemRef::constant(0.0);
    now_playing_.text_ = TVarRef::constant(TVar(QObject::tr("now playing")));
    now_playing_.font_ = font_small_;
    now_playing_.font_.setBold(true);
    now_playing_.fontSize_ = ItemRef::scale(font_size_,
                                            kPropertyHeight,
                                            0.7 * kDpiScale);

    eyetv_badge_.font_ = font_large_;
    eyetv_badge_.anchors_.top_ = ItemRef::constant(0.0);
    eyetv_badge_.anchors_.left_ = ItemRef::constant(0.0);
    eyetv_badge_.text_ = TVarRef::constant(TVar(QObject::tr("eyetv")));
    eyetv_badge_.font_.setBold(false);
    eyetv_badge_.fontSize_ = ItemRef::scale(font_size_,
                                            kPropertyHeight,
                                            0.7 * kDpiScale);

    layout_root_.reset(new GroupListLayout());
    layout_group_.reset(new ItemListLayout());
    layout_item_.reset(new ItemListRowLayout());
  }

}

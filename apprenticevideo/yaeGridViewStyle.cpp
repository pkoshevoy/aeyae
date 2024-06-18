// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>

// local interfaces:
#include "yaeGridViewStyle.h"
#include "yaeText.h"
#include "yaeTexture.h"


namespace yae
{

  //----------------------------------------------------------------
  // GridViewStyle::GridViewStyle
  //
  GridViewStyle::GridViewStyle(const char * id, PlaylistView & playlist):
    PlaylistViewStyle(id, playlist)
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_small_.setHintingPreference(QFont::PreferFullHinting);
#endif
    font_small_.setStyleHint(QFont::SansSerif);

    uint32_t style_strategy = QFont::PreferOutline;
    style_strategy |= QFont::PreferAntialias;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    style_strategy |= QFont::OpenGLCompatible;
#endif
    font_small_.setStyleStrategy((QFont::StyleStrategy)style_strategy);

    // main font:
    font_large_ = font_small_;

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_large_.setFamily("impact");

#if !(defined(_WIN32) ||                        \
      defined(__APPLE__) ||                     \
      QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
      font_large_.setStretch(QFont::Condensed);
#endif
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_large_.setStretch(QFont::Condensed);
      font_large_.setWeight(QFont::Black);
    }

    font_ = font_large_;

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

    // color palette:
    bg_xbutton_ = ColorRef::constant(Color(0x000000, 0.0));
    fg_xbutton_ = ColorRef::constant(Color(0xffffff, 0.5));

    bg_hint_ = ColorRef::constant(Color(0x1f1f1f, 0.0));
    fg_hint_ = ColorRef::constant(Color(0xffffff, 0.5));

    bg_badge_ = ColorRef::constant(Color(0x3f3f3f, 0.25));
    fg_badge_ = ColorRef::constant(Color(0xffffff, 0.5));

    bg_label_ = ColorRef::constant(Color(0x3f3f3f, 0.5));
    fg_label_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_label_selected_ = ColorRef::constant(Color(0xffffff, 0.75));
    fg_label_selected_ = ColorRef::constant(Color(0x3f3f3f, 0.75));

    bg_group_ = ColorRef::constant(Color(0x1f1f1f, 0.0));
    fg_group_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_item_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    bg_item_playing_ = ColorRef::constant(Color(0x1f1f1f, 0.5));
    bg_item_selected_ = ColorRef::constant(Color(0xffffff, 0.75));

    // generate an x-button texture:
    {
      QImage img = xbuttonImage(128,
                                fg_xbutton_.get(),
                                bg_xbutton_.get().transparent());
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

    // configure common style attributes:
    yae::shared_ptr<GridCellWidth, TDoubleExpr> cell_width = cell_width_expr();
    YAE_ASSERT(cell_width);

    cell_width->path_to_grid_container_ = "scrollview";

    now_playing_.anchors_.top_ = ItemRef::constant(0.0);
    now_playing_.anchors_.left_ = ItemRef::constant(0.0);
    now_playing_.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
    now_playing_.font_ = font_large_;
    now_playing_.font_.setBold(false);
    now_playing_.fontSize_ = ItemRef::reference(font_size_, 0.8);

    eyetv_badge_.font_ = font_large_;
    eyetv_badge_.anchors_.top_ = ItemRef::constant(0.0);
    eyetv_badge_.anchors_.left_ = ItemRef::constant(0.0);
    eyetv_badge_.text_ = TVarRef::constant(TVar(QObject::tr("eyetv")));
    eyetv_badge_.font_.setBold(false);
    eyetv_badge_.fontSize_ = ItemRef::reference(font_size_, 0.8);

    layout_root_.reset(new GroupListLayout());
    layout_group_.reset(new ItemGridLayout());
    layout_item_.reset(new ItemGridCellLayout());
  }

}

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
  // GetFontSize
  //
  struct GetFontSize : public TDoubleExpr
  {
    GetFontSize(const Item & titleHeight, double titleHeightScale,
                const Item & cellHeight, double cellHeightScale):
      titleHeight_(titleHeight),
      cellHeight_(cellHeight),
      titleHeightScale_(titleHeightScale),
      cellHeightScale_(cellHeightScale)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double t = 0.0;
      titleHeight_.get(kPropertyHeight, t);
      t *= titleHeightScale_;

      double c = 0.0;
      cellHeight_.get(kPropertyHeight, c);
      c *= cellHeightScale_;

      result = std::min(t, c);
    }

    const Item & titleHeight_;
    const Item & cellHeight_;

    double titleHeightScale_;
    double cellHeightScale_;
  };

  //----------------------------------------------------------------
  // GridCellWidth
  //
  struct GridCellWidth : public TDoubleExpr
  {
    GridCellWidth(const PlaylistView & playlist,
                  unsigned int maxCells = 5,
                  double minWidth = 160):
      playlist_(playlist),
      maxCells_(maxCells),
      minWidth_(minWidth)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & root = *(playlist_.root());
      Scrollview & sview = root.get<Scrollview>("scrollview");

      double rowWidth = 0.0;
      sview.get(kPropertyWidth, rowWidth);

      unsigned int numCells =
        std::min(maxCells_, calcItemsPerRow(rowWidth, minWidth_));

      result = (numCells < 2) ? rowWidth : (rowWidth / double(numCells));
    }

    const PlaylistView & playlist_;
    unsigned int maxCells_;
    double minWidth_;
  };


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
    font_small_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
#if 0 // (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
                                  QFont::NoSubpixelAntialias |
#endif
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));
    // main font:
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

    // timeline shadow gradient:
    timeline_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *timeline_shadow_;
      gradient[0.000000] = Color(0x000000, 0.004);
      gradient[0.135417] = Color(0x000000, 0.016);
      gradient[0.208333] = Color(0x000000, 0.031);
      gradient[0.260417] = Color(0x000000, 0.047);
      gradient[0.354167] = Color(0x000000, 0.090);
      gradient[0.447917] = Color(0x000000, 0.149);
      gradient[0.500000] = Color(0x000000, 0.192);
      gradient[1.000000] = Color(0x000000, 0.690);
    }

    // color palette:
    bg_ = Color(0x1f1f1f, 0.87);
    fg_ = Color(0xffffff, 1.0);

    border_ = Color(0x7f7f7f, 1.0);
    cursor_ = Color(0xf12b24, 1.0);
    scrollbar_ = Color(0x7f7f7f, 0.5);
    separator_ = scrollbar_;
    underline_ = cursor_;

    bg_xbutton_ = Color(0x000000, 0.0);
    fg_xbutton_ = Color(0xffffff, 0.5);

    bg_focus_ = Color(0x7f7f7f, 0.5);
    fg_focus_ = Color(0xffffff, 1.0);

    bg_edit_selected_ = Color(0xffffff, 1.0);
    fg_edit_selected_ = Color(0x000000, 1.0);

    bg_timecode_ = Color(0x7f7f7f, 0.25);
    fg_timecode_ = Color(0xFFFFFF, 0.5);

    bg_controls_ = bg_timecode_;
    fg_controls_ = fg_timecode_;

    bg_hint_ = Color(0x1f1f1f, 0.0);
    fg_hint_ = Color(0xffffff, 0.5);

    bg_badge_ = Color(0x3f3f3f, 0.25);
    fg_badge_ = Color(0xffffff, 0.5);

    bg_label_ = Color(0x3f3f3f, 0.5);
    fg_label_ = Color(0xffffff, 1.0);

    bg_label_selected_ = Color(0xffffff, 0.75);
    fg_label_selected_ = Color(0x3f3f3f, 0.75);

    bg_group_ = Color(0x1f1f1f, 0.0);
    fg_group_ = Color(0xffffff, 1.0);

    bg_item_ = Color(0x7f7f7f, 0.5);
    bg_item_playing_ = Color(0x1f1f1f, 0.5);
    bg_item_selected_ = Color(0xffffff, 0.75);

    timeline_excluded_ = Color(0xFFFFFF, 0.2);
    timeline_included_ = Color(0xFFFFFF, 0.5);
    timeline_played_ = cursor_;

    // configure common style attributes:
    title_height_.height_ =
      title_height_.addExpr(new CalcTitleHeight(playlist_, 24.0));

    // generate an x-button texture:
    {
      QImage img =
        xbuttonImage(128, fg_xbutton_, bg_xbutton_.transparent());
      xbutton_->setImage(img);
    }

    // generate collapsed group button texture:
    {
      QImage img =
        triangleImage(128, fg_group_, bg_group_.transparent(), 90.0);
      collapsed_->setImage(img);
    }

    // generate expanded group button texture:
    {
      QImage img =
        triangleImage(128, fg_group_, bg_group_.transparent(), 180.0);
      expanded_->setImage(img);
    }

    // generate pause button texture:
    {
      QImage img =
        barsImage(128, fg_controls_, bg_controls_.transparent(), 2, 0.8);
      pause_->setImage(img);
    }

    // generate play button texture:
    {
      QImage img =
        triangleImage(128, fg_controls_, bg_controls_.transparent(), 90.0);
      play_->setImage(img);
    }

    // generate playlist grid on button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_,
                             bg_controls_.transparent(),
                             3, 0.7, 90);
      grid_on_->setImage(img);
    }

    // generate playlist grid off button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_.scale_a(0.5),
                             bg_controls_.transparent(),
                             3, 0.7, 90);
      grid_off_->setImage(img);
    }

    cell_width_.width_ = cell_width_.
      addExpr(new GridCellWidth(playlist_), 1.0, -2.0);

    cell_height_.height_ =
      ItemRef::reference(cell_width_, kPropertyWidth);

    font_size_.height_ = font_size_.
      addExpr(new GetFontSize(title_height_, 0.52,
                              cell_height_, 0.15));

    now_playing_.anchors_.top_ = ItemRef::constant(0.0);
    now_playing_.anchors_.left_ = ItemRef::constant(0.0);
    now_playing_.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
    now_playing_.font_ = font_large_;
    now_playing_.font_.setBold(false);
    now_playing_.fontSize_ = ItemRef::scale(font_size_,
                                            kPropertyHeight,
                                            0.8 * kDpiScale);

    eyetv_badge_.font_ = font_large_;
    eyetv_badge_.anchors_.top_ = ItemRef::constant(0.0);
    eyetv_badge_.anchors_.left_ = ItemRef::constant(0.0);
    eyetv_badge_.text_ = TVarRef::constant(TVar(QObject::tr("eyetv")));
    eyetv_badge_.font_.setBold(false);
    eyetv_badge_.fontSize_ = ItemRef::scale(font_size_,
                                            kPropertyHeight,
                                            0.8 * kDpiScale);

    layout_root_.reset(new GroupListLayout());
    layout_group_.reset(new ItemGridLayout());
    layout_item_.reset(new ItemGridCellLayout());
  }

}

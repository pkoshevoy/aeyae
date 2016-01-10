// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaePlaylistViewStyle.h"
#include "yaeText.h"
#include "yaeTexture.h"


namespace yae
{

  //----------------------------------------------------------------
  // calcItemsPerRow
  //
  unsigned int
  calcItemsPerRow(double rowWidth, double cellWidth)
  {
    double n = std::max(1.0, std::floor(rowWidth / cellWidth));
    return (unsigned int)n;
  }

  //----------------------------------------------------------------
  // xbuttonImage
  //
  QImage
  xbuttonImage(unsigned int w,
               const Color & color,
               const Color & background,
               double thickness)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    int w2 = w / 2;
    double diameter = double(w);
    double center = diameter * 0.5;
    Segment sa(-center, diameter);
    Segment sb(-diameter * thickness * 0.5, diameter * thickness);

    TVec2D origin(0.0, 0.0);
    TVec2D u_axis(0.707106781186548, 0.707106781186548);
    TVec2D v_axis(-0.707106781186548, 0.707106781186548);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);
          double oh = sa.pixelOverlap(pt.x()) * sb.pixelOverlap(pt.y());
          double ov = sb.pixelOverlap(pt.x()) * sa.pixelOverlap(pt.y());
          double innerOverlap = std::max<double>(oh, ov);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }


  //----------------------------------------------------------------
  // PlaylistViewStyle::PlaylistViewStyle
  //
  PlaylistViewStyle::PlaylistViewStyle(const char * id,
                                       PlaylistView & playlist):
    Item(id),
    playlist_(playlist),
    title_height_(Item::addNewHidden<Item>("title_height")),
    xbutton_(Item::addHidden<Texture>(new Texture("xbutton", QImage()))),
    cell_width_(Item::addNewHidden<Item>("cell_width")),
    cell_height_(Item::addNewHidden<Item>("cell_height")),
    font_size_(Item::addNewHidden<Item>("font_size")),
    now_playing_(Item::addNewHidden<Text>("now_playing")),
    eyetv_badge_(Item::addNewHidden<Text>("eyetv_badge"))
  {
    anchors_.top_ = ItemRef::constant(0);
    anchors_.left_ = ItemRef::constant(0);
    width_ = ItemRef::constant(0);
    height_ = ItemRef::constant(0);
  }

  //----------------------------------------------------------------
  // PlaylistViewStyle::color
  //
  const Color &
  PlaylistViewStyle::color(PlaylistViewStyle::ColorId id) const
  {
    switch (id)
    {
      case kBg:
        return bg_;

      case kFg:
        return fg_;

      case kBorder:
        return border_;

      case kCursor:
        return cursor_;

      case kScrollbar:
        return scrollbar_;

      case kSeparator:
        return separator_;

      case kUnderline:
        return underline_;

      case kBgXButton:
        return bg_xbutton_;

      case kFgXButton:
        return fg_xbutton_;

      case kBgFocus:
        return bg_focus_;

      case kFgFocus:
        return fg_focus_;

      case kBgEditSelected:
        return bg_edit_selected_;

      case kFgEditSelected:
        return fg_edit_selected_;

      case kBgTimecode:
        return bg_timecode_;

      case kFgTimecode:
        return fg_timecode_;

      case kBgHint:
        return bg_hint_;

      case kFgHint:
        return fg_hint_;

      case kBgBadge:
        return bg_badge_;

      case kFgBadge:
        return fg_badge_;

      case kBgLabel:
        return bg_label_;

      case kFgLabel:
        return fg_label_;

      case kBgLabelSelected:
        return bg_label_selected_;

      case kFgLabelSelected:
        return fg_label_selected_;

      case kBgGroup:
        return bg_group_;

      case kFgGroup:
        return fg_group_;

      case kBgItem:
        return bg_item_;

      case kBgItemPlaying:
        return bg_item_playing_;

      case kBgItemSelected:
        return bg_item_selected_;

      case kTimelineExcluded:
        return timeline_excluded_;

      case kTimelineIncluded:
        return timeline_included_;

      case kTimelinePlayed:
        return timeline_played_;

      default:
        break;
    }

    YAE_ASSERT(false);
    return bg_;
  }

}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_VIEW_STYLE_H_
#define YAE_PLAYLIST_VIEW_STYLE_H_

// Qt library:
#include <QFont>
#include <QImage>

// local interfaces:
#include "yaeColor.h"
#include "yaePlaylistView.h"


namespace yae
{

  // forward declarations:
  class PlaylistView;
  class Texture;
  class Text;


  //----------------------------------------------------------------
  // calcItemsPerRow
  //
  YAE_API unsigned int
  calcItemsPerRow(double rowWidth, double cellWidth);

  //----------------------------------------------------------------
  // xbuttonImage
  //
  YAE_API QImage
  xbuttonImage(unsigned int w, const Color & color);


  //----------------------------------------------------------------
  // PlaylistViewStyle
  //
  struct YAE_API PlaylistViewStyle : public Item
  {
    PlaylistViewStyle(const char * id, PlaylistView & playlist);

    // the view:
    PlaylistView & playlist_;

    // shared common properties:
    Item & title_height_;
    Texture & xbutton_;
    Item & cell_width_;
    Item & cell_height_;
    Item & font_size_;
    Text & now_playing_;
    Text & eyetv_badge_;

    // font palette:
    QFont font_;
    QFont font_small_;
    QFont font_large_;

    enum ColorId
    {
      kBg,
      kFg,
      kBorder,
      kCursor,
      kScrollbar,
      kSeparator,
      kUnderline,
      kBgFocus,
      kFgFocus,
      kBgEditSelected,
      kFgEditSelected,
      kBgTimecode,
      kFgTimecode,
      kBgHint,
      kFgHint,
      kBgBadge,
      kFgBadge,
      kBgLabel,
      kFgLabel,
      kBgLabelSelected,
      kFgLabelSelected,
      kBgGroup,
      kFgGroup,
      kBgItem,
      kBgItemPlaying,
      kBgItemSelected,
      kTimelineExcluded,
      kTimelineIncluded,
      kTimelinePlayed
    };

    const Color & color(ColorId id) const;

    // color palette:
    Color bg_;
    Color fg_;

    Color border_;
    Color cursor_;
    Color scrollbar_;
    Color separator_;
    Color underline_;

    Color bg_focus_;
    Color fg_focus_;

    Color bg_edit_selected_;
    Color fg_edit_selected_;

    Color bg_timecode_;
    Color fg_timecode_;

    Color bg_hint_;
    Color fg_hint_;

    Color bg_badge_;
    Color fg_badge_;

    Color bg_label_;
    Color fg_label_;

    Color bg_label_selected_;
    Color fg_label_selected_;

    Color bg_group_;
    Color fg_group_;

    Color bg_item_;
    Color bg_item_playing_;
    Color bg_item_selected_;

    Color timeline_excluded_;
    Color timeline_included_;
    Color timeline_played_;

    // gradients:
    TGradientPtr filter_shadow_;
    TGradientPtr timeline_shadow_;

    // layout delegates:
    TPlaylistViewLayoutPtr layout_root_;
    TPlaylistViewLayoutPtr layout_group_;
    TPlaylistViewLayoutPtr layout_item_;
  };


  //----------------------------------------------------------------
  // StyleFilterShadow
  //
  struct StyleFilterShadow : public TGradientExpr
  {
    StyleFilterShadow(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TGradientPtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.filter_shadow_;
    }

    const PlaylistView & playlist_;
  };


  //----------------------------------------------------------------
  // StyleTimelineShadow
  //
  struct StyleTimelineShadow : public TGradientExpr
  {
    StyleTimelineShadow(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TGradientPtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.timeline_shadow_;
    }

    const PlaylistView & playlist_;
  };


  //----------------------------------------------------------------
  // StyleColor
  //
  struct StyleColor : public TColorExpr
  {
    StyleColor(const PlaylistView & playlist,
               PlaylistViewStyle::ColorId id,
               double alphaScale = 1.0,
               double alphaTranslate = 0.0):
      playlist_(playlist),
      id_(id),
      alphaScale_(alphaScale),
      alphaTranslate_(alphaTranslate)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.color(id_).scale_a(alphaScale_, alphaTranslate_);
    }

    const PlaylistView & playlist_;
    PlaylistViewStyle::ColorId id_;
    double alphaScale_;
    double alphaTranslate_;
  };


  //----------------------------------------------------------------
  // StyleTitleHeight
  //
  struct StyleTitleHeight : public TDoubleExpr
  {
    StyleTitleHeight(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      style.title_height_.get(kPropertyHeight, result);
    }

    const PlaylistView & playlist_;
  };

}


#endif // YAE_PLAYLIST_VIEW_STYLE_H_

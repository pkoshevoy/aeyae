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
#include "yaeTexture.h"


namespace yae
{

  // forward declarations:
  class PlaylistView;
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
  xbuttonImage(unsigned int w,
               const Color & color,
               const Color & background = Color(0x000000, 0.0),
               double thickness = 0.2,
               double rotateAngle = 45.0);

  //----------------------------------------------------------------
  // triangleImage
  //
  // create an image of an equilateral triangle inscribed within
  // an invisible circle of diameter w, and rotated about the center
  // of the circle by a given rotation angle (expressed in degrees):
  //
  YAE_API QImage
  triangleImage(unsigned int w,
                const Color & color,
                const Color & background = Color(0x0000000, 0.0),
                double rotateAngle = 0.0);

  //----------------------------------------------------------------
  // barsImage
  //
  YAE_API QImage
  barsImage(unsigned int w,
            const Color & color,
            const Color & background = Color(0x0000000, 0.0),
            unsigned int nbars = 2,
            double thickness = 0.8,
            double rotateAngle = 0.0);

  //----------------------------------------------------------------
  // PlaylistViewStyle
  //
  struct YAE_API PlaylistViewStyle : public ItemViewStyle
  {
    PlaylistViewStyle(const char * id, PlaylistView & playlist);

    virtual void uncache();

    // the view:
    PlaylistView & playlist_;

    // shared common properties:
    ItemRef cell_width_;
    ItemRef cell_height_;
    Text & now_playing_;
    Text & eyetv_badge_;

    // color palette:
    ColorRef bg_xbutton_;
    ColorRef fg_xbutton_;

    ColorRef bg_hint_;
    ColorRef fg_hint_;

    ColorRef bg_badge_;
    ColorRef fg_badge_;

    ColorRef bg_label_;
    ColorRef fg_label_;

    ColorRef bg_label_selected_;
    ColorRef fg_label_selected_;

    ColorRef bg_group_;
    ColorRef fg_group_;

    ColorRef bg_item_;
    ColorRef bg_item_playing_;
    ColorRef bg_item_selected_;

    ColorRef timeline_played_;

    // gradients:
    TGradientPtr filter_shadow_;
    TGradientPtr timeline_shadow_;

    // textures:
    TTexturePtr xbutton_;
    TTexturePtr collapsed_;
    TTexturePtr expanded_;
    TTexturePtr pause_;
    TTexturePtr play_;
    TTexturePtr grid_on_;
    TTexturePtr grid_off_;

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
  // StyleXbuttonTexture
  //
  struct StyleXbuttonTexture : public TTextureExpr
  {
    StyleXbuttonTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.xbutton_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StyleCollapsedTexture
  //
  struct StyleCollapsedTexture : public TTextureExpr
  {
    StyleCollapsedTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.collapsed_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StyleExpandedTexture
  //
  struct StyleExpandedTexture : public TTextureExpr
  {
    StyleExpandedTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.expanded_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StylePauseTexture
  //
  struct StylePauseTexture : public TTextureExpr
  {
    StylePauseTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.pause_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StylePlayTexture
  //
  struct StylePlayTexture : public TTextureExpr
  {
    StylePlayTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.play_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StyleGridOnTexture
  //
  struct StyleGridOnTexture : public TTextureExpr
  {
    StyleGridOnTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.grid_on_;
    }

    const PlaylistView & playlist_;
  };

  //----------------------------------------------------------------
  // StyleGridOffTexture
  //
  struct StyleGridOffTexture : public TTextureExpr
  {
    StyleGridOffTexture(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const PlaylistViewStyle & style = playlist_.playlistViewStyle();
      result = style.grid_off_;
    }

    const PlaylistView & playlist_;
  };

}


#endif // YAE_PLAYLIST_VIEW_STYLE_H_

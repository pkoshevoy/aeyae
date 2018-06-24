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
#include "yaeItemViewStyle.h"
#include "yaePlaylistView.h"
#include "yaeTexture.h"


namespace yae
{

  // forward declarations:
  class PlaylistView;
  class Text;


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

    // gradients:
    TGradientPtr filter_shadow_;

    // textures:
    TTexturePtr xbutton_;
    TTexturePtr collapsed_;
    TTexturePtr expanded_;

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

}


#endif // YAE_PLAYLIST_VIEW_STYLE_H_

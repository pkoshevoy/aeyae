// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_VIEW_H_
#define YAE_PLAYLIST_VIEW_H_

// standard libraries:
#include <map>

// boost includes:
#include <boost/shared_ptr.hpp>

// local interfaces:
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaePlaylistModel.h"
#include "yaePlaylistModelProxy.h"
#include "yaeScrollview.h"


namespace yae
{

  // forward declarations:
  struct PlaylistViewStyle;
  class PlaylistView;
  class Texture;
  class Text;

  //----------------------------------------------------------------
  // TPlaylistModelItem
  //
  typedef ModelItem<PlaylistModelProxy> TPlaylistModelItem;

  //----------------------------------------------------------------
  // TModelInputArea
  //
  typedef ModelInputArea<PlaylistModelProxy> TModelInputArea;

  //----------------------------------------------------------------
  // TClickablePlaylistModelItem
  //
  typedef ClickableModelItem<PlaylistModelProxy> TClickablePlaylistModelItem;

  //----------------------------------------------------------------
  // ILayoutDelegate
  //
  template <typename TView, typename Model>
  struct YAE_API ILayoutDelegate
  {
    virtual ~ILayoutDelegate() {}

    virtual void layout(Item & item,
                        TView & view,
                        Model & model,
                        const QModelIndex & itemIndex,
                        const PlaylistViewStyle & style) = 0;
  };

  //----------------------------------------------------------------
  // TPlaylistViewLayout
  //
  typedef ILayoutDelegate<PlaylistView, PlaylistModelProxy> TPlaylistViewLayout;

  //----------------------------------------------------------------
  // TPlaylistViewLayoutPtr
  //
  typedef boost::shared_ptr<TPlaylistViewLayout> TPlaylistViewLayoutPtr;

  //----------------------------------------------------------------
  // GroupListLayout
  //
  struct GroupListLayout : public TPlaylistViewLayout
  {
    // virtual:
    void layout(Item & groups,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & rootIndex,
                const PlaylistViewStyle & style);
  };

  //----------------------------------------------------------------
  // ItemGridLayout
  //
  struct ItemGridLayout : public TPlaylistViewLayout
  {
    // virtual:
    void layout(Item & group,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & groupIndex,
                const PlaylistViewStyle & style);
  };

  //----------------------------------------------------------------
  // ItemGridCellLayout
  //
  struct ItemGridCellLayout : public TPlaylistViewLayout
  {
    // virtual:
    void layout(Item & cell,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & index,
                const PlaylistViewStyle & style);
  };

  //----------------------------------------------------------------
  // ItemListLayout
  //
  struct ItemListLayout : public TPlaylistViewLayout
  {
    // virtual:
    void layout(Item & group,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & groupIndex,
                const PlaylistViewStyle & style);
  };

  //----------------------------------------------------------------
  // ItemListRowLayout
  //
  struct ItemListRowLayout : public TPlaylistViewLayout
  {
    void layout(Item & cell,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & index,
                const PlaylistViewStyle & style);
  };


  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public ItemView
  {
    Q_OBJECT;

  public:
    PlaylistView();

    // data source:
    void setModel(PlaylistModelProxy * model);

    inline PlaylistModelProxy * model() const
    { return model_; }

    //----------------------------------------------------------------
    // StyleId
    //
    // currently only two styles are implemented:
    //
    enum StyleId
    {
      kGridView,
      kListView
    };

    void setStyleId(StyleId styleId);

    // current style:
    PlaylistViewStyle & playlistViewStyle() const;

    // restyle the view according to current style:
    void restyle();

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool resizeTo(const Canvas * canvas);

  public slots:
    // adjust scrollview position to ensure a given item is visible:
    void ensureVisible(const QModelIndex & itemIndex);

    void dataChanged(const QModelIndex & topLeft,
                     const QModelIndex & bottomRight);

    void layoutAboutToBeChanged();
    void layoutChanged();

    void modelAboutToBeReset();
    void modelReset();

    void rowsAboutToBeInserted(const QModelIndex & parent, int start, int end);
    void rowsInserted(const QModelIndex & parent, int start, int end);

    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
    void rowsRemoved(const QModelIndex & parent, int start, int end);

  protected:
    PlaylistModelProxy * model_;
    std::string style_;
  };


  //----------------------------------------------------------------
  // ContrastColor
  //
  struct ContrastColor : public TColorExpr
  {
    ContrastColor(const Item & item, Property prop, double scaleAlpha = 0.0):
      scaleAlpha_(scaleAlpha),
      item_(item),
      prop_(prop)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      Color c0;
      item_.get(prop_, c0);
      result = c0.bw_contrast();
      double a = scaleAlpha_ * double(result.a());
      result.set_a((unsigned char)(std::min(255.0, std::max(0.0, a))));
    }

    double scaleAlpha_;
    const Item & item_;
    Property prop_;
  };

  //----------------------------------------------------------------
  // PremultipliedTransparent
  //
  struct PremultipliedTransparent : public TColorExpr
  {
    PremultipliedTransparent(const Item & item, Property prop):
      item_(item),
      prop_(prop)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      Color c0;
      item_.get(prop_, c0);
      result = c0.premultiplied_transparent();
    }

    const Item & item_;
    Property prop_;
  };

  //----------------------------------------------------------------
  // GetScrollviewWidth
  //
  struct GetScrollviewWidth : public TDoubleExpr
  {
    GetScrollviewWidth(const PlaylistView & playlist):
      playlist_(playlist)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & root = *(playlist_.root());
      Scrollview & sview = root.get<Scrollview>("scrollview");
      sview.get(kPropertyWidth, result);
    }

    const PlaylistView & playlist_;
  };

}


#endif // YAE_PLAYLIST_VIEW_H_

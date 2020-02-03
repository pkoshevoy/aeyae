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

// aeyae:
#include "yae/api/yae_shared_ptr.h"

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
  class MainWindow;
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
  struct YAEUI_API ILayoutDelegate
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
  typedef yae::shared_ptr<TPlaylistViewLayout> TPlaylistViewLayoutPtr;

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
  class YAEUI_API PlaylistView : public ItemView
  {
    Q_OBJECT;

  public:
    PlaylistView();

    // need to reference main window for current playback status:
    void setup(MainWindow * mainWindow);

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
    ItemViewStyle * style() const;

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool resizeTo(const Canvas * canvas);

  public slots:
    // adjust scrollview position to ensure a given item is visible:
    void ensureVisible(const QModelIndex & itemIndex);

    // shortcut:
    void ensureCurrentItemIsVisible();

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
    MainWindow * mainWindow_;
    PlaylistModelProxy * model_;
    std::string style_;
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

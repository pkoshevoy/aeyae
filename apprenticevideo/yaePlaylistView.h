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


namespace yae
{

  //----------------------------------------------------------------
  // TPlaylistModelItem
  //
  typedef ModelItem<PlaylistModelProxy> TPlaylistModelItem;


  //----------------------------------------------------------------
  // TClickablePlaylistModelItem
  //
  typedef ClickableItem<PlaylistModelProxy> TClickablePlaylistModelItem;


  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct CalcTitleHeight : public TDoubleExpr
  {
    CalcTitleHeight(const Item & titleContainer, double minHeight);

    // virtual:
    void evaluate(double & result) const;

    const Item & titleContainer_;
    double minHeight_;
  };

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
                        const QModelIndex & itemIndex) = 0;
  };

  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public ItemView
  {
    Q_OBJECT;

  public:
    typedef ILayoutDelegate<PlaylistView, PlaylistModelProxy> TLayoutDelegate;
    typedef boost::shared_ptr<TLayoutDelegate> TLayoutPtr;
    typedef PlaylistModel::LayoutHint TLayoutHint;
    typedef std::map<TLayoutHint, TLayoutPtr> TLayoutDelegates;

    PlaylistView();

    // data source:
    void setModel(PlaylistModelProxy * model);

    inline PlaylistModelProxy * model() const
    { return model_; }

    // accessors:
    inline const TLayoutDelegates & layouts() const
    { return layoutDelegates_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void resizeTo(const Canvas * canvas);

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
    TLayoutDelegates layoutDelegates_;
  };

}


#endif // YAE_PLAYLIST_VIEW_H_

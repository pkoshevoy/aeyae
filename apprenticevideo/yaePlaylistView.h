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

// Qt library:
#include <QFont>

// local interfaces:
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaePlaylistModel.h"
#include "yaePlaylistModelProxy.h"


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
  typedef ClickableItem<PlaylistModelProxy> TClickablePlaylistModelItem;


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

    // color palette:
    Color bg_;
    Color fg_;

    Color cursor_;
    Color separator_;

    Color bg_focus_;
    Color fg_focus_;

    Color bg_edit_selected_;
    Color fg_edit_selected_;

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

    // gradients:
    std::map<double, Color> filter_shadow_;
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
                        const QModelIndex & itemIndex,
                        const PlaylistViewStyle & style) = 0;
  };


  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public ItemView
  {
    Q_OBJECT;

  public:
    typedef PlaylistModel::LayoutHint TLayoutHint;
    typedef ILayoutDelegate<PlaylistView, PlaylistModelProxy> TLayoutDelegate;
    typedef boost::shared_ptr<TLayoutDelegate> TLayoutPtr;
    typedef std::map<TLayoutHint, TLayoutPtr> TLayoutDelegates;

    PlaylistView();

    // data source:
    void setModel(PlaylistModelProxy * model);

    inline PlaylistModelProxy * model() const
    { return model_; }

    // accessors:
    inline const TLayoutDelegates & layouts() const
    { return layoutDelegates_; }

    inline const PlaylistViewStyle & playlistViewStyle() const
    { return root_->get<PlaylistViewStyle>(styleId_.c_str()); }

    // virtual:
    void paint(Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

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
    std::string styleId_;
  };

}


#endif // YAE_PLAYLIST_VIEW_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:37:40 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_WIDGET_H_
#define YAE_PLAYLIST_WIDGET_H_

// std includes:
#include <vector>

// Qt includes:
#include <QAbstractScrollArea>
#include <QEvent>
#include <QRect>
#include <QScrollBar>
#include <QRubberBand>

// yae includes:
#include <yaeAPI.h>
#include <yaeTree.h>


namespace yae
{

  //----------------------------------------------------------------
  // TPlaylistTree
  // 
  typedef Tree<QString, QString> TPlaylistTree;

  //----------------------------------------------------------------
  // PlaylistItem
  // 
  struct PlaylistItem
  {
    PlaylistItem();
    
    // playlist item key within the fringe group it belongs to:
    QString key_;
    
    // absolute path to the playlist item:
    QString path_;

    // human friendly text describing this playlist item:
    QString name_;
    
    // file extension:
    QString ext_;
    
    // geometry bounding box, used for drawing the playlist item:
    QRect bbox_;
    
    // a flag indicating whether this item is currently selected:
    bool selected_;
    
    // a flag indicating whether this item is excluded from the list:
    bool excluded_;
  };
  
  //----------------------------------------------------------------
  // PlaylistGroup
  // 
  struct PlaylistGroup
  {
    PlaylistGroup();
    
    // complete key path to the fringe group that corresponds to this
    // playlist group:
    std::list<QString> keyPath_;
    
    // human friendly text describing this playlist item group:
    QString name_;
    
    // playlist items belonging to this group:
    std::vector<PlaylistItem> items_;
    
    // bounding box of the group header:
    QRect bbox_;
    
    // bounding box of the group items:
    QRect bboxItems_;
    
    // number of items stored in other playlist groups preceding this group:
    std::size_t offset_;
    
    // a flag indicating whether this group is collapsed for brevity:
    bool collapsed_;
    
    // a flag indicating whether this group is collapsed for brevity:
    bool excluded_;
  };
  
  //----------------------------------------------------------------
  // PlaylistWidget
  // 
  class PlaylistWidget : public QAbstractScrollArea
  {
    Q_OBJECT;

  public:
    PlaylistWidget(QWidget * parent = NULL, Qt::WindowFlags f = 0);
    
    // virtual:
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    
    // use this to add items to the playlist:
    void add(const std::list<QString> & playlist);
    
    // return index of the current item:
    std::size_t currentItem() const;
    
    // return number of items in the playlist:
    std::size_t countItems() const;
    
    // this is used to check whether previous/next navigation is possible:
    std::size_t countItemsAhead() const;
    std::size_t countItemsBehind() const;
    
    // lookup a playlist item by index:
    PlaylistGroup * lookupGroup(std::size_t index);
    PlaylistItem * lookup(std::size_t index, PlaylistGroup ** group = NULL);
    
    enum TDirection {
      kBehind = 0,
      kAhead = 1
    };
    
    // lookup non-excluded group closest (in a given direction)
    // to the specified item index:
    PlaylistGroup * closestGroup(std::size_t itemIndex,
                                 TDirection where = kAhead);
    
    // lookup non-excluded item closest (in a given direction)
    // to the specified item index:
    std::size_t closestItem(std::size_t itemIndex,
                            TDirection where = kAhead,
                            PlaylistGroup ** group = NULL);

  public slots:
    // item filter:
    void filterChanged(const QString & filter);
    
    // playlist navigation controls:
    void setCurrentItem(std::size_t index, bool force = false);
    
    // selection set management:
    void selectAll();
    void selectGroup(PlaylistGroup * group);
    void selectItem(std::size_t indexSel, bool exclusive = true);
    void removeSelected();
    
  signals:
    // this signal may be emitted if the user activates an item,
    // or otherwise changes the playlist to invalidate the
    // existing current item:
    void currentItemChanged(std::size_t index);
    
  protected:
    // virtual:
    void paintEvent(QPaintEvent * e);
    void mousePressEvent(QMouseEvent * e);
    void mouseReleaseEvent(QMouseEvent * e);
    void mouseMoveEvent(QMouseEvent * e);
    void mouseDoubleClickEvent(QMouseEvent * e);
    void wheelEvent(QWheelEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void resizeEvent(QResizeEvent * e);
    
    // helper: return viewport position relative to the playlist widget:
    inline QPoint getViewOffset() const
    {
      return QPoint(horizontalScrollBar()->value(),
                    verticalScrollBar()->value());
    }
    
    // helpers:
    void updateGeometries();
    void updateScrollBars();
    void draw(QPainter & painter, const QRect & region);

    void updateSelection(const QPoint & mousePos,
                         bool toggleSelection = false,
                         bool scrollToItem = false,
                         bool allowGroupSelection = true);
    
    void selectItems(const QRect & bboxSel,
                     bool toggleSelection);
    
    void scrollTo(const PlaylistGroup * group,
                  const PlaylistItem * item);
    
    void scrollTo(std::size_t index,
                  PlaylistItem ** item = NULL);
    
    std::size_t lookupGroupIndex(const QPoint & pt, bool findClosest = true);
    PlaylistGroup * lookupGroup(const QPoint & pt, bool findClosest = true);
    
    std::size_t lookupItemIndex(PlaylistGroup * group, const QPoint & pt);
    PlaylistItem * lookup(PlaylistGroup * group, const QPoint & pt);
    PlaylistItem * lookup(const QPoint & pt, PlaylistGroup ** group = NULL);
    
    enum TMouseState {
      kNotReady = 0,
      kUpdateSelection = 1,
      kToggleCollapsedGroup = 2
    };
    
    // a helper used to distinguish between various mouse actions:
    TMouseState mouseState_;
    
    // selection rubber-band widget:
    QRubberBand rubberBand_;
    
    // selection anchor:
    QPoint anchor_;
    
    // a playlist tree:
    TPlaylistTree tree_;
    
    // a list of playlist item groups, derived from playlist tree fringes:
    std::vector<PlaylistGroup> groups_;
    
    // total number of items:
    std::size_t numItems_;
    
    // number of non-excluded items:
    std::size_t numShown_;
    
    // number of non-excluded item groups:
    std::size_t numShownGroups_;
    
    // current item index:
    std::size_t current_;
    
    // highlighted item index:
    std::size_t highlighted_;
  };
  
}


#endif // YAE_PLAYLIST_WIDGET_H_

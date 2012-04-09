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
  };
  
  //----------------------------------------------------------------
  // PlaylistGroup
  // 
  struct PlaylistGroup
  {
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
  };

  //----------------------------------------------------------------
  // PlaylistWidget
  // 
  class PlaylistWidget : public QAbstractScrollArea
  {
    Q_OBJECT;

  public:
    PlaylistWidget(QWidget * parent = NULL, Qt::WindowFlags f = 0);
    
    void setPlaylist(const std::list<QString> & playlist);
    
    // accessor to the current playlist group:
    const PlaylistGroup * currentGroup() const;
    
    // playlist navigation controls:
    void skipToNext();
    void backToPrev();
    
  protected:
    // virtual:
    bool event(QEvent * e);
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
    void updateSelection(const QPoint & mousePos, bool scrollToItem = false);
    void selectItems(const QRect & bboxSel);
    void scrollTo(const PlaylistGroup * group,
                  const PlaylistItem * item);
    PlaylistGroup * lookupGroup(const QPoint & pt);
    PlaylistItem * lookup(PlaylistGroup * group, const QPoint & pt);
    PlaylistItem * lookup(const QPoint & pt);
    
    // selection rubber-band widget:
    QRubberBand rubberBand_;
    
    // selection anchor:
    QPoint anchor_;
    
    // a playlist tree:
    TPlaylistTree tree_;
    
    // a list of playlist item groups, derived from playlist tree fringes:
    std::vector<PlaylistGroup> groups_;
    
    // active item index:
    std::size_t playing_;
    std::size_t current_;
  };
  
}


#endif // YAE_PLAYLIST_WIDGET_H_

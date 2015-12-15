// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_VIEW_H_
#define YAE_PLAYLIST_VIEW_H_

// standard libraries:
#include <cmath>
#include <list>
#include <map>
#include <vector>

// boost includes:
#include <boost/shared_ptr.hpp>

// Qt interfaces:
#include <QFont>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QObject>
#include <QPersistentModelIndex>
#include <QString>
#include <QTimer>
#include <QVariant>

// local interfaces:
#include "yaeBBox.h"
#include "yaeCanvas.h"
#include "yaeColor.h"
#include "yaeExpression.h"
#include "yaeFlickableArea.h"
#include "yaeGradient.h"
#include "yaeImage.h"
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeItemRef.h"
#include "yaePlaylistModelProxy.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeScrollview.h"
#include "yaeSegment.h"
#include "yaeText.h"
#include "yaeTexture.h"
#include "yaeTexturedRect.h"
#include "yaeThumbnailProvider.h"
#include "yaeTransform.h"
#include "yaeTriangle.h"
#include "yaeVec.h"


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
  class YAE_API PlaylistView : public QObject,
                               public Canvas::ILayer
  {
    Q_OBJECT;

  public:
    typedef ILayoutDelegate<PlaylistView, PlaylistModelProxy> TLayoutDelegate;
    typedef boost::shared_ptr<TLayoutDelegate> TLayoutPtr;
    typedef PlaylistModel::LayoutHint TLayoutHint;
    typedef std::map<TLayoutHint, TLayoutPtr> TLayoutDelegates;

    typedef boost::shared_ptr<ThumbnailProvider> TImageProviderPtr;
    typedef std::map<QString, TImageProviderPtr> TImageProviders;

    //----------------------------------------------------------------
    // RequestRepaintEvent
    //
    enum { kRequestRepaintEvent };
    typedef BufferedEvent<kRequestRepaintEvent> RequestRepaintEvent;

    PlaylistView();

    // virtual:
    bool event(QEvent * event);

    // virtual:
    void requestRepaint();

    // virtual:
    void resizeTo(const Canvas * canvas);

    // virtual:
    void paint(Canvas * canvas);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);

    // helpers:
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);
    bool processWheelEvent(Canvas * canvas, QWheelEvent * event);

    // data source:
    void setModel(PlaylistModelProxy * model);

    void addImageProvider(const QString & providerId,
                          const TImageProviderPtr & p);

    // accessors:
    inline const TImageProviders & imageProviders() const
    { return imageProviders_; }

    inline const TLayoutDelegates & layouts() const
    { return layoutDelegates_; }

    inline const ItemPtr & root() const
    { return root_; }

  public slots:

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
    RequestRepaintEvent::TPayload requestRepaintEvent_;
    PlaylistModelProxy * model_;
    TLayoutDelegates layoutDelegates_;
    TImageProviders imageProviders_;
    ItemPtr root_;
    double w_;
    double h_;

    // input handlers corresponding to the point where a mouse
    // button press occurred, will be cleared if layout changes
    // or mouse button release occurs:
    std::list<InputHandler> inputHandlers_;

    // mouse event handling house keeping helpers:
    InputHandler * pressed_;
    InputHandler * dragged_;
    TVec2D startPt_;
  };

}


#endif // YAE_PLAYLIST_VIEW_H_

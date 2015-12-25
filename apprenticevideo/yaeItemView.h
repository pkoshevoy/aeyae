// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 22:29:25 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_VIEW_H_
#define YAE_ITEM_VIEW_H_

// standard libraries:
#include <list>

// Qt interfaces:
#include <QMouseEvent>
#include <QObject>
#include <QString>

// local interfaces:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeThumbnailProvider.h"
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // ItemView
  //
  class YAE_API ItemView : public QObject,
                           public Canvas::ILayer
  {
    Q_OBJECT;

  public:
    //----------------------------------------------------------------
    // RequestRepaintEvent
    //
    enum { kRequestRepaintEvent };
    typedef BufferedEvent<kRequestRepaintEvent> RequestRepaintEvent;

    ItemView(const char * name);

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

    void addImageProvider(const QString & providerId,
                          const TImageProviderPtr & p);

    // accessors:
    inline const TImageProviders & imageProviders() const
    { return imageProviders_; }

    inline const ItemPtr & root() const
    { return root_; }

    // virtual:
    TImageProviderPtr
    getImageProvider(const QString & imageUrl, QString & imageId) const
    { return lookupImageProvider(imageProviders(), imageUrl, imageId); }

    // override this to receive mouse movement notification
    // regardless whether any mouse buttons are pressed:
    virtual bool processMouseTracking(const TVec2D & mousePt)
    { (void)mousePt; return false; }

    // accessor to last-known mouse position:
    inline const TVec2D & mousePt() const
    { return mousePt_; }

  protected:
    RequestRepaintEvent::TPayload requestRepaintEvent_;
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
    TVec2D mousePt_;
  };

}


#endif // YAE_ITEM_VIEW_H_

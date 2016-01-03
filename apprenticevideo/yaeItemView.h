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
    void setEnabled(bool enable);

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
    virtual bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    virtual bool processMouseEvent(Canvas * canvas, QMouseEvent * event);
    virtual bool processWheelEvent(Canvas * canvas, QWheelEvent * event);

    void addImageProvider(const QString & providerId,
                          const TImageProviderPtr & p);

    // accessors:
    inline const TImageProviders & imageProviders() const
    { return imageProviders_; }

    inline const ItemPtr & root() const
    { return root_; }

    inline double width() const
    { return w_; }

    inline double height() const
    { return h_; }

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

  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct CalcTitleHeight : public TDoubleExpr
  {
    CalcTitleHeight(const ItemView & itemView, double minHeight):
      itemView_(itemView),
      minHeight_(minHeight)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = itemView_.width();
      result = std::max<double>(minHeight_, 24.0 * w / 800.0);
    }

    const ItemView & itemView_;
    double minHeight_;
  };

  //----------------------------------------------------------------
  // OddRoundUp
  //
  struct OddRoundUp : public TDoubleExpr
  {
    OddRoundUp(const Item & item,
               Property property,
               double scale = 1.0,
               double translate = 0.0):
      item_(item),
      property_(property),
      scale_(scale),
      translate_(translate)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double v = 0.0;
      item_.get(property_, v);
      v *= scale_;
      v += translate_;

      int i = 1 | int(ceil(v));
      result = double(i);
    }

    const Item & item_;
    Property property_;
    double scale_;
    double translate_;
  };

  //----------------------------------------------------------------
  // Repaint
  //
  struct Repaint : public Item::Observer
  {
    Repaint(ItemView & timeline):
      timeline_(timeline)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      (void) item;
      (void) e;

      timeline_.requestRepaint();
    }

    ItemView & timeline_;
  };

  //----------------------------------------------------------------
  // Uncache
  //
  struct Uncache : public Item::Observer
  {
    Uncache(Item & item):
      item_(item)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      item_.uncache();
    }

    Item & item_;
  };

}


#endif // YAE_ITEM_VIEW_H_

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
#include <map>
#include <set>

// boost libraries:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// Qt interfaces:
#include <QMouseEvent>
#include <QObject>
#include <QString>
#include <QTimer>

// yae includes:
#include "yae/utils/yae_benchmark.h"

// local interfaces:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeThumbnailProvider.h"
#include "yaeVec.h"


namespace yae
{

  // helper: convert from device independent "pixels" to device pixels:
  template <typename TEvent>
  TVec2D
  device_pixel_pos(Canvas * canvas, const TEvent * e)
  {
    QPoint pos = e->pos();
    double devicePixelRatio = canvas->devicePixelRatio();
    double x = devicePixelRatio * pos.x();
    double y = devicePixelRatio * pos.y();
    return TVec2D(x, y);
  }

  //----------------------------------------------------------------
  // PostponeEvent
  //
  class PostponeEvent : public QObject
  {
    Q_OBJECT

  public:
    PostponeEvent();
    virtual ~PostponeEvent();

    void postpone(int msec, QObject * target, QEvent * event);

  protected slots:
    void onTimeout();

  protected:
    struct TPrivate;
    TPrivate * private_;

  private:
    PostponeEvent(const PostponeEvent &);
    PostponeEvent & operator = (const PostponeEvent &);
  };

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
    struct RequestRepaintEvent : public BufferedEvent
    {
      RequestRepaintEvent(TPayload & payload):
        BufferedEvent(payload)
      {
        YAE_LIFETIME_START(lifetime, "02 -- RequestRepaintEvent");
      }

      YAE_LIFETIME(lifetime);
    };

    ItemView(const char * name);

    // virtual:
    void setEnabled(bool enable);

    // virtual:
    bool event(QEvent * event);

    // virtual:
    void requestRepaint();

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    void paint(Canvas * canvas);

    //----------------------------------------------------------------
    // IAnimator
    //
    typedef Canvas::ILayer::IAnimator IAnimator;

    //----------------------------------------------------------------
    // TAnimatorPtr
    //
    typedef boost::shared_ptr<Canvas::ILayer::IAnimator> TAnimatorPtr;

    // virtual:
    void addAnimator(const boost::shared_ptr<IAnimator> & a);

    // virtual:
    void delAnimator(const boost::shared_ptr<IAnimator> & a);

    // helper: uncache a given item at the next repaint
    //
    // NOTE: avoid uncaching if possible due to
    //       relatively expensive runtime cost O(N),
    //       where N is the number of items below root:
    void requestUncache(Item * root = NULL);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);

    // helpers:
    virtual bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    virtual bool processMouseEvent(Canvas * canvas, QMouseEvent * event);
    virtual bool processWheelEvent(Canvas * canvas, QWheelEvent * event);

    // override this to receive mouse movement notification
    // regardless whether any mouse buttons are pressed:
    virtual bool processMouseTracking(const TVec2D & mousePt);

    // accessor to last-known mouse position:
    inline const TVec2D & mousePt() const
    { return mousePt_; }

    // virtual:
    TImageProviderPtr
    getImageProvider(const QString & imageUrl, QString & imageId) const
    { return lookupImageProvider(imageProviders(), imageUrl, imageId); }

    void addImageProvider(const QString & providerId,
                          const TImageProviderPtr & p);

    // accessors:
    inline const TImageProviders & imageProviders() const
    { return imageProviders_; }

    inline const ItemPtr & root() const
    { return root_; }

    inline double devicePixelRatio() const
    { return devicePixelRatio_; }

    inline double width() const
    { return w_; }

    inline double height() const
    { return h_; }

  public slots:
    void repaint();
    void animate();

  protected:
    std::map<Item *, boost::weak_ptr<Item> > uncache_;
    RequestRepaintEvent::TPayload requestRepaintEvent_;

    TImageProviders imageProviders_;
    ItemPtr root_;
    double devicePixelRatio_;
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

    // on-demand repaint timer:
    QTimer repaintTimer_;

    // ~60 fps animation timer, runs as long as the animators set is not empty:
    QTimer animateTimer_;

    // a set of animators, referenced by the animation timer:
    std::set<boost::weak_ptr<IAnimator> > animators_;
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
      double s = itemView_.devicePixelRatio();
      result = std::max<double>(minHeight_ * s, 24.0 * w / 800.0);
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
    Repaint(ItemView & itemView, bool requestUncache = false):
      itemView_(itemView),
      requestUncache_(requestUncache)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      (void) item;
      (void) e;

      if (requestUncache_)
      {
        itemView_.requestUncache();
      }

      itemView_.requestRepaint();
    }

    ItemView & itemView_;
    bool requestUncache_;
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

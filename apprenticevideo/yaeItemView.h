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

// Qt interfaces:
#include <QFont>
#include <QMouseEvent>
#include <QObject>
#include <QString>
#include <QTimer>

// yae includes:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_utils.h"

// local interfaces:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeImageProvider.h"
#include "yaeVec.h"


namespace yae
{

  // forward declarations:
  struct ItemViewStyle;


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
  class YAEUI_API ItemView : public QObject,
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

    void setRoot(const ItemPtr & root);

    virtual const ItemViewStyle * style() const
    { return NULL; }

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
    typedef yae::shared_ptr<Canvas::ILayer::IAnimator> TAnimatorPtr;

    // virtual:
    void addAnimator(const yae::shared_ptr<IAnimator> & a);

    // virtual:
    void delAnimator(const yae::shared_ptr<IAnimator> & a);

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

    // accessor to the list of items under last-known mouse position:
    inline const std::list<VisibleItem> & mouseOverItems() const
    { return mouseOverItems_; }

    // helper for checking mouseOverItems:
    bool isMouseOverItem(const Item & item) const;

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

  public:
    // a mechanism to query and change external properties
    // without tight (static) coupling to those properties:
    ContextQuery<bool> query_fullscreen_;
    ContextCallback toggle_fullscreen_;

  protected:
    std::map<Item *, yae::weak_ptr<Item> > uncache_;
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

    // a list of items under the mouse,
    // will be cleared if layout changes:
    std::list<VisibleItem> mouseOverItems_;
    std::map<const Item *, TVec2D> itemsUnderMouse_;

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
    std::set<yae::weak_ptr<IAnimator> > animators_;
  };


  //----------------------------------------------------------------
  // ViewDpi
  //
  struct YAEUI_API ViewDpi : TDoubleExpr
  {
    ViewDpi(const ItemView & view):
      view_(view),
      dpi_(0.0)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = view_.delegate()->logical_dpi_y();

      if (result != dpi_ && dpi_ > 0.0)
      {
        // force all geometry to be recalculated:
        view_.root()->uncache();
      }

      // cache the result:
      dpi_ = result;
    }

    const ItemView & view_;
    mutable double dpi_;
  };


  //----------------------------------------------------------------
  // ViewDevicePixelRatio
  //
  struct YAEUI_API ViewDevicePixelRatio : TDoubleExpr
  {
    ViewDevicePixelRatio(const ItemView & view):
      view_(view),
      scale_(1.0)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = view_.delegate()->device_pixel_ratio();

      if (result != scale_ && scale_ > 0.0)
      {
        // force all geometry to be recalculated:
        view_.root()->uncache();
      }

      // cache the result:
      scale_ = result;
    }

    const ItemView & view_;
    mutable double scale_;
  };


  //----------------------------------------------------------------
  // get_row_height
  //
  YAEUI_API double
  get_row_height(const ItemView & view);


  //----------------------------------------------------------------
  // calc_items_per_row
  //
  YAEUI_API unsigned int
  calc_items_per_row(double row_width, double cell_width);


  //----------------------------------------------------------------
  // GetRowHeight
  //
  struct YAEUI_API GetRowHeight : TDoubleExpr
  {
    GetRowHeight(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = get_row_height(view_);
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct YAEUI_API CalcTitleHeight : public TDoubleExpr
  {
    CalcTitleHeight(const ItemView & itemView, double minHeight):
      itemView_(itemView),
      minHeight_(minHeight)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = itemView_.width();
      double s = itemView_.delegate()->logical_dpi_y() / 96.0;
      s = std::max(s, itemView_.devicePixelRatio());
      result = std::max<double>(minHeight_ * s, 24.0 * w / 800.0);
    }

    const ItemView & itemView_;
    double minHeight_;
  };

  //----------------------------------------------------------------
  // GetFontSize
  //
  struct YAEUI_API GetFontSize : public TDoubleExpr
  {
    GetFontSize(const ItemRef & titleHeight, double titleHeightScale,
                const ItemRef & cellHeight, double cellHeightScale):
      titleHeight_(titleHeight),
      cellHeight_(cellHeight),
      titleHeightScale_(titleHeightScale),
      cellHeightScale_(cellHeightScale)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double t = titleHeight_.get();
      t *= titleHeightScale_;

      double c = cellHeight_.get();
      c *= cellHeightScale_;

      result = std::min(t, c);
    }

    const ItemRef & titleHeight_;
    const ItemRef & cellHeight_;

    double titleHeightScale_;
    double cellHeightScale_;
  };

  //----------------------------------------------------------------
  // GridCellWidth
  //
  struct YAEUI_API GridCellWidth : public TDoubleExpr
  {
    GridCellWidth(const ItemView & view,
                  const char * path_to_grid_container = "/",
                  unsigned int max_cells = 5,
                  double min_width = 160):
      view_(view),
      path_to_grid_container_(path_to_grid_container),
      max_cells_(max_cells),
      min_width_(min_width)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & container = view_.root()->
        find<Item>(path_to_grid_container_.c_str());

      double row_width = container.width();

      unsigned int num_cells =
        std::min(max_cells_, calc_items_per_row(row_width, min_width_));

      result = (num_cells < 2) ? row_width : (row_width / double(num_cells));
    }

    const ItemView & view_;
    std::string path_to_grid_container_;
    unsigned int max_cells_;
    double min_width_;
  };

  //----------------------------------------------------------------
  // OddRoundUp
  //
  struct YAEUI_API OddRoundUp : public TDoubleExpr
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

      int i = 1 | int(ceil(v));
      result = double(i) + translate_;
    }

    const Item & item_;
    Property property_;
    double scale_;
    double translate_;
  };

  //----------------------------------------------------------------
  // Repaint
  //
  struct YAEUI_API Repaint : public Item::Observer
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
  struct YAEUI_API Uncache : public Item::Observer
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


  //----------------------------------------------------------------
  // IsFullscreen
  //
  struct YAEUI_API IsFullscreen : public TBoolExpr
  {
    IsFullscreen(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      view_.query_fullscreen_(result);
    }

    const ItemView & view_;
  };

}


#endif // YAE_ITEM_VIEW_H_

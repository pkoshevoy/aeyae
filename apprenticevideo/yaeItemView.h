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
#include <QFont>
#include <QMouseEvent>
#include <QObject>
#include <QString>
#include <QTimer>

// yae includes:
#include "yae/utils/yae_benchmark.h"

// local interfaces:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeImageProvider.h"
#include "yaeVec.h"


namespace yae
{

  // forward declarations:
  class ItemViewStyle;


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
  // ItemViewStyle
  //
  struct YAE_API ItemViewStyle : public Item
  {
    ItemViewStyle(const char * id, const ItemView & view);

    virtual void uncache();
    /*
    // for user-defined item attributes:
    template <typename TData>
    inline DataRef<TData>
    setStyleAttr(const char * key, Expression<TData> * e)
    {
      attr_[std::string(key)] = TPropertiesBasePtr(e);
      return DataRef<TData>::expression(*e);
    }

    template <typename TData>
    inline DataRef<TData>
    setStyleAttr(const char * key, const TData & value)
    {
      Expression<TData> * e = new ConstExpression<TData>(value);
      return this->setStyleAttr(key, e);
    }

    template <typename TData>
    inline bool
    getStyleAttr(const std::string & key, TData & value) const
    {
      std::map<std::string, TPropertiesBasePtr>::const_iterator
        found = attr_.find(key);

      if (found == attr_.end())
      {
        return false;
      }

      const IPropertiesBase * base = found->second.get();
      const IProperties<TData> * styleAttr =
        dynamic_cast<const IProperties<TData> *>(base);

      if (!styleAttr)
      {
        YAE_ASSERT(false);
        return false;
      }

      styleAttr->get(kPropertyExpression, value);
      return true;
    }

    template <typename TData>
    TData
    styleAttr(const char * attr, const TData & defaultValue = TData())
    {
      TData value = defaultValue;
      getStyleAttr(std::string(attr), value);
      return value;
    }

    // user-defined properties associated with this item:
    std::map<std::string, TPropertiesBasePtr> attr_;
    */
    // the view that owns this style:
    const ItemView & view_;

    // font palette:
    QFont font_;
    QFont font_small_;
    QFont font_large_;
    QFont font_fixed_;

    // shared common properties:
    ItemRef title_height_;
    ItemRef font_size_;

    // color palette:
    ColorRef bg_;
    ColorRef fg_;

    ColorRef border_;
    ColorRef cursor_;
    ColorRef scrollbar_;
    ColorRef separator_;
    ColorRef underline_;

    ColorRef bg_controls_;
    ColorRef fg_controls_;

    ColorRef bg_focus_;
    ColorRef fg_focus_;

    ColorRef bg_edit_selected_;
    ColorRef fg_edit_selected_;

    ColorRef bg_timecode_;
    ColorRef fg_timecode_;

    ColorRef timeline_excluded_;
    ColorRef timeline_included_;
  };


  //----------------------------------------------------------------
  // StyleAttr
  //
  template <typename TStyle, typename TDataRef>
  struct YAE_API StyleAttr : public Expression<typename TDataRef::value_type>
  {
    typedef typename TDataRef::value_type TData;

    StyleAttr(const ItemView & view,
              TDataRef TStyle::* const attr):
      view_(view),
      attr_(attr)
    {}

    // virtual:
    void evaluate(TData & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
    }

    const ItemView & view_;
    TDataRef TStyle::* const attr_;
  };

  //----------------------------------------------------------------
  // StyleAttr<TStyle, ItemRef>
  //
  template <typename TStyle>
  struct YAE_API StyleAttr<TStyle, ItemRef> : public TDoubleExpr
  {
    StyleAttr(const ItemView & view,
              ItemRef TStyle::* const attr,
              double scale = 1.0,
              double translate = 0.0,
              bool odd_round_up = false):
      view_(view),
      attr_(attr),
      scale_(scale),
      translate_(translate)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
      result *= scale_;
      result += translate_;

      if (odd_round_up_)
      {
        int i = 1 | int(ceil(result));
        result = double(i);
      }
    }

    const ItemView & view_;
    ItemRef TStyle::* const attr_;
    double scale_;
    double translate_;
    bool odd_round_up_;
  };

  //----------------------------------------------------------------
  // style_item_ref
  //
  template <typename TStyle>
  inline StyleAttr<TStyle, ItemRef> *
  style_item_ref(const ItemView & view,
                 ItemRef TStyle::* const attr,
                 double s = 1.0,
                 double t = 0.0,
                 bool odd_round_up = false)
  {
    return new StyleAttr<TStyle, ItemRef>(view, attr, s, t, odd_round_up);
  }

  //----------------------------------------------------------------
  // StyleAttr<TStyle, ColorRef>
  //
  template <typename TStyle>
  struct YAE_API StyleAttr<TStyle, ColorRef> : public TColorExpr
  {
    StyleAttr(const ItemView & view,
              ColorRef TStyle::* const attr,
              double alphaScale = 1.0,
              double alphaTranslate = 0.0):
      view_(view),
      attr_(attr),
      alphaScale_(alphaScale),
      alphaTranslate_(alphaTranslate)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
      result = result.a_scaled(alphaScale_, alphaTranslate_);
    }

    const ItemView & view_;
    ColorRef TStyle::* const attr_;
    double alphaScale_;
    double alphaTranslate_;
  };

  //----------------------------------------------------------------
  // style_color_ref
  //
  template <typename TStyle>
  inline StyleAttr<TStyle, ColorRef> *
  style_color_ref(const ItemView & view,
                  ColorRef TStyle::* const attr,
                  double s = 1.0,
                  double t = 0.0)
  {
    return new StyleAttr<TStyle, ColorRef>(view, attr, s, t);
  }

  //----------------------------------------------------------------
  // StyleTitleHeight
  //
  struct StyleTitleHeight : public StyleAttr<ItemViewStyle, ItemRef>
  {
    typedef StyleAttr<ItemViewStyle, ItemRef> TBase;

    StyleTitleHeight(const ItemView & view,
                     double s = 1.0,
                     double t = 0.0,
                     bool odd_round_up = false):
      TBase(view, &ItemViewStyle::title_height_, s, t, odd_round_up)
    {}
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

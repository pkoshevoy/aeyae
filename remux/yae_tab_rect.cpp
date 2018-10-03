// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Sep  8 17:22:46 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// aeyae:
#include "yae/utils/yae_log.h"

// local interfaces:
#include "yae_tab_rect.h"
#include "yaeTexture.h"


//----------------------------------------------------------------
// YAE_DEBUG_TAB_RECT
//
// #define YAE_DEBUG_TAB_RECT

namespace yae
{

  //----------------------------------------------------------------
  // TabRect::TPrivate
  //
  struct TabRect::TPrivate
  {
    TPrivate(TabPosition orientation);
    ~TPrivate();

    //----------------------------------------------------------------
    // Signature
    //
    struct Signature
    {
      Signature():
        r1_(-std::numeric_limits<double>::max()),
        r2_(-std::numeric_limits<double>::max())
      {}

      Signature(const TabRect & tab)
      {
        assign(tab);
      }

      inline bool operator == (const Signature & sig) const
      {
        return (r1_ == sig.r1_ &&
                r2_ == sig.r2_ &&
                fg_ == sig.fg_ &&
                bg_ == sig.bg_);
      }

      inline bool operator != (const Signature & sig) const
      {
        return !(this->operator == (sig));
      }

      void assign(const TabRect & tab)
      {
        r1_ = tab.r1_.get();
        r2_ = tab.r2_.get();
        fg_ = tab.color_.get();
        bg_ = tab.background_.get();
      }

      double r1_; // exterior corner radius
      double r2_; // interior corner radius
      Vec<double, 4> fg_; // color
      Vec<double, 4> bg_; // background
    };

    void uncache();
    bool uploadTexture(const TabRect & item);
    void paint(const TabRect & item);

    TabPosition orientation_;
    Signature sig_;
    BoolRef ready_;
    GLuint texId_;
    int iw_;
    int ih_;
  };

  //----------------------------------------------------------------
  // TabRect::TPrivate::TPrivate
  //
  TabRect::TPrivate::TPrivate(TabPosition orientation):
    orientation_(orientation),
    texId_(0),
    iw_(0),
    ih_(0)
  {}

  //----------------------------------------------------------------
  // TabRect::TPrivate::~TPrivate
  //
  TabRect::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // TabRect::TPrivate::uncache
  //
  void
  TabRect::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // TabRect::TPrivate::uploadTexture
  //
  bool
  TabRect::TPrivate::uploadTexture(const TabRect & item)
  {
    sig_.assign(item);

    // get the corner radia:
    double rr = sig_.r1_ + sig_.r2_;

    // NOTE: the texture is symmetric...
    //
    // I could take advantage of symmetry to store only half of the image and
    // use texture coords at painting stage to make a full symmetric image.
    //
    // The same optimization would be applicable to RoundRect
    // where only a quarter of the image could be stored because its
    // symmetric horizontally and vertically.
    //
    // Not going to take advantage of the symmetry for now.

    // 1x1 textures are legal in OpenGL
    ih_ = std::max(1, int(std::ceil(rr)));
    iw_ = std::max(1, int(std::ceil(rr * 2.0)));

    // make sure texture width is even:
    iw_ = (iw_ & 1) ? (iw_ + 1) : iw_;

    // put origin at the center:
    int iw2 = iw_ / 2;
    double w2 = iw2;

    // we'll be comparing against radius values,
    Segment r1_segment(0, sig_.r1_ + 1);
    Segment r2_segment(0, sig_.r2_);

    TVec2D inflection_pt(w2 - sig_.r2_, sig_.r1_);
    TVec2D origin_r1(w2 - rr, sig_.r1_);
    TVec2D origin_r2(w2, sig_.r1_);

    // supersample each pixel:
#ifndef YAE_DEBUG_TAB_RECT
    static const TVec2D sp[] = { TVec2D(-0.25, -0.25), TVec2D(+0.25, -0.25),
                                 TVec2D(-0.25, +0.25), TVec2D(+0.25, +0.25) };
#else
    static const TVec2D sp[] = { TVec2D(+0.00, +0.00) };
#endif

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    QImage img(iw_, ih_, QImage::Format_ARGB32);
    {
      TVec2D samplePoint;

      for (int j = 0; j < ih_; j++)
      {
        uchar * row = img.scanLine(j);
        uchar * dst = row;
        samplePoint.set_y(double(j));

        for (int i = 0; i < iw2; i++, dst += sizeof(int))
        {
          double overlap = 0.0;
          samplePoint.set_x(double(i));

          for (unsigned int k = 0; k < supersample; k++)
          {
            const TVec2D & jitter = sp[k];
            TVec2D pt = samplePoint + jitter;

            if (pt.x() < inflection_pt.x() &&
                pt.y() < inflection_pt.y())
            {
              // do not allow supersampling to push the point
              // beyond segment start, that would cause background color
              // to be blended in when it shouldn't:
              pt.x() = std::max<double>(0.0, pt.x());
              pt.y() = std::max<double>(0.0, pt.y());

              double p = (pt - origin_r1).norm();
              overlap += (1.0 - r1_segment.pixelOverlap(p));
            }
            else if (inflection_pt.x() <= pt.x() &&
                     inflection_pt.y() <= pt.y())
            {
              double p = (pt - origin_r2).norm();
              overlap += r2_segment.pixelOverlap(p);
            }
            else if (inflection_pt.x() <= pt.x() &&
                     pt.y() < inflection_pt.y())
            {
              overlap += 1.0;
            }
          }

          double fg_weight = overlap / double(supersample);
          double bg_weight = std::max<double>(0.0, 1.0 - fg_weight);

          Color c(sig_.fg_ * fg_weight +
                  sig_.bg_ * bg_weight);

          memcpy(dst, &(c.argb_), sizeof(c.argb_));

          // mirror horizontally:
          uchar * mirror = row + (iw_ - (i + 1)) * sizeof(int);
          memcpy(mirror, &(c.argb_), sizeof(c.argb_));
        }
      }
    }

    bool ok = yae::uploadTexture2D(img, texId_, GL_NEAREST);
    return ok;
  }

  //----------------------------------------------------------------
  // TabRect::TPrivate::paint
  //
  void
  TabRect::TPrivate::paint(const TabRect & item)
  {
    BBox bbox;
    item.Item::get(kPropertyBBox, bbox);

    double h = double(ih_);
    double w = double(iw_);

    // NOTE: the texture is always stored in kTabBottom orientation,
    // so we must account for other orientations when painting.
    bool vertical = is_vertical(orientation_);
    bool mirrored = is_mirrored(orientation_);

    // avoid rendering at fractional pixel coordinates:
    double dw =
      vertical ?
      (bbox.w_ < w ? (w - bbox.w_) : 0.0) :
      (bbox.w_ < h ? (h - bbox.w_) : 0.0);

    double dh =
      vertical ?
      (bbox.h_ < h ? (h - bbox.h_) : 0.0) :
      (bbox.h_ < w ? (w - bbox.h_) : 0.0);

    if (vertical)
    {
      bbox.x_ -= dw * 0.5;
      bbox.w_ += dw;
      bbox.y_ -= dh;
      bbox.h_ += dh;
    }
    else
    {
      bbox.x_ -= dw;
      bbox.w_ += dw;
      bbox.y_ -= dh * 0.5;
      bbox.h_ += dh;
    }

    const int iw2 = iw_ / 2;
    double w2 = double(iw2);

    // texture width:
    double wt = double(powerOfTwoGEQ<int>(iw_));
    double ht = double(powerOfTwoGEQ<int>(ih_));

    double u[4];
    double v[4];

    u[0] = 0;
    u[1] = w2 / wt;
    u[2] = u[1];
    u[3] = w / wt;

    if (mirrored)
    {
      v[3] = 0;
      v[2] = sig_.r1_ / ht;
      v[1] = v[2];
      v[0] = h / ht;
    }
    else
    {
      v[0] = 0;
      v[1] = sig_.r1_ / ht;
      v[2] = v[1];
      v[3] = h / ht;
    }

    double x[4];
    double y[4];

    if (vertical)
    {
      x[0] = floor(bbox.x_ - sig_.r1_ + 0.5);
      x[1] = x[0] + iw2;
      x[3] = x[0] + floor(bbox.w_ + 2.0 * sig_.r1_ + 0.5);
      x[2] = x[3] - iw2;

      if (mirrored)
      {
        y[0] = floor(bbox.y_ + 0.5);
        y[1] = y[0] + sig_.r2_;
        y[3] = y[0] + floor(bbox.h_ + 0.5);
        y[2] = y[3] - sig_.r1_;
      }
      else
      {
        y[0] = floor(bbox.y_ + 0.5);
        y[1] = y[0] + sig_.r1_;
        y[3] = y[0] + floor(bbox.h_ + 0.5);
        y[2] = y[3] - sig_.r2_;
      }
    }
    else
    {
      y[0] = floor(bbox.y_ - sig_.r1_ + 0.5);
      y[1] = y[0] + iw2;
      y[3] = y[0] + floor(bbox.h_ + 2.0 * sig_.r1_ + 0.5);
      y[2] = y[3] - iw2;

      if (mirrored)
      {
        x[0] = floor(bbox.x_ + 0.5);
        x[1] = x[0] + sig_.r2_;
        x[3] = x[0] + floor(bbox.w_ + 0.5);
        x[2] = x[3] - sig_.r1_;
      }
      else
      {
        x[0] = floor(bbox.x_ + 0.5);
        x[1] = x[0] + sig_.r1_;
        x[3] = x[0] + floor(bbox.w_ + 0.5);
        x[2] = x[3] - sig_.r2_;
      }
    }

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));

    YAE_OPENGL_HERE();
    if (glActiveTexture)
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId_));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

    double opacity = item.opacity_.get();
    YAE_OGL_11(glColor4d(1.0, 1.0, 1.0, opacity));
    YAE_OGL_11(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));

    for (int j = 0; j < 3; j++)
    {
      if (y[j] == y[j + 1])
      {
        continue;
      }

      for (int i = 0; i < 3; i++)
      {
        if (x[i] == x[i + 1])
        {
          continue;
        }

        YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
        if (vertical)
        {
          YAE_OGL_11(glTexCoord2d(u[i], v[j]));
          YAE_OGL_11(glVertex2d(x[i], y[j]));

          YAE_OGL_11(glTexCoord2d(u[i], v[j + 1]));
          YAE_OGL_11(glVertex2d(x[i], y[j + 1]));

          YAE_OGL_11(glTexCoord2d(u[i + 1], v[j]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j]));

          YAE_OGL_11(glTexCoord2d(u[i + 1], v[j + 1]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j + 1]));
        }
        else
        {
          YAE_OGL_11(glTexCoord2d(u[j], v[i]));
          YAE_OGL_11(glVertex2d(x[i], y[j]));

          YAE_OGL_11(glTexCoord2d(u[j + 1], v[i]));
          YAE_OGL_11(glVertex2d(x[i], y[j + 1]));

          YAE_OGL_11(glTexCoord2d(u[j], v[i + 1]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j]));

          YAE_OGL_11(glTexCoord2d(u[j + 1], v[i + 1]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j + 1]));
        }
        YAE_OGL_11(glEnd());
      }
    }

    // un-bind:
    if (glActiveTexture)
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, 0));
    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
  }


  //----------------------------------------------------------------
  // TabAnimator
  //
  struct TabAnimator : public ItemView::IAnimator
  {
    TabAnimator(TabRect & tab):
      tab_(tab)
    {}

    // helper:
    inline TransitionItem & transition_item() const
    { return tab_.Item::get<TransitionItem>("hover_opacity"); }

    // helper:
    inline bool in_progress() const
    { return transition_item().transition().in_progress(); }

    // helper:
    inline bool item_overlaps_mouse_pt() const
    { return tab_.view_.isMouseOverItem(tab_); }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animator)
    {
      TransitionItem & opacity = transition_item();
      const Transition & xn = opacity.transition();

      if (item_overlaps_mouse_pt() && xn.is_steady())
      {
        opacity.pause(ItemRef::constant(xn.get_value()));
        tab_.view_.delAnimator(animator);
      }
      else if (xn.is_done())
      {
        tab_.view_.delAnimator(animator);
      }

      opacity.uncache();
    }

    TabRect & tab_;
  };


  //----------------------------------------------------------------
  // TabRect::TabRect
  //
  TabRect::TabRect(const char * id, ItemView & view, TabPosition orientation):
    Item(id),
    view_(view),
    p_(new TabRect::TPrivate(orientation)),
    r1_(ItemRef::constant(3.0)),
    r2_(ItemRef::constant(3.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    background_(ColorRef::constant(Color(0x000000, 0.0)))
  {
    p_->ready_ = addExpr(new UploadTexture<TabRect>(*this));

    typedef Transition::Polyline TPolyline;

    TransitionItem & hover_opacity = this->
      addHidden(new TransitionItem("hover_opacity",
                                   TPolyline(0.25, 0.0, 1.0, 10),
                                   TPolyline(0.05, 1.0, 1.0),
                                   TPolyline(0.25, 1.0, 0.0, 10)));

    TabAnimator * hover_animator = NULL;
    this->hover_.reset(hover_animator = new TabAnimator(*this));

    opacity_ = ItemRef::reference(hover_opacity, kPropertyTransition);
    opacity_.disableCaching();
  }

  //----------------------------------------------------------------
  // TabRect::~TabRect
  //
  TabRect::~TabRect()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // TabRect::uncache
  //
  void
  TabRect::uncache()
  {
    r1_.uncache();
    r2_.uncache();
    opacity_.uncache();
    color_.uncache();
    background_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // TabRect::paint
  //
  void
  TabRect::paintContent() const
  {
    if (p_->ready_.get() && p_->sig_ != TPrivate::Signature(*this))
    {
      p_->uncache();
    }

    if (p_->ready_.get())
    {
      animate_hover();
      p_->paint(*this);
    }
  }

  //----------------------------------------------------------------
  // TabRect::unpaintContent
  //
  void
  TabRect::unpaintContent() const
  {
    p_->uncache();
  }

  //----------------------------------------------------------------
  // TabRect::get
  //
  void
  TabRect::get(Property property, double & value) const
  {
    if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else if (property == kPropertyR1)
    {
      value = r1_.get();
    }
    else if (property == kPropertyR2)
    {
      value = r2_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TabRect::get
  //
  void
  TabRect::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else if (property == kPropertyColorBg)
    {
      value = background_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TabRect::animate_hover
  //
  void
  TabRect::animate_hover(bool force) const
  {
    TabAnimator & animator = dynamic_cast<TabAnimator &>(*(hover_.get()));
    TransitionItem & opacity = Item::find<TransitionItem>("hover_opacity");

    if (force || animator.item_overlaps_mouse_pt())
    {
      opacity.start();
      view_.addAnimator(hover_);
    }
    else if (opacity.is_paused())
    {
      opacity.start();
      view_.addAnimator(hover_);
    }
  }

}

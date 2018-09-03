// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeRoundRect.h"
#include "yaeTexture.h"


//----------------------------------------------------------------
// YAE_DEBUG_ROUND_RECT
//
// #define YAE_DEBUG_ROUND_RECT

namespace yae
{

  //----------------------------------------------------------------
  // RoundRect::TPrivate
  //
  struct RoundRect::TPrivate
  {
    TPrivate();
    ~TPrivate();

    struct Signature
    {
      Signature():
        r_(-std::numeric_limits<double>::max()),
        b_(-std::numeric_limits<double>::max())
      {}

      Signature(const RoundRect & rr)
      {
        assign(rr);
      }

      inline bool operator == (const Signature & sig) const
      {
        return (r_ == sig.r_ &&
                b_ == sig.b_ &&
                fg_ == sig.fg_ &&
                cb_ == sig.cb_ &&
                bg_ == sig.bg_);
      }

      inline bool operator != (const Signature & sig) const
      {
        return !(this->operator == (sig));
      }

      void assign(const RoundRect & rr)
      {
        r_ = rr.radius_.get();
        b_ = rr.border_.get();
        fg_ = rr.color_.get();
        cb_ = rr.colorBorder_.get();
        bg_ = rr.background_.get();
      }

      double r_;
      double b_;
      Vec<double, 4> fg_; // color
      Vec<double, 4> cb_; // color border
      Vec<double, 4> bg_; // background
    };

    void uncache();
    bool uploadTexture(const RoundRect & item);
    void paint(const RoundRect & item);

    Signature sig_;
    BoolRef ready_;
    GLuint texId_;
    int iw_;
  };

  //----------------------------------------------------------------
  // RoundRect::TPrivate::TPrivate
  //
  RoundRect::TPrivate::TPrivate():
    texId_(0),
    iw_(0)
  {}

  //----------------------------------------------------------------
  // RoundRect::TPrivate::~TPrivate
  //
  RoundRect::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::uncache
  //
  void
  RoundRect::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::uploadTexture
  //
  bool
  RoundRect::TPrivate::uploadTexture(const RoundRect & item)
  {
    sig_.assign(item);

    // get the corner radius:
    double r = item.radius_.get();

    // get the border width:
    double b = item.border_.get();

    // make sure radius is not less than border width:
    double c = (b <= 0) ? r : std::max(r, b + 2.0);

    // inner radius:
    double r0 = (b <= 0) ? r : std::max(0.0, r - b);

    // make sure image is at least 2 pixels wide:
    iw_ = (int)(1.0 + std::ceil(2.0 * c));

    // make sure texture size is even:
    iw_ = (iw_ & 1) ? (iw_ + 1) : iw_;

    // put origin at the center:
    int iw2 = iw_ / 2;
    double w2 = iw2;

    // we'll be comparing against radius values,
    double cr = c - r;
    double cb = c - b;

    TVec2D cornerOrigin = (r < c) ? TVec2D(cr, cr) : TVec2D(0, 0);
    Segment outerCorner(0, r);
    Segment innerCorner(0, r0);
#if 0
    // NOTE: shift sgement start to prevent supersampling at (0, 0)
    //       from being interpreted as sampling outside inner rect
    //
    // NOTE: segments are defined by (start, length), so to
    //       offset start to -1 length has to be compensted by +1
    //
    Segment outerSegment(-1, c + 1);
    Segment innerSegment(-1, cb + 1);
#else
    Segment outerSegment(0, c);
    Segment innerSegment(0, cb);
#endif

    // supersample each pixel:
#ifndef YAE_DEBUG_ROUND_RECT
    static const TVec2D sp[] = { TVec2D(-0.25, -0.25), TVec2D(+0.25, -0.25),
                                 TVec2D(-0.25, +0.25), TVec2D(+0.25, +0.25) };
    // static const TVec2D sp[] = { TVec2D(+0.00, +0.00) };


    Vec<double, 4> outerColor(item.colorBorder_.get());
    Vec<double, 4> innerColor(item.color_.get());
    Vec<double, 4> bgColor(item.background_.get());
#else
    static const TVec2D sp[] = { TVec2D(+0.00, +0.00) };

    Vec<double, 4> outerColor = Color(0x1d2e3b);
    Vec<double, 4> innerColor = Color(0x4f7da1);
    Vec<double, 4> bgColor = Color(0xffffff);
#endif

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    QImage img(iw_, iw_, QImage::Format_ARGB32);
    {
      TVec2D samplePoint;

      for (int j = 0; j < iw2; j++)
      {
        uchar * row = img.scanLine(iw2 + j);
        uchar * dst = row + sizeof(int) * iw2;
        samplePoint.set_y(double(j));

        for (int i = 0; i < iw2; i++, dst += sizeof(int))
        {
          samplePoint.set_x(double(i));

          double outer = 0.0;
          double inner = 0.0;

          for (unsigned int k = 0; k < supersample; k++)
          {
            const TVec2D & jitter = sp[k];
            TVec2D pt = samplePoint + jitter;

            if (cornerOrigin.x() <= pt.x() &&
                cornerOrigin.y() <= pt.y())
            {
              // do not allow supersampling to push the point
              // beyond segment start, that would cause background color
              // to be blended in when it shouldn't:
              pt.x() = std::max<double>(0.0, pt.x());
              pt.y() = std::max<double>(0.0, pt.y());

              double p = (pt - cornerOrigin).norm();
              double outerOverlap = outerCorner.pixelOverlap(p);
              double innerOverlap = ((r0 < r) ?
                                     innerCorner.pixelOverlap(p) :
                                     outerOverlap);

              outerOverlap = std::max<double>(0.0, outerOverlap - innerOverlap);

              outer += outerOverlap;
              inner += innerOverlap;
            }
            else
            {
              // do not supersample vertical/horizontal bands
              // to avoid making them look blurry:
              pt -= jitter;

              double p = (pt.x() < pt.y()) ? pt.y() : pt.x();
              double outerOverlap = outerSegment.pixelOverlap(p);
              double innerOverlap = ((b <= 0) ?
                                     outerOverlap :
                                     innerSegment.pixelOverlap(p));

              outerOverlap = std::max<double>(0.0, outerOverlap - innerOverlap);

              outer += outerOverlap;
              inner += innerOverlap;
            }
          }

          double outerWeight = outer / double(supersample);
          double innerWeight = inner / double(supersample);
          double backgroundWeight =
            std::max<double>(0.0, 1.0 - (outerWeight + innerWeight));

          Color c(outerColor * outerWeight +
                  innerColor * innerWeight +
                  bgColor * backgroundWeight);

          memcpy(dst, &(c.argb_), sizeof(c.argb_));

          // mirror horizontally:
          uchar * mirror = row + (iw2 - (i + 1)) * sizeof(int);
          memcpy(mirror, &(c.argb_), sizeof(c.argb_));
        }

        // mirror vertically:
        uchar * mirror = img.scanLine(iw2 - (j + 1));
        memcpy(mirror, row, sizeof(int) * iw_);
      }
    }

    bool ok = yae::uploadTexture2D(img, texId_, GL_NEAREST);
    return ok;
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::paint
  //
  void
  RoundRect::TPrivate::paint(const RoundRect & item)
  {
    BBox bbox;
    item.Item::get(kPropertyBBox, bbox);

    // avoid rendering at fractional pixel coordinates:
    double w = double(iw_);
    double dw = bbox.w_ < w ? (w - bbox.w_) : 0.0;
    double dh = bbox.h_ < w ? (w - bbox.h_) : 0.0;
    bbox.x_ -= dw * 0.5;
    bbox.y_ -= dh * 0.5;
    bbox.w_ += dw;
    bbox.h_ += dh;

    // texture width:
    double wt = double(powerOfTwoGEQ<int>(iw_));

    double t[4];
    t[0] = 0.0;
    t[1] = (double(iw_ / 2)) / wt;
    t[2] = t[1];
    t[3] = w / wt;

    double x[4];
    x[0] = floor(bbox.x_ + 0.5);
    x[1] = x[0] + iw_ / 2;
    x[3] = x[0] + floor(bbox.w_ + 0.5);
    x[2] = x[3] - iw_ / 2;

    double y[4];
    y[0] = floor(bbox.y_ + 0.5);
    y[1] = y[0] + iw_ / 2;
    y[3] = y[0] + floor(bbox.h_ + 0.5);
    y[2] = y[3] - iw_ / 2;

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
        {
          YAE_OGL_11(glTexCoord2d(t[i], t[j]));
          YAE_OGL_11(glVertex2d(x[i], y[j]));

          YAE_OGL_11(glTexCoord2d(t[i], t[j + 1]));
          YAE_OGL_11(glVertex2d(x[i], y[j + 1]));

          YAE_OGL_11(glTexCoord2d(t[i + 1], t[j]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j]));

          YAE_OGL_11(glTexCoord2d(t[i + 1], t[j + 1]));
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
  // RoundRect::RoundRect
  //
  RoundRect::RoundRect(const char * id):
    Item(id),
    p_(new RoundRect::TPrivate()),
    radius_(ItemRef::constant(0.0)),
    border_(ItemRef::constant(0.0)),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25))),
    background_(ColorRef::constant(Color(0x000000, 0.0)))
  {
    p_->ready_ = addExpr(new UploadTexture<RoundRect>(*this));
  }

  //----------------------------------------------------------------
  // RoundRect::~RoundRect
  //
  RoundRect::~RoundRect()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // RoundRect::uncache
  //
  void
  RoundRect::uncache()
  {
    radius_.uncache();
    border_.uncache();
    opacity_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    background_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::paint
  //
  void
  RoundRect::paintContent() const
  {
    if (p_->ready_.get() && p_->sig_ != TPrivate::Signature(*this))
    {
      p_->uncache();
    }

    if (p_->ready_.get())
    {
      p_->paint(*this);
    }
  }

  //----------------------------------------------------------------
  // RoundRect::unpaintContent
  //
  void
  RoundRect::unpaintContent() const
  {
    p_->uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::get
  //
  void
  RoundRect::get(Property property, double & value) const
  {
    if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // RoundRect::get
  //
  void
  RoundRect::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else if (property == kPropertyColorBorder)
    {
      value = colorBorder_.get();
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

}

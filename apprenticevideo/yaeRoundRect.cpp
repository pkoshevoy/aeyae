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


namespace yae
{

  //----------------------------------------------------------------
  // RoundRect::TPrivate
  //
  struct RoundRect::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void uncache();
    bool uploadTexture(const RoundRect & item);
    void paint(const RoundRect & item);

    BoolRef ready_;
    GLuint texId_;
    GLuint iw_;
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
    // get the corner radius:
    double r = item.radius_.get();

    // get the border width:
    double b = item.border_.get();

    // make sure radius is not less than border width:
    r = std::max<double>(r, b);

    // inner radius:
    double r0 = r - b;

    // we'll be comparing against radius values,
    Segment outerSegment(0.0, r);
    Segment innerSegment(0.0, r0);

    // make sure image is at least 2 pixels wide:
    iw_ = (int)std::ceil(std::max<double>(2.0, 2.0 * r));

    // make sure texture size is even:
    iw_ = (iw_ & 1) ? (iw_ + 1) : iw_;

    // put origin at the center:
    double w2 = iw_ / 2;

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    QImage img(iw_, iw_, QImage::Format_ARGB32);
    {
      Vec<double, 4> outerColor(item.colorBorder_.get());
      Vec<double, 4> innerColor(item.color_.get());
      TVec2D samplePoint;

      for (int j = 0; j < int(iw_); j++)
      {
        uchar * row = img.scanLine(j);
        uchar * dst = row;
        samplePoint.set_y(double(j - w2));

        for (int i = 0; i < int(iw_); i++, dst += sizeof(int))
        {
          samplePoint.set_x(double(i - w2));

          double outer = 0.0;
          double inner = 0.0;

          for (unsigned int k = 0; k < supersample; k++)
          {
            double p =
              std::max<double>(0.0, (samplePoint + sp[k]).norm() - 1.0);

            double outerOverlap = outerSegment.pixelOverlap(p);
            double innerOverlap =
              (r0 < r) ? innerSegment.pixelOverlap(p) : outerOverlap;

            outerOverlap = std::max<double>(0.0, outerOverlap - innerOverlap);
            outer += outerOverlap;
            inner += innerOverlap;
          }

          double outerWeight = outer / double(supersample);
          double innerWeight = inner / double(supersample);
          Color c(outerColor * outerWeight + innerColor * innerWeight);
          memcpy(dst, &(c.argb_), sizeof(c.argb_));
        }
      }
    }

    bool ok = yae::uploadTexture2D(img, texId_, iw_, iw_, GL_NEAREST);
    return ok;
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::paint
  //
  void
  RoundRect::TPrivate::paint(const RoundRect & item)
  {
    BBox bbox;
    item.get(kPropertyBBox, bbox);

    // avoid rendering at fractional pixel coordinates:
    double w = double(iw_);
    double dw = bbox.w_ < w ? (w - bbox.w_) : 0.0;
    double dh = bbox.h_ < w ? (w - bbox.h_) : 0.0;
    bbox.x_ -= dw * 0.5;
    bbox.y_ -= dh * 0.5;
    bbox.w_ += dw;
    bbox.h_ += dh;

    // get the corner radius:
    double r = item.radius_.get();

    // get the border width:
    double b = item.border_.get();

    // make sure radius is not less than border width:
    r = std::max<double>(r, b);

    // texture width:
    double wt = double(powerOfTwoGEQ<GLsizei>(iw_));

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
    YAE_OGL_11(glColor3f(1.f, 1.f, 1.f));
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
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25)))
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
    color_.uncache();
    colorBorder_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::paint
  //
  void
  RoundRect::paintContent() const
  {
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

}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Jun  5 22:03:10 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QApplication>
#include <QFontInfo>

// local:
#include "yaeItemViewStyle.h"


namespace yae
{


  //----------------------------------------------------------------
  // xbuttonImage
  //
  QImage
  xbuttonImage(unsigned int w,
               const Color & color,
               const Color & background,
               double thickness,
               double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    int w2 = w / 2;
    double diameter = double(w);
    double center = diameter * 0.5;
    Segment sa(-center, diameter);
    Segment sb(-diameter * thickness * 0.5, diameter * thickness);

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);
          double oh = sa.pixelOverlap(pt.x()) * sb.pixelOverlap(pt.y());
          double ov = sb.pixelOverlap(pt.x()) * sa.pixelOverlap(pt.y());
          double innerOverlap = std::max<double>(oh, ov);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }

  //----------------------------------------------------------------
  // triangleImage
  //
  QImage
  triangleImage(unsigned int w,
                const Color & color,
                const Color & background,
                double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);
    static const double sqrt_3 = 1.732050807568877;

    int w2 = w / 2;
    double diameter = double(w);
    double radius = diameter * 0.5;
    double half_r = diameter * 0.25;
    double base_w = radius * sqrt_3;
    double half_b = base_w * 0.5;
    double height = radius + half_r;

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);

          double ty = pt.y() + half_r;
          double t = 1.0 - ty / height;
          double tb = (t > 1.0) ? 0.0 : (t * half_b);
          double tx = (tb <= 0.0) ? -1.0 : (1.0 - fabs(pt.x()) / tb);
          double innerOverlap = (tx >= 0.0);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }

  //----------------------------------------------------------------
  // twobarsImage
  //
  QImage
  barsImage(unsigned int w,
            const Color & color,
            const Color & background,
            unsigned int nbars,
            double thickness,
            double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    YAE_ASSERT(nbars > 0 && nbars < w && thickness < 1.0);

    int w2 = w / 2;
    double diameter = double(w);
    double radius = diameter * 0.5;
    Segment sv(-radius, diameter);
    double band_w = diameter / double(nbars);
    double bar_w = thickness * band_w;
    double spacing = band_w - bar_w;
    Segment sh(0.5 * spacing, bar_w);
    double offset = diameter + ((nbars & 1) ? (0.5 * band_w) : 0.0);

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);

          // the image is periodic and symmetric:
          double px = fmod(pt.x() + offset, band_w);
          double py = fabs(pt.y());
          double innerOverlap = sh.pixelOverlap(px) * sv.pixelOverlap(py);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }


  //----------------------------------------------------------------
  // ItemViewStyle::ItemViewStyle
  //
  ItemViewStyle::ItemViewStyle(const char * id, const ItemView & view):
    Item(id),
    view_(view)
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_small_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_small_.setFamily("Segoe UI, "
                          "Arial, "
                          "Helvetica, "
                          "DejaVu Sans, "
                          "Bitstream Vera Sans");

    font_small_.setStyleHint(QFont::SansSerif);
    font_small_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    // main font:
    font_ = font_small_;
    font_large_ = font_small_;

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_large_.setFamily("impact");

#if !(defined(_WIN32) ||                        \
      defined(__APPLE__) ||                     \
      QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
      font_large_.setStretch(QFont::Condensed);
#endif
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_large_.setStretch(QFont::Condensed);
      font_large_.setWeight(QFont::Black);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_fixed_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_fixed_.setFamily("Menlo, "
                          "Monaco, "
                          "Droid Sans Mono, "
                          "DejaVu Sans Mono, "
                          "Bitstream Vera Sans Mono, "
                          "Consolas, "
                          "Lucida Sans Typewriter, "
                          "Lucida Console, "
                          "Courier New");
    font_fixed_.setStyleHint(QFont::Monospace);
    font_fixed_.setFixedPitch(true);
    font_fixed_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    anchors_.top_ = ItemRef::constant(0);
    anchors_.left_ = ItemRef::constant(0);
    width_ = ItemRef::constant(0);
    height_ = ItemRef::constant(0);

    dpi_ = addExpr(new ViewDpi(view));
    dpi_.disableCaching();
    row_height_ = addExpr(new GetRowHeight(view));

    title_height_ = addExpr(new CalcTitleHeight(view, 50.0));
    font_size_ = ItemRef::reference(title_height_, 0.15);

    // color palette:
    bg_ = ColorRef::constant(Color(0x1f1f1f, 0.87));
    fg_ = ColorRef::constant(Color(0xffffff, 1.0));

    border_ = ColorRef::constant(Color(0x7f7f7f, 1.0));
    cursor_ = ColorRef::constant(Color(0xf12b24, 1.0));
    scrollbar_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    separator_ = ColorRef::constant(scrollbar_.get());
    underline_ = ColorRef::constant(cursor_.get());

    bg_timecode_ = ColorRef::constant(Color(0x7f7f7f, 0.25));
    fg_timecode_ = ColorRef::constant(Color(0xFFFFFF, 0.5));

    bg_controls_ = ColorRef::constant(bg_timecode_.get());
    fg_controls_ = ColorRef::constant(fg_timecode_.get().opaque(0.75));

    bg_focus_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    fg_focus_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_edit_selected_ = ColorRef::constant(Color(0xffffff, 1.0));
    fg_edit_selected_ = ColorRef::constant(Color(0x000000, 1.0));

    timeline_excluded_ = ColorRef::constant(Color(0xFFFFFF, 0.2));
    timeline_included_ = ColorRef::constant(Color(0xFFFFFF, 0.5));
    timeline_played_ = cursor_;

    // timeline shadow gradient:
    timeline_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *timeline_shadow_;
      gradient[0.000000] = Color(0x000000, 0.004);
      gradient[0.135417] = Color(0x000000, 0.016);
      gradient[0.208333] = Color(0x000000, 0.031);
      gradient[0.260417] = Color(0x000000, 0.047);
      gradient[0.354167] = Color(0x000000, 0.090);
      gradient[0.447917] = Color(0x000000, 0.149);
      gradient[0.500000] = Color(0x000000, 0.192);
      gradient[1.000000] = Color(0x000000, 0.690);
    }

    pause_ = Item::addHidden<Texture>
      (new Texture("pause", QImage())).sharedPtr<Texture>();

    play_ = Item::addHidden<Texture>
      (new Texture("play", QImage())).sharedPtr<Texture>();

    // generate pause button texture:
    {
      QImage img = barsImage(128,
                             fg_controls_.get(),
                             bg_controls_.get().transparent(),
                             2, 0.8);
      pause_->setImage(img);
    }

    // generate play button texture:
    {
      QImage img = triangleImage(256,
                                 fg_controls_.get(),
                                 bg_controls_.get().transparent(),
                                 90.0);
      play_->setImage(img);
    }
  }

  //----------------------------------------------------------------
  // ItemViewStyle::uncache
  //
  void
  ItemViewStyle::uncache()
  {
    title_height_.uncache();
    font_size_.uncache();

    bg_.uncache();
    fg_.uncache();

    border_.uncache();
    cursor_.uncache();
    scrollbar_.uncache();
    separator_.uncache();
    underline_.uncache();

    bg_controls_.uncache();
    fg_controls_.uncache();

    bg_focus_.uncache();
    fg_focus_.uncache();

    bg_edit_selected_.uncache();
    fg_edit_selected_.uncache();

    bg_timecode_.uncache();
    fg_timecode_.uncache();

    timeline_excluded_.uncache();
    timeline_included_.uncache();
    timeline_played_.uncache();

    Item::uncache();
  }
}

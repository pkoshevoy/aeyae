// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Aug 30 20:34:33 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/video/yae_colorspace.h"
#include "yae/video/yae_texture_generator.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // TextureGenerator::TextureGenerator
  //
  TextureGenerator::TextureGenerator(int luma_w,
                                     int luma_h,
                                     const Colorspace * csp):
    csp_(csp ? csp : Colorspace::get(AVCOL_SPC_BT709,
                                     AVCOL_PRI_BT709,
                                     AVCOL_TRC_BT709)),
    luma_w_(luma_w),
    luma_h_(luma_h),
    offset_(0)
  {}

  //----------------------------------------------------------------
  // TextureGenerator::set_offset
  //
  void
  TextureGenerator::set_offset(unsigned int offset)
  {
    offset_ = offset;
  }

  //----------------------------------------------------------------
  // TextureGenerator::get_ypbpr
  //
  v3x1_t
  TextureGenerator::get_ypbpr(int row, int col) const
  {
    v3x1_t rgb = get_rgb(row, col);
    v3x1_t ypbpr = csp_->rgb_to_ypbpr_ * rgb;
    return ypbpr;
  }


  //----------------------------------------------------------------
  // ColorbarsGenerator::ColorbarsGenerator
  //
  ColorbarsGenerator::ColorbarsGenerator(int luma_w, int luma_h, const Colorspace * csp):
    TextureGenerator(luma_w, luma_h, csp),
    ww_((luma_w + 1) >> 1),
    hh_((luma_h + 1) >> 1)
  {}

  //----------------------------------------------------------------
  // ColorbarsGenerator::get_ypbpr
  //
  v3x1_t
  ColorbarsGenerator::get_ypbpr(int row, int col) const
  {
    v3x1_t ypbpr;

    unsigned int rr = (row >> 1);
    unsigned int cc = (col >> 1);

    double t = double(rr) / double(hh_ - 1);
    double s = double(cc) / double(ww_ - 1);

    uint16_t y = 0;
    uint16_t pb = 0;
    uint16_t pr = 0;

    static const double kc = 0.75 / 7.0;
    static const double kd = 1.0 / 8.0;
    static const double ke = 1.0 / 12.0;

    if (t < ke * 7)
    {
      if (s < kd)
      {
        // 40% gray
        y = 414;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + kc))
      {
        // 75% white
        y = 721;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + kc * 2))
      {
        // yellow
        y = 674;
        pb = 176;
        pr = 543;
      }
      else if (s < (kd + kc * 3))
      {
        // cyan
        y = 581;
        pb = 589;
        pr = 176;
      }
      else if (s < (kd + kc * 4))
      {
        // green
        y = 534;
        pb = 253;
        pr = 207;
      }
      else if (s < (kd + kc * 5))
      {
        // magenta
        y = 251;
        pb = 771;
        pr = 817;
      }
      else if (s < (kd + kc * 6))
      {
        // red
        y = 204;
        pb = 435;
        pr = 848;
      }
      else if (s < (1.0 - kd))
      {
        // blue
        y = 111;
        pb = 848;
        pr = 481;
      }
      else
      {
        // 40% gray
        y = 414;
        pb = 512;
        pr = 512;
      }
    }
    else if (t < ke * 8)
    {
      if (s < kd)
      {
        // 100% cyan
        y = 754;
        pb = 615;
        pr = 64;
      }
      else if (s < (kd + kc))
      {
        // +I signal
        y = 245;
        pb = 412;
        pr = 629;
      }
      else if (s < (1 - kd))
      {
        // 75% white
        y = 721;
        pb = 512;
        pr = 512;
      }
      else
      {
        // 100% blue
        y = 127;
        pb = 960;
        pr = 471;
      }
    }
    else if (t < ke * 9)
    {
      if (s < kd)
      {
        // 100% yellow
        y = 877;
        pb = 64;
        pr = 553;
      }
      else if (s < (kd + kc))
      {
        // 100% black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else if (s < (1 - kd - kc))
      {
        // Y ramp
        double q = (s - (kd + kc)) / (1 - 2 * (kd + kc));
        y = uint16_t(64 * (1.0 - q) + 940 * q);
        pb = 512;
        pr = 512;
      }
      else if (s < (1 - kd))
      {
        // 100% white
        y = 940;
        pb = 512;
        pr = 512;
      }
      else
      {
        // 100% red
        y = 250;
        pb = 409;
        pr = 950;
      }
    }
    else
    {
      if (s < kd)
      {
        // 15% gray
        y = 195;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 9 * kc / 6))
      {
        // 0% black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 21 * kc / 6))
      {
        // 100% white
        y = 940;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 26 * kc / 6))
      {
        // 0% black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 28 * kc / 6))
      {
        // -2% black
        y = 46;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 30 * kc / 6))
      {
        // 0 black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 32 * kc / 6))
      {
        // +2% black
        y = 82;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 34 * kc / 6))
      {
        // 0 black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 36 * kc / 6))
      {
        // +4% black
        y = 99;
        pb = 512;
        pr = 512;
      }
      else if (s < (kd + 42 * kc / 6))
      {
        // 0% black
        y = 64;
        pb = 512;
        pr = 512;
      }
      else
      {
        // 15% gray
        y = 195;
        pb = 512;
        pr = 512;
      }
    }

    ypbpr[0] = (y - 64.0) / (940.0 - 64.0);
    ypbpr[1] = (pb - 64.0) / (960.0 - 64.0) - 0.5;
    ypbpr[2] = (pr - 64.0) / (960.0 - 64.0) - 0.5;

    return ypbpr;
  }

  //----------------------------------------------------------------
  // ColorbarsGenerator::get_rgb
  //
  v3x1_t
  ColorbarsGenerator::get_rgb(int row, int col) const
  {
    v3x1_t ypbpr = get_ypbpr(row, col);
    v3x1_t rgb = csp_->ypbpr_to_rgb_ * ypbpr;

    // not all YUV values can be represented in RGB due to clipping.
    // to avoid expected:actual mismatches we should pre-clip the YUV
    // values.
    rgb = yae::clip(rgb, 0.0, 1.0);

    return rgb;
  }


  //----------------------------------------------------------------
  // ChromaWheelGenerator::ChromaWheelGenerator
  //
  ChromaWheelGenerator::ChromaWheelGenerator(int luma_w,
                                             int luma_h,
                                             const Colorspace * csp):
    TextureGenerator(luma_w, luma_h, csp),
    rotate_chroma_(yae::make_identity_m3x3())
  {}

  //----------------------------------------------------------------
  // ChromaWheelGenerator::set_offset
  //
  void
  ChromaWheelGenerator::set_offset(unsigned int offset)
  {
    TextureGenerator::set_offset(offset);

    // update the rotation matrix:
    rotate_chroma_ = make_m3x3(// axis of rotation:
                               1, 0, 0,
                               // angle of rotation:
                               M_PI * double(offset_ % 24) / 12.0);
  }

  //----------------------------------------------------------------
  // ChromaWheelGenerator::get_rgb
  //
  v3x1_t
  ChromaWheelGenerator::get_rgb(int row, int col) const
  {
    double t = double(row >> 1) / double((luma_h_ >> 1) - 1);
    double s = double(col >> 1) / double((luma_w_ >> 1) - 1);

    // not all YUV values can be represented in RGB due to clipping.
    // to avoid expected:actual mismatches we should pre-clip the YUV
    // values.
    v3x1_t ypbpr = make_v3x1(t, s - 0.5, t - 0.5);

    // rotate chroma:
    ypbpr = rotate_chroma_ * ypbpr;

    v3x1_t rgb = csp_->ypbpr_to_rgb_ * ypbpr;
    rgb = yae::clip(rgb, 0.0, 1.0);

    return rgb;
  }


  //----------------------------------------------------------------
  // ShadedSquaresGenerator::ShadedSquaresGenerator
  //
  ShadedSquaresGenerator::ShadedSquaresGenerator(int luma_w,
                                                 int luma_h,
                                                 const Colorspace * csp):
    TextureGenerator(luma_w, luma_h, csp),
    ww_((luma_w_ + 15) >> 4),
    hh_((luma_h_ + 15) >> 4)
  {}

  //----------------------------------------------------------------
  // ShadedSquaresGenerator::get_rgb
  //
  v3x1_t
  ShadedSquaresGenerator::get_rgb(int row, int col) const
  {
    unsigned int rr = (row >> 4) % hh_;
    unsigned int cc = ((col + (offset_ << 1)) >> 4) % ww_;

    double hue = cc / double(ww_);
    YAE_ASSERT(0.0 <= hue && hue <= 1.0);

    double sat = ((cc % 8) / 7.0 + (rr % 8) / 7.0 +
                  2 * rr / double(hh_ - 1)) / 4.0;
    YAE_ASSERT(0.0 <= sat && sat <= 1.0);

#if 0
    double val = (((cc % 8) / 7.0 + (rr % 8) / 7.0) * 2 +
                  rr / double(hh_ - 1) + cc / double(ww_ - 1)) / 6.0;
#else
    double val = 1.0 - (((cc % 8) / 7.0 + (rr % 8) / 7.0) * 2 +
                        rr / double(hh_ - 1) + cc / double(ww_ - 1)) / 7.0;
#endif
    YAE_ASSERT(0.0 <= val && val <= 1.0);

    v3x1_t hsv = make_v3x1(hue, sat, val);
    v3x1_t rgb = hsv_to_rgb(hsv);
    return rgb;
  }

}


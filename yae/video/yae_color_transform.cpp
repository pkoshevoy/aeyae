// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Sep  9 12:32:22 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <algorithm>
#include <cmath>

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/video/yae_color_transform.h"


namespace yae
{

  //----------------------------------------------------------------
  // ToneMapPiecewise::apply
  //
  void
  ToneMapPiecewise::apply(const Colorspace::DynamicRange & src_dynamic_range,
                          const Colorspace::DynamicRange & dst_dynamic_range,
                          const double * src_rgb_cdm2,
                          double * dst_rgb_cdm2) const
  {
    // pick the brightest component:
    const double src = std::max(std::max(src_rgb_cdm2[0], src_rgb_cdm2[1]),
                                src_rgb_cdm2[2]) / src_dynamic_range.Lw_;

    const double peak_ratio = src_dynamic_range.Lw_ / dst_dynamic_range.Lw_;
    const double t = src * peak_ratio;

    double out = src;
    if (t <= 1.0)
    {
      // 0-100 cd/m2 --> 0-75 cd/m2
      out = t * 0.75;
    }
    else if (t <= 2.0)
    {
      out = 0.75 + 0.10 * (t - 1.0);
    }
    else if (t <= 3.0)
    {
      out = 0.85 + 0.05 * (t - 2.0);
    }
    else if (t <= 4.0)
    {
      out = 0.90 + 0.03 * (t - 3.0);
    }
    else if (t <= 5.0)
    {
      out = 0.93 + 0.02 * (t - 5.0);
    }
    else
    {
      out = 0.95 + 0.05 * (t - 5.0) / 5.0;
    }

    const double rescale =
      (src == 0.0) ? 0.0 :
      (out / src) * (dst_dynamic_range.Lw_ / src_dynamic_range.Lw_);

    dst_rgb_cdm2[0] = src_rgb_cdm2[0] * rescale;
    dst_rgb_cdm2[1] = src_rgb_cdm2[1] * rescale;
    dst_rgb_cdm2[2] = src_rgb_cdm2[2] * rescale;
  }


  //----------------------------------------------------------------
  // ToneMapLog::apply
  //
  void
  ToneMapLog::apply(const Colorspace::DynamicRange & src_dynamic_range,
                    const Colorspace::DynamicRange & dst_dynamic_range,
                    const double * src_rgb_cdm2,
                    double * dst_rgb_cdm2) const
  {
    static const double knee = 10; // cd/m2
    static const double n = 5;
    static const double m = -0.5;
    static const double e = 2.7182818284590452354;
    static const double em = std::pow(e, m);
    static const double en = std::pow(e, n);
    static const double en_inv = 1.0 / en;

    // pick the brightest component:
    const double src = std::max(std::max(src_rgb_cdm2[0], src_rgb_cdm2[1]),
                                src_rgb_cdm2[2]);
    double out = src;

    if (knee <= src)
    {
      out = knee +
        (n + std::log(en_inv +
                      (em - en_inv) * (src - knee) /
                      src_dynamic_range.Lw_)) *
        (dst_dynamic_range.Lw_ - knee) / (n + m);
    }

    const double rescale = (src == 0.0) ? 0.0 : (out / src);
    dst_rgb_cdm2[0] = src_rgb_cdm2[0] * rescale;
    dst_rgb_cdm2[1] = src_rgb_cdm2[1] * rescale;
    dst_rgb_cdm2[2] = src_rgb_cdm2[2] * rescale;
  }

}

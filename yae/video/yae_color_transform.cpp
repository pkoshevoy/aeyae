// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Sep  9 12:32:22 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <algorithm>
#include <cmath>

// ffmpeg includes:
extern "C"
{
#include <libavutil/pixdesc.h>
}

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/video/yae_color_transform.h"


namespace yae
{

  //----------------------------------------------------------------
  // get_ycbcr_to_ypbpr
  //
  // setup an affine transform to from full/narrow
  // range normalized [0, 1] pixel values to Y'PbPr
  // where Y is [0, 1] and Pb,Pr are [-0.5, 0.5]
  //
  bool
  get_ycbcr_to_ypbpr(m4x4_t & ycbcr_to_ypbpr,
                     AVPixelFormat av_fmt,
                     AVColorRange av_rng)
  {
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(av_fmt);
    if (!desc)
    {
      return false;
    }

    // shortcut:
    const bool narrow_range =
      (av_rng == AVCOL_RANGE_MPEG);

    const bool flag_rgb =
      (desc->flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB;

    const bool flag_alpha =
      (desc->flags & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;

    const AVComponentDescriptor & luma = desc->comp[0];

    // is accounting for bitdepth greater than 8 bits actually correct?
    // it affects the scaling factors, and IDK if that is intended or
    // if those are supposed to be the same regardless of the bitdepth...
#if 1
    const unsigned int bit_depth = luma.shift + luma.depth;
#else
    const unsigned int bit_depth = 8;
#endif

    const unsigned int y_full = ~((~0) << bit_depth);
    const unsigned int lshift = bit_depth - 8;

    const unsigned int y_min = narrow_range ? (16 << lshift) : 0;
    const unsigned int y_rng = narrow_range ? (219 << lshift) : y_full;

    const double y_offset = double(y_min) / double(y_full);
    const double sy = double(y_full) / double(y_rng);
    const double a = -y_offset * sy;

    if (yae::is_ycbcr(*desc))
    {
      const unsigned int c_rng = narrow_range ? (224 << lshift) : y_full;
      const double sc = double(y_full) / double(c_rng);
      const double b = -y_offset * sc - 0.5;

      /*
        double Y' = (Y - y_offset) * scale_luma;
        double Pb = (Cb - y_offset) * scale_chroma - 0.5;
        double Pr = (Cr - y_offset) * scale_chroma - 0.5;
      */

      // affine transform from Y'CbCr to Y'PbPr:
      ycbcr_to_ypbpr = make_m4x4(sy,  0.0, 0.0, a,
                                 0.0, sc,  0.0, b,
                                 0.0, 0.0, sc,  b,
                                 0.0, 0.0, 0.0, 1.0);
    }
    else if (narrow_range &&
             (flag_rgb ||
              (desc->nb_components == 2 && flag_alpha) ||
              (desc->nb_components == 1)))
    {
      // NOTE: the input is not actually Y'CbCr, and the output won't be Y'PbPr
      //       so treat "chroma" the same as "luma"

      // affine transform from narrow range to full range:
      ycbcr_to_ypbpr = make_m4x4(sy,  0.0, 0.0, a,
                                 0.0, sy,  0.0, a,
                                 0.0, 0.0, sy,  a,
                                 0.0, 0.0, 0.0, 1.0);
    }
    else
    {
      // nothing to do, use the identity matrix:
      static const m4x4_t identity = make_identity_m4x4();
      ycbcr_to_ypbpr = identity;
    }

    return true;
  }

  //----------------------------------------------------------------
  // get_ypbpr_to_ycbcr
  //
  // setup an affine transform to map from Y'PbPr
  // to [0, 1] normalized full/narrow range:
  //
  bool
  get_ypbpr_to_ycbcr(m4x4_t & ycbcr_to_ypbpr,
                     AVPixelFormat av_fmt,
                     AVColorRange av_rng)
  {
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(av_fmt);
    if (!desc)
    {
      return false;
    }

    // shortcut:
    const bool narrow_range =
      (av_rng == AVCOL_RANGE_MPEG);

    const bool flag_rgb =
      (desc->flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB;

    const bool flag_alpha =
      (desc->flags & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;

    const AVComponentDescriptor & luma = desc->comp[0];

    // is accounting for bitdepth greater than 8 bits actually correct?
    // it affects the scaling factors, and IDK if that is intended or
    // if those are supposed to be the same regardless of the bitdepth...
#if 1
    const unsigned int bit_depth = luma.shift + luma.depth;
#else
    const unsigned int bit_depth = 8;
#endif

    const unsigned int y_full = ~((~0) << bit_depth);
    const unsigned int lshift = bit_depth - 8;

    const unsigned int y_min = narrow_range ? (16 << lshift) : 0;
    const unsigned int y_rng = narrow_range ? (219 << lshift) : y_full;

    const double y_offset = double(y_min) / double(y_full);
    const double sy = double(y_rng) / double(y_full);
    const double a = y_offset;

    if (yae::is_ycbcr(*desc))
    {
      const unsigned int c_rng = narrow_range ? (224 << lshift) : y_full;
      const double sc = double(c_rng) / double(y_full);
      const double b = 0.5 * sc + y_offset;

      /*
        double Y' = Y * scale_luma + y_offset;
        double Cb = (Cb + 0.5) * scale_chroma + y_offset;
        double Cr = (Cr + 0.5) * scale_chroma + y_offset;
      */

      // affine transform from Y'CbCr to Y'PbPr:
      ycbcr_to_ypbpr = make_m4x4(sy,  0.0, 0.0, a,
                                 0.0, sc,  0.0, b,
                                 0.0, 0.0, sc,  b,
                                 0.0, 0.0, 0.0, 1.0);
    }
    else if (narrow_range &&
             (flag_rgb ||
              (desc->nb_components == 2 && flag_alpha) ||
              (desc->nb_components == 1)))
    {
      // NOTE: the input is not actually Y'PbPr, and the output won't be Y'CbCr
      //       so treat "chroma" the same as "luma"

      // affine transform from narrow range to full range:
      ycbcr_to_ypbpr = make_m4x4(sy,  0.0, 0.0, a,
                                 0.0, sy,  0.0, a,
                                 0.0, 0.0, sy,  a,
                                 0.0, 0.0, 0.0, 1.0);
    }
    else
    {
      // nothing to do, use the identity matrix:
      static const m4x4_t identity = make_identity_m4x4();
      ycbcr_to_ypbpr = identity;
    }

    return true;
  }


  //----------------------------------------------------------------
  // ToneMapGamma::ToneMapGamma
  //
  ToneMapGamma::ToneMapGamma(double gamma):
    inv_gamma_(1.0 / gamma)
  {}

  //----------------------------------------------------------------
  // ToneMapGamma::apply
  //
  void
  ToneMapGamma::apply(const Colorspace::TransferFunc::Context & src_ctx,
                      const Colorspace::TransferFunc::Context & dst_ctx,
                      const double * src_rgb_cdm2,
                      double * dst_rgb_cdm2) const
  {
    // pick the brightest component, normalize to [0, 1] range:
    const double src =
      std::max(std::max(src_ctx.Lb_, src_rgb_cdm2[0]),
               std::max(src_rgb_cdm2[1], src_rgb_cdm2[2])) /
      src_ctx.Lw_;

    // threshold for linear portion of the curve:
    static const double threshold = 0.05;

    const double out =
      (src > threshold) ? std::pow(src, inv_gamma_) :
      (src / threshold) * std::pow(threshold, inv_gamma_);

    const double rescale =
      (src == 0.0) ? 0.0 :
      (out / src) * (dst_ctx.Lw_ / src_ctx.Lw_);

    dst_rgb_cdm2[0] = src_rgb_cdm2[0] * rescale;
    dst_rgb_cdm2[1] = src_rgb_cdm2[1] * rescale;
    dst_rgb_cdm2[2] = src_rgb_cdm2[2] * rescale;
  }


  //----------------------------------------------------------------
  // ToneMapPiecewise::apply
  //
  void
  ToneMapPiecewise::apply(const Colorspace::TransferFunc::Context & src_ctx,
                          const Colorspace::TransferFunc::Context & dst_ctx,
                          const double * src_rgb_cdm2,
                          double * dst_rgb_cdm2) const
  {
    // pick the brightest component:
    const double src = std::max(std::max(src_rgb_cdm2[0], src_rgb_cdm2[1]),
                                src_rgb_cdm2[2]) / src_ctx.Lw_;

    const double peak_ratio = src_ctx.Lw_ / dst_ctx.Lw_;
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
      (out / src) * (dst_ctx.Lw_ / src_ctx.Lw_);

    dst_rgb_cdm2[0] = src_rgb_cdm2[0] * rescale;
    dst_rgb_cdm2[1] = src_rgb_cdm2[1] * rescale;
    dst_rgb_cdm2[2] = src_rgb_cdm2[2] * rescale;
  }


  //----------------------------------------------------------------
  // ColorTransform::ColorTransform
  //
  ColorTransform::ColorTransform(unsigned int log2_edge):
    log2_edge_(log2_edge),
    size_3d_(1ull << (log2_edge * 3)),
    size_2d_(1ull << (log2_edge * 2)),
    size_1d_(1ull << (log2_edge)),
    granularity_(1.0 / size_1d_),
    z1_(size_1d_ - 1)
  {
    YAE_ASSERT(log2_edge < 11);
    cube_.resize(size_3d_);
  }

  //----------------------------------------------------------------
  // ColorTransform::fill
  //
  void
  ColorTransform::fill(const Colorspace & src_csp,
                       const Colorspace & dst_csp,

                       const Colorspace::TransferFunc::Context & src_ctx,
                       const Colorspace::TransferFunc::Context & dst_ctx,

                       // pre-transform, maps from source full/narrow
                       // range normalized [0, 1] pixel values to Y'PbPr
                       // where Y is [0, 1] and Pb,Pr are [-0.5, 0.5]
                       const m4x4_t & src_ycbcr_to_ypbpr,

                       // post-transform, maps from output Y'PbPr to
                       // normalized full/narrow [0, 1] range:
                       const m4x4_t & dst_ypbpr_to_ycbcr,

                       // optional, for HDR -> SDR conversion:
                       const ToneMap * tone_map)
  {
    // shortcuts:
    const bool is_src_rgb = src_csp.av_csp_ == AVCOL_SPC_RGB;
    const bool is_dst_rgb = dst_csp.av_csp_ == AVCOL_SPC_RGB;
    const m4x4_t to_rgb = m4x4_t(src_csp.ypbpr_to_rgb_);
    const m4x4_t to_ypbpr = m4x4_t(dst_csp.rgb_to_ypbpr_);
    const m4x4_t src_rgb_to_xyz = m4x4_t(src_csp.rgb_to_xyz_);
    const m4x4_t dst_xyz_to_rgb = m4x4_t(dst_csp.xyz_to_rgb_);
    const v3x1_t src_w_xyz = xyY_to_XYZ(src_csp.w_);
    const v3x1_t dst_w_xyz = xyY_to_XYZ(dst_csp.w_);

    const m4x4_t xyz_src_to_dst =
      (src_csp.w_ == dst_csp.w_) ? make_identity_m4x4() :
      m4x4_t(get_xyz_to_xyz(src_w_xyz, dst_w_xyz));

    const m4x4_t rgb_to_xyz_to_rgb =
      (src_csp.r_ == dst_csp.r_ &&
       src_csp.g_ == dst_csp.g_ &&
       src_csp.b_ == dst_csp.b_) ? make_identity_m4x4() :
      (dst_xyz_to_rgb * xyz_src_to_dst * src_rgb_to_xyz);

    // temporaries:
    v4x1_t input = make_v4x1(0, 0, 0, 1);
    v4x1_t ypbpr = make_v4x1(0, 0, 0, 1);
    v4x1_t rgb = make_v4x1(0, 0, 0, 1);
    v4x1_t rgb_cdm2 = make_v4x1(0, 0, 0, 1);
    v4x1_t output = make_v4x1(0, 0, 0, 1);

    // shortcuts:
    const double rescale = double(size_1d_ - 1);
    double * src = input.begin();
    Pixel * cube = &cube_[0];

    for (std::size_t i = 0; i < size_1d_; i++)
    {
      Pixel * plane = cube + i * size_2d_;
      src[0] = double(i) / rescale;

      for (std::size_t j = 0; j < size_1d_; j++)
      {
        Pixel * line = plane + j * size_1d_;
        src[1] = double(j) / rescale;

        for (std::size_t k = 0; k < size_1d_; k++)
        {
          float * pixel = (line + k)->data_;
          src[2] = double(k) / rescale;

          // transform to Y'PbPr (or full-range R'G'B')
          ypbpr = src_ycbcr_to_ypbpr * input;
#if 0
          if (!is_src_rgb)
          {
            // clip out-of-range values:
            ypbpr[0] = clip(ypbpr[0],  0.0, 1.0);
            ypbpr[1] = clip(ypbpr[1], -0.5, 0.5);
            ypbpr[2] = clip(ypbpr[2], -0.5, 0.5);
          }
#endif

          // transform to input non-linear R'G'B':
          rgb = is_src_rgb ? ypbpr : (to_rgb * ypbpr);
#if 1
          // NOTE: ST 2084 EOTF expects input in the [0, 1] range,
          // but xvYCC (wide gammut) supports negative RGB values...
          //
          // Therefore, it is up to the TransferFunc to do input parameter
          // sanitization (as in rgb = clip(rgb, 0.0, 1.0))

          // transform to linear RGB:
          src_csp.transfer_.eotf_rgb(src_csp,
                                     src_ctx,
                                     rgb.begin(),
                                     rgb_cdm2.begin());

          if (tone_map)
          {
            tone_map->apply(src_ctx,
                            dst_ctx,
                            rgb_cdm2.begin(),
                            rgb_cdm2.begin());
          }

#ifndef NDEBUG
          // check for NaN:
          YAE_ASSERT(rgb_cdm2[0] == rgb_cdm2[0]);
          YAE_ASSERT(rgb_cdm2[1] == rgb_cdm2[1]);
          YAE_ASSERT(rgb_cdm2[2] == rgb_cdm2[2]);
#endif

          // transform RGB to XYZ to RGB:
          rgb_cdm2 = rgb_to_xyz_to_rgb * rgb_cdm2;

#ifndef NDEBUG
          // check for NaN:
          YAE_ASSERT(rgb_cdm2[3] == rgb_cdm2[3]);
#endif

          // transform to output non-linear R'G'B':
          dst_csp.transfer_.oetf_rgb(dst_csp,
                                     dst_ctx,
                                     rgb_cdm2.begin(),
                                     rgb.begin());
#endif
          ypbpr = is_dst_rgb ? rgb : (to_ypbpr * rgb);

          // transform to Y'CbCr (or intended range R'G'B')
          output = dst_ypbpr_to_ycbcr * ypbpr;

          // clamp to [0, 1] output range:
          output = clip(output, 0.0, 1.0);
#if 1
          // memoize the output value:
          pixel[0] = float(output[0]);
          pixel[1] = float(output[1]);
          pixel[2] = float(output[2]);
#else
          pixel[0] = float(src[0]);
          pixel[1] = float(src[1]);
          pixel[2] = float(src[2]);
#endif
        }
      }
    }
  }

}

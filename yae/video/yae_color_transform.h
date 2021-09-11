// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Sep  9 12:17:26 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_COLOR_TRANSFORM_H_
#define YAE_COLOR_TRANSFORM_H_

// standard:
#include <vector>

// ffmpeg includes:
extern "C"
{
#include <libavutil/pixfmt.h>
}

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_linear_algebra.h"
#include "yae/video/yae_colorspace.h"


namespace yae
{

  //----------------------------------------------------------------
  // get_ycbcr_to_ypbpr
  //
  // setup an affine transform to from full/narrow
  // range normalized [0, 1] pixel values to Y'PbPr
  // where Y is [0, 1] and Pb,Pr are [-0.5, 0.5]
  //
  YAE_API bool get_ycbcr_to_ypbpr(m4x4_t & ycbcr_to_ypbpr,
                                  AVPixelFormat av_fmt,
                                  AVColorRange av_rng);

  //----------------------------------------------------------------
  // get_ypbpr_to_ycbcr
  //
  // setup an affine transform to map from Y'PbPr
  // to [0, 1] normalized full/narrow range:
  //
  YAE_API bool get_ypbpr_to_ycbcr(m4x4_t & ypbpr_to_ycbcr,
                                  AVPixelFormat av_fmt,
                                  AVColorRange av_rng);


  //----------------------------------------------------------------
  // ToneMap
  //
  struct YAE_API ToneMap
  {
    virtual ~ToneMap() {}
    virtual void apply(const Colorspace::TransferFunc::Context & src_ctx,
                       const Colorspace::TransferFunc::Context & dst_ctx,
                       const double * src_rgb_cdm2,
                       double * dst_rgb_cdm2) const = 0;
  };

  //----------------------------------------------------------------
  // ToneMapGamma
  //
  struct YAE_API ToneMapGamma : ToneMap
  {
    ToneMapGamma(double gamma = 1.8);

    // virtual:
    void apply(const Colorspace::TransferFunc::Context & src_ctx,
               const Colorspace::TransferFunc::Context & dst_ctx,
               const double * src_rgb_cdm2,
               double * dst_rgb_cdm2) const;

    // 1.0/gamma
    const double inv_gamma_;
  };


  //----------------------------------------------------------------
  // ToneMapPiecewise
  //
  struct YAE_API ToneMapPiecewise : ToneMap
  {
    // virtual:
    void apply(const Colorspace::TransferFunc::Context & src_ctx,
               const Colorspace::TransferFunc::Context & dst_ctx,
               const double * src_rgb_cdm2,
               double * dst_rgb_cdm2) const;
  };


  //----------------------------------------------------------------
  // ColorTransform
  //
  struct YAE_API ColorTransform
  {

    //----------------------------------------------------------------
    // Pixel
    //
    struct YAE_API Pixel
    {
      float data_[3];
    };

    //----------------------------------------------------------------
    // ColorTransform
    //
    ColorTransform(// log base 2 of 3D LUT cube edge size,
                   // default is 6, for 64 x 64 x 64 cube:
                   unsigned int log2_edge = 6);

    void fill(const Colorspace & src_csp,
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
              const ToneMap * tone_map = NULL);

    // accessors:
    inline const Pixel * get_data() const
    { return &cube_[0]; }

    inline const Pixel & at(std::size_t offset) const
    { return cube_.at(offset); }

    const unsigned int log2_edge_;
    const std::size_t size_3d_;
    const std::size_t size_2d_;
    const std::size_t size_1d_;

  protected:
    std::vector<Pixel> cube_;
  };

  //----------------------------------------------------------------
  // TColorTransformPtr
  //
  typedef yae::shared_ptr<ColorTransform> TColorTransformPtr;

}


#endif // YAE_COLOR_TRANSFORM_H_

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
      operator v3x1_t() const
      { return make_v3x1(data_[0], data_[1], data_[2]); }

      operator v4x1_t() const
      { return make_v4x1(data_[0], data_[1], data_[2], 1.0); }

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

    inline const Pixel & at(std::size_t i,
                            std::size_t j,
                            std::size_t k) const
    {
      std::size_t offset = k + size_1d_ * (j + size_1d_ * i);
      return cube_.at(offset);
    }

    inline const Pixel & get_nn(double u,
                                double v,
                                double w) const
    {
      std::size_t i = std::size_t(u * z1_);
      std::size_t j = std::size_t(v * z1_);
      std::size_t k = std::size_t(w * z1_);
      return this->at(i, j, k);
    }

    inline const v3x1_t get(double u,
                            double v,
                            double w) const
    {
      u *= z1_;
      v *= z1_;
      w *= z1_;

      std::size_t i0 = std::size_t(u);
      std::size_t j0 = std::size_t(v);
      std::size_t k0 = std::size_t(w);
      std::size_t i1 = std::min(i0 + 1, size_1d_ - 1);
      std::size_t j1 = std::min(j0 + 1, size_1d_ - 1);
      std::size_t k1 = std::min(k0 + 1, size_1d_ - 1);

      double u1 = u - i0;
      double v1 = v - j0;
      double w1 = w - k0;

      double u0 = 1.0 - u1;
      double v0 = 1.0 - v1;
      double w0 = 1.0 - w1;

      v3x1_t p000 = this->at(i0, j0, k0);
      v3x1_t p001 = this->at(i0, j0, k1);
      v3x1_t p010 = this->at(i0, j1, k0);
      v3x1_t p011 = this->at(i0, j1, k1);
      v3x1_t p100 = this->at(i1, j0, k0);
      v3x1_t p101 = this->at(i1, j0, k1);
      v3x1_t p110 = this->at(i1, j1, k0);
      v3x1_t p111 = this->at(i1, j1, k1);

      v3x1_t p00 = p000 * u0 + p100 * u1;
      v3x1_t p01 = p001 * u0 + p101 * u1;
      v3x1_t p10 = p010 * u0 + p110 * u1;
      v3x1_t p11 = p011 * u0 + p111 * u1;

      v3x1_t p0 = p00 * v0 + p10 * v1;
      v3x1_t p1 = p01 * v0 + p11 * v1;

      v3x1_t p = p0 * w0 + p1 * w1;
      return p;
    }

    const unsigned int log2_edge_;
    const std::size_t size_3d_;
    const std::size_t size_2d_;
    const std::size_t size_1d_;
    const double granularity_; // 1 / size_1d_
    const double z1_; // size_1d_ - 1

  protected:
    std::vector<Pixel> cube_;
  };

  //----------------------------------------------------------------
  // TColorTransformPtr
  //
  typedef yae::shared_ptr<ColorTransform> TColorTransformPtr;

}


#endif // YAE_COLOR_TRANSFORM_H_

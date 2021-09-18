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
#include "yae/api/yae_api.h"
#include "yae/api/yae_assert.h"
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
  // Pixel
  //
  template <typename DataType = float, unsigned int MaxPixelValue = 1>
  struct Pixel
  {
    typedef Pixel<DataType, MaxPixelValue> TSelf;
    typedef DataType TData;

    enum { Max = MaxPixelValue };

    operator v3x1_t() const
    { return make_v3x1(data_[0], data_[1], data_[2]); }

    operator v4x1_t() const
    { return make_v4x1(data_[0], data_[1], data_[2], 1.0); }

    DataType data_[3];
  };

  //----------------------------------------------------------------
  // TPixel3f32
  //
  typedef Pixel<float, 1> TPixel3f32;

  //----------------------------------------------------------------
  // TPixel3u8
  //
  typedef Pixel<uint8_t, 255> TPixel3u8;

  //----------------------------------------------------------------
  // ColorTransform
  //
  template <typename PixelType = TPixel3f32>
  struct ColorTransform
  {
    typedef ColorTransform<PixelType> TSelf;
    typedef typename PixelType::TData TData;
    typedef PixelType TPixel;

    enum { MaxPixelValue = PixelType::Max };

    //----------------------------------------------------------------
    // ColorTransform
    //
    ColorTransform(// log base 2 of 3D LUT cube edge size,
                   // default is 6, for 64 x 64 x 64 cube:
                   unsigned int log2_edge = 6):
      log2_edge_(log2_edge),
      size_3d_(1ull << (log2_edge * 3)),
      size_2d_(1ull << (log2_edge * 2)),
      size_1d_(1ull << (log2_edge)),
      z1_(size_1d_ - 1),
      z2_(size_1d_ - 2),
      dz_(1.0 / z2_),
      zs_(z2_ / z1_)
    {
      YAE_ASSERT(log2_edge > 1);
      YAE_ASSERT(log2_edge < 11);
      cube_.resize(size_3d_);
    }

    //----------------------------------------------------------------
    // fill
    //
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
              const ToneMap * tone_map = NULL)
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
      const double rescale = double(size_1d_ - 2);
      double * src = input.begin();
      TPixel * cube = &cube_[0];

      for (std::size_t i = 0; i < size_1d_; i++)
      {
        TPixel * plane = cube + i * size_2d_;
        src[0] = double(i) / rescale;

        for (std::size_t j = 0; j < size_1d_; j++)
        {
          TPixel * line = plane + j * size_1d_;
          src[1] = double(j) / rescale;

          for (std::size_t k = 0; k < size_1d_; k++)
          {
            TData * pixel = (line + k)->data_;
            src[2] = double(k) / rescale;

            // transform to Y'PbPr (or full-range R'G'B')
            ypbpr = src_ycbcr_to_ypbpr * input;

            // transform to input non-linear R'G'B':
            rgb = is_src_rgb ? ypbpr : (to_rgb * ypbpr);

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

            ypbpr = is_dst_rgb ? rgb : (to_ypbpr * rgb);

            // transform to Y'CbCr (or intended range R'G'B')
            output = dst_ypbpr_to_ycbcr * ypbpr;

            // clamp to [0, 1] output range:
            output = clip(output, 0.0, 1.0);
#if 1
            // memoize the output value:
            pixel[0] = TData(output[0] * MaxPixelValue);
            pixel[1] = TData(output[1] * MaxPixelValue);
            pixel[2] = TData(output[2] * MaxPixelValue);
#else
            // identity transform, for debugging:
            pixel[0] = TData(src[0] * MaxPixelValue);
            pixel[1] = TData(src[1] * MaxPixelValue);
            pixel[2] = TData(src[2] * MaxPixelValue);
#endif
          }
        }
      }
    }

    // accessors:
    inline const TPixel * get_data() const
    { return &cube_[0]; }

    inline const TPixel & at(std::size_t offset) const
    { return cube_.at(offset); }

    inline const TPixel & at(std::size_t i,
                             std::size_t j,
                             std::size_t k) const
    {
      std::size_t offset = k + size_1d_ * (j + size_1d_ * i);
      return cube_.at(offset);
    }

    inline const TPixel & get_nn(double u,
                                 double v,
                                 double w) const
    {
      std::size_t i = std::size_t(u * z2_);
      std::size_t j = std::size_t(v * z2_);
      std::size_t k = std::size_t(w * z2_);
      return this->at(i, j, k);
    }

    inline const v3x1_t get(double u,
                            double v,
                            double w) const
    {
      u *= z2_;
      v *= z2_;
      w *= z2_;

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

    // The [0, 1] sampling of the input domain maps to [0, z2]
    // cube coordinates -- z1 coordinate maps outside the [0, 1]
    // input domain.  This is done in order to exactly represent
    // samples with 0.5 coordinates because they have special
    // significance in Y'CbCr for representing pure black and
    // pure white precisely.
    //
    // The cube actually represents a sampling of the
    // [0, 1 + dz] x [0, 1 + dz] x [0, 1 + dz] input space.
    //
    // Direct access to the cube data should be adjusted accordingly.
    // This is mainly of interest to OpenGL fragment shaders as they'll
    // need to scale the texture coordinates by 1 / (1 + dz)
    // in order to access the intended region of the cube (3D texture).
    //
    // This also constrains the minimum size of the cube to 4x4x4,
    // which corresponds to log2_edge = 2
    //
    const double z1_; // size_1d - 1
    const double z2_; // size_1d - 2
    const double dz_; // 1 / z2
    const double zs_; // z2 / z1

  protected:
    std::vector<TPixel> cube_;
  };

  //----------------------------------------------------------------
  // TColorTransform3f32
  //
  typedef ColorTransform<TPixel3f32> TColorTransform3f32;

  //----------------------------------------------------------------
  // TColorTransform3u8
  //
  typedef ColorTransform<TPixel3u8> TColorTransform3u8;

}


#endif // YAE_COLOR_TRANSFORM_H_

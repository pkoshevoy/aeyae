// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 21 12:37:47 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_COLORSPACE_H_
#define YAE_COLORSPACE_H_

// standard:
#include <string>

// ffmpeg:
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_linear_algebra.h"


namespace yae
{

  //----------------------------------------------------------------
  // is_nv12
  //
  inline bool is_nv12(const AVPixFmtDescriptor & desc)
  {
    return (desc.log2_chroma_w == 1 &&
            desc.log2_chroma_h == 1 &&
            desc.comp[0].plane == 0 &&
            desc.comp[1].plane == 1 &&
            desc.comp[2].plane == 1);
  }

  //----------------------------------------------------------------
  // is_yuv420p
  //
  inline bool is_yuv420p(const AVPixFmtDescriptor & desc)
  {
    return (desc.log2_chroma_w == 1 &&
            desc.log2_chroma_h == 1 &&
            desc.comp[0].plane == 0 &&
            desc.comp[1].plane == 1 &&
            desc.comp[2].plane == 2);
  }

  //----------------------------------------------------------------
  // is_yuv422p
  //
  inline bool is_yuv422p(const AVPixFmtDescriptor & desc)
  {
    return (desc.log2_chroma_w == 1 &&
            desc.log2_chroma_h == 0 &&
            desc.comp[0].plane == 0 &&
            desc.comp[1].plane == 1 &&
            desc.comp[2].plane == 2);
  }

  //----------------------------------------------------------------
  // is_yuv444p
  //
  inline bool is_yuv444p(const AVPixFmtDescriptor & desc)
  {
    return (desc.log2_chroma_w == 0 &&
            desc.log2_chroma_h == 0 &&
            desc.comp[0].plane == 0 &&
            desc.comp[1].plane == 1 &&
            desc.comp[2].plane == 2);
  }

  //----------------------------------------------------------------
  // is_ycbcr
  //
  inline bool is_ycbcr(const AVPixFmtDescriptor & desc)
  {
    return ((desc.flags & AV_PIX_FMT_FLAG_RGB) == 0 &&
            desc.nb_components > 2);
  }

  //----------------------------------------------------------------
  // is_rgb
  //
  inline bool is_rgb(const AVPixFmtDescriptor & desc)
  {
    return ((desc.flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB &&
            desc.nb_components >= 3 &&
            desc.log2_chroma_w == 0 &&
            desc.log2_chroma_h == 0);
  }

  //----------------------------------------------------------------
  // has_alpha
  //
  inline bool has_alpha(const AVPixFmtDescriptor & desc)
  {
    return (desc.flags & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;
  }

  //----------------------------------------------------------------
  // xyY_to_XYZ
  //
  // convert from CIE xyY to CEI XYZ:
  //
  inline v3x1_t
  xyY_to_XYZ(double x, double y, double Y = 1.0)
  {
    if (y == 0.0)
    {
      return make_v3x1(0.3333333, 0.3333333, 0);
    }

    double Yy = Y / y;
    double X = Yy * x;
    double Z = Yy * (1 - x - y);
    return make_v3x1(X, Y, Z);
  }

  //----------------------------------------------------------------
  // XYZ_to_xyY
  //
  // convert from CIE XYZ to CEI xyY:
  //
  inline v3x1_t
  XYZ_to_xyY(double X, double Y, double Z)
  {
    double XYZ = X + Y + Z;
    if (XYZ == 0.0)
    {
      // black, chromaticity may not matter
      // but ideally should be set to the
      // chromaticity coordinates of your reference white.
      return make_v3x1(0.3333333, 0.3333333, 0);
    }

    double x = X / XYZ;
    double y = Y / XYZ;
    return make_v3x1(x, y, Y);
  }


  //----------------------------------------------------------------
  // xyY_to_XYZ
  //
  inline v3x1_t
  xyY_to_XYZ(const v3x1_t & xyY)
  {
    return xyY_to_XYZ(xyY[0], xyY[1], xyY[2]);
  }

  //----------------------------------------------------------------
  // XYZ_to_xyY
  //
  inline v3x1_t
  XYZ_to_xyY(const v3x1_t & xyz)
  {
    return XYZ_to_xyY(xyz[0], xyz[1], xyz[2]);
  }

  //----------------------------------------------------------------
  // get_xyz_to_xyz
  //
  // Chromatic Adaptation
  //
  // http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
  //
  YAE_API m3x3_t
  get_xyz_to_xyz(// CIE XYZ source reference white point:
                 const v3x1_t & w_src_xyz,
                 // CIE XYZ destination reference white point:
                 const v3x1_t & w_dst_xyz);

  //----------------------------------------------------------------
  // get_ypbpr_to_rgb
  //
  // https://en.wikipedia.org/wiki/YCbCr
  //
  // input domain, output range:
  //
  //  Y': [ 0.0, 1.0]
  //  Pb: [-0.5, 0.5]
  //  Pr: [-0.5, 0.5]
  //
  //  R': [ 0.0, 1.0]
  //  G': [ 0.0, 1.0]
  //  B': [ 0.0, 1.0]
  //
  inline m3x3_t
  get_ypbpr_to_rgb(// luma coefficients:
                   double kr,
                   double kg,
                   double kb)
  {
    return make_m3x3(1, 0, 2 - 2 * kr,
                     1, (kb / kg) * (2 * kb - 2), (kr / kg) * (2 * kr - 2),
                     1, 2 - 2 * kb, 0);
  }

  //----------------------------------------------------------------
  // get_rgb_to_ypbpr
  //
  // https://en.wikipedia.org/wiki/YCbCr
  //
  // input domain, output range:
  //
  //  Y': [ 0.0, 1.0]
  //  Pb: [-0.5, 0.5]
  //  Pr: [-0.5, 0.5]
  //
  //  R': [ 0.0, 1.0]
  //  G': [ 0.0, 1.0]
  //  B': [ 0.0, 1.0]
  //
  inline m3x3_t
  get_rgb_to_ypbpr(// luma coefficients:
                   double kr,
                   double kg,
                   double kb)
  {
    return make_m3x3(kr, kg, kb,
                     0.5 * kr / (kb - 1), 0.5 * kg / (kb - 1), 0.5,
                     0.5, 0.5 * kg / (kr - 1), 0.5 * kb / (kr - 1));
  }


  //----------------------------------------------------------------
  // get_hlg_gamma
  //
  YAE_API double get_hlg_gamma(double Lw);

  //----------------------------------------------------------------
  // get_hlg_beta
  //
  YAE_API double get_hlg_beta(double Lw, double Lb, double gamma);


  //----------------------------------------------------------------
  // get_ycbcr_to_ypbpr
  //
  // setup an affine transform to from full/narrow
  // range normalized [0, 1] pixel values to Y'PbPr
  // where Y is [0, 1] and Pb,Pr are [-0.5, 0.5]
  //
  YAE_API m4x4_t
  get_ycbcr_to_ypbpr(const AVPixFmtDescriptor * desc,
                     AVColorRange av_rng);

  //----------------------------------------------------------------
  // get_ypbpr_to_ycbcr
  //
  // setup an affine transform to map from Y'PbPr
  // to [0, 1] normalized full/narrow range:
  //
  YAE_API m4x4_t
  get_ypbpr_to_ycbcr(const AVPixFmtDescriptor * desc,
                     AVColorRange av_rng);

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
  // Colorspace
  //
  // linuxtv.org/downloads/v4l-dvb-apis/userspace-api/v4l/colorspaces.html
  //
  // The colorspace definition itself consists of
  // - the three chromaticity primaries
  // - the white reference chromaticity
  // - a transfer function
  // - the luma coefficients needed to transform R’G’B’ to Y’CbCr
  //
  // See SMPTE RP 177-1993 for derivation of luma coefficients
  //
  struct YAE_API Colorspace
  {
    // helper:
    static const Colorspace * get(AVColorSpace av_csp,
                                  AVColorPrimaries av_pri,
                                  AVColorTransferCharacteristic av_trc);

    //----------------------------------------------------------------
    // Format
    //
    struct YAE_API Format
    {
      Format(AVPixelFormat av_fmt, AVColorRange av_rng);

      inline bool is_narrow_range() const
      { return (av_rng_ == AVCOL_RANGE_MPEG); }

      inline bool is_rgb() const
      { return desc_ ? yae::is_rgb(*desc_) : false; }

      inline bool is_ycbcr() const
      { return desc_ ? yae::is_ycbcr(*desc_) : false; }

      inline bool has_alpha() const
      { return desc_ ? yae::has_alpha(*desc_) : false; }

      const AVPixFmtDescriptor * desc_;
      const AVPixelFormat av_fmt_;
      const AVColorRange av_rng_;

      // pre-transform, maps from source full/narrow range
      // normalized [0, 1] pixel values to Y'PbPr (or R'G'B')
      // where Y is [0, 1] and Pb,Pr are [-0.5, 0.5]:
      const m4x4_t native_to_full_;

      // inverse of the above:
      const m4x4_t full_to_native_;
    };

    //----------------------------------------------------------------
    // DynamicRange
    //
    struct YAE_API DynamicRange
    {
      DynamicRange(double cdm2_nominal_peak_luminance_of_the_display = 100.0,
                   double cdm2_display_luminance_for_black = 1e-6,
                   double max_fall = 33.0,
                   double max_cll = 100.0):
        Lw_(cdm2_nominal_peak_luminance_of_the_display),
        Lb_(cdm2_display_luminance_for_black),
        gamma_(yae::get_hlg_gamma(Lw_)),
        beta_(yae::get_hlg_beta(Lw_, Lb_, gamma_)),
        max_fall_(max_fall),
        max_cll_(max_cll)
      {}

      inline bool operator == (const DynamicRange & r) const
      {
        return (Lw_ == r.Lw_ &&
                Lb_ == r.Lb_ &&
                max_fall_ == r.max_fall_ &&
                max_cll_ == r.max_cll_);
      }

      // nominal peak luminance of the scene/display in cd/m2:
      double Lw_;

      // nominal black luminance of the scene/display in cd/m2:
      double Lb_;

      // precomputed constants for HLG EOTF, OETF...
      // see Rec. ITU-R BT.2100-2
      double gamma_;
      double beta_;

      // Maximum Frame Average Light Level, indicates the maximum value
      // of the frame average light level of the entire playback sequence:
      double max_fall_; // cd/m2

      // Maximum Content Light Level, indicates the maximum light level
      // of any single pixel of the entire playback sequence:
      double max_cll_; // cd/m2
    };

    //----------------------------------------------------------------
    // TransferFunc
    //
    struct YAE_API TransferFunc
    {
      virtual ~TransferFunc() {}

      // linear cd/m2 RGB components to non-linear encoded R'G'B'.
      // default implementation rescales and delegates to normalized oetf:
      virtual void oetf_rgb(const Colorspace & csp,
                            const DynamicRange & ctx,
                            const double * rgb_cdm2,
                            double * rgb) const
      {
        (void) csp;
        rgb[0] = this->oetf(rgb_cdm2[0] / ctx.Lw_);
        rgb[1] = this->oetf(rgb_cdm2[1] / ctx.Lw_);
        rgb[2] = this->oetf(rgb_cdm2[2] / ctx.Lw_);
      }

      // non-linear encoded R'G'B' to linear cd/m2 RGB components.
      // default implementation delegates to normalized eotf and rescales:
      virtual void eotf_rgb(const Colorspace & csp,
                            const DynamicRange & ctx,
                            const double * rgb,
                            double * rgb_cdm2) const
      {
        (void) csp;
        rgb_cdm2[0] = this->eotf(rgb[0]) * ctx.Lw_;
        rgb_cdm2[1] = this->eotf(rgb[1]) * ctx.Lw_;
        rgb_cdm2[2] = this->eotf(rgb[2]) * ctx.Lw_;
      }

      // normalized [0, 1] linear RGB components to non-linear encoded R'G'B'.
      // default implementation is linear ... output same as input:
      virtual double oetf(double L) const
      { return L; }

      // non-linear encoded R'G'B' to normalized [0, 1] linear RGB components.
      // default implementation is linear ... output same as input:
      virtual double eotf(double V) const
      { return V; }
    };

    // NOTE: this will derive the luma coefficients (kr, kg, kb)
    // from the Y row of the RGB to XYZ matrix:
    Colorspace(const char * name,

               // corresponding ffmpeg color specs, if known:
               AVColorSpace av_csp,
               AVColorPrimaries av_pri,
               AVColorTransferCharacteristic av_trc,

               // xyY coordinates of the primaries:
               double rx, double ry,
               double gx, double gy,
               double bx, double by,

               // xyY coordinates of the reference white point:
               double wx, double wy,

               const TransferFunc & transfer);

    // override the derived luma coefficients if necessary
    // for some color spaces (eg BT.601)
    //
    // NOTE: kr + kg + kb = 1.0
    void set_luma_coefficients(double kr, double kg, double kb);

    const std::string name_;

    // corresponding ffmpeg color specs, if known:
    const AVColorSpace av_csp_;
    const AVColorPrimaries av_pri_;
    const AVColorTransferCharacteristic av_trc_;

    // xyY coordinates of the primaries:
    const v3x1_t r_;
    const v3x1_t g_;
    const v3x1_t b_;

    // xyY coordinates of the reference white point:
    const v3x1_t w_;

    // linear RGB transforms:
    m3x3_t rgb_to_xyz_;
    m3x3_t xyz_to_rgb_;

    m3x3_t rgb_to_ypbpr_;
    m3x3_t ypbpr_to_rgb_;

    // Both the CIE XYZ and the RGB colorspace that are derived from
    // the specific chromaticity primaries are linear colorspaces.
    // But neither the eye, nor display technology is linear.
    // Doubling the values of all components in the linear colorspace
    // will not be perceived as twice the intensity of the color.
    //
    // So each colorspace also defines a transfer function that
    // takes a linear color component value and transforms it
    // to the non-linear component value, which is a closer match
    // to the non-linear performance of both the eye and displays.
    //
    // Linear component values are denoted RGB,
    // non-linear are denoted as R’G’B’.
    //
    // In general colors used in graphics are all R’G’B’,
    // except in openGL which uses linear RGB.
    const TransferFunc & transfer_;

    // the final piece that defines a colorspace is a function that
    // transforms non-linear R’G’B’ to non-linear Y’CbCr.
    // This function is determined by the so-called luma coefficients:
    double kr_;
    double kg_;
    double kb_;
  };

  //----------------------------------------------------------------
  // get_transfer_func
  //
  YAE_API const Colorspace::TransferFunc &
  get_transfer_func(AVColorTransferCharacteristic av_trc);

}


#endif // YAE_COLORSPACE_H_

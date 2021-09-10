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
}

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_linear_algebra.h"


namespace yae
{

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
    // TransferFunc
    //
    struct YAE_API TransferFunc
    {
      virtual ~TransferFunc() {}

      // linear RGB components to non-linear encoded R'G'B':
      virtual double oetf(double L) const = 0;

      // non-linear encoded R'G'B' to linear cd/m2 RGB components:
      virtual double eotf(double V) const = 0;
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
  const Colorspace::TransferFunc &
  get_transfer_func(AVColorTransferCharacteristic av_trc);

}


#endif // YAE_COLORSPACE_H_

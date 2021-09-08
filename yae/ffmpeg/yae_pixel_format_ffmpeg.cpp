// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 20 16:44:22 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <cstring>

// ffmpeg includes:
extern "C"
{
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

// yae includes:
#include "yae_ffmpeg_utils.h"
#include "yae_pixel_format_ffmpeg.h"
#include "../utils/yae_linear_algebra.h"
#include "../video/yae_pixel_format_traits.h"
#include "../video/yae_video.h"


namespace yae
{
  //----------------------------------------------------------------
  // ffmpeg_to_yae
  //
  TPixelFormatId
  ffmpeg_to_yae(enum AVPixelFormat ffmpegPixelFormat)
  {
    switch (ffmpegPixelFormat)
    {
      case AV_PIX_FMT_YUV420P:
        return kPixelFormatYUV420P;

      case AV_PIX_FMT_YUYV422:
        return kPixelFormatYUYV422;

      case AV_PIX_FMT_RGB24:
        return kPixelFormatRGB24;

      case AV_PIX_FMT_BGR24:
        return kPixelFormatBGR24;

      case AV_PIX_FMT_YUV422P:
        return kPixelFormatYUV422P;

      case AV_PIX_FMT_YUV444P:
        return kPixelFormatYUV444P;

      case AV_PIX_FMT_YUV410P:
        return kPixelFormatYUV410P;

      case AV_PIX_FMT_YUV411P:
        return kPixelFormatYUV411P;

      case AV_PIX_FMT_GRAY8:
        return kPixelFormatGRAY8;

      case AV_PIX_FMT_MONOWHITE:
        return kPixelFormatMONOWHITE;

      case AV_PIX_FMT_MONOBLACK:
        return kPixelFormatMONOBLACK;

      case AV_PIX_FMT_PAL8:
        return kPixelFormatPAL8;

      case AV_PIX_FMT_UYVY422:
        return kPixelFormatUYVY422;

      case AV_PIX_FMT_UYYVYY411:
        return kPixelFormatUYYVYY411;

      case AV_PIX_FMT_BGR8:
        return kPixelFormatBGR8;

      case AV_PIX_FMT_BGR4:
        return kPixelFormatBGR4;

      case AV_PIX_FMT_BGR4_BYTE:
        return kPixelFormatBGR4_BYTE;

      case AV_PIX_FMT_RGB8:
        return kPixelFormatRGB8;

      case AV_PIX_FMT_RGB4:
        return kPixelFormatRGB4;

      case AV_PIX_FMT_RGB4_BYTE:
        return kPixelFormatRGB4_BYTE;

      case AV_PIX_FMT_NV12:
        return kPixelFormatNV12;

      case AV_PIX_FMT_NV21:
        return kPixelFormatNV21;

      case AV_PIX_FMT_ARGB:
        return kPixelFormatARGB;

      case AV_PIX_FMT_RGBA:
        return kPixelFormatRGBA;

      case AV_PIX_FMT_ABGR:
        return kPixelFormatABGR;

      case AV_PIX_FMT_BGRA:
        return kPixelFormatBGRA;

      case AV_PIX_FMT_GRAY16BE:
        return kPixelFormatGRAY16BE;

      case AV_PIX_FMT_GRAY16LE:
        return kPixelFormatGRAY16LE;

      case AV_PIX_FMT_YUV440P:
        return kPixelFormatYUV440P;

      case AV_PIX_FMT_YUVA420P:
        return kPixelFormatYUVA420P;

      case AV_PIX_FMT_RGB48BE:
        return kPixelFormatRGB48BE;

      case AV_PIX_FMT_RGB48LE:
        return kPixelFormatRGB48LE;

      case AV_PIX_FMT_RGB565BE:
        return kPixelFormatRGB565BE;

      case AV_PIX_FMT_RGB565LE:
        return kPixelFormatRGB565LE;

      case AV_PIX_FMT_RGB555BE:
        return kPixelFormatRGB555BE;

      case AV_PIX_FMT_RGB555LE:
        return kPixelFormatRGB555LE;

      case AV_PIX_FMT_BGR565BE:
        return kPixelFormatBGR565BE;

      case AV_PIX_FMT_BGR565LE:
        return kPixelFormatBGR565LE;

      case AV_PIX_FMT_BGR555BE:
        return kPixelFormatBGR555BE;

      case AV_PIX_FMT_BGR555LE:
        return kPixelFormatBGR555LE;

      case AV_PIX_FMT_YUV420P16LE:
        return kPixelFormatYUV420P16LE;

      case AV_PIX_FMT_YUV420P16BE:
        return kPixelFormatYUV420P16BE;

      case AV_PIX_FMT_YUV422P16LE:
        return kPixelFormatYUV422P16LE;

      case AV_PIX_FMT_YUV422P16BE:
        return kPixelFormatYUV422P16BE;

      case AV_PIX_FMT_YUV444P16LE:
        return kPixelFormatYUV444P16LE;

      case AV_PIX_FMT_YUV444P16BE:
        return kPixelFormatYUV444P16BE;

      case AV_PIX_FMT_RGB444BE:
        return kPixelFormatRGB444BE;

      case AV_PIX_FMT_RGB444LE:
        return kPixelFormatRGB444LE;

      case AV_PIX_FMT_BGR444BE:
        return kPixelFormatBGR444BE;

      case AV_PIX_FMT_BGR444LE:
        return kPixelFormatBGR444LE;

      case AV_PIX_FMT_Y400A:
        return kPixelFormatY400A;

      case AV_PIX_FMT_YUVJ420P:
        return kPixelFormatYUVJ420P;

      case AV_PIX_FMT_YUVJ422P:
        return kPixelFormatYUVJ422P;

      case AV_PIX_FMT_YUVJ444P:
        return kPixelFormatYUVJ444P;

      case AV_PIX_FMT_YUVJ440P:
        return kPixelFormatYUVJ440P;

      case AV_PIX_FMT_YUV420P9BE:
        return kPixelFormatYUV420P9BE;

      case AV_PIX_FMT_YUV420P9LE:
        return kPixelFormatYUV420P9LE;

      case AV_PIX_FMT_YUV422P9BE:
        return kPixelFormatYUV422P9BE;

      case AV_PIX_FMT_YUV422P9LE:
        return kPixelFormatYUV422P9LE;

      case AV_PIX_FMT_YUV444P9BE:
        return kPixelFormatYUV444P9BE;

      case AV_PIX_FMT_YUV444P9LE:
        return kPixelFormatYUV444P9LE;

      case AV_PIX_FMT_YUV420P10BE:
        return kPixelFormatYUV420P10BE;

      case AV_PIX_FMT_YUV420P10LE:
        return kPixelFormatYUV420P10LE;

      case AV_PIX_FMT_YUV422P10BE:
        return kPixelFormatYUV422P10BE;

      case AV_PIX_FMT_YUV422P10LE:
        return kPixelFormatYUV422P10LE;

      case AV_PIX_FMT_YUV444P10BE:
        return kPixelFormatYUV444P10BE;

      case AV_PIX_FMT_YUV444P10LE:
        return kPixelFormatYUV444P10LE;

      case AV_PIX_FMT_RGBA64BE:
        return kPixelFormatRGBA64BE;

      case AV_PIX_FMT_RGBA64LE:
        return kPixelFormatRGBA64LE;

      case AV_PIX_FMT_BGRA64BE:
        return kPixelFormatBGRA64BE;

      case AV_PIX_FMT_BGRA64LE:
        return kPixelFormatBGRA64LE;

      case AV_PIX_FMT_GBRP:
        return kPixelFormatGBRP;

      case AV_PIX_FMT_GBRP9BE:
        return kPixelFormatGBRP9BE;

      case AV_PIX_FMT_GBRP9LE:
        return kPixelFormatGBRP9LE;

      case AV_PIX_FMT_GBRP10BE:
        return kPixelFormatGBRP10BE;

      case AV_PIX_FMT_GBRP10LE:
        return kPixelFormatGBRP10LE;

      case AV_PIX_FMT_GBRP16BE:
        return kPixelFormatGBRP16BE;

      case AV_PIX_FMT_GBRP16LE:
        return kPixelFormatGBRP16LE;

      case AV_PIX_FMT_YUVA420P9BE:
        return kPixelFormatYUVA420P9BE;

      case AV_PIX_FMT_YUVA420P9LE:
        return kPixelFormatYUVA420P9LE;

      case AV_PIX_FMT_YUVA422P9BE:
        return kPixelFormatYUVA422P9BE;

      case AV_PIX_FMT_YUVA422P9LE:
        return kPixelFormatYUVA422P9LE;

      case AV_PIX_FMT_YUVA444P9BE:
        return kPixelFormatYUVA444P9BE;

      case AV_PIX_FMT_YUVA444P9LE:
        return kPixelFormatYUVA444P9LE;

      case AV_PIX_FMT_YUVA420P10BE:
        return kPixelFormatYUVA420P10BE;

      case AV_PIX_FMT_YUVA420P10LE:
        return kPixelFormatYUVA420P10LE;

      case AV_PIX_FMT_YUVA422P10BE:
        return kPixelFormatYUVA422P10BE;

      case AV_PIX_FMT_YUVA422P10LE:
        return kPixelFormatYUVA422P10LE;

      case AV_PIX_FMT_YUVA444P10BE:
        return kPixelFormatYUVA444P10BE;

      case AV_PIX_FMT_YUVA444P10LE:
        return kPixelFormatYUVA444P10LE;

      case AV_PIX_FMT_YUVA420P16BE:
        return kPixelFormatYUVA420P16BE;

      case AV_PIX_FMT_YUVA420P16LE:
        return kPixelFormatYUVA420P16LE;

      case AV_PIX_FMT_YUVA422P16BE:
        return kPixelFormatYUVA422P16BE;

      case AV_PIX_FMT_YUVA422P16LE:
        return kPixelFormatYUVA422P16LE;

      case AV_PIX_FMT_YUVA444P16BE:
        return kPixelFormatYUVA444P16BE;

      case AV_PIX_FMT_YUVA444P16LE:
        return kPixelFormatYUVA444P16LE;

      case AV_PIX_FMT_XYZ12BE:
        return kPixelFormatXYZ12BE;

      case AV_PIX_FMT_XYZ12LE:
        return kPixelFormatXYZ12LE;

      case AV_PIX_FMT_NV16:
        return kPixelFormatNV16;

      case AV_PIX_FMT_NV20BE:
        return kPixelFormatNV20BE;

      case AV_PIX_FMT_NV20LE:
        return kPixelFormatNV20LE;

      case AV_PIX_FMT_YVYU422:
        return kPixelFormatYVYU422;

      case AV_PIX_FMT_YA16BE:
        return kPixelFormatYA16BE;

      case AV_PIX_FMT_YA16LE:
        return kPixelFormatYA16LE;

#if LIBAVUTIL_VERSION_INT > AV_VERSION_INT(54, 3, 0)
      case AV_PIX_FMT_0RGB:
        return kPixelFormat0RGB;

      case AV_PIX_FMT_RGB0:
        return kPixelFormatRGB0;

      case AV_PIX_FMT_0BGR:
        return kPixelFormat0BGR;

      case AV_PIX_FMT_BGR0:
        return kPixelFormatBGR0;

      case AV_PIX_FMT_YUVA422P:
        return kPixelFormatYUVA422P;

      case AV_PIX_FMT_YUVA444P:
        return kPixelFormatYUVA444P;

      case AV_PIX_FMT_YUV420P12BE:
        return kPixelFormatYUV420P12BE;

      case AV_PIX_FMT_YUV420P12LE:
        return kPixelFormatYUV420P12LE;

      case AV_PIX_FMT_YUV420P14BE:
        return kPixelFormatYUV420P14BE;

      case AV_PIX_FMT_YUV420P14LE:
        return kPixelFormatYUV420P14LE;

      case AV_PIX_FMT_YUV422P12BE:
        return kPixelFormatYUV422P12BE;

      case AV_PIX_FMT_YUV422P12LE:
        return kPixelFormatYUV422P12LE;

      case AV_PIX_FMT_YUV422P14BE:
        return kPixelFormatYUV422P14BE;

      case AV_PIX_FMT_YUV422P14LE:
        return kPixelFormatYUV422P14LE;

      case AV_PIX_FMT_YUV444P12BE:
        return kPixelFormatYUV444P12BE;

      case AV_PIX_FMT_YUV444P12LE:
        return kPixelFormatYUV444P12LE;

      case AV_PIX_FMT_YUV444P14BE:
        return kPixelFormatYUV444P14BE;

      case AV_PIX_FMT_YUV444P14LE:
        return kPixelFormatYUV444P14LE;

      case AV_PIX_FMT_GBRP12BE:
        return kPixelFormatGBRP12BE;

      case AV_PIX_FMT_GBRP12LE:
        return kPixelFormatGBRP12LE;

      case AV_PIX_FMT_GBRP14BE:
        return kPixelFormatGBRP14BE;

      case AV_PIX_FMT_GBRP14LE:
        return kPixelFormatGBRP14LE;

      case AV_PIX_FMT_GBRAP:
        return kPixelFormatGBRAP;

      case AV_PIX_FMT_GBRAP16BE:
        return kPixelFormatGBRAP16BE;

      case AV_PIX_FMT_GBRAP16LE:
        return kPixelFormatGBRAP16LE;

      case AV_PIX_FMT_YUVJ411P:
        return kPixelFormatYUVJ411P;

      case AV_PIX_FMT_BAYER_BGGR8:
        return kPixelFormatBayerBGGR8;

      case AV_PIX_FMT_BAYER_RGGB8:
        return kPixelFormatBayerRGGB8;

      case AV_PIX_FMT_BAYER_GBRG8:
        return kPixelFormatBayerGBRG8;

      case AV_PIX_FMT_BAYER_GRBG8:
        return kPixelFormatBayerGRBG8;

      case AV_PIX_FMT_BAYER_BGGR16LE:
        return kPixelFormatBayerBGGR16LE;

      case AV_PIX_FMT_BAYER_BGGR16BE:
        return kPixelFormatBayerBGGR16BE;

      case AV_PIX_FMT_BAYER_RGGB16LE:
        return kPixelFormatBayerRGGB16LE;

      case AV_PIX_FMT_BAYER_RGGB16BE:
        return kPixelFormatBayerRGGB16BE;

      case AV_PIX_FMT_BAYER_GBRG16LE:
        return kPixelFormatBayerGBRG16LE;

      case AV_PIX_FMT_BAYER_GBRG16BE:
        return kPixelFormatBayerGBRG16BE;

      case AV_PIX_FMT_BAYER_GRBG16LE:
        return kPixelFormatBayerGRBG16LE;

      case AV_PIX_FMT_BAYER_GRBG16BE:
        return kPixelFormatBayerGRBG16BE;

      case AV_PIX_FMT_YUV440P10LE:
        return kPixelFormatYUV440P10LE;

      case AV_PIX_FMT_YUV440P10BE:
        return kPixelFormatYUV440P10BE;

      case AV_PIX_FMT_YUV440P12LE:
        return kPixelFormatYUV440P12LE;

      case AV_PIX_FMT_YUV440P12BE:
        return kPixelFormatYUV440P12BE;

      case AV_PIX_FMT_AYUV64LE:
        return kPixelFormatAYUV64LE;

      case AV_PIX_FMT_AYUV64BE:
        return kPixelFormatAYUV64BE;

      case AV_PIX_FMT_P010LE:
        return kPixelFormatP010LE;

      case AV_PIX_FMT_P010BE:
        return kPixelFormatP010BE;

      case AV_PIX_FMT_GBRAP12BE:
        return kPixelFormatGBRAP12BE;

      case AV_PIX_FMT_GBRAP12LE:
        return kPixelFormatGBRAP12LE;

      case AV_PIX_FMT_GBRAP10BE:
        return kPixelFormatGBRAP10BE;

      case AV_PIX_FMT_GBRAP10LE:
        return kPixelFormatGBRAP10LE;

      case AV_PIX_FMT_GRAY12BE:
        return kPixelFormatGRAY12BE;

      case AV_PIX_FMT_GRAY12LE:
        return kPixelFormatGRAY12LE;

      case AV_PIX_FMT_GRAY10BE:
        return kPixelFormatGRAY10BE;

      case AV_PIX_FMT_GRAY10LE:
        return kPixelFormatGRAY10LE;

      case AV_PIX_FMT_P016LE:
        return kPixelFormatP016LE;

      case AV_PIX_FMT_P016BE:
        return kPixelFormatP016BE;

      case AV_PIX_FMT_GRAY9BE:
        return kPixelFormatGRAY9BE;

      case AV_PIX_FMT_GRAY9LE:
        return kPixelFormatGRAY9LE;

      case AV_PIX_FMT_GBRPF32BE:
        return kPixelFormatGBRPF32BE;

      case AV_PIX_FMT_GBRPF32LE:
        return kPixelFormatGBRPF32LE;

      case AV_PIX_FMT_GBRAPF32BE:
        return kPixelFormatGBRAPF32BE;

      case AV_PIX_FMT_GBRAPF32LE:
        return kPixelFormatGBRAPF32LE;

      case AV_PIX_FMT_GRAY14BE:
        return kPixelFormatGRAY14BE;

      case AV_PIX_FMT_GRAY14LE:
        return kPixelFormatGRAY14LE;

      case AV_PIX_FMT_GRAYF32BE:
        return kPixelFormatGRAYF32BE;

      case AV_PIX_FMT_GRAYF32LE:
        return kPixelFormatGRAYF32LE;

      case AV_PIX_FMT_YUVA422P12BE:
        return kPixelFormatYUVA422P12BE;

      case AV_PIX_FMT_YUVA422P12LE:
        return kPixelFormatYUVA422P12LE;

      case AV_PIX_FMT_YUVA444P12BE:
        return kPixelFormatYUVA444P12BE;

      case AV_PIX_FMT_YUVA444P12LE:
        return kPixelFormatYUVA444P12LE;

      case AV_PIX_FMT_NV24:
        return kPixelFormatNV24;

      case AV_PIX_FMT_NV42:
        return kPixelFormatNV42;
#endif

      default:
        break;
    }

    return kInvalidPixelFormat;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  enum AVPixelFormat
  yae_to_ffmpeg(TPixelFormatId yaePixelFormat)
  {
    switch (yaePixelFormat)
    {
      case kPixelFormatYUV420P:
        return AV_PIX_FMT_YUV420P;

      case kPixelFormatYUYV422:
        return AV_PIX_FMT_YUYV422;

      case kPixelFormatRGB24:
        return AV_PIX_FMT_RGB24;

      case kPixelFormatBGR24:
        return AV_PIX_FMT_BGR24;

      case kPixelFormatYUV422P:
        return AV_PIX_FMT_YUV422P;

      case kPixelFormatYUV444P:
        return AV_PIX_FMT_YUV444P;

      case kPixelFormatYUV410P:
        return AV_PIX_FMT_YUV410P;

      case kPixelFormatYUV411P:
        return AV_PIX_FMT_YUV411P;

      case kPixelFormatGRAY8:
        return AV_PIX_FMT_GRAY8;

      case kPixelFormatMONOWHITE:
        return AV_PIX_FMT_MONOWHITE;

      case kPixelFormatMONOBLACK:
        return AV_PIX_FMT_MONOBLACK;

      case kPixelFormatPAL8:
        return AV_PIX_FMT_PAL8;

      case kPixelFormatUYVY422:
        return AV_PIX_FMT_UYVY422;

      case kPixelFormatUYYVYY411:
        return AV_PIX_FMT_UYYVYY411;

      case kPixelFormatBGR8:
        return AV_PIX_FMT_BGR8;

      case kPixelFormatBGR4:
        return AV_PIX_FMT_BGR4;

      case kPixelFormatBGR4_BYTE:
        return AV_PIX_FMT_BGR4_BYTE;

      case kPixelFormatRGB8:
        return AV_PIX_FMT_RGB8;

      case kPixelFormatRGB4:
        return AV_PIX_FMT_RGB4;

      case kPixelFormatRGB4_BYTE:
        return AV_PIX_FMT_RGB4_BYTE;

      case kPixelFormatNV12:
        return AV_PIX_FMT_NV12;

      case kPixelFormatNV21:
        return AV_PIX_FMT_NV21;

      case kPixelFormatARGB:
        return AV_PIX_FMT_ARGB;

      case kPixelFormatRGBA:
        return AV_PIX_FMT_RGBA;

      case kPixelFormatABGR:
        return AV_PIX_FMT_ABGR;

      case kPixelFormatBGRA:
        return AV_PIX_FMT_BGRA;

      case kPixelFormatGRAY16BE:
        return AV_PIX_FMT_GRAY16BE;

      case kPixelFormatGRAY16LE:
        return AV_PIX_FMT_GRAY16LE;

      case kPixelFormatYUV440P:
        return AV_PIX_FMT_YUV440P;

      case kPixelFormatYUVA420P:
        return AV_PIX_FMT_YUVA420P;

      case kPixelFormatRGB48BE:
        return AV_PIX_FMT_RGB48BE;

      case kPixelFormatRGB48LE:
        return AV_PIX_FMT_RGB48LE;

      case kPixelFormatRGB565BE:
        return AV_PIX_FMT_RGB565BE;

      case kPixelFormatRGB565LE:
        return AV_PIX_FMT_RGB565LE;

      case kPixelFormatRGB555BE:
        return AV_PIX_FMT_RGB555BE;

      case kPixelFormatRGB555LE:
        return AV_PIX_FMT_RGB555LE;

      case kPixelFormatBGR565BE:
        return AV_PIX_FMT_BGR565BE;

      case kPixelFormatBGR565LE:
        return AV_PIX_FMT_BGR565LE;

      case kPixelFormatBGR555BE:
        return AV_PIX_FMT_BGR555BE;

      case kPixelFormatBGR555LE:
        return AV_PIX_FMT_BGR555LE;

      case kPixelFormatYUV420P16LE:
        return AV_PIX_FMT_YUV420P16LE;

      case kPixelFormatYUV420P16BE:
        return AV_PIX_FMT_YUV420P16BE;

      case kPixelFormatYUV422P16LE:
        return AV_PIX_FMT_YUV422P16LE;

      case kPixelFormatYUV422P16BE:
        return AV_PIX_FMT_YUV422P16BE;

      case kPixelFormatYUV444P16LE:
        return AV_PIX_FMT_YUV444P16LE;

      case kPixelFormatYUV444P16BE:
        return AV_PIX_FMT_YUV444P16BE;

      case kPixelFormatRGB444BE:
        return AV_PIX_FMT_RGB444BE;

      case kPixelFormatRGB444LE:
        return AV_PIX_FMT_RGB444LE;

      case kPixelFormatBGR444BE:
        return AV_PIX_FMT_BGR444BE;

      case kPixelFormatBGR444LE:
        return AV_PIX_FMT_BGR444LE;

      case kPixelFormatY400A:
        return AV_PIX_FMT_Y400A;

      case kPixelFormatYUVJ420P:
        return AV_PIX_FMT_YUVJ420P;

      case kPixelFormatYUVJ422P:
        return AV_PIX_FMT_YUVJ422P;

      case kPixelFormatYUVJ444P:
        return AV_PIX_FMT_YUVJ444P;

      case kPixelFormatYUVJ440P:
        return AV_PIX_FMT_YUVJ440P;

      case kPixelFormatYUV420P9BE:
        return AV_PIX_FMT_YUV420P9BE;

      case kPixelFormatYUV420P9LE:
        return AV_PIX_FMT_YUV420P9LE;

      case kPixelFormatYUV422P9BE:
        return AV_PIX_FMT_YUV422P9BE;

      case kPixelFormatYUV422P9LE:
        return AV_PIX_FMT_YUV422P9LE;

      case kPixelFormatYUV444P9BE:
        return AV_PIX_FMT_YUV444P9BE;

      case kPixelFormatYUV444P9LE:
        return AV_PIX_FMT_YUV444P9LE;

      case kPixelFormatYUV420P10BE:
        return AV_PIX_FMT_YUV420P10BE;

      case kPixelFormatYUV420P10LE:
        return AV_PIX_FMT_YUV420P10LE;

      case kPixelFormatYUV422P10BE:
        return AV_PIX_FMT_YUV422P10BE;

      case kPixelFormatYUV422P10LE:
        return AV_PIX_FMT_YUV422P10LE;

      case kPixelFormatYUV444P10BE:
        return AV_PIX_FMT_YUV444P10BE;

      case kPixelFormatYUV444P10LE:
        return AV_PIX_FMT_YUV444P10LE;

      case kPixelFormatRGBA64BE:
        return AV_PIX_FMT_RGBA64BE;

      case kPixelFormatRGBA64LE:
        return AV_PIX_FMT_RGBA64LE;

      case kPixelFormatBGRA64BE:
        return AV_PIX_FMT_BGRA64BE;

      case kPixelFormatBGRA64LE:
        return AV_PIX_FMT_BGRA64LE;

      case kPixelFormatGBRP:
        return AV_PIX_FMT_GBRP;

      case kPixelFormatGBRP9BE:
        return AV_PIX_FMT_GBRP9BE;

      case kPixelFormatGBRP9LE:
        return AV_PIX_FMT_GBRP9LE;

      case kPixelFormatGBRP10BE:
        return AV_PIX_FMT_GBRP10BE;

      case kPixelFormatGBRP10LE:
        return AV_PIX_FMT_GBRP10LE;

      case kPixelFormatGBRP16BE:
        return AV_PIX_FMT_GBRP16BE;

      case kPixelFormatGBRP16LE:
        return AV_PIX_FMT_GBRP16LE;

      case kPixelFormatYUVA420P9BE:
        return AV_PIX_FMT_YUVA420P9BE;

      case kPixelFormatYUVA420P9LE:
        return AV_PIX_FMT_YUVA420P9LE;

      case kPixelFormatYUVA422P9BE:
        return AV_PIX_FMT_YUVA422P9BE;

      case kPixelFormatYUVA422P9LE:
        return AV_PIX_FMT_YUVA422P9LE;

      case kPixelFormatYUVA444P9BE:
        return AV_PIX_FMT_YUVA444P9BE;

      case kPixelFormatYUVA444P9LE:
        return AV_PIX_FMT_YUVA444P9LE;

      case kPixelFormatYUVA420P10BE:
        return AV_PIX_FMT_YUVA420P10BE;

      case kPixelFormatYUVA420P10LE:
        return AV_PIX_FMT_YUVA420P10LE;

      case kPixelFormatYUVA422P10BE:
        return AV_PIX_FMT_YUVA422P10BE;

      case kPixelFormatYUVA422P10LE:
        return AV_PIX_FMT_YUVA422P10LE;

      case kPixelFormatYUVA444P10BE:
        return AV_PIX_FMT_YUVA444P10BE;

      case kPixelFormatYUVA444P10LE:
        return AV_PIX_FMT_YUVA444P10LE;

      case kPixelFormatYUVA420P16BE:
        return AV_PIX_FMT_YUVA420P16BE;

      case kPixelFormatYUVA420P16LE:
        return AV_PIX_FMT_YUVA420P16LE;

      case kPixelFormatYUVA422P16BE:
        return AV_PIX_FMT_YUVA422P16BE;

      case kPixelFormatYUVA422P16LE:
        return AV_PIX_FMT_YUVA422P16LE;

      case kPixelFormatYUVA444P16BE:
        return AV_PIX_FMT_YUVA444P16BE;

      case kPixelFormatYUVA444P16LE:
        return AV_PIX_FMT_YUVA444P16LE;

      case kPixelFormatXYZ12BE:
        return AV_PIX_FMT_XYZ12BE;

      case kPixelFormatXYZ12LE:
        return AV_PIX_FMT_XYZ12LE;

      case kPixelFormatNV16:
        return AV_PIX_FMT_NV16;

      case kPixelFormatNV20BE:
        return AV_PIX_FMT_NV20BE;

      case kPixelFormatNV20LE:
        return AV_PIX_FMT_NV20LE;

      case kPixelFormatYVYU422:
        return AV_PIX_FMT_YVYU422;

      case kPixelFormatYA16BE:
        return AV_PIX_FMT_YA16BE;

      case kPixelFormatYA16LE:
        return AV_PIX_FMT_YA16LE;

#if LIBAVUTIL_VERSION_INT > AV_VERSION_INT(54, 3, 0)
      case kPixelFormat0RGB:
        return AV_PIX_FMT_0RGB;

      case kPixelFormatRGB0:
        return AV_PIX_FMT_RGB0;

      case kPixelFormat0BGR:
        return AV_PIX_FMT_0BGR;

      case kPixelFormatBGR0:
        return AV_PIX_FMT_BGR0;

      case kPixelFormatYUVA422P:
        return AV_PIX_FMT_YUVA422P;

      case kPixelFormatYUVA444P:
        return AV_PIX_FMT_YUVA444P;

      case kPixelFormatYUV420P12BE:
        return AV_PIX_FMT_YUV420P12BE;

      case kPixelFormatYUV420P12LE:
        return AV_PIX_FMT_YUV420P12LE;

      case kPixelFormatYUV420P14BE:
        return AV_PIX_FMT_YUV420P14BE;

      case kPixelFormatYUV420P14LE:
        return AV_PIX_FMT_YUV420P14LE;

      case kPixelFormatYUV422P12BE:
        return AV_PIX_FMT_YUV422P12BE;

      case kPixelFormatYUV422P12LE:
        return AV_PIX_FMT_YUV422P12LE;

      case kPixelFormatYUV422P14BE:
        return AV_PIX_FMT_YUV422P14BE;

      case kPixelFormatYUV422P14LE:
        return AV_PIX_FMT_YUV422P14LE;

      case kPixelFormatYUV444P12BE:
        return AV_PIX_FMT_YUV444P12BE;

      case kPixelFormatYUV444P12LE:
        return AV_PIX_FMT_YUV444P12LE;

      case kPixelFormatYUV444P14BE:
        return AV_PIX_FMT_YUV444P14BE;

      case kPixelFormatYUV444P14LE:
        return AV_PIX_FMT_YUV444P14LE;

      case kPixelFormatGBRP12BE:
        return AV_PIX_FMT_GBRP12BE;

      case kPixelFormatGBRP12LE:
        return AV_PIX_FMT_GBRP12LE;

      case kPixelFormatGBRP14BE:
        return AV_PIX_FMT_GBRP14BE;

      case kPixelFormatGBRP14LE:
        return AV_PIX_FMT_GBRP14LE;

      case kPixelFormatGBRAP:
        return AV_PIX_FMT_GBRAP;

      case kPixelFormatGBRAP16BE:
        return AV_PIX_FMT_GBRAP16BE;

      case kPixelFormatGBRAP16LE:
        return AV_PIX_FMT_GBRAP16LE;

      case kPixelFormatYUVJ411P:
        return AV_PIX_FMT_YUVJ411P;

      case kPixelFormatBayerBGGR8:
        return AV_PIX_FMT_BAYER_BGGR8;

      case kPixelFormatBayerRGGB8:
        return AV_PIX_FMT_BAYER_RGGB8;

      case kPixelFormatBayerGBRG8:
        return AV_PIX_FMT_BAYER_GBRG8;

      case kPixelFormatBayerGRBG8:
        return AV_PIX_FMT_BAYER_GRBG8;

      case kPixelFormatBayerBGGR16LE:
        return AV_PIX_FMT_BAYER_BGGR16LE;

      case kPixelFormatBayerBGGR16BE:
        return AV_PIX_FMT_BAYER_BGGR16BE;

      case kPixelFormatBayerRGGB16LE:
        return AV_PIX_FMT_BAYER_RGGB16LE;

      case kPixelFormatBayerRGGB16BE:
        return AV_PIX_FMT_BAYER_RGGB16BE;

      case kPixelFormatBayerGBRG16LE:
        return AV_PIX_FMT_BAYER_GBRG16LE;

      case kPixelFormatBayerGBRG16BE:
        return AV_PIX_FMT_BAYER_GBRG16BE;

      case kPixelFormatBayerGRBG16LE:
        return AV_PIX_FMT_BAYER_GRBG16LE;

      case kPixelFormatBayerGRBG16BE:
        return AV_PIX_FMT_BAYER_GRBG16BE;

      case kPixelFormatYUV440P10LE:
        return AV_PIX_FMT_YUV440P10LE;

      case kPixelFormatYUV440P10BE:
        return AV_PIX_FMT_YUV440P10BE;

      case kPixelFormatYUV440P12LE:
        return AV_PIX_FMT_YUV440P12LE;

      case kPixelFormatYUV440P12BE:
        return AV_PIX_FMT_YUV440P12BE;

      case kPixelFormatAYUV64LE:
        return AV_PIX_FMT_AYUV64LE;

      case kPixelFormatAYUV64BE:
        return AV_PIX_FMT_AYUV64BE;

      case kPixelFormatP010LE:
        return AV_PIX_FMT_P010LE;

      case kPixelFormatP010BE:
        return AV_PIX_FMT_P010BE;

      case kPixelFormatGBRAP12BE:
        return AV_PIX_FMT_GBRAP12BE;

      case kPixelFormatGBRAP12LE:
        return AV_PIX_FMT_GBRAP12LE;

      case kPixelFormatGBRAP10BE:
        return AV_PIX_FMT_GBRAP10BE;

      case kPixelFormatGBRAP10LE:
        return AV_PIX_FMT_GBRAP10LE;

      case kPixelFormatGRAY12BE:
        return AV_PIX_FMT_GRAY12BE;

      case kPixelFormatGRAY12LE:
        return AV_PIX_FMT_GRAY12LE;

      case kPixelFormatGRAY10BE:
        return AV_PIX_FMT_GRAY10BE;

      case kPixelFormatGRAY10LE:
        return AV_PIX_FMT_GRAY10LE;

      case kPixelFormatP016LE:
        return AV_PIX_FMT_P016LE;

      case kPixelFormatP016BE:
        return AV_PIX_FMT_P016BE;

      case kPixelFormatGRAY9BE:
        return AV_PIX_FMT_GRAY9BE;

      case kPixelFormatGRAY9LE:
        return AV_PIX_FMT_GRAY9LE;

      case kPixelFormatGBRPF32BE:
        return AV_PIX_FMT_GBRPF32BE;

      case kPixelFormatGBRPF32LE:
        return AV_PIX_FMT_GBRPF32LE;

      case kPixelFormatGBRAPF32BE:
        return AV_PIX_FMT_GBRAPF32BE;

      case kPixelFormatGBRAPF32LE:
        return AV_PIX_FMT_GBRAPF32LE;

      case kPixelFormatGRAY14BE:
        return AV_PIX_FMT_GRAY14BE;

      case kPixelFormatGRAY14LE:
        return AV_PIX_FMT_GRAY14LE;

      case kPixelFormatGRAYF32BE:
        return AV_PIX_FMT_GRAYF32BE;

      case kPixelFormatGRAYF32LE:
        return AV_PIX_FMT_GRAYF32LE;

      case kPixelFormatYUVA422P12BE:
        return AV_PIX_FMT_YUVA422P12BE;

      case kPixelFormatYUVA422P12LE:
        return AV_PIX_FMT_YUVA422P12LE;

      case kPixelFormatYUVA444P12BE:
        return AV_PIX_FMT_YUVA444P12BE;

      case kPixelFormatYUVA444P12LE:
        return AV_PIX_FMT_YUVA444P12LE;

      case kPixelFormatNV24:
        return AV_PIX_FMT_NV24;

      case kPixelFormatNV42:
        return AV_PIX_FMT_NV42;
#endif

      default:
        break;
    }

    return AV_PIX_FMT_NONE;
  }

  //----------------------------------------------------------------
  // fixed16_to_double
  //
  inline static double fixed16_to_double(int fixed16)
  {
    int whole = fixed16 >> 16;
    int fract = fixed16 & 65535;
    double t = double(whole) + double(fract) / 65536.0;
    return t;
  }

  //----------------------------------------------------------------
  // init_abc_to_rgb_matrix
  //
  // Fill in the m3x4 matrix for color conversion from
  // input color format ABC to full-range RGB:
  //
  // [R, G, B]T = m3x4 * [A, B, C, 1]T
  //
  // NOTE: ABC and RGB are expressed in the [0, 1] range,
  //       not [0, 255].
  //
  // NOTE: Here ABC typically refers to YUV input color format,
  //       however it doesn't have to be YUV.
  //
  bool
  VideoTraits::initAbcToRgbMatrix(double * m3x4) const
  {
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(this->av_fmt_);
    if (!desc)
    {
      return false;
    }

    // shortcut:
    const bool narrow_range =
      (this->av_rng_ == AVCOL_RANGE_MPEG);

    const bool flag_rgb =
      (desc->flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB;

    const bool flag_alpha =
      (desc->flags & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;

    const AVComponentDescriptor & luma = desc->comp[0];
    const unsigned int bit_depth = luma.shift + luma.depth;
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
        double Pb = (Cb - y_offset) * scale_chroma;
        double Pr = (Cr - y_offset) * scale_chroma;
      */

      // affine transform from Y'CbCr to Y'PbPr
      m4x4_t ycbcr_to_ypbpr = make_m4x4(sy,  0.0, 0.0, a,
                                        0.0, sc,  0.0, b,
                                        0.0, 0.0, sc,  b,
                                        0.0, 0.0, 0.0, 1.0);
      m4x4_t ypbpr_to_rgb = this->colorspace_->ypbpr_to_rgb_;
      m4x4_t ycbcr_to_rgb = ypbpr_to_rgb * ycbcr_to_ypbpr;

      // NOTE: dropping the bottom row of ycbcr_to_rgb
      // since the computed value would be discarded anyway
      memcpy(m3x4, ycbcr_to_rgb.begin(), 3 * 4 * sizeof(double));
    }
    else if (narrow_range &&
             (flag_rgb ||
              (desc->nb_components == 2 && flag_alpha) ||
              (desc->nb_components == 1)))
    {
      // affine transform from narrow range to full range
      m4x4_t narrow_to_full = make_m4x4(sy,  0.0, 0.0, a,
                                        0.0, sy,  0.0, a,
                                        0.0, 0.0, sy,  a,
                                        0.0, 0.0, 0.0, 1.0);

      // NOTE: dropping the bottom row of ycbcr_to_rgb
      // since the computed value would be discarded anyway
      memcpy(m3x4, narrow_to_full.begin(), 3 * 4 * sizeof(double));
    }
    else
    {
      // nothing to do, use the identity matrix:
      static const double identity[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
      };

      std::memcpy(m3x4, identity, sizeof(identity));
    }

    return true;
  }
}

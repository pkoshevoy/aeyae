// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 20 16:44:22 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include <yaePixelFormatFFMPEG.h>

// ffmpeg includes:
extern "C"
{
#include <libavutil/avutil.h>
}


namespace yae
{
  //----------------------------------------------------------------
  // ffmpeg_to_yae
  // 
  TPixelFormatId
  ffmpeg_to_yae(enum PixelFormat ffmpegPixelFormat)
  {
    switch (ffmpegPixelFormat)
    {
      case PIX_FMT_YUV420P:
        return kPixelFormatYUV420P;

      case PIX_FMT_YUYV422:
        return kPixelFormatYUYV422;

      case PIX_FMT_RGB24:
        return kPixelFormatRGB24;

      case PIX_FMT_BGR24:
        return kPixelFormatBGR24;

      case PIX_FMT_YUV422P:
        return kPixelFormatYUV422P;

      case PIX_FMT_YUV444P:
        return kPixelFormatYUV444P;

      case PIX_FMT_YUV410P:
        return kPixelFormatYUV410P;

      case PIX_FMT_YUV411P:
        return kPixelFormatYUV411P;

      case PIX_FMT_GRAY8:
        return kPixelFormatGRAY8;

      case PIX_FMT_MONOWHITE:
        return kPixelFormatMONOWHITE;

      case PIX_FMT_MONOBLACK:
        return kPixelFormatMONOBLACK;

      case PIX_FMT_PAL8:
        return kPixelFormatPAL8;

      case PIX_FMT_UYVY422:
        return kPixelFormatUYVY422;

      case PIX_FMT_UYYVYY411:
        return kPixelFormatUYYVYY411;

      case PIX_FMT_BGR8:
        return kPixelFormatBGR8;

      case PIX_FMT_BGR4:
        return kPixelFormatBGR4;

      case PIX_FMT_BGR4_BYTE:
        return kPixelFormatBGR4_BYTE;

      case PIX_FMT_RGB8:
        return kPixelFormatRGB8;

      case PIX_FMT_RGB4:
        return kPixelFormatRGB4;

      case PIX_FMT_RGB4_BYTE:
        return kPixelFormatRGB4_BYTE;

      case PIX_FMT_NV12:
        return kPixelFormatNV12;

      case PIX_FMT_NV21:
        return kPixelFormatNV21;

      case PIX_FMT_ARGB:
        return kPixelFormatARGB;

      case PIX_FMT_RGBA:
        return kPixelFormatRGBA;

      case PIX_FMT_ABGR:
        return kPixelFormatABGR;

      case PIX_FMT_BGRA:
        return kPixelFormatBGRA;

      case PIX_FMT_GRAY16BE:
        return kPixelFormatGRAY16BE;

      case PIX_FMT_GRAY16LE:
        return kPixelFormatGRAY16LE;

      case PIX_FMT_YUV440P:
        return kPixelFormatYUV440P;

      case PIX_FMT_YUVA420P:
        return kPixelFormatYUVA420P;

      case PIX_FMT_RGB48BE:
        return kPixelFormatRGB48BE;

      case PIX_FMT_RGB48LE:
        return kPixelFormatRGB48LE;

      case PIX_FMT_RGB565BE:
        return kPixelFormatRGB565BE;

      case PIX_FMT_RGB565LE:
        return kPixelFormatRGB565LE;

      case PIX_FMT_RGB555BE:
        return kPixelFormatRGB555BE;

      case PIX_FMT_RGB555LE:
        return kPixelFormatRGB555LE;

      case PIX_FMT_BGR565BE:
        return kPixelFormatBGR565BE;

      case PIX_FMT_BGR565LE:
        return kPixelFormatBGR565LE;

      case PIX_FMT_BGR555BE:
        return kPixelFormatBGR555BE;

      case PIX_FMT_BGR555LE:
        return kPixelFormatBGR555LE;

      case PIX_FMT_YUV420P16LE:
        return kPixelFormatYUV420P16LE;

      case PIX_FMT_YUV420P16BE:
        return kPixelFormatYUV420P16BE;

      case PIX_FMT_YUV422P16LE:
        return kPixelFormatYUV422P16LE;

      case PIX_FMT_YUV422P16BE:
        return kPixelFormatYUV422P16BE;

      case PIX_FMT_YUV444P16LE:
        return kPixelFormatYUV444P16LE;

      case PIX_FMT_YUV444P16BE:
        return kPixelFormatYUV444P16BE;

      case PIX_FMT_RGB444BE:
        return kPixelFormatRGB444BE;

      case PIX_FMT_RGB444LE:
        return kPixelFormatRGB444LE;

      case PIX_FMT_BGR444BE:
        return kPixelFormatBGR444BE;

      case PIX_FMT_BGR444LE:
        return kPixelFormatBGR444LE;

      case PIX_FMT_Y400A:
        return kPixelFormatY400A;

      case PIX_FMT_YUVJ420P:
        return kPixelFormatYUVJ420P;

      case PIX_FMT_YUVJ422P:
        return kPixelFormatYUVJ422P;

      case PIX_FMT_YUVJ444P:
        return kPixelFormatYUVJ444P;

      case PIX_FMT_YUVJ440P:
        return kPixelFormatYUVJ440P;

      case PIX_FMT_YUV420P9:
        return kPixelFormatYUV420P9;

      case PIX_FMT_YUV422P9:
        return kPixelFormatYUV422P9;

      case PIX_FMT_YUV444P9:
        return kPixelFormatYUV444P9;

      case PIX_FMT_YUV420P10:
        return kPixelFormatYUV420P10;

      case PIX_FMT_YUV422P10:
        return kPixelFormatYUV422P10;

      case PIX_FMT_YUV444P10:
        return kPixelFormatYUV444P10;

      default:
        break;
    }

    return kInvalidPixelFormat;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  // 
  enum PixelFormat
  yae_to_ffmpeg(TPixelFormatId yaePixelFormat)
  {
    switch (yaePixelFormat)
    {
      case kPixelFormatYUV420P:
        return PIX_FMT_YUV420P;

      case kPixelFormatYUYV422:
        return PIX_FMT_YUYV422;

      case kPixelFormatRGB24:
        return PIX_FMT_RGB24;

      case kPixelFormatBGR24:
        return PIX_FMT_BGR24;

      case kPixelFormatYUV422P:
        return PIX_FMT_YUV422P;

      case kPixelFormatYUV444P:
        return PIX_FMT_YUV444P;

      case kPixelFormatYUV410P:
        return PIX_FMT_YUV410P;

      case kPixelFormatYUV411P:
        return PIX_FMT_YUV411P;

      case kPixelFormatGRAY8:
        return PIX_FMT_GRAY8;

      case kPixelFormatMONOWHITE:
        return PIX_FMT_MONOWHITE;

      case kPixelFormatMONOBLACK:
        return PIX_FMT_MONOBLACK;

      case kPixelFormatPAL8:
        return PIX_FMT_PAL8;

      case kPixelFormatUYVY422:
        return PIX_FMT_UYVY422;

      case kPixelFormatUYYVYY411:
        return PIX_FMT_UYYVYY411;

      case kPixelFormatBGR8:
        return PIX_FMT_BGR8;

      case kPixelFormatBGR4:
        return PIX_FMT_BGR4;

      case kPixelFormatBGR4_BYTE:
        return PIX_FMT_BGR4_BYTE;

      case kPixelFormatRGB8:
        return PIX_FMT_RGB8;

      case kPixelFormatRGB4:
        return PIX_FMT_RGB4;

      case kPixelFormatRGB4_BYTE:
        return PIX_FMT_RGB4_BYTE;

      case kPixelFormatNV12:
        return PIX_FMT_NV12;

      case kPixelFormatNV21:
        return PIX_FMT_NV21;

      case kPixelFormatARGB:
        return PIX_FMT_ARGB;

      case kPixelFormatRGBA:
        return PIX_FMT_RGBA;

      case kPixelFormatABGR:
        return PIX_FMT_ABGR;

      case kPixelFormatBGRA:
        return PIX_FMT_BGRA;

      case kPixelFormatGRAY16BE:
        return PIX_FMT_GRAY16BE;

      case kPixelFormatGRAY16LE:
        return PIX_FMT_GRAY16LE;

      case kPixelFormatYUV440P:
        return PIX_FMT_YUV440P;

      case kPixelFormatYUVA420P:
        return PIX_FMT_YUVA420P;

      case kPixelFormatRGB48BE:
        return PIX_FMT_RGB48BE;

      case kPixelFormatRGB48LE:
        return PIX_FMT_RGB48LE;

      case kPixelFormatRGB565BE:
        return PIX_FMT_RGB565BE;

      case kPixelFormatRGB565LE:
        return PIX_FMT_RGB565LE;

      case kPixelFormatRGB555BE:
        return PIX_FMT_RGB555BE;

      case kPixelFormatRGB555LE:
        return PIX_FMT_RGB555LE;

      case kPixelFormatBGR565BE:
        return PIX_FMT_BGR565BE;

      case kPixelFormatBGR565LE:
        return PIX_FMT_BGR565LE;

      case kPixelFormatBGR555BE:
        return PIX_FMT_BGR555BE;

      case kPixelFormatBGR555LE:
        return PIX_FMT_BGR555LE;

      case kPixelFormatYUV420P16LE:
        return PIX_FMT_YUV420P16LE;

      case kPixelFormatYUV420P16BE:
        return PIX_FMT_YUV420P16BE;

      case kPixelFormatYUV422P16LE:
        return PIX_FMT_YUV422P16LE;

      case kPixelFormatYUV422P16BE:
        return PIX_FMT_YUV422P16BE;

      case kPixelFormatYUV444P16LE:
        return PIX_FMT_YUV444P16LE;

      case kPixelFormatYUV444P16BE:
        return PIX_FMT_YUV444P16BE;

      case kPixelFormatRGB444BE:
        return PIX_FMT_RGB444BE;

      case kPixelFormatRGB444LE:
        return PIX_FMT_RGB444LE;

      case kPixelFormatBGR444BE:
        return PIX_FMT_BGR444BE;

      case kPixelFormatBGR444LE:
        return PIX_FMT_BGR444LE;

      case kPixelFormatY400A:
        return PIX_FMT_Y400A;

      case kPixelFormatYUVJ420P:
        return PIX_FMT_YUVJ420P;

      case kPixelFormatYUVJ422P:
        return PIX_FMT_YUVJ422P;

      case kPixelFormatYUVJ444P:
        return PIX_FMT_YUVJ444P;

      case kPixelFormatYUVJ440P:
        return PIX_FMT_YUVJ440P;

      case kPixelFormatYUV420P9:
        return PIX_FMT_YUV420P9;

      case kPixelFormatYUV422P9:
        return PIX_FMT_YUV422P9;

      case kPixelFormatYUV444P9:
        return PIX_FMT_YUV444P9;

      case kPixelFormatYUV420P10:
        return PIX_FMT_YUV420P10;

      case kPixelFormatYUV422P10:
        return PIX_FMT_YUV422P10;

      case kPixelFormatYUV444P10:
        return PIX_FMT_YUV444P10;

      default:
        break;
    }

    return PIX_FMT_NONE;
  }
}

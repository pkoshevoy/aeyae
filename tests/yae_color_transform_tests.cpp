// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Sep  9 20:19:26 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <cmath>

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/video/yae_color_transform.h"

// namespace access:
using namespace yae;


//----------------------------------------------------------------
// yae_colorspace_transfer_eotf_oetf
//
BOOST_AUTO_TEST_CASE(yae_colorspace_transfer_eotf_oetf)
{
  for (int i = AVCOL_TRC_RESERVED0; i < AVCOL_TRC_NB; i++)
  {
    AVColorTransferCharacteristic av_trc = (AVColorTransferCharacteristic)i;
    const Colorspace::TransferFunc & transfer = get_transfer_func(av_trc);

    for (int j = 0; j <= 100; j++)
    {
      double non_linear_in = double(j) / 100.0;
      double linear_cdm2 = transfer.eotf(non_linear_in);
      double non_linear_out = transfer.oetf(linear_cdm2);
      BOOST_CHECK(fabs(non_linear_in - non_linear_out) < 1e-6);
    }
  }
}

//----------------------------------------------------------------
// ycbcr_to_ypbpr_to_ycbcr_narrow_8bit
//
BOOST_AUTO_TEST_CASE(ycbcr_to_ypbpr_to_ycbcr_narrow_8bit)
{
  m4x4_t ycbcr_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(ycbcr_to_ypbpr,
                                 AV_PIX_FMT_YUV420P,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_ycbcr;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_ycbcr,
                                 AV_PIX_FMT_YUV420P,
                                 AVCOL_RANGE_MPEG));

  double y_min = 16.0 / 255.0;
  double y_rng = 219.0 / 255.0;
  double c_rng = 224.0 / 255.0;

  // temporaries:
  v4x1_t yuv = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr;

  // check out-of-range luma, chroma:
  {
    ypbpr = ycbcr_to_ypbpr * make_v4x1(0.0, 0.0, 0.0);
    BOOST_CHECK(ypbpr[0] < 0.0);
    BOOST_CHECK(ypbpr[1] < -0.5);
    BOOST_CHECK(ypbpr[2] < -0.5);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(1.0, 1.0, 1.0);
    BOOST_CHECK(ypbpr[0] > 1.0);
    BOOST_CHECK(ypbpr[1] > 0.5);
    BOOST_CHECK(ypbpr[2] > 0.5);
  }

  // check luma, chroma range:
  {
    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min,
                                       y_min,
                                       y_min);
    BOOST_CHECK(std::fabs(ypbpr[0] - 0.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] + 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] + 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min + y_rng,
                                       y_min + c_rng,
                                       y_min + c_rng);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min + y_rng,
                                       y_min,
                                       y_min + c_rng);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] + 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min + y_rng,
                                       y_min + c_rng,
                                       y_min);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] + 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min + y_rng * 0.5,
                                       y_min + c_rng * 0.5,
                                       y_min + c_rng * 0.5);
    BOOST_CHECK(std::fabs(ypbpr[0] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.0) < 1e-6);
  }

  // check identity transform:
  m4x4_t m = ypbpr_to_ycbcr * ycbcr_to_ypbpr;

  for (int i = 0; i <= 100; i++)
  {
    yuv[0] = y_min + (y_rng * double(i)) / 100.0;

    for (int j = 0; j <= 100; j++)
    {
      yuv[1] = y_min + (c_rng * double(j)) / 100.0;

      for (int k = 0; k <= 100; k++)
      {
        yuv[2] = y_min + (c_rng * double(k)) / 100.0;

        ypbpr = ycbcr_to_ypbpr * yuv;
        v4x1_t out_1 = ypbpr_to_ycbcr * ypbpr;
        v4x1_t out_2 = m * yuv;

        double err_1 = norm_squared(out_1 - yuv);
        double err_2 = norm_squared(out_2 - yuv);

        BOOST_CHECK(err_1 < 1e-12);
        BOOST_CHECK(err_2 < 1e-12);
        BOOST_CHECK(std::fabs(err_1 - err_2) < 1e-12);
      }
    }
  }
}

//----------------------------------------------------------------
// ycbcr_to_ypbpr_to_ycbcr_full_8bit
//
BOOST_AUTO_TEST_CASE(ycbcr_to_ypbpr_to_ycbcr_full_8bit)
{
  m4x4_t ycbcr_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(ycbcr_to_ypbpr,
                                 AV_PIX_FMT_YUV420P,
                                 AVCOL_RANGE_JPEG));

  m4x4_t ypbpr_to_ycbcr;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_ycbcr,
                                 AV_PIX_FMT_YUV420P,
                                 AVCOL_RANGE_JPEG));

  double y_min = 16.0 / 255.0;
  double y_rng = 219.0 / 255.0;
  double c_rng = 224.0 / 255.0;

  // temporaries:
  v4x1_t yuv = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr;

  // check narrow-range luma, chroma:
  {
    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min, y_min, y_min);
    BOOST_CHECK(ypbpr[0] > 0.0);
    BOOST_CHECK(ypbpr[1] > -0.5);
    BOOST_CHECK(ypbpr[2] > -0.5);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(y_min + y_rng,
                                       y_min + c_rng,
                                       y_min + c_rng);
    BOOST_CHECK(ypbpr[0] < 1.0);
    BOOST_CHECK(ypbpr[1] < 0.5);
    BOOST_CHECK(ypbpr[2] < 0.5);
  }

  // check chroma range:
  {
    ypbpr = ycbcr_to_ypbpr * make_v4x1(0.0, 0.0, 0.0);
    BOOST_CHECK(std::fabs(ypbpr[0] - 0.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] + 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] + 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(1.0, 1.0, 1.0);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(1.0, 0.0, 1.0);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] + 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(1.0, 1.0, 0.0);
    BOOST_CHECK(std::fabs(ypbpr[0] - 1.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] + 0.5) < 1e-6);

    ypbpr = ycbcr_to_ypbpr * make_v4x1(0.5, 0.5, 0.5);
    BOOST_CHECK(std::fabs(ypbpr[0] - 0.5) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[1] - 0.0) < 1e-6);
    BOOST_CHECK(std::fabs(ypbpr[2] - 0.0) < 1e-6);
  }

  // check identity transform:
  m4x4_t m = ypbpr_to_ycbcr * ycbcr_to_ypbpr;

  for (int i = 0; i <= 100; i++)
  {
    yuv[0] = 0.0 + (255.0 * double(i)) / 100.0;

    for (int j = 0; j <= 100; j++)
    {
      yuv[1] = 0.0 + (255.0 * double(j)) / 100.0;

      for (int k = 0; k <= 100; k++)
      {
        yuv[2] = 0.0 + (255.0 * double(k)) / 100.0;

        ypbpr = ycbcr_to_ypbpr * yuv;
        v4x1_t out_1 = ypbpr_to_ycbcr * ypbpr;
        v4x1_t out_2 = m * yuv;

        double err_1 = norm_squared(out_1 - yuv);
        double err_2 = norm_squared(out_2 - yuv);

        BOOST_CHECK(err_1 < 1e-12);
        BOOST_CHECK(err_2 < 1e-12);
        BOOST_CHECK(std::fabs(err_1 - err_2) < 1e-12);
      }
    }
  }
}

#if 1
BOOST_AUTO_TEST_CASE(yae_color_transform_hlg_to_sdr_yuv444)
{
  const Colorspace * csp_hlg = Colorspace::get(AVCOL_SPC_BT2020_NCL,
                                               AVCOL_PRI_BT2020,
                                               AVCOL_TRC_ARIB_STD_B67);
  BOOST_CHECK(!!csp_hlg);

  const Colorspace * csp_sdr = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!csp_sdr);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_P010,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_MPEG));

  ToneMapGamma tone_map(1000, 1.8);

  ColorTransform lut3d(7);
  lut3d.fill(*csp_hlg,
             *csp_sdr,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  const unsigned int log2_w = lut3d.log2_edge_ + (lut3d.log2_edge_ + 1) / 2;
  const unsigned int log2_h = lut3d.log2_edge_ * 3 - log2_w;

  const unsigned int clut_h = 1 << log2_h;
  const unsigned int clut_w = 1 << log2_w;

  AvFrm frm = make_avfrm(AV_PIX_FMT_YUV444P,
                         clut_w,
                         clut_h,
                         csp_sdr->av_csp_,
                         csp_sdr->av_pri_,
                         csp_sdr->av_trc_,
                         AVCOL_RANGE_MPEG);

  AVFrame & frame = frm.get();
  for (unsigned int i = 0; i < clut_h; i++)
  {
    for (unsigned int j = 0; j < clut_w; j++)
    {
      const unsigned int offset = i * clut_w + j;
      if (offset >= lut3d.size_3d_)
      {
        break;
      }

      const ColorTransform::Pixel & pixel = lut3d.at(offset);

      unsigned char * dst_y = frame.data[0] + frame.linesize[0] * i + j;
      unsigned char * dst_u = frame.data[1] + frame.linesize[1] * i + j;
      unsigned char * dst_v = frame.data[2] + frame.linesize[2] * i + j;

      *dst_y = (unsigned char)(255.0 * pixel.data_[0]);
      *dst_u = (unsigned char)(255.0 * pixel.data_[1]);
      *dst_v = (unsigned char)(255.0 * pixel.data_[2]);
    }
  }
  std::string fn_prefix = "/tmp/clut-hlg-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}
#endif

#if 1
BOOST_AUTO_TEST_CASE(yae_color_transform_hdr10_to_sdr_yuv444)
{
  const Colorspace * csp_hdr10 = Colorspace::get(AVCOL_SPC_BT2020_NCL,
                                                 AVCOL_PRI_BT2020,
                                                 AVCOL_TRC_SMPTEST2084);
  BOOST_CHECK(!!csp_hdr10);

  const Colorspace * csp_sdr = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!csp_sdr);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_P010,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_YUV444P,
                                 AVCOL_RANGE_MPEG));

  ToneMapPiecewise tone_map(10000, 100);

  ColorTransform lut3d(7);
  lut3d.fill(*csp_hdr10,
             *csp_sdr,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  const unsigned int log2_w = lut3d.log2_edge_ + (lut3d.log2_edge_ + 1) / 2;
  const unsigned int log2_h = lut3d.log2_edge_ * 3 - log2_w;

  const unsigned int clut_h = 1 << log2_h;
  const unsigned int clut_w = 1 << log2_w;

  AvFrm frm = make_avfrm(AV_PIX_FMT_YUV444P,
                         clut_w,
                         clut_h,
                         csp_sdr->av_csp_,
                         csp_sdr->av_pri_,
                         csp_sdr->av_trc_,
                         AVCOL_RANGE_MPEG);

  AVFrame & frame = frm.get();
  for (unsigned int i = 0; i < clut_h; i++)
  {
    for (unsigned int j = 0; j < clut_w; j++)
    {
      const unsigned int offset = i * clut_w + j;
      if (offset >= lut3d.size_3d_)
      {
        break;
      }

      const ColorTransform::Pixel & pixel = lut3d.at(offset);

      unsigned char * dst_y = frame.data[0] + frame.linesize[0] * i + j;
      unsigned char * dst_u = frame.data[1] + frame.linesize[1] * i + j;
      unsigned char * dst_v = frame.data[2] + frame.linesize[2] * i + j;

      *dst_y = (unsigned char)(255.0 * pixel.data_[0]);
      *dst_u = (unsigned char)(255.0 * pixel.data_[1]);
      *dst_v = (unsigned char)(255.0 * pixel.data_[2]);
    }
  }

  std::string fn_prefix = "/tmp/clut-hdr10-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}
#endif

#if 1
BOOST_AUTO_TEST_CASE(yae_color_transform_hdr10_to_sdr_rgb24)
{
  const Colorspace * csp_hdr10 = Colorspace::get(AVCOL_SPC_BT2020_NCL,
                                               AVCOL_PRI_BT2020,
                                               AVCOL_TRC_SMPTEST2084);
  BOOST_CHECK(!!csp_hdr10);

  const Colorspace * csp_sdr = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!csp_sdr);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_P010,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  ToneMapPiecewise tone_map(10000, 100);

  ColorTransform lut3d(7);
  lut3d.fill(*csp_hdr10,
             *csp_sdr,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  const unsigned int log2_w = lut3d.log2_edge_ + (lut3d.log2_edge_ + 1) / 2;
  const unsigned int log2_h = lut3d.log2_edge_ * 3 - log2_w;

  const unsigned int clut_h = 1 << log2_h;
  const unsigned int clut_w = 1 << log2_w;

  AvFrm frm = make_avfrm(AV_PIX_FMT_RGB24,
                         clut_w,
                         clut_h,
                         csp_sdr->av_csp_,
                         csp_sdr->av_pri_,
                         csp_sdr->av_trc_,
                         AVCOL_RANGE_JPEG);

  AVFrame & frame = frm.get();
  for (unsigned int i = 0; i < clut_h; i++)
  {
    for (unsigned int j = 0; j < clut_w; j++)
    {
      const unsigned int offset = i * clut_w + j;
      if (offset >= lut3d.size_3d_)
      {
        break;
      }

      const ColorTransform::Pixel & pixel = lut3d.at(offset);

      unsigned char * rgb = frame.data[0] + frame.linesize[0] * i + j * 3;

      rgb[0] = (unsigned char)(255.0 * pixel.data_[0]);
      rgb[1] = (unsigned char)(255.0 * pixel.data_[1]);
      rgb[2] = (unsigned char)(255.0 * pixel.data_[2]);
    }
  }

  std::string fn_prefix = "/tmp/clut-hdr10-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}
#endif

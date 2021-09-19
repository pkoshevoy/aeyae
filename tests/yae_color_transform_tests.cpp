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
#include "yae/video/yae_texture_generator.h"

// namespace access:
using namespace yae;


#if 1
//----------------------------------------------------------------
// yae_colorspace_transfer_eotf_oetf
//
BOOST_AUTO_TEST_CASE(yae_colorspace_rgb_to_ypbpr_to_rgb)
{
  const Colorspace * csp = Colorspace::get(AVCOL_SPC_BT709,
                                           AVCOL_PRI_BT709,
                                           AVCOL_TRC_BT709);

  v4x1_t rgb_in = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_out = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr = make_v4x1(0, 0, 0, 1);

  const m4x4_t to_ypbpr = m4x4_t(csp->rgb_to_ypbpr_);
  const m4x4_t to_rgb = m4x4_t(csp->ypbpr_to_rgb_);

  for (int i = 0; i <= 100; i++)
  {
    const double r = double(i) / 100.0;
    rgb_in[0] = r;

    for (int j = 0; j <= 100; j++)
    {
      const double g = double(j) / 100.0;
      rgb_in[1] = g;

      for (int k = 0; k <= 100; k++)
      {
        const double b = double(k) / 100.0;
        rgb_in[2] = b;

        ypbpr = to_ypbpr * rgb_in;
        rgb_out = to_rgb * ypbpr;

        YAE_ASSERT(fabs(rgb_in[0] - rgb_out[0]) < 1e-6);
        YAE_ASSERT(fabs(rgb_in[1] - rgb_out[1]) < 1e-6);
        YAE_ASSERT(fabs(rgb_in[2] - rgb_out[2]) < 1e-6);
      }
    }
  }
}

//----------------------------------------------------------------
// yae_colorspace_transfer_eotf_oetf_xvYCC
//
BOOST_AUTO_TEST_CASE(yae_colorspace_transfer_eotf_oetf_bt709)
{
  const Colorspace::TransferFunc::Context ctx(1000.0);
  const Colorspace * csp = Colorspace::get(AVCOL_SPC_BT709,
                                           AVCOL_PRI_BT709,
                                           AVCOL_TRC_BT709);

  double rgb_in[3];
  double rgb_out[3];
  double rgb_cdm2[3];

  // xvYCC (superset of bt709), supports values outside [0, 1] range
  // including negative value ... I should test that:
  const Colorspace::TransferFunc & transfer =
    get_transfer_func(AVCOL_TRC_BT709);

  for (int j = -20; j <= 120; j++)
  {
    const double s = double(j) / 100.0;
    rgb_in[0] = s;
    rgb_in[1] = s;
    rgb_in[2] = s;

    transfer.eotf_rgb(*csp, ctx, rgb_in, rgb_cdm2);
    transfer.oetf_rgb(*csp, ctx, rgb_cdm2, rgb_out);
    YAE_ASSERT(fabs(rgb_in[0] - rgb_out[0]) < 1e-6);
    YAE_ASSERT(fabs(rgb_in[1] - rgb_out[1]) < 1e-6);
    YAE_ASSERT(fabs(rgb_in[2] - rgb_out[2]) < 1e-6);
  }
}

//----------------------------------------------------------------
// yae_colorspace_transfer_eotf_oetf
//
BOOST_AUTO_TEST_CASE(yae_colorspace_transfer_eotf_oetf_hlg)
{
  const Colorspace::TransferFunc::Context ctx(1000.0);
  const Colorspace * csp = Colorspace::get(AVCOL_SPC_BT2020_NCL,
                                           AVCOL_PRI_BT2020,
                                           AVCOL_TRC_ARIB_STD_B67);

  double rgb_in[3];
  double rgb_out[3];
  double rgb_cdm2[3];

  for (int i = AVCOL_TRC_RESERVED0; i < AVCOL_TRC_NB; i++)
  {
    AVColorTransferCharacteristic av_trc = (AVColorTransferCharacteristic)i;
    const Colorspace::TransferFunc & transfer = get_transfer_func(av_trc);

    for (int j = 0; j <= 100; j++)
    {
      const double s = double(j) / 100.0;
      rgb_in[0] = s;
      rgb_in[1] = s;
      rgb_in[2] = s;

      transfer.eotf_rgb(*csp, ctx, rgb_in, rgb_cdm2);
      transfer.oetf_rgb(*csp, ctx, rgb_cdm2, rgb_out);
      YAE_ASSERT(fabs(rgb_in[0] - rgb_out[0]) < 1e-6);
      YAE_ASSERT(fabs(rgb_in[1] - rgb_out[1]) < 1e-6);
      YAE_ASSERT(fabs(rgb_in[2] - rgb_out[2]) < 1e-6);
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

//----------------------------------------------------------------
// yae_color_transform_hlg_to_sdr_yuv444
//
BOOST_AUTO_TEST_CASE(yae_color_transform_hlg_to_sdr_yuv444)
{
  const Colorspace::TransferFunc::Context src_ctx(1000.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

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

  ToneMapLog tone_map;
  // ToneMapPiecewise tone_map;

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*csp_hlg,
             *csp_sdr,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  AvFrm frm = lut_3d_to_2d_yuv(lut3d, *csp_sdr);

  std::string fn_prefix = "/tmp/clut-hlg-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}

//----------------------------------------------------------------
// yae_color_transform_hdr10_to_sdr_yuv444
//
BOOST_AUTO_TEST_CASE(yae_color_transform_hdr10_to_sdr_yuv444)
{
  const Colorspace::TransferFunc::Context src_ctx(10000.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

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

  ToneMapPiecewise tone_map;

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*csp_hdr10,
             *csp_sdr,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  AvFrm frm = lut_3d_to_2d_yuv(lut3d, *csp_sdr);

  std::string fn_prefix = "/tmp/clut-hdr10-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}

//----------------------------------------------------------------
// yae_color_transform_hdr10_to_sdr_rgb24
//
BOOST_AUTO_TEST_CASE(yae_color_transform_hdr10_to_sdr_rgb24)
{
  const Colorspace::TransferFunc::Context src_ctx(10000.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

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

  ToneMapPiecewise tone_map;
  // ToneMapLog tone_map;

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*csp_hdr10,
             *csp_sdr,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst,
             &tone_map);

  // convert 3D LUT to a 2D CLUT:
  AvFrm frm = lut_3d_to_2d_rgb(lut3d, *csp_sdr);

  std::string fn_prefix = "/tmp/clut-hdr10-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}

//----------------------------------------------------------------
// yae_color_transform_sdr_to_sdr_rgb24
//
BOOST_AUTO_TEST_CASE(yae_color_transform_sdr_to_sdr_rgb24)
{
  const Colorspace::TransferFunc::Context src_ctx(100.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

  const Colorspace * src_csp = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!src_csp);

  const Colorspace * dst_csp = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!dst_csp);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*src_csp,
             *dst_csp,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst);

  // convert 3D LUT to a 2D CLUT:
  AvFrm frm = lut_3d_to_2d_rgb(lut3d, *dst_csp);

  std::string fn_prefix = "/tmp/clut-sdr-to-sdr-";
  BOOST_CHECK(save_as_png(frm, fn_prefix, TTime(1, 30)));
}

//----------------------------------------------------------------
// yae_color_transform_sdr_to_sdr_color_check_ycbcr
//
BOOST_AUTO_TEST_CASE(yae_color_transform_sdr_to_sdr_color_check_ycbcr)
{
  const Colorspace::TransferFunc::Context src_ctx(100.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

  const Colorspace * src_csp = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!src_csp);

  const Colorspace * dst_csp = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!dst_csp);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t ypbpr_to_src;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_src,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*src_csp,
             *dst_csp,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst);

  v4x1_t rgb_expect = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_actual = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr = make_v4x1(0, 0, 0, 1);
  v4x1_t ycbcr = make_v4x1(0, 0, 0, 1);
  double * yuv = ycbcr.begin();

  const m4x4_t to_ypbpr = m4x4_t(src_csp->rgb_to_ypbpr_);
  const m4x4_t to_rgb = m4x4_t(dst_csp->ypbpr_to_rgb_);
  const int n = lut3d.size_1d_ - 1;

  for (int i = 0; i < n; i++)
  {
    const double y = double(i) / lut3d.z2_;
    yuv[0] = y;

    for (int j = 0; j < n; j++)
    {
      const double u = double(j) / lut3d.z2_;
      yuv[1] = u;

      for (int k = 0; k < n; k++)
      {
        const double v = double(k) / lut3d.z2_;
        yuv[2] = v;

        ypbpr = src_to_ypbpr * ycbcr;

        // clip out-of-range values:
        ypbpr[0] = clip(ypbpr[0],  0.0, 1.0);
        ypbpr[1] = clip(ypbpr[1], -0.5, 0.5);
        ypbpr[2] = clip(ypbpr[2], -0.5, 0.5);

        rgb_expect = to_rgb * ypbpr;
        rgb_expect[0] = clip(rgb_expect[0], 0.0, 1.0);
        rgb_expect[1] = clip(rgb_expect[1], 0.0, 1.0);
        rgb_expect[2] = clip(rgb_expect[2], 0.0, 1.0);

        // YAE_BREAKPOINT_IF(i == 1 && j == 0 && k == 69);

        const TColorTransform3f32::TPixel & p000 = lut3d.get_nn(yuv[0],
                                                                yuv[1],
                                                                yuv[2]);
        rgb_actual = v4x1_t(p000);

        YAE_ASSERT(fabs(rgb_expect[0] - rgb_actual[0]) < 1e-6);
        YAE_ASSERT(fabs(rgb_expect[1] - rgb_actual[1]) < 1e-6);
        YAE_ASSERT(fabs(rgb_expect[2] - rgb_actual[2]) < 1e-6);
      }
    }
  }
}

//----------------------------------------------------------------
// yae_color_transform_sdr_to_sdr_color_check_grayscale
//
BOOST_AUTO_TEST_CASE(yae_color_transform_sdr_to_sdr_color_check_grayscale)
{
  const Colorspace::TransferFunc::Context src_ctx(100.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

  const Colorspace * src_csp = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!src_csp);

  const Colorspace * dst_csp = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!dst_csp);

  m4x4_t src_ycbcr_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_ycbcr_to_ypbpr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t src_ypbpr_to_ycbcr;
  BOOST_CHECK(get_ypbpr_to_ycbcr(src_ypbpr_to_ycbcr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  m4x4_t dst_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(dst_to_ypbpr,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  TColorTransform3f32 lut3d(8);
  lut3d.fill(*src_csp,
             *dst_csp,
             src_ctx,
             dst_ctx,
             src_ycbcr_to_ypbpr,
             ypbpr_to_dst);

  v4x1_t rgb_sample = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_expect = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_actual = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr = make_v4x1(0, 0, 0, 1);
  v4x1_t ycbcr = make_v4x1(0, 0, 0, 1);
  double * rgb = rgb_sample.begin();
  double * yuv = ycbcr.begin();

  const m4x4_t to_ypbpr = m4x4_t(src_csp->rgb_to_ypbpr_);
  const m4x4_t from_rgb = m4x4_t(dst_csp->rgb_to_ypbpr_);
  const m4x4_t to_rgb = m4x4_t(dst_csp->ypbpr_to_rgb_);
  const int n = lut3d.size_1d_ - 1;

  for (int i = 0; i < n; i++)
  {
    const double s = double(i) / lut3d.z2_;
    rgb[0] = s;
    rgb[1] = s;
    rgb[2] = s;

    ypbpr = from_rgb * rgb_sample;

    // clip out-of-range values:
    ypbpr[0] = clip(ypbpr[0],  0.0, 1.0);
    ypbpr[1] = clip(ypbpr[1], -0.5, 0.5);
    ypbpr[2] = clip(ypbpr[2], -0.5, 0.5);

    ycbcr = src_ypbpr_to_ycbcr * ypbpr;

    rgb_expect = to_rgb * ypbpr;

    // clip out-of-range values:
    rgb_expect[0] = clip(rgb_expect[0], 0.0, 1.0);
    rgb_expect[1] = clip(rgb_expect[1], 0.0, 1.0);
    rgb_expect[2] = clip(rgb_expect[2], 0.0, 1.0);

    // YAE_BREAKPOINT_IF(i == 1 && j == 0 && k == 69);

    v3x1_t trilinear_approx = lut3d.get(yuv[0], yuv[1], yuv[2]);
    rgb_actual = v4x1_t(trilinear_approx);

    YAE_ASSERT(fabs(rgb_expect[0] - rgb_actual[0]) < 2e-3);
    YAE_ASSERT(fabs(rgb_expect[1] - rgb_actual[1]) < 2e-3);
    YAE_ASSERT(fabs(rgb_expect[2] - rgb_actual[2]) < 2e-3);
  }
}

//----------------------------------------------------------------
// yae_color_transform_sdr_to_sdr_color_check_rgb
//
BOOST_AUTO_TEST_CASE(yae_color_transform_sdr_to_sdr_color_check_rgb)
{
  const Colorspace::TransferFunc::Context src_ctx(100.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

  const Colorspace * src_csp = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!src_csp);

  const Colorspace * dst_csp = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!dst_csp);

  m4x4_t src_ycbcr_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_ycbcr_to_ypbpr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t src_ypbpr_to_ycbcr;
  BOOST_CHECK(get_ypbpr_to_ycbcr(src_ypbpr_to_ycbcr,
                                 AV_PIX_FMT_NV12,
                                 AVCOL_RANGE_JPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  m4x4_t dst_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(dst_to_ypbpr,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  TColorTransform3f32 lut3d(7);
  lut3d.fill(*src_csp,
             *dst_csp,
             src_ctx,
             dst_ctx,
             src_ycbcr_to_ypbpr,
             ypbpr_to_dst);

  v4x1_t rgb_sample = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_expect = make_v4x1(0, 0, 0, 1);
  v4x1_t rgb_actual = make_v4x1(0, 0, 0, 1);
  v4x1_t ypbpr = make_v4x1(0, 0, 0, 1);
  v4x1_t ycbcr = make_v4x1(0, 0, 0, 1);
  double * rgb = rgb_sample.begin();
  double * yuv = ycbcr.begin();

  const m4x4_t to_ypbpr = m4x4_t(src_csp->rgb_to_ypbpr_);
  const m4x4_t from_rgb = m4x4_t(dst_csp->rgb_to_ypbpr_);
  const m4x4_t to_rgb = m4x4_t(dst_csp->ypbpr_to_rgb_);
  const int n = lut3d.size_1d_ - 1;

  for (int i = 0; i < n; i++)
  {
    const double r = double(i) / lut3d.z2_;
    rgb[0] = r;

    for (int j = 0; j < n; j++)
    {
      const double g = double(j) / lut3d.z2_;
      rgb[1] = g;

      for (int k = 0; k < n; k++)
      {
        const double b = double(k) / lut3d.z2_;
        rgb[2] = b;

        ypbpr = from_rgb * rgb_sample;

        // clip out-of-range values:
        ypbpr[0] = clip(ypbpr[0],  0.0, 1.0);
        ypbpr[1] = clip(ypbpr[1], -0.5, 0.5);
        ypbpr[2] = clip(ypbpr[2], -0.5, 0.5);

        ycbcr = src_ypbpr_to_ycbcr * ypbpr;

        rgb_expect = to_rgb * ypbpr;

        // clip out-of-range values:
        rgb_expect[0] = clip(rgb_expect[0], 0.0, 1.0);
        rgb_expect[1] = clip(rgb_expect[1], 0.0, 1.0);
        rgb_expect[2] = clip(rgb_expect[2], 0.0, 1.0);

        // YAE_BREAKPOINT_IF(i == 1 && j == 0 && k == 69);

        v3x1_t trilinear_approx = lut3d.get(yuv[0], yuv[1], yuv[2]);
        rgb_actual = v4x1_t(trilinear_approx);

        YAE_ASSERT(fabs(rgb_expect[0] - rgb_actual[0]) < 4e-3);
        YAE_ASSERT(fabs(rgb_expect[1] - rgb_actual[1]) < 4e-3);
        YAE_ASSERT(fabs(rgb_expect[2] - rgb_actual[2]) < 4e-3);
      }
    }
  }
}
#endif
#if 1
//----------------------------------------------------------------
// yae_color_transform_yuv_to_rgb_colorbars
//
BOOST_AUTO_TEST_CASE(yae_color_transform_yuv_to_rgb_colorbars)
{
  const Colorspace::TransferFunc::Context src_ctx(100.0);
  const Colorspace::TransferFunc::Context dst_ctx(100.0);

  const Colorspace * src_csp = Colorspace::get(AVCOL_SPC_BT709,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!src_csp);

  const Colorspace * dst_csp = Colorspace::get(AVCOL_SPC_RGB,
                                               AVCOL_PRI_BT709,
                                               AVCOL_TRC_BT709);
  BOOST_CHECK(!!dst_csp);

  m4x4_t src_to_ypbpr;
  BOOST_CHECK(get_ycbcr_to_ypbpr(src_to_ypbpr,
                                 AV_PIX_FMT_YUV444P,
                                 AVCOL_RANGE_MPEG));

  m4x4_t ypbpr_to_dst;
  BOOST_CHECK(get_ypbpr_to_ycbcr(ypbpr_to_dst,
                                 AV_PIX_FMT_RGB24,
                                 AVCOL_RANGE_JPEG));

  TColorTransform3f32 lut3d(6);
  lut3d.fill(*src_csp,
             *dst_csp,
             src_ctx,
             dst_ctx,
             src_to_ypbpr,
             ypbpr_to_dst);

  const int w = 1280;
  const int h = 720;
  ColorbarsGenerator tex_gen(w, h, src_csp);

  AvFrm yuv_frm = make_textured_frame(tex_gen, AV_PIX_FMT_YUV444P, w, h);
  const AVFrame & yuv_frame = yuv_frm.get();

  std::string fn_prefix_yuv = "/tmp/colorbars-source-";
  BOOST_CHECK(save_as_png(yuv_frm, fn_prefix_yuv));

  AvFrm rgb_frm = make_avfrm(AV_PIX_FMT_RGB24,
                             w,
                             h,
                             AVCOL_SPC_RGB,
                             AVCOL_PRI_BT709,
                             AVCOL_TRC_BT709,
                             AVCOL_RANGE_JPEG);
  const AVFrame & rgb_frame = rgb_frm.get();

  // copy data from YUV frame to the RGB frame,
  // using the LUT to convert from from YUV to RGB:
  for (int i = 0; i < h; i++)
  {
    const uint8_t * y_data = yuv_frame.data[0] + i * yuv_frame.linesize[0];
    const uint8_t * u_data = yuv_frame.data[1] + i * yuv_frame.linesize[1];
    const uint8_t * v_data = yuv_frame.data[2] + i * yuv_frame.linesize[2];
    uint8_t * rgb_data = rgb_frame.data[0] + i * rgb_frame.linesize[0];

    for (int j = 0; j < w; j++, y_data++, u_data++, v_data++, rgb_data += 3)
    {
      double y = double(*y_data) / 255.0;
      double u = double(*u_data) / 255.0;
      double v = double(*v_data) / 255.0;

      v3x1_t rgb = lut3d.get(y, u, v);
      rgb_data[0] = uint8_t(rgb[0] * 255.0);
      rgb_data[1] = uint8_t(rgb[1] * 255.0);
      rgb_data[2] = uint8_t(rgb[2] * 255.0);
    }
  }

  std::string fn_prefix_rgb = "/tmp/colorbars-output-";
  BOOST_CHECK(save_as_png(rgb_frm, fn_prefix_rgb));
}
#endif

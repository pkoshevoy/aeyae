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
#include "yae/video/yae_color_transform.h"

// namespace access:
using namespace yae;


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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Oct 27 10:48:32 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_ransac.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <inttypes.h>
#include <limits>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/test/unit_test.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// shortcut:
using namespace yae;


BOOST_AUTO_TEST_CASE(yae_bitmask_width)
{
  BOOST_CHECK_EQUAL(0, bitmask_width(0));
  BOOST_CHECK_EQUAL(1, bitmask_width(1));
  BOOST_CHECK_EQUAL(2, bitmask_width(2));
  BOOST_CHECK_EQUAL(2, bitmask_width(3));
  BOOST_CHECK_EQUAL(12, bitmask_width(3 << 10));
  BOOST_CHECK_EQUAL(14, bitmask_width(0xFF << 6));
  BOOST_CHECK_EQUAL(64, bitmask_width(255ull + (255ull << 56)));
  BOOST_CHECK_EQUAL(57, bitmask_width(1ull << 56));
  BOOST_CHECK_EQUAL(32, bitmask_width(0xFFFFFFFF));
  BOOST_CHECK_EQUAL(64, bitmask_width(std::numeric_limits<uint64_t>::max()));
}


BOOST_AUTO_TEST_CASE(yae_get_po2_size)
{
  BOOST_CHECK(yae::get_po2_size(0) == 1);
  BOOST_CHECK(yae::get_po2_size(1) == 1);
  BOOST_CHECK(yae::get_po2_size(2) == 2);
  BOOST_CHECK(yae::get_po2_size(3) == 4);
  BOOST_CHECK(yae::get_po2_size(4) == 4);
  BOOST_CHECK(yae::get_po2_size(5) == 8);
  BOOST_CHECK(yae::get_po2_size(6) == 8);
  BOOST_CHECK(yae::get_po2_size(7) == 8);
  BOOST_CHECK(yae::get_po2_size(8) == 8);
  BOOST_CHECK(yae::get_po2_size(9) == 16);
  BOOST_CHECK(yae::get_po2_size(4095) == 4096);
  BOOST_CHECK(yae::get_po2_size(4096) == 4096);
}


BOOST_AUTO_TEST_CASE(yae_replace)
{
  std::string src = "  func_b                                :        1  call";
  std::string out = yae::replace(src, "  ", " ");
  BOOST_CHECK(out == " func_b : 1 call");
}


BOOST_AUTO_TEST_CASE(yae_ransac)
{
  std::vector<int> dataset(1000);

  // generate a dataset with 50% outliers:
  for (int i = 0, n = dataset.size(); i < n; ++i)
  {
    dataset[i] = (i % 2) ? 768 : i;
  }

  typedef yae::RANSAC<int> ransac_t;
  ransac_t::Median model;
  ransac_t::TSubSet bestfit;

  double fit_error_threshold = 1.0;
  double bestfit_err_avg =
    ransac_t(&dataset[0], dataset.size()).
    find_inliers(model, fit_error_threshold, bestfit);

  BOOST_CHECK(bestfit_err_avg < fit_error_threshold);
  model.reset(bestfit);

  int mean = int(model.median_ + 0.5);
  BOOST_CHECK(mean == 768);
}


BOOST_AUTO_TEST_CASE(yae_time_arithmetic)
{
  yae::TTime x0(2048, 48000);
  yae::TTime x1(6144, 48000);

  yae::TTime x0_mul_x1 = (x0 * x1);
  BOOST_CHECK(x0_mul_x1.base_ <= 48000);
  BOOST_CHECK(x0_mul_x1.get(48000) == 262);

  yae::TTime x1_div_x0 = x1 / x0;
  BOOST_CHECK(x1_div_x0.time_ == 3 && x1_div_x0.base_ == 1);

  yae::TTime x0_div_x1 = x0 / x1;
  BOOST_CHECK(x0_div_x1.time_ == 1 && x0_div_x1.base_ == 3);


  yae::TTime a(22276, 48000);
  yae::TTime b(164784, 48000);
  yae::TTime c(163200, 48000);
  std::size_t n = 10;
  std::size_t m = 10;

  yae::TTime t = (n * c + n * b - m * a) / (n * b + m * b);
  BOOST_CHECK(t.time_ == 76427 && t.base_ == 82392);

  yae::TTime t0(2246540, 48000);
  yae::TTime t1 = t0 + (a + b) + b * (t - yae::TTime(1, 1));
  BOOST_CHECK(t1.time_ == 2421670 && t1.base_ == 48000);
  BOOST_CHECK(t1.get(1000) == 50451);
}

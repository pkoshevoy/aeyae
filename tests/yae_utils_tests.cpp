// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Oct 27 10:48:32 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Oct 27 10:48:32 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard lib:
#include <inttypes.h>
#include <limits>

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/utils/yae_utils.h"

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

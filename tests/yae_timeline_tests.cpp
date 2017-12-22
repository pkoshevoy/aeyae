// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 16:34:55 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/utils/yae_time.h"

// shortcut:
using namespace yae;


BOOST_AUTO_TEST_CASE(yae_timeline)
{
  std::string track_id("test");

  TTime t1(100, 1000);
  TTime t2(200, 1000);
  TTime t3(300, 1000);
  TTime t4(400, 1000);
  TTime t5(500, 1000);
  TTime t6(600, 1000);

  Timespan s12(t1, t2);
  Timespan s23(t2, t3);
  Timespan s46(t4, t6);
  Timespan s25(t2, t5);

  Timeline timeline;
  BOOST_CHECK(timeline.extend(track_id, s12));
  BOOST_CHECK(timeline.tracks_[track_id].size() == 1);

  // shortcut:
  const std::list<Timespan> & track = timeline.tracks_[track_id];

  // this should extend the last timespan:
  BOOST_CHECK(timeline.extend(track_id, s23));
  BOOST_CHECK(track.size() == 1);

  // this should append a new timespan:
  BOOST_CHECK(timeline.extend(track_id, s46));
  BOOST_CHECK(track.size() == 2);

  Timespan bbox = timeline.bbox(track_id);
  BOOST_CHECK(bbox.t0_ == t1);
  BOOST_CHECK(bbox.t1_ == t6);

  // non-monotonically increasing time is not allowed by default:
  BOOST_CHECK(!timeline.extend(track_id, s25));
  BOOST_CHECK(track.size() == 2);

  // force merging of overlapping timespans:
  BOOST_CHECK(timeline.extend(track_id, s25, 0.0, false));

  BOOST_CHECK(track.size() == 1);
  BOOST_CHECK(track.front().t0_ == t1);
  BOOST_CHECK(track.front().t1_ == t6);
}

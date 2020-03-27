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
#include "yae/utils/yae_utils.h"

// shortcut:
using namespace yae;


BOOST_AUTO_TEST_CASE(yae_time)
{
  BOOST_CHECK_LT(TTime(0, 1), TTime(0.9));
  BOOST_CHECK_GT(TTime(0.9), TTime(0, 1));

  BOOST_CHECK_LE(TTime(0, 1001), TTime(0.0));
  BOOST_CHECK_GE(TTime(0.0), TTime(0, 1001));

  BOOST_CHECK_LE(TTime(1, 1001), TTime(0.001));
  BOOST_CHECK_GE(TTime(0.001), TTime(1, 1001));

  TTime t1(100, 1000);
  TTime t2(200, 1000);
  TTime t3(300, 1000);
  TTime t4(400, 1000);
  TTime t5(500, 1000);
  TTime t6(600, 1000);

  BOOST_CHECK_EQUAL(t1.rebased(30).time_, 3);
  BOOST_CHECK_EQUAL(t2, t1 + t1);
  BOOST_CHECK_EQUAL(t1, t2 - t1);
  BOOST_CHECK_EQUAL(-t2, t4 - t6);

  BOOST_CHECK_LT(t1, t2);
  BOOST_CHECK_GT(t2, t1);
  BOOST_CHECK_LT(-t1, t1);
  BOOST_CHECK_GT(t1, -t1);

  TTime s1(1, 1001);
  BOOST_CHECK_LE(t1, t1 + s1);


  TTime t(1 + 30 * (1 + 60 * (1 + 60)), 30);
  std::string tc = t.to_hhmmss_ff(29.97, ":", ";");
  BOOST_CHECK_EQUAL(tc, std::string("01:01:01;01"));

  std::string ms = t.to_hhmmss_ms();
  BOOST_CHECK_EQUAL(ms, std::string("01:01:01.033"));

  TTime za(std::numeric_limits<int64_t>::max(), 1);
  TTime zb(-2000, 1000);
  TTime zc(std::numeric_limits<int64_t>::min(), 2);
  BOOST_CHECK_GT(za, zb);
  BOOST_CHECK_LT(zb, za);
  BOOST_CHECK_LT(zc, za);
  BOOST_CHECK_LT(zc, zb);

  TTime apb = za + zb;
  BOOST_CHECK_EQUAL(apb.time_, std::numeric_limits<int64_t>::max() - 2);

  TTime cmb = zc - zb;
  BOOST_CHECK_LT(zc, cmb);

  TTime cmcpb = zc - cmb;
  BOOST_CHECK_EQUAL(cmcpb, zb);

  TTime apc = za + zc;
  BOOST_CHECK_EQUAL(apc.time_, std::numeric_limits<int64_t>::max() / 2);
}

BOOST_AUTO_TEST_CASE(yae_time_now)
{
  TTime now = TTime::now();

  struct tm tm_utc;
  struct tm tm_local;

  yae::unix_epoch_time_to_utc(now.get(1), tm_utc);
  yae::unix_epoch_time_to_localtime(now.get(1), tm_local);

  int64_t t_utc = yae::utc_to_unix_epoch_time(tm_utc);
  int64_t t_local = yae::localtime_to_unix_epoch_time(tm_local);

  BOOST_CHECK_EQUAL(now.get(1), t_utc);
  BOOST_CHECK_EQUAL(now.get(1), t_local);

  int64_t t_now = yae::unix_epoch_time_at_utc_time(tm_utc.tm_year + 1900,
                                                   tm_utc.tm_mon + 1,
                                                   tm_utc.tm_mday,
                                                   tm_utc.tm_hour,
                                                   tm_utc.tm_min,
                                                   tm_utc.tm_sec);
  BOOST_CHECK_EQUAL(now.get(1), t_now);
}

BOOST_AUTO_TEST_CASE(yae_parse_time)
{
  TTime t;
  BOOST_CHECK(parse_time(t, "0s"));
  BOOST_CHECK_EQUAL(t.time_, 0);
  BOOST_CHECK_EQUAL(t.base_, 1);

  BOOST_CHECK(parse_time(t, "1m"));
  BOOST_CHECK_EQUAL(t.time_, 60);
  BOOST_CHECK_EQUAL(t.base_, 1);

  BOOST_CHECK(parse_time(t, "1m 1s"));
  BOOST_CHECK_EQUAL(t.time_, 61);
  BOOST_CHECK_EQUAL(t.base_, 1);

  BOOST_CHECK(parse_time(t, "-5s100ms"));
  BOOST_CHECK_EQUAL(t.time_, -5100);
  BOOST_CHECK_EQUAL(t.base_, 1000);

  BOOST_CHECK(parse_time(t, "-1.1"));
  BOOST_CHECK_EQUAL(t.time_, -11);
  BOOST_CHECK_EQUAL(t.base_, 10);

  BOOST_CHECK(parse_time(t, "00:00:00;01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.time_, 1001);
  BOOST_CHECK_EQUAL(t.base_, 30000);

  BOOST_CHECK(parse_time(t, "-00:00:00;01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.time_, -1001);
  BOOST_CHECK_EQUAL(t.base_, 30000);

  BOOST_CHECK(parse_time(t, "00:00:00.001", ":", "."));
  BOOST_CHECK_EQUAL(t.time_, 1);
  BOOST_CHECK_EQUAL(t.base_, 1000);

  BOOST_CHECK(parse_time(t, "01:01:01:01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.time_, 1001 + 30000 * (1 + 60 * (1 + 60)));
  BOOST_CHECK_EQUAL(t.base_, 30000);

  BOOST_CHECK(parse_time(t, "01:01:01.01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.time_, 1 + 100 * (1 + 60 * (1 + 60)));
  BOOST_CHECK_EQUAL(t.base_, 100);

  // 1s
  BOOST_CHECK(parse_time(t, "1", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.get(1000), 1000);

  // 1.1s
  BOOST_CHECK(parse_time(t, "1.1", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.get(1000), 1100);

  // 1m 1s
  BOOST_CHECK(parse_time(t, "01:01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.get(1000), 61000);

  // 1h 1m 1s
  BOOST_CHECK(parse_time(t, "01:01:01", NULL, NULL, 30000.0 / 1001.0));
  BOOST_CHECK_EQUAL(t.get(1000), 1000 * (1 + 60 * (1 + 60)));

  // 20:53:28.57
  BOOST_CHECK(parse_time(t, "20:53:28.57", ":", "."));
  BOOST_CHECK_EQUAL(100, t.base_);

  int64_t x = t.time_;
  int64_t cs = x % 100;
  BOOST_CHECK_EQUAL(57, cs);

  x /= 100;
  int64_t sec = x % 60;
  BOOST_CHECK_EQUAL(28, sec);

  x /= 60;
  int64_t min = x % 60;
  BOOST_CHECK_EQUAL(53, min);

  x /= 60;
  BOOST_CHECK_EQUAL(20, x);

  std::string str = t.to_hhmmss_ms();
  BOOST_CHECK(str == "20:53:28.570");
}

BOOST_AUTO_TEST_CASE(yae_timeline)
{
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

  std::list<Timespan> track;
  BOOST_CHECK(extend(track, s12));
  BOOST_CHECK(track.size() == 1);

  // shortcut:
  // this should extend the last timespan:
  BOOST_CHECK(extend(track, s23));
  BOOST_CHECK(track.size() == 1);

  // this should append a new timespan:
  BOOST_CHECK(extend(track, s46));
  BOOST_CHECK(track.size() == 2);

  Timespan bbox = yae::bbox(track);
  BOOST_CHECK(bbox.t0_ == t1);
  BOOST_CHECK(bbox.t1_ == t6);

  // non-monotonically increasing time is not allowed by default:
  BOOST_CHECK(!extend(track, s25));
  BOOST_CHECK(track.size() == 2);

  // force merging of overlapping timespans:
  BOOST_CHECK(extend(track, s25, 0.0, false));

  BOOST_CHECK(track.size() == 1);
  BOOST_CHECK(track.front().t0_ == t1);
  BOOST_CHECK(track.front().t1_ == t6);
}

BOOST_AUTO_TEST_CASE(yae_timeline_track)
{
  const std::string track_id("test");
  Timeline timeline;

  //
  //    PTS:  0   3   1   2   6   4   5   9   7   8  12  10  11  13
  //
  //   TYPE:  I   P   B   B   P   B   B   P   B   B   P   B   B   I
  //
  //    DTS: -2  -1   0   1   2   3   4   5   6   7   8   9  10  11
  //
  // SAMPLE:  0   1   2   3   4   5   6   7   8   9  10  11  12  13
  //
  for (int i = 0; i < 52; i++)
  {
    const char f =
      (i % 13) == 0 ? 'I' :
      (i % 3) == 1 ? 'P' :
      'B';

    const int pts =
      (f == 'I') ? i :
      (f == 'P') ? i + 2 :
      i - 1;

    const int dts = i - 2;
    const bool keyframe = (f == 'I');

    timeline.add_packet(track_id,
                        keyframe,
                        1000,
                        TTime(dts, 1),
                        TTime(pts, 1),
                        TTime(1, 1),
                        0.0);
  }

  // shortcut:
  const Timeline::Track & tt = yae::at(timeline.tracks_, track_id);

  //
  // [0.9,  5.1), samples:  0  1  2  3  4  5
  //
  // [5.1, 10.1), samples:  0  1  2  3  4  5  6  7  8  9
  //

  std::size_t ka = std::numeric_limits<std::size_t>::max();
  std::size_t kb = std::numeric_limits<std::size_t>::max();
  std::size_t kc = std::numeric_limits<std::size_t>::max();
  std::size_t kd = std::numeric_limits<std::size_t>::max();
  std::size_t ia = std::numeric_limits<std::size_t>::max();
  std::size_t ib = std::numeric_limits<std::size_t>::max();

  Timespan pts_span(TTime(0.9), TTime(5.1));
  BOOST_CHECK(tt.find_samples_for(pts_span, ka, kb, kc, kd, ia, ib));
  BOOST_CHECK_EQUAL(ka, 0);
  BOOST_CHECK_EQUAL(kb, 13);
  BOOST_CHECK_EQUAL(kc, 0);
  BOOST_CHECK_EQUAL(kd, 13);
  BOOST_CHECK_EQUAL(ia, 0);
  BOOST_CHECK_EQUAL(ib, 5);

  pts_span.reset(TTime(5.1), TTime(10.1));
  BOOST_CHECK(tt.find_samples_for(pts_span, ka, kb, kc, kd, ia, ib));
  BOOST_CHECK_EQUAL(ka, 0);
  BOOST_CHECK_EQUAL(kb, 13);
  BOOST_CHECK_EQUAL(kc, 0);
  BOOST_CHECK_EQUAL(kd, 13);
  BOOST_CHECK_EQUAL(ia, 6);
  BOOST_CHECK_EQUAL(ib, 9);

  pts_span.reset(TTime(10.1), TTime(18.5));
  BOOST_CHECK(tt.find_samples_for(pts_span, ka, kb, kc, kd, ia, ib));
  BOOST_CHECK_EQUAL(ka, 0);
  BOOST_CHECK_EQUAL(kb, 13);
  BOOST_CHECK_EQUAL(kc, 13);
  BOOST_CHECK_EQUAL(kd, 26);
  BOOST_CHECK_EQUAL(ia, 11);
  BOOST_CHECK_EQUAL(ib, 18);
}

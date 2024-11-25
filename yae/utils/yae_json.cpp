// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Dec 15 14:25:18 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_json.h"
#include "yae/utils/yae_time.h"


namespace yae
{

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const struct tm & tm)
  {
    uint64_t ts = yae::localtime_to_unix_epoch_time(tm);
    save(json, ts);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, struct tm & tm)
  {
    uint64_t ts = 0;
    load(json, ts);
    yae::unix_epoch_time_to_localtime(ts, tm);
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const yae::TTime & t)
  {
    save(json["time"], t.time_);
    save(json["base"], t.base_);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, yae::TTime & t)
  {
    load(json["time"], t.time_);
    load(json["base"], t.base_);
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const yae::Timespan & timespan)
  {
    save(json["t0"], timespan.t0_);
    save(json["t1"], timespan.t1_);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, yae::Timespan & timespan)
  {
    load(json["t0"], timespan.t0_);
    load(json["t1"], timespan.t1_);
  }

}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 21 20:34:33 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/api/yae_settings_interface.h"
#include "yae/api/yae_settings.h"

using namespace yae;


BOOST_AUTO_TEST_CASE(yae_settings)
{
  const char * onoffId = "some-nasty-guid";
  const char * onoffLabel = "On/Off";
  const char * onoffUnits = "n/a";
  const char * onoffTooltip = "This is a tooltip";
  const char * onoffSummary = "A brief explanation of the setting";
  TSettingBool onoff(onoffId);
  BOOST_CHECK(strcmp(onoff.id(), onoffId) == 0);
  BOOST_CHECK(!onoff.traits().value());

  onoff.traits().setValue(true);
  BOOST_CHECK(onoff.traits().value());

  BOOST_CHECK(onoff.optional());
  onoff.setOptional(false);

  BOOST_CHECK(!onoff.optional());
  BOOST_CHECK(!onoff.specified());

  onoff.
    setOptional(true).
    setSpecified(true).
    setLabel(onoffLabel).
    setUnits(onoffUnits).
    setTooltip(onoffTooltip).
    setSummary(onoffSummary);
  BOOST_CHECK(onoff.optional());
  BOOST_CHECK(onoff.specified());
  BOOST_CHECK(strcmp(onoff.label(), onoffLabel) == 0);
  BOOST_CHECK(strcmp(onoff.units(), onoffUnits) == 0);
  BOOST_CHECK(strcmp(onoff.tooltip(), onoffTooltip) == 0);
  BOOST_CHECK(strcmp(onoff.summary(), onoffSummary) == 0);

  TSettingGroup group("all_settings", "All Settings");
  group.traits().addSetting(&onoff);
  BOOST_CHECK(group.traits().size() == 1);
  BOOST_CHECK(group.traits().setting(0) == &onoff);
}

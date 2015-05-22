// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 21 20:34:33 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#define BOOST_TEST_MODULE "aeyae test suite"
#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

#include <aeyae_settings_interface.hxx>
#include <aeyae_settings.hxx>

using namespace yae;

BOOST_AUTO_TEST_CASE( aeyae_settings )
{
  TSettingBool onoff("test_toggle_id");
  BOOST_CHECK(!onoff.attributes().isOptionalSetting());

  onoff.
    setOptional(true).
    setLabel("Hello World").
    setTooltip("This should be a checkbox").
    setSummary("Summary explanation of the setting");
  BOOST_CHECK(onoff.attributes().isOptionalSetting());

  TSettingGroup group("all_settings", "All Settings");
  group.traits().addSetting(&onoff);
  BOOST_CHECK(group.traits().size() == 1);
  BOOST_CHECK(group.traits().setting(0) == &onoff);
}

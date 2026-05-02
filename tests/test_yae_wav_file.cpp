// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Apr 22 21:44:28 MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <cmath>

// aeyae:
#include "yae/utils/yae_utils.h"
#include "yae/utils/yae_wav_file.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// shortcut:
namespace fs = boost::filesystem;


BOOST_AUTO_TEST_CASE(yae_wav_file)
{
  std::string exe_folder_path_utf8;
  BOOST_CHECK(yae::get_current_executable_folder(exe_folder_path_utf8));

  // create a sample file:
  fs::path wav_path = fs::path(exe_folder_path_utf8) / "stereo_s16.wav";
  {
    yae::WavFile wav;
    BOOST_REQUIRE(wav.open(wav_path.string(), 2, 48000, 16));

    // generate an stereo signal:
    static const double two_pi = M_PI * 2.0;
    uint16_t sample[2];

    for (int i = 0, n = 48000 * 60; i < n; i += 1)
    {
      double s = double(i) / 36; // 1320Hz
      double t = double(i) / 120; // 400Hz
      double p = double(i) / 96000; // 0.5Hz
      double q = double(i) / 160000; // 0.3Hz
      sample[0] =
        yae::ntol_16<int16_t>(32767 * sin(two_pi * s) * sin(two_pi * p));
      sample[1] =
        yae::ntol_16<int16_t>(32767 * sin(two_pi * t) * sin(two_pi * q));
      wav.save(1, sample);
    }
  }

  // read the file:
  {
    yae::WavFileReader wav;
    BOOST_REQUIRE(wav.open(wav_path.string()));

    BOOST_CHECK(wav.audio_format_ == yae::WavFileReader::kPCM_integer);
    BOOST_CHECK(wav.num_channels_ == 2);
    BOOST_CHECK(wav.sample_rate_ == 48000);
    BOOST_CHECK(wav.bytes_per_block_ == 4);
    BOOST_CHECK(wav.bytes_per_sec_ == 48000 * 4);
    BOOST_CHECK(wav.bits_per_sample_ == 16);
    BOOST_CHECK(wav.get_dur() == 48000 * 60);

    wav.rewind();
    BOOST_CHECK(wav.get_pts() == 0);

    wav.seek_to(48000);
    BOOST_CHECK(wav.get_pts() == 48000);

    yae::Data samples;
    BOOST_CHECK(wav.load_frame(samples, 1536) == 1536);
    BOOST_CHECK(wav.get_pts() == 48000 + 1536);
  }
}

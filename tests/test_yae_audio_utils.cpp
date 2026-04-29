// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 25 01:20:48 PM MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <cmath>

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/utils/yae_utils.h"
#include "yae/utils/yae_wav_file.h"
#include "yae/video/yae_audio_utils.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// shortcuts:
namespace al = boost::algorithm;
namespace fs = boost::filesystem;
using yae::generate_stereo_f32;
using yae::s16_to_max_amp;
using yae::s16_to_mono;
using yae::s16_mono_downsample;
using yae::s16_to_f32;
using yae::f32_to_s16;
using yae::draw_wav_s16_mono;
using yae::draw_wav_amp;
using yae::draw_wav_overlap;


//----------------------------------------------------------------
// yae_audio_utils
//
BOOST_AUTO_TEST_CASE(yae_audio_utils)
{
  std::string exe_folder_path_utf8;
  BOOST_CHECK(yae::get_current_executable_folder(exe_folder_path_utf8));
  fs::path results_dir = fs::path(exe_folder_path_utf8);

  // create a sample file:
  std::string wav_path = (results_dir/"stereo_f32.wav").string();
  yae::WavFileReader stereo_f32 =
    yae::generate_stereo_f32(48000 * 60, // num_samples
                             48000, // sample_rate
                             1320, // ch0 Hz
                             400, // ch1 Hz
                             0.5, // ch0 modulate Hz
                             0.3); // ch1 modulate Hz
  BOOST_CHECK(stereo_f32.save(wav_path));

  yae::WavFileReader wav(wav_path);
  BOOST_CHECK_EQUAL(stereo_f32.get_dur(), wav.get_dur());

  yae::TAudioFrame a_f32;
  wav.seek_to(15360);
  yae::load(wav, a_f32, 4096);
  BOOST_CHECK_EQUAL(15360, a_f32.time_.get(48000));

  yae::TAudioFrame b_f32;
  wav.seek_to(15360 + 1024);
  yae::load(wav, b_f32, 4096);
  BOOST_CHECK_EQUAL(15360 + 1024, b_f32.time_.get(48000));

  yae::Data a_s16 = f32_to_s16(a_f32.get_data(), 32767.f);
  yae::Data b_s16 = f32_to_s16(b_f32.get_data(), 32767.f);

  // convert to mono:
  yae::Data a_mono_s16 = s16_to_mono(a_s16, 2);
  yae::Data b_mono_s16 = s16_to_mono(b_s16, 2);

  // downsample to 12kHz:
  while (a_mono_s16.num<int16_t>() > 1024)
  {
    a_mono_s16 = s16_mono_downsample(a_mono_s16);
    b_mono_s16 = s16_mono_downsample(b_mono_s16);
  }

  // save both fragments as PNG
  yae::AvFrm a_frm = draw_wav_s16_mono(a_mono_s16);
  yae::AvFrm b_frm = draw_wav_s16_mono(b_mono_s16);

  std::string a_pfx = (results_dir/"wav-a-").string();
  std::string b_pfx = (results_dir/"wav-b-").string();

  BOOST_CHECK(yae::save_as_png(a_frm, a_pfx, a_f32.duration()));
  BOOST_CHECK(yae::save_as_png(b_frm, b_pfx, b_f32.duration()));

  yae::AvFrm ab_frm = draw_wav_overlap(a_mono_s16, b_mono_s16, -256);
  std::string ab_pfx = (results_dir/"overlap-").string();
  BOOST_CHECK(yae::save_as_png(ab_frm, ab_pfx, a_f32.duration()));

  // create 2 copies of the waveform:
  yae::WavFileReader wav_a = wav;
  yae::WavFileReader wav_b = wav;

  // clip wav_a from the back, and wav_b from the front
  // by the same number of samples:
  // int misalignment = 1234;
  int misalignment = 5678;
  wav_a.data_start_byte_pos_ += wav_a.bytes_per_block_ * misalignment;
  wav_b.sample_data_size_ -= wav_b.bytes_per_block_ * misalignment;

  double avg_diff = std::numeric_limits<double>::max();
  int64_t offset = yae::find_alignment_offset(wav_a, wav_b, 1024, avg_diff);
  BOOST_CHECK_EQUAL(misalignment, offset);
#if 1
  // clip wav_a from the back, and wav_b from the front
  // by the same number of samples:
  wav_a = wav;
  wav_b = wav;

  wav_a.sample_data_size_ -= wav_a.bytes_per_block_ * misalignment;
  wav_b.data_start_byte_pos_ += wav_b.bytes_per_block_ * misalignment;

  avg_diff = std::numeric_limits<double>::max();
  offset = yae::find_alignment_offset(wav_a, wav_b, 1024, avg_diff);
  BOOST_CHECK_EQUAL(misalignment, -offset);
#endif
}

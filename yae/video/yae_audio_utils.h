// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Apr 22 21:44:28 MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_UTILS_H_
#define YAE_AUDIO_UTILS_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_utils.h"
#include "yae/utils/yae_wav_file.h"
#include "yae/video/yae_video.h"

namespace yae
{

  //----------------------------------------------------------------
  // generate_stereo_f32
  //
  YAE_API yae::WavFileReader
  generate_stereo_f32(std::size_t num_samples,
                      int sample_rate,
                      float ch0_tone_hz,
                      float ch1_tone_hz,
                      float ch0_modulate_hz = 0.f,
                      float ch1_modulate_hz = 0.f);


  //----------------------------------------------------------------
  // to_s16
  //
  YAE_API yae::TAudioFrame to_s16(const yae::TAudioFrame & src);

  //----------------------------------------------------------------
  // s16_to_max_amp
  //
  // returns mono out[i] = src[max(abs(amp[i, ch]))]
  // where i is sample index, and ch is channel index
  //
  // this is mostly useful for visualizing the waveform amplitude
  //
  YAE_API yae::Data s16_to_max_amp(const yae::TAudioFrame & src);

  YAE_API yae::Data s16_to_max_amp(const yae::Data & src,
                                   int num_channels);

  YAE_API yae::Data s16_to_max_amp(const int16_t * src,
                                   const int16_t * end,
                                   int num_channels);

  //----------------------------------------------------------------
  // s16_to_mono
  //
  // returns mono out[i] = sum(src[i, ch]) / num_channels)
  //
  // downmix to mono by simple average of samples values of input channels
  //
  YAE_API yae::Data s16_to_mono(const yae::TAudioFrame & src);

  YAE_API yae::Data s16_to_mono(const yae::Data & src,
                                int num_channels);

  YAE_API yae::Data s16_to_mono(const int16_t * src,
                                const int16_t * end,
                                int num_channels);

  //----------------------------------------------------------------
  // s16_mono_downsample
  //
  // returns out[i / 2] = (src[i] + src[i + 1]) / 2
  //
  // simple downsampling by a factor of 2
  //
  YAE_API yae::Data s16_mono_downsample(const yae::Data & src);

  //----------------------------------------------------------------
  // s16_to_f32
  //
  // convert int16_t samples to float samples in [-1, 1] range,
  // extending or truncating the frame according to out_samples.
  //
  // NOTE: out_samples <= 0 means the output number of samples
  // is the same as the input number of samples.
  //
  YAE_API yae::Data s16_to_f32(const yae::Data & src,
                               int out_samples = 0);

  //----------------------------------------------------------------
  // f32_to_s16
  //
  // convert float samples to int16_t (out[i] = scale * src[i])
  // extending or truncating the frame according to out_samples.
  //
  // NOTE: out_samples <= 0 means the output number of samples
  // is the same as the input number of samples.
  //
  YAE_API yae::Data f32_to_s16(const yae::Data & src,
                               float scale,
                               int out_samples = 0);

  //----------------------------------------------------------------
  // draw_wav_s16_mono
  //
  // returns a drawing of the given mono waveform
  //
  YAE_API yae::AvFrm draw_wav_s16_mono(const yae::Data & s16_mono);

  //----------------------------------------------------------------
  // draw_wav_amp
  //
  // returns a drawing of max(abs(abs(src[i, ch]))) max amplitude waveform.
  //
  YAE_API yae::AvFrm draw_wav_amp(const yae::TAudioFrame & src);
  YAE_API yae::AvFrm draw_wav_amp(yae::Data s16_samples, int num_channels);

  //----------------------------------------------------------------
  // draw_wav_overlap
  //
  // returns a drawing of frame_a[i] + frame_b[i - offset]
  // max amplitude waveforms, painted in complimentary colors.
  //
  YAE_API yae::AvFrm draw_wav_overlap(const yae::TAudioFrame & frame_a,
                                      const yae::TAudioFrame & frame_b,
                                      int offset);

  YAE_API yae::AvFrm draw_wav_overlap(yae::Data s16_mono_a,
                                      yae::Data s16_mono_b,
                                      int offset);

  //----------------------------------------------------------------
  // find_alignment_offset
  //
  // return best alignment offset between a and b waveforms
  //
  YAE_API int find_alignment_offset(const yae::TAudioFrame & a,
                                    const yae::TAudioFrame & b);

  //----------------------------------------------------------------
  // get_format
  //
  // returns corresponding ffmpeg AVSampleFormat
  // matching a given wav file:
  //
  YAE_API void get_traits(const yae::WavFileReader & wav,
                          yae::AudioTraits & traits);

  //----------------------------------------------------------------
  // load
  //
  // loads upto max_samples and updates output frame pts and duration
  //
  YAE_API int load(yae::WavFileReader & wav,
                   yae::TAudioFrame & frame,
                   int max_samples = 1536);

  //----------------------------------------------------------------
  // find_alignment_offset
  //
  // return best alignment offset between two wav files.
  //
  // the returned offset specifies to the number of samples that
  // must be removed from the start of wav_b to align it with wav_a.
  //
  YAE_API int find_alignment_offset(yae::WavFileReader wav_a,
                                    yae::WavFileReader wav_b);

  //----------------------------------------------------------------
  // calc_avg_abs_diff
  //
  // return average absolute difference in waveform amplitudes
  // caused by misalignment or due to other waveform differences/
  //
  // the offset specifies to the number of samples that must be
  // removed from the start of wav_b to align it with wav_a.
  //
  YAE_API double calc_avg_abs_diff(yae::WavFileReader wav_a,
                                   yae::WavFileReader wav_b,
                                   int offset);
}


#endif // YAE_AUDIO_UTILS_H_

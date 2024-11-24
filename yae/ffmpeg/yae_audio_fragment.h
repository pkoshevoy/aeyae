// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu May  3 16:30:36 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_FRAGMENT_H_
#define YAE_AUDIO_FRAGMENT_H_

#ifdef _WIN32
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// std includes:
#include <math.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <limits>

// yae includes:
#include "../utils/yae_utils.h"
#include "../video/yae_video.h"
#include "yae/ffmpeg/yae_ffmpeg_rdft.h"

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avfft.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // AudioFragment
  //
  struct YAE_API AudioFragment
  {

    //----------------------------------------------------------------
    // AudioFragment
    //
    AudioFragment():
      numSamples_(0),
      numChannels_(0),
      stride_(0)
    {
      // input position:
      position_[0] = 0;

      // output position:
      position_[1] = 0;
    }

    //----------------------------------------------------------------
    // clear
    //
    void clear()
    {
      position_[0] = 0;
      position_[1] = 0;
      numSamples_ = 0;
      numChannels_ = 0;
      stride_ = 0;
    }

    //----------------------------------------------------------------
    // init
    //
    void init(int64 fragmentPosition,
              std::size_t numberOfSamples,
              std::size_t numberOfChannels,
              std::size_t sampleStride)
    {
      data_.resize(numberOfSamples * sampleStride);
      position_[0] = fragmentPosition;
      numSamples_ = numberOfSamples;
      numChannels_ = numberOfChannels;
      stride_ = sampleStride;
    }

    //----------------------------------------------------------------
    // tx_r2c
    //
    template <typename TSample>
    void
    tx_r2c(yae::rdft_t & rdft,
           rdft_t::re_t min0 = rdft_t::re_t(std::numeric_limits<TSample>::min()),
           rdft_t::re_t max0 = rdft_t::re_t(std::numeric_limits<TSample>::max()))
    {
      // shortcuts:
      const unsigned char * src = data_.empty() ? NULL : &data_[0];
      const unsigned char * srcEnd = src + numSamples_ * stride_;

      // init data buffers used for rDFT and Cross-Correlation:
      re_.resize<rdft_t::re_t>(rdft.po2_size());
      re_.memset(0);

      cx_.resize<rdft_t::cx_t>(rdft.po2_size() / 2 + 1);
      cx_.memset(0);

      if (numChannels_ == 1)
      {
        rdft_t::re_t * xdat = re_.template data<rdft_t::re_t>();
        TSample tmp;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          *xdat = rdft_t::re_t(tmp);
          xdat++;
        }
      }
      else
      {
        rdft_t::re_t * xdat = re_.template data<rdft_t::re_t>();

        // temporary buffer for a row of samples:
        TSample tmp;
        rdft_t::re_t s;
        rdft_t::re_t max;
        rdft_t::re_t ti;
        rdft_t::re_t si;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          max = rdft_t::re_t(tmp);
          s = std::min<rdft_t::re_t>(max0, fabsf(max));

          for (std::size_t i = 1; i < numChannels_; i++)
          {
            tmp = *(const TSample *)src;
            src += sizeof(TSample);

            ti = rdft_t::re_t(tmp);
            si = std::min<rdft_t::re_t>(max0, fabsf(ti));

            // store max amplitude only:
            if (s < si)
            {
              s   = si;
              max = ti;
            }
          }

          *xdat = max;
          xdat++;
        }
      }

      // apply rDFT:
      rdft_t::re_t * re = re_.template data<rdft_t::re_t>();
      rdft_t::cx_t * cx = cx_.template data<rdft_t::cx_t>();
      rdft.r2c(re, cx);
    }

    //----------------------------------------------------------------
    // alignTo
    //
    // align this fragment to the given fragment using Cross-Correlation,
    // returns alignment offset of this fragment relative to previous.
    //
    int
    alignTo(const AudioFragment & other,
            const int deltaMax,
            const int drift,
            yae::rdft_t & rdft)
    {
      // shortcuts:
      const uint32_t window = rdft.po2_size() / 2;
      const uint32_t half_window = window / 2;
      rdft_t::re_t * correlation = rdft.re_buffer().data<rdft_t::re_t>();

      // calculate cross correlation in frequency domain:
      {
        const rdft_t::cx_t * xa = other.cx_.data<rdft_t::cx_t>();
        const rdft_t::cx_t * xb = this->cx_.data<rdft_t::cx_t>();
        rdft_t::cx_t * xc = rdft.cx_buffer().data<rdft_t::cx_t>();

        for (uint32_t i = 0; i <= half_window; i++, xa++, xb++, xc++)
        {
          xc->re = (xa->re * xb->re + xa->im * xb->im);
          xc->im = (xa->im * xb->re - xa->re * xb->im);
        }

        // apply inverse rDFT transform:
        xc = rdft.cx_buffer().data<rdft_t::cx_t>();
        rdft.c2r(xc, correlation);
      }

      // identify peaks:
      int bestOffset = -drift;
      rdft_t::re_t bestMetric = -std::numeric_limits<rdft_t::re_t>::max();

      int i0 = std::max<int>(half_window - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);

      int i1 = std::min<int>(half_window + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);

      rdft_t::re_t * xc = correlation + i0;
      for (int i = i0; i < i1; i++, xc++)
      {
        rdft_t::re_t metric = *xc;

        // normalize:
        rdft_t::re_t drifti = rdft_t::re_t(drift + i);
        metric *= drifti * rdft_t::re_t(i - i0) * rdft_t::re_t(i1 - i);

        if (metric > bestMetric)
        {
          bestMetric = metric;
          bestOffset = i - half_window;
        }
      }

      return bestOffset;
    }

    // index of the first sample of this segment in the overall waveform:
    int64 position_[2];

    // original packed multi-channel samples:
    std::vector<unsigned char> data_;

    // number of samples in this segment:
    std::size_t numSamples_;

    // number of channels in the original waveform data:
    std::size_t numChannels_;

    // row of bytes to skip from one sample to next, across multiple channels;
    // stride = (number-of-channels * bits-per-sample-per-channel) / 8
    std::size_t stride_;

    // downmixed mono fragment:
    TDataBuffer re_;

    // rDFT transform of the downmixed mono fragment,
    // used for waveform alignment via correlation:
    TDataBuffer cx_;
  };

  //----------------------------------------------------------------
  // TAudioFragment
  //
  typedef AudioFragment TAudioFragment;

}


#endif // YAE_AUDIO_FRAGMENT_H_

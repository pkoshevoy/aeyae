// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu May  3 16:30:36 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_FRAGMENT_H_
#define YAE_AUDIO_FRAGMENT_H_

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_rdft.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_video.h"

#ifdef _WIN32
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// standard:
#include <math.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <limits>

// ffmpeg:
extern "C"
{
#include <libavutil/tx.h>
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
    tx_r2c(yae::rDFT & rdft,
           float min0 = float(std::numeric_limits<TSample>::min()),
           float max0 = float(std::numeric_limits<TSample>::max()))
    {
      // shortcuts:
      const unsigned char * src = data_.empty() ? NULL : &data_[0];
      const unsigned char * srcEnd = src + numSamples_ * stride_;

      // init data buffers used for rDFT and Cross-Correlation:
      buffer_.init(rdft);

      if (numChannels_ == 1)
      {
        rDFT::re_t * xdat = buffer_.re_.template get<rDFT::re_t>();
        TSample tmp;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          *xdat = rDFT::re_t(tmp);
          xdat++;
        }
      }
      else
      {
        rDFT::re_t * xdat = buffer_.re_.template get<rDFT::re_t>();

        // temporary buffer for a row of samples:
        TSample tmp;
        rDFT::re_t s;
        rDFT::re_t max;
        rDFT::re_t ti;
        rDFT::re_t si;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          max = rDFT::re_t(tmp);
          s = std::min<rDFT::re_t>(max0, fabsf(max));

          for (std::size_t i = 1; i < numChannels_; i++)
          {
            tmp = *(const TSample *)src;
            src += sizeof(TSample);

            ti = rDFT::re_t(tmp);
            si = std::min<rDFT::re_t>(max0, fabsf(ti));

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
      rDFT::re_t * re = buffer_.re_.template get<rDFT::re_t>();
      rDFT::cx_t * cx = buffer_.cx_.template get<rDFT::cx_t>();
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
            yae::rDFT & rdft)
    {
      // shortcuts:
      const uint32_t window = rdft.po2_size() / 2;
      const uint32_t half_window = window / 2;

      // rdft.re_buffer().memset(0);
      YAE_ASSERT(rdft.re_buffer().num<rDFT::re_t>() == window * 2);
      rDFT::re_t * correlation = rdft.re_buffer().get<rDFT::re_t>();

      // calculate cross correlation in frequency domain:
      {
        const rDFT::cx_t * xa = other.buffer_.cx_.get<rDFT::cx_t>();
        const rDFT::cx_t * xb = this->buffer_.cx_.get<rDFT::cx_t>();

        rdft.cx_buffer().memset(0);
        YAE_ASSERT(rdft.cx_buffer().num<rDFT::cx_t>() == window + 1);
        rDFT::cx_t * xc = rdft.cx_buffer().get<rDFT::cx_t>();

        for (uint32_t i = 0; i <= half_window; i++, xa++, xb++, xc++)
        {
          xc->re = (xa->re * xb->re + xa->im * xb->im);
          xc->im = (xa->im * xb->re - xa->re * xb->im);
        }

        // apply inverse rDFT transform:
        xc = rdft.cx_buffer().get<rDFT::cx_t>();
        rdft.c2r(xc, correlation);
      }

      // identify peaks:
      int bestOffset = -drift;
      rDFT::re_t bestMetric = -std::numeric_limits<rDFT::re_t>::max();

      int i0 = std::max<int>(half_window - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);

      int i1 = std::min<int>(half_window + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);

      rDFT::re_t * xc = correlation + i0;
      for (int i = i0; i < i1; i++, xc++)
      {
        rDFT::re_t metric = *xc;

        // normalize:
        rDFT::re_t drifti = rDFT::re_t(drift + i);
        metric *= drifti * rDFT::re_t(i - i0) * rDFT::re_t(i1 - i);

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

    // downmixed mono fragment,
    // and rDFT transform of the downmixed mono fragment,
    // used for waveform alignment via correlation:
    yae::rDFT::Frame buffer_;
  };

  //----------------------------------------------------------------
  // TAudioFragment
  //
  typedef AudioFragment TAudioFragment;

}


#endif // YAE_AUDIO_FRAGMENT_H_

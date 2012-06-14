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
#include <yaeAPI.h>
#include <yaeUtils.h>

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
  struct AudioFragment
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
    // downsample
    //
    template <typename TSample>
    void
    downsample(const int window,
               float min0 = float(std::numeric_limits<TSample>::min()),
               float max0 = float(std::numeric_limits<TSample>::max()))
    {
      // shortcuts:
      const unsigned char * src = data_.empty() ? NULL : &data_[0];
      const unsigned char * srcEnd = src + numSamples_ * stride_;

      unsigned int nlevels = floor_log2(window);
      YAE_ASSERT(window == (1 << nlevels));

      // init complex data buffer used for rDFT and Cross-Correlation:
      xdat_.resize<FFTComplex>(window + 1);
      memset(xdat_.data(), 0, xdat_.rowBytes());

      if (numChannels_ == 1)
      {
        FFTSample * xdat = xdat_.template data<FFTSample>();
        TSample tmp;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          *xdat = (FFTSample)tmp;
          xdat++;
        }
      }
      else
      {
        FFTSample * xdat = xdat_.template data<FFTSample>();

        // temporary buffer for a row of samples:
        TSample tmp;
        float s;
        float max;
        float ti;
        float si;

        while (src < srcEnd)
        {
          tmp = *(const TSample *)src;
          src += sizeof(TSample);

          max = (float)tmp;
          s = std::min<float>(max0, fabsf(max));

          for (std::size_t i = 1; i < numChannels_; i++)
          {
            tmp = *(const TSample *)src;
            src += sizeof(TSample);

            ti = (float)tmp;
            si = std::min<float>(max0, fabsf(ti));

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
    }

    //----------------------------------------------------------------
    // transform
    //
    void transform(RDFTContext * rdft)
    {
      // apply rDFT:
      FFTSample * xdat = xdat_.data<FFTSample>();
      av_rdft_calc(rdft, xdat);
    }

    //----------------------------------------------------------------
    // alignTo
    //
    // align this fragment to the given fragment using Cross-Correlation
    //
    int
    alignTo(const AudioFragment & fragment,
            const int window,
            const int deltaMax,
            const int drift,
            FFTSample * correlation,
            RDFTContext * complexToReal)
    {
      // calculate cross correlation in frequency domain:
      {
        const FFTComplex * xa = fragment.xdat_.data<FFTComplex>();
        const FFTComplex * xb = xdat_.data<FFTComplex>();
        FFTComplex * xc = (FFTComplex *)correlation;

        // NOTE: first element requires special care -- Given Y = rDFT(X),
        // Im(Y[0]) and Im(Y[N/2]) are always zero, therefore av_rdft_calc
        // stores Re(Y[N/2]) in place of Im(Y[0]).

        xc->re = xa->re * xb->re;
        xc->im = xa->im * xb->im;
        xa++;
        xb++;
        xc++;

        for (int i = 1; i < window; i++, xa++, xb++, xc++)
        {
          xc->re = (xa->re * xb->re + xa->im * xb->im);
          xc->im = (xa->im * xb->re - xa->re * xb->im);
        }

        // apply inverse rDFT transform:
        av_rdft_calc(complexToReal, correlation);
      }

      // identify peaks:
      int bestOffset = -drift;
      FFTSample bestMetric = -std::numeric_limits<FFTSample>::max();

      int i0 = std::max<int>(window / 2 - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);

      int i1 = std::min<int>(window / 2 + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);

      FFTSample * xc = correlation + i0;
      for (int i = i0; i < i1; i++, xc++)
      {
        FFTSample metric = *xc;

        // normalize:
        FFTSample drifti = FFTSample(drift + i);
        metric *= drifti;

        if (metric > bestMetric)
        {
          bestMetric = metric;
          bestOffset = i - window / 2;
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

    // row of bytes to skip from one sample to next, accros multple channels;
    // stride = (number-of-channels * bits-per-sample-per-channel) / 8
    std::size_t stride_;

    // rDFT transform of the downmixed mono fragment, used for
    // waveform alignment via correlation:
    TSamplePlane xdat_;
  };

  //----------------------------------------------------------------
  // TAudioFragment
  //
  typedef AudioFragment TAudioFragment;

}


#endif // YAE_AUDIO_FRAGMENT_H_

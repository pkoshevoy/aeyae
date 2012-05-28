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

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avfft.h>
}


namespace yae
{
  
  //----------------------------------------------------------------
  // powerOfTwoLessThanOrEqual
  // 
  template <typename TScalar>
  TScalar
  powerOfTwoLessThanOrEqual(TScalar given, unsigned int * pyramidLevels = NULL)
  {
    unsigned int levels = 0;
    TScalar pot = 0;
    
    if (given >= 1)
    {
      levels = 1;
      pot = 1;
      
      while (given >= 2)
      {
        pot *= 2;
        given /= 2;
        levels++;
      }
    }
    
    if (pyramidLevels)
    {
      *pyramidLevels = levels;
    }
    
    return pot;
  }
  
  //----------------------------------------------------------------
  // AudioFragment
  //
  struct AudioFragment
  {
    
    typedef char TScalar;
    enum { kMin = -128 };
    enum { kMax = 127 };
    
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
    downsample(const float * blend,
               float min0 = float(std::numeric_limits<TSample>::min()),
               float max0 = float(std::numeric_limits<TSample>::max()))
    {
      // shortcuts:
      const unsigned char * src = data_.empty() ? NULL : &data_[0];
      const unsigned char * srcEnd = src + numSamples_ * stride_;
      
      unsigned int numLevels = 0;
      std::size_t pot = powerOfTwoLessThanOrEqual(numSamples_, &numLevels);
      
      if (pot < numSamples_)
      {
        pot *= 2;
        numLevels++;
      }
      
      if (!numLevels)
      {
        // empty fragment, nothing to do here:
        return;
      }
      
      // init complex data buffer used for FFT and Correlation:
      xdat_.resize<FFTComplex>(1 << numLevels);
      memset(xdat_.data(), 0, xdat_.rowBytes());
      
      if (numChannels_ == 1)
      {
        FFTComplex * xdat = xdat_.template data<FFTComplex>();
        TSample tmp;
        
        for (; src < srcEnd; src += stride_, blend++)
        {
          memcpy(&tmp, src, stride_);
          float s = float(tmp);
          
          xdat->re = s;
          xdat->im = 0;
          xdat++;
        }
      }
      else
      {
        FFTComplex * xdat = xdat_.template data<FFTComplex>();
        
        // temporary buffer for a row of samples:
        std::vector<TSample> tmp(numChannels_);
        
        for (; src < srcEnd; src += stride_, blend++)
        {
          memcpy(&tmp[0], src, stride_);
          
          float t0 = float(tmp[0]);
          float s = std::min<float>(max0, fabsf(t0));
          float max = float(t0);
          
          for (std::size_t i = 1; i < numChannels_; i++)
          {
            float ti = float(tmp[i]);
            float s0 = std::min<float>(max0, fabsf(ti));
            
            // store max amplitude only:
            if (s < s0)
            {
              s = s0;
              max = ti;
            }
          }
          
          xdat->re = max;
          xdat->im = 0;
          xdat++;
        }
      }
    }
    
    //----------------------------------------------------------------
    // transform
    // 
    void transform(FFTContext * fft)
    {
      // apply FFT:
      FFTComplex * xdat = xdat_.data<FFTComplex>();
      av_fft_permute(fft, xdat);
      av_fft_calc(fft, xdat);
    }
    
    //----------------------------------------------------------------
    // alignTo
    // 
    // align this fragment to the given fragment using Correlation
    // 
    int
    alignTo(const AudioFragment & fragment,
            const int window,
            const int deltaMax,
            const int drift,
            FFTComplex * correlation,
            FFTContext * fftInverse)
    {
      const FFTComplex * xa = fragment.xdat_.data<FFTComplex>();
      const FFTComplex * xb = xdat_.data<FFTComplex>();
      FFTComplex * xc = correlation;
      
      for (int i = 0; i < window * 2; i++, xa++, xb++, xc++)
      {
        xc->re = (xa->re * xb->re + xa->im * xb->im);
        xc->im = (xa->im * xb->re - xa->re * xb->im);
      }
      
      // apply inverse FFT:
      av_fft_permute(fftInverse, correlation);
      av_fft_calc(fftInverse, correlation);
      
      // identify peaks:
      int bestOffset = -drift;
      FFTSample bestMetric = -std::numeric_limits<FFTSample>::max();
      
      int i0 = std::max<int>(window / 2 - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);
      
      int i1 = std::min<int>(window / 2 + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);
      
      xc = correlation + i0;
      for (int i = i0; i < i1; i++, xc++)
      {
        FFTSample metric = xc->re;
        
        // normalize: 
        FFTSample drifti = FFTSample(drift + i);
        metric *= drifti * drifti;
       
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
    
    // FFT transform of the downmixed mono fragment, used for
    // waveform alignment via correlation:
    TSamplePlane xdat_;
  };
  
  //----------------------------------------------------------------
  // TAudioFragment
  // 
  typedef AudioFragment TAudioFragment;
  
}


#endif // YAE_AUDIO_FRAGMENT_H_

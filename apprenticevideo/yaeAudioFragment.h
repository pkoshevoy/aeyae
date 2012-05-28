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

//----------------------------------------------------------------
// YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
// 
#define YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT 1

#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
// ffmpeg includes:
extern "C"
{
#include <libavcodec/avfft.h>
}
#endif


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
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
      pyramid_.clear();
      poffset_.clear();
#endif
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
      
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
      // avoid downsampling too much, since the granularity will become
      // so coarse that it will be useless in practice:
      int lowestLevel = 3;
      numLevels = std::min<int>(numLevels, lowestLevel + 1);
      
      // resize the pyramid:
      poffset_.resize(numLevels);
#endif
      
      if (!numLevels)
      {
        // empty fragment, nothing to do here:
        return;
      }
      
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
      // setup the pyramid level pointers:
      {
        pyramid_.resize(pot * 2);
        poffset_[0] = 0;
        
        int prevSamples = numSamples_;
        for (unsigned int i = 1; i < numLevels; i++)
        {
          poffset_[i] = poffset_[i - 1] + prevSamples;
          prevSamples /= 2;
        }
      }
      
      // init the base pyramid level:
      TScalar * dst = pyramidLevel(0);
#else
      // init complex data buffer used for FFT and Correlation:
      xdat_.resize<FFTComplex>(1 << numLevels);
      memset(xdat_.data(), 0, xdat_.rowBytes());
#endif
      
      // shortcuts:
      const float rng0 = max0 - min0;
      const float hlf0 = rng0 / 2.f;
      const float mid0 = min0 + hlf0;
      const float min1 = float(kMin);
      const float max1 = float(kMax);
      const float rng1 = max1 - min1;

      if (numChannels_ == 1)
      {
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
        FFTComplex * xdat = xdat_.template data<FFTComplex>();
#endif
        TSample tmp;
        
        for (; src < srcEnd; src += stride_, blend++)
        {
          memcpy(&tmp, src, stride_);
          float s = float(tmp);
          
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
          float t = (s - mid0) / hlf0;
          xdat->re = t * *blend;
          xdat->im = 0;
          xdat++;
#else
          // affine transform sample into local sample format:
          *dst = TScalar(min1 + rng1 * ((s - min0) / rng0));
          dst++;
#endif
        }
      }
      else
      {
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
        FFTComplex * xdat = xdat_.template data<FFTComplex>();
#endif
        
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
          
          float t = (max - mid0) / hlf0;
          
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
          xdat->re = t * *blend;
          xdat->im = 0;
          xdat++;
#else
          // affine transform sample into local sample format:
          *dst = TScalar(max1 * t);
          dst++;
#endif
        }
      }
      
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
      // fill in the remaining levels of the pyramid:
      std::size_t numSamples = numSamples_;
      
      for (unsigned int i = 1; i < numLevels; i++)
      {
        numSamples /= 2;
        
        if (numSamples)
        {
          const TScalar * src = pyramidLevel(i - 1);
          TScalar * dst = pyramidLevel(i);
          
          for (unsigned int j = 0; j < numSamples; j++, src += 2, dst++)
          {
            *dst = abs(src[0]) < abs(src[1]) ? src[1] : src[0];
          }
        }
      }
#endif
    }
    
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
    //----------------------------------------------------------------
    // calcAlignmentMetric
    // 
    float calcAlignmentMetric(const TScalar * a,
                              const TScalar * b,
                              int overlap) const
    {
      if (overlap < 3)
      {
        return -std::numeric_limits<float>::max();
      }
      
      // skip the last sample:
      const TScalar * aEnd = a + overlap - 1;
      
      // skip the first sample:
      a++;
      b++;
      
      float mismatch = 0.f;
      float corr = 0.f;
      for (; a < aEnd; a++, b++)
      {
        float g = float(*a);
        float h = float(*b);
        corr += g * h;
        
        float d = g - h;
        mismatch += d * d;
      }
      
      // normalize:
      corr /= float(overlap - 2);
      mismatch /= float((overlap - 2) * (overlap - 2));
      
      return corr * (1.f / (1.f + mismatch));
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
            float * alignment)
    {
      float * xc = alignment;
      memset(xc, 0, sizeof(float) * window);
      
      int i0 = std::max<int>(window / 2 - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);
      
      int i1 = std::min<int>(window / 2 + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);
      
      float metricThreshold = -std::numeric_limits<float>::max();
      
      for (int level = 3; level >= 0; level--)
      {
        const int granularity = 1 << level;
        int j0 = i0 / granularity;
        int j1 = i1 / granularity;
        
        float metricSum = 0.0;
        int measurements = 0;
        xc = alignment + j0 * granularity;
        
        for (int j = j0; j < j1; j++, xc += granularity)
        {
          const int overlap = window / granularity - j;
          const TScalar * xa = fragment.pyramidLevel(level) + j;
          const TScalar * xb = this->pyramidLevel(level);
          
          float metric = *xc;
          if (*xc > metricThreshold)
          {
            metric = calcAlignmentMetric(xa, xb, overlap);
            metricSum += metric;
            measurements++;
          }
          
          for (int i = 0; i < granularity; i++)
          {
            xc[i] = metric;
          }
        }
        
        if (measurements)
        {
          // update threshold for the next level:
          float metricAvg = metricSum / float(measurements);
          metricThreshold = std::max<float>(0, metricAvg);
        }
      }
      
      float bestMetric = -std::numeric_limits<float>::max();
      int bestOffset = -drift;
      
      xc = alignment + i0;
      for (int i = i0; i < i1; i++, xc++)
      {
        float metric = *xc;
        if (bestMetric < metric ||
            bestMetric == metric &&
            abs(i - window / 2 + drift) < abs(bestOffset + drift))
        {
          bestMetric = metric;
          bestOffset = i - window / 2;
        }
      }
      
      return bestOffset;
    }
#endif
    
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
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
            float * alignment,
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
      int i0 = std::max<int>(window / 2 - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);
      
      int i1 = std::min<int>(window / 2 + deltaMax - drift,
                             window - window / 16);
      i1 = std::max<int>(i1, 0);
      
      // subtract low-pass filtered correlation from
      // denoised correlation to enhance peaks:
      xc = correlation + i0;
      
      int bestOffset = -drift;
      FFTSample bestMetric = 0;
      float * zc = alignment + i0;
      
      for (int i = i0; i < i1; i++, xc++, zc++)
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
        
        // this is for debugging/visualization:
        *zc = metric;
      }
      
      // this is for debugging/visualization:
      memset(alignment, 0, sizeof(float) * i0);
      memset(alignment + i1, 0, sizeof(float) * (window - i1));
      
      return bestOffset;
    }
#endif
    
#if !YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
    // helper:
    inline TScalar * pyramidLevel(unsigned int level)
    { return level < poffset_.size() ? &pyramid_[0] + poffset_[level] : NULL; }
    
    inline const TScalar * pyramidLevel(unsigned int level) const
    { return level < poffset_.size() ? &pyramid_[0] + poffset_[level] : NULL; }
#endif
    
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
    
#if YAE_FFT_AUDIO_FRAGMENT_ALIGNMENT
    // FFT transform of the downmixed mono fragment, used for
    // waveform alignment via correlation:
    TSamplePlane xdat_;
#else
    // multi-resolution pyramid of mono waveform samples segment
    // stored in the TScalar sample format:
    std::vector<TScalar> pyramid_;
    std::vector<int> poffset_;
#endif
  };
  
  //----------------------------------------------------------------
  // TAudioFragment
  // 
  typedef AudioFragment TAudioFragment;
  
}


#endif // YAE_AUDIO_FRAGMENT_H_

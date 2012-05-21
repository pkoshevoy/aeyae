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

//----------------------------------------------------------------
// YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
// 
// #define YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT

//----------------------------------------------------------------
// YAE_MAX_AMPLITUDE_PYRAMID
//
#ifndef YAE_MAX_AMPLITUDE_PYRAMID
#define YAE_MAX_AMPLITUDE_PYRAMID 0
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
  template <typename TScalar, int tmin, int tmax>
  struct AudioFragment
  {
    //----------------------------------------------------------------
    // TData
    // 
    typedef TScalar TData;
    
    enum
    {
      kMin = tmin,
      kMax = tmax
    };
    
    //----------------------------------------------------------------
    // AudioFragment
    // 
    AudioFragment():
      numSamples_(0),
      numChannels_(0),
      stride_(0),
      magnitude_(0.0)
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
      xdat_.clear();
      pyramid_.clear();
      position_[0] = 0;
      position_[1] = 0;
      numSamples_ = 0;
      numChannels_ = 0;
      stride_ = 0;
    }
    
    //----------------------------------------------------------------
    // init
    // 
    bool init(int64 position,
              const unsigned char * data,
              std::size_t numSamples,
              const AudioTraits & traits)
    {
      if (traits.channelFormat_ != kAudioChannelsPacked &&
          traits.channelLayout_ != kAudioMono)
      {
        // incompatible channel layout format:
        return false;
      }
      
      std::size_t numChannels = getNumberOfChannels(traits.channelLayout_);
      std::size_t bitsPerSample = getBitsPerSample(traits.sampleFormat_);
      
      // packed sample stride, expressed in bits:
      std::size_t stride = numChannels * bitsPerSample;
      
      // sanity check:
      if (stride % 8)
      {
        // incompatible sample format, stride can't be expressed in bytes:
        return false;
      }
      
      // convert to sample row bytes:
      stride /= 8;
      
      // load the data:
      init(position, data, numSamples, numChannels, stride);
      
      // NOTE: someone still has to downsample this data...
      return true;
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
    // init
    // 
    void init(int64 fragmentPosition,
              const unsigned char * data,
              std::size_t numberOfSamples,
              std::size_t numberOfChannels,
              std::size_t sampleStride)
    {
      data_.assign(data, data + numberOfSamples * sampleStride);
      position_[0] = fragmentPosition;
      numSamples_ = numberOfSamples;
      numChannels_ = numberOfChannels;
      stride_ = sampleStride;
    }

    //----------------------------------------------------------------
    // downsample
    // 
    bool
    downsample(const AudioTraits & traits)
    {
      // initialize the pyramid levels:
      if (traits.sampleFormat_ == kAudio8BitOffsetBinary)
      {
        downsample<unsigned char>();
      }
      else if (traits.sampleFormat_ == kAudio16BitNative)
      {
        downsample<short int>();
      }
      else if (traits.sampleFormat_ == kAudio32BitNative)
      {
        downsample<int>();
      }
      else if (traits.sampleFormat_ == kAudio32BitFloat)
      {
        downsample<float>(-1.0, 1.0);
      }
      else
      {
        // unsupported sample format:
        return false;
      }
      
      return true;
    }
    
    //----------------------------------------------------------------
    // downsample
    // 
    template <typename TSample>
    void
    downsample(double min0 = double(std::numeric_limits<TSample>::min()),
               double max0 = double(std::numeric_limits<TSample>::max()))
    {
      // shortcuts:
      const unsigned char * src = data_.empty() ? NULL : &data_[0];
      const unsigned char * srcEnd = src + numSamples_ * stride_;
      
      unsigned int numLevels = 0;
      std::size_t pot = powerOfTwoLessThanOrEqual(numSamples_, &numLevels);
      
      if (pot < numSamples_)
      {
        numLevels++;
      }
      
      // avoid downsampling too much, since the granularity will become
      // so coarse that it will be useless in practice:
#ifndef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
      int lowestLevel = 6;
      // numLevels = std::min<int>(numLevels, lowestLevel + 1);
#endif
      
      // resize the pyramid:
      pyramid_.resize(numLevels);
      magnitude_ = 0;
      
      if (!numLevels)
      {
        // empty fragment, nothing to do here:
        return;
      }
      
      // init complex data buffer used for FFT and Correlation:
      xdat_.resize(1 << (numLevels));
      memset(&xdat_.front(), 0, xdat_.size() * sizeof(FFTComplex));
      
      // init the base pyramid level:
      pyramid_[0].resize(numSamples_);
      TScalar * dst = &pyramid_[0][0];
      
      // shortcuts:
      const double rng0 = max0 - min0;
      const double hlf0 = rng0 / 2.0;
      const double mid0 = min0 + hlf0;
      const double min1 = double(tmin);
      const double max1 = double(tmax);
      const double rng1 = max1 - min1;

      if (numChannels_ == 1)
      {
        FFTComplex * xdat = &xdat_.front();
        TSample tmp;
        
        for (; src < srcEnd; src += stride_, dst++, xdat++)
        {
          memcpy(&tmp, src, stride_);
          
#if YAE_MAX_AMPLITUDE_PYRAMID
          double s = std::min<double>(max0, fabs(double(tmp)));
          
          // affine transform sample into local sample format:
          *dst = TScalar(max1 * ((s - mid0) / hlf0));
          magnitude_ += double(*dst);
#else
          double s = double(tmp);
          
          // affine transform sample into local sample format:
          *dst = TScalar(min1 + rng1 * ((s - min0) / rng0));
          magnitude_ += fabs(double(*dst));
#endif
          
          // xdat->re = FFTSample(-1.0 + 2.0 * (double(tmp) - min0) / rng0);
          xdat->re = FFTSample(tmp);
          xdat->im = 0;
        }
        
        magnitude_ /= double(numSamples_);
      }
      else
      {
        FFTComplex * xdat = &xdat_.front();
        
        // temporary buffer for a row of samples:
        std::vector<TSample> tmp(numChannels_);
        
        for (; src < srcEnd; src += stride_, dst++, xdat++)
        {
          memcpy(&tmp[0], src, stride_);
          
#if YAE_MAX_AMPLITUDE_PYRAMID
          double t0 = tmp[0];
          double s = std::min<double>(max0, fabs(double(t0)));
          double avg = double(t0);
          
          for (std::size_t i = 1; i < numChannels_; i++)
          {
            double ti = tmp[i];
            double s0 = std::min<double>(max0, fabs(ti));
            avg += ti;
            
            // store max amplitude only:
            s = std::max<double>(s, s0);
          }
          
          // affine transform sample into local sample format:
          *dst = TScalar(max1 * ((s - mid0) / hlf0));
          magnitude_ += double(*dst);
          
          avg /= double(numChannels_);
          // xdat->re = FFTSample(-1.0 + 2.0 * (avg - min0) / rng0);
          xdat->re = FFTSample(avg);
          xdat->im = 0;
#else
          double avg = tmp[0];
          
          for (std::size_t i = 1; i < numChannels_; i++)
          {
            avg += double(tmp[i]);
          }
          
          avg /= double(numChannels_);
          
          // affine transform sample into local sample format:
          *dst = TScalar(min1 + rng1 * ((avg - min0) / rng0));
          magnitude_ += fabs(double(*dst));
          
          // xdat->re = FFTSample(-1.0 + 2.0 * (avg - min0) / rng0);
          xdat->re = FFTSample(avg);
          xdat->im = 0;
#endif
        }
        
        magnitude_ /= double(numSamples_);
      }
      
      // fill in the remaining levels of the pyramid:
      std::size_t numSamples = numSamples_;
      
      for (unsigned int i = 1; i < numLevels; i++)
      {
        numSamples /= 2;
        pyramid_[i].resize(numSamples);
        
        if (numSamples)
        {
          const TScalar * src = &(pyramid_[i - 1][0]);
          TScalar * dst = &(pyramid_[i][0]);
          
          for (unsigned int j = 0; j < numSamples; j++, src += 2, dst++)
          {
#if YAE_MAX_AMPLITUDE_PYRAMID
            *dst = src[0] < src[1] ? src[1] : src[0];
            *dst = j + 1 < numSamples ? (*dst < src[2] ? src[2] : *dst) : *dst;
#else
            *dst = TScalar((double(src[0]) + double(src[1])) / 2.0);
            *dst = j + 1 < numSamples ?
              TScalar((double(*dst) + double(src[2]) * 0.5) / 1.5) : *dst;
#endif
          }
        }
      }
    }
    
#if defined(_WIN64) || !defined(_WIN32)
    //----------------------------------------------------------------
    // transform
    // 
    void transform(FFTContext * fft)
    {
      // apply FFT:
      FFTComplex * xdat = &xdat_.front();
      av_fft_permute(fft, xdat);
      av_fft_calc(fft, xdat);
    }
#endif
    
    //----------------------------------------------------------------
    // calcOverlapMetric
    // 
    // alignment metric used by alignTo(..):
    // 
    double calcOverlapMetric(const TScalar * a,
                             const TScalar * b,
                             unsigned int window,
                             int offset,
                             unsigned int granularity) const
    {
      // calculate reference frame position envelope:
      int aoffset = (window / 2 + offset) / granularity;
      int overlap = (window / granularity) - aoffset;
      
      if (overlap < 3)
      {
        return std::numeric_limits<double>::max();
      }
      
      // calculate the overlap metric:
      double metric = 0.0;
      for (int i = 1; i < overlap - 1; i++)
      {
        double d = double(a[aoffset + i]) - double(b[i]);
        metric += d * d;
      }
      
      // normalize the metric:
      metric /= double((overlap - 2) * (overlap - 2));
      
      return metric;
    }
    
#if defined(_WIN64) || !defined(_WIN32)
    //----------------------------------------------------------------
    // alignTo
    // 
    // align this fragment to the given fragment using Correlation
    // 
    int
    alignTo(const AudioFragment & fragment,
            unsigned int deltaMax,
            int drift,
            const FFTSample * denoiseFilter,
            const FFTSample * lowpassFilter,
            FFTComplex * correlation,
            FFTComplex * correlationFiltered,
            FFTContext * fftInverse)
    {
      int bestOffset = -drift;
      
#if 0
      if (fragment.magnitude_ < 1.0 && magnitude_ < 1.0)
      {
        // both fragments are really quiet,
        // don't bother refining their alignment
        // since nobody will be able to notice anyway:
        return bestOffset;
      }
#endif
      
      const int window = xdat_.size() / 2;
      const FFTComplex * xa = &fragment.xdat_.front();
      const FFTComplex * xb = &xdat_.front();
      FFTComplex * xc = correlation;
      FFTComplex * zc = correlationFiltered;
      const FFTSample * dz = denoiseFilter;
      const FFTSample * lp = lowpassFilter;
      
      for (int i = 0; i < window * 2; i++, xa++, xb++, xc++, zc++, dz++, lp++)
      {
        xc->re = *dz * (xa->re * xb->re + xa->im * xb->im);
        xc->im = *dz * (xa->im * xb->re - xa->re * xb->im);
        
#if 0
        FFTSample magnitude = 1e-6 + sqrtf(xc->re * xc->re + xc->im * xc->im);
        xc->re /= magnitude;
        xc->im /= magnitude;
#endif
        
        zc->re = *lp * xc->re;
        zc->im = *lp * xc->im;
      }
      
      // apply inverse FFT:
      av_fft_permute(fftInverse, correlation);
      av_fft_calc(fftInverse, correlation);
      
      av_fft_permute(fftInverse, correlationFiltered);
      av_fft_calc(fftInverse, correlationFiltered);
      
      // identify peaks:
      int i0 = std::max<int>(window / 2 - deltaMax - drift, 0);
      i0 = std::min<int>(i0, window);
      
      int i1 = std::min<int>(window / 2 + deltaMax - drift, window);
      i1 = std::max<int>(i1, 0);
      
      // subtract low-pass filtered correlation from
      // denoised correlation to enhance peaks:
      xc = correlation + i0;
      zc = correlationFiltered + i0;
      
      FFTSample bestMetric = 0;
      for (int i = i0; i < i1; i++, xc++, zc++)
      {
        FFTSample h = xc->re; // * xc->re;
        FFTSample l = zc->re; // * zc->re;
        FFTSample metric =
          (l < h) ? (h < 0 ? FFTSample(0) : (h - l) * h) : FFTSample(0);
        
        // normalize:
        int overlap = 1 + abs(window - i);
        metric /= FFTSample(overlap);
        
        if (metric > bestMetric)
        {
          bestMetric = metric;
          bestOffset = i - window / 2;
        }
        
        // this is for debugging/visualization:
        // zc->re = metric;
      }
#if 0
      // this is for debugging/visualization:
      zc = correlationFiltered;
      for (int i = 0; i < i0; i++, zc++)
      {
        zc->re = 0.0;
      }
      
      zc = correlationFiltered + i1;
      for (int i = i1; i < window; i++, zc++)
      {
        zc->re = 0.0;
      }
#endif
#ifdef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
      std::cerr << "best offset: " << bestOffset << std::endl
                << std::endl;
#endif
      
#if 0
      return bestOffset;
#else
      return refineAlignment(fragment,
                             deltaMax,
                             drift,
                             bestOffset,
                             2);
#endif
    }
#endif
    
    //----------------------------------------------------------------
    // refineAlignment
    // 
    int
    refineAlignment(const AudioFragment & fragment,
                    unsigned int deltaMax,
                    int drift,
                    int correction,
                    int lowestLevel)
    {
#ifdef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
      std::cerr << "\ninitial correction: " << correction << std::endl;
#endif
      
      // get window size so that nominal 50% overlap can be calculated:
      const int window = pyramid_[0].size();
      
      for (int level = lowestLevel; level >= 0; level--)
      {
        const int granularity = 1 << level;
        const int n = std::max<int>(1, deltaMax >> (level + 1));
        
        for (int i = 0; i < n; i++)
        {
          int bestOffset = alignTo(fragment,
                                   drift,
                                   correction,
                                   window,
                                   level,
                                   deltaMax,
                                   granularity);
          if (!bestOffset)
          {
            // convergence:
            break;
          }
          
          correction += bestOffset;
          
#ifdef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
          std::cerr
            << "1:" << granularity << ", "
            << i + 1 << " of " << n
            << ", best offset: " << bestOffset
            << ", correction: " << correction << std::endl;
#endif
        }
      }
      
#ifdef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
      std::cerr << "final correction: " << correction << std::endl
                << std::endl;
#endif
      
      return correction;
    }
    
    //----------------------------------------------------------------
    // alignTo
    // 
    // align this fragment to the given fragment:
    // 
    int
    alignTo(const AudioFragment & fragment,
            unsigned int deltaMax,
            int drift)
    {
      int correction = -drift;
      
      const std::size_t numLevels = pyramid_.size();
      if (numLevels != fragment.pyramid_.size())
      {
        // mismatched pyramids:
        return correction;
      }

      if (fragment.magnitude_ < 1.0 && magnitude_ < 1.0)
      {
        // both fragments are really quiet,
        // don't bother refining their alignment
        // since nobody will be able to notice anyway:
        return correction;
      }
      
      // deterine appropriate pyramid level to start from:
      int lowestLevel = std::min<int>(numLevels - 1, 6);
      
      return refineAlignment(fragment,
                             deltaMax,
                             drift,
                             correction,
                             lowestLevel);
    }

    //----------------------------------------------------------------
    // alignTo
    // 
    // align a level of this fragment to a matchin level of
    // the given fragment, return alignment correction value:
    // 
    int
    alignTo(const AudioFragment & fragment,
            int64 drift,
            int correction,
            unsigned int window,
            unsigned int level,
            unsigned int deltaMax,
            unsigned int offsetMax)
    {
      if (fragment.pyramid_.size() <= level ||
          pyramid_.size() <= level)
      {
        // mismatched pyramids:
        return 0;
      }
      
      int granularity = 1 << level;
      if ((offsetMax * 2) / granularity <= 1)
      {
        // granularity is too coarse, skip to a higher resolution level:
        return 0;
      }
      
      // shortcuts:
      const TScalar * a = &fragment.pyramid_[level][0];
      const TScalar * b = &pyramid_[level][0];
      
      int bestOffset = 0;
      double bestMetric =
        calcOverlapMetric(a, b, window, correction, granularity);
      
      if (bestMetric == std::numeric_limits<double>::max())
      {
        // granularity is too coarse, skip to a higher resolution level:
        return 0;
      }
      
      // refine the correction estimate:
      for (int i = -int(offsetMax); i <= int(offsetMax); i += granularity)
      {
        int deltai = drift + correction + i;
        if (i == 0 || deltai < -int(deltaMax) || deltai > int(deltaMax))
        {
          // don't step outside the search envelope:
          continue;
        }
        
        double metric =
          calcOverlapMetric(a, b, window, correction + i, granularity);
        
        if (metric < bestMetric ||
            metric == bestMetric && abs(i) < abs(bestOffset))
        {
          bestMetric = metric;
          bestOffset = i;
        }
      }
      
      return bestOffset;
    }

    //----------------------------------------------------------------
    // calcOverlapMetric
    // 
    double
    calcOverlapMetric(const AudioFragment & fragment,
                      unsigned int level,
                      int offset = 0) const
    {
      const std::size_t numLevels = pyramid_.size();
      
      if (numLevels <= level ||
          numLevels != fragment.pyramid_.size())
      {
        // mismatched pyramids:
        return 0.0;
      }
      
      // get window size so that nominal 50% overlap can be calculated:
      const int window = pyramid_[0].size();
      
      // shortcuts:
      const TScalar * a = &fragment.pyramid_[level][0];
      const TScalar * b = &pyramid_[level][0];
      
      int granularity = 1 << level;
      return calcOverlapMetric(a, b, window, offset, granularity);
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
    std::vector<FFTComplex> xdat_;
    
    // multi-resolution pyramid of mono waveform samples segment
    // stored in the TScalar sample format:
    std::vector<std::vector<TScalar> > pyramid_;
    
    // average sample amplitude:
    double magnitude_;
  };
  
  //----------------------------------------------------------------
  // TAudioFragment
  //
#if YAE_MAX_AMPLITUDE_PYRAMID
  typedef AudioFragment<unsigned char, 0, 255> TAudioFragment;
#else
  // typedef AudioFragment<short int, -32768, 32767> TAudioFragment;
  typedef AudioFragment<char, -128, 127> TAudioFragment;
#endif
  
}


#endif // YAE_AUDIO_FRAGMENT_H_

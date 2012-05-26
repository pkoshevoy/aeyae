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
// YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
// 
// #define YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT


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
      stride_(0),
      magnitude_(0.f)
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
    downsample(float min0 = float(std::numeric_limits<TSample>::min()),
               float max0 = float(std::numeric_limits<TSample>::max()))
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
      int lowestLevel = 3;
      numLevels = std::min<int>(numLevels, lowestLevel + 1);
#endif
      
      // resize the pyramid:
      pyramid_.resize(numLevels);
      magnitude_ = 0;
      
      if (!numLevels)
      {
        // empty fragment, nothing to do here:
        return;
      }
      
      // init the base pyramid level:
      pyramid_[0].resize(numSamples_);
      TScalar * dst = &pyramid_[0][0];
      
      // shortcuts:
      const float rng0 = max0 - min0;
      const float hlf0 = rng0 / 2.0;
      const float mid0 = min0 + hlf0;
      const float min1 = float(kMin);
      const float max1 = float(kMax);
      const float rng1 = max1 - min1;

      if (numChannels_ == 1)
      {
        TSample tmp;
        
        for (; src < srcEnd; src += stride_, dst++)
        {
          memcpy(&tmp, src, stride_);
          float s = float(tmp);
          
          // affine transform sample into local sample format:
          *dst = TScalar(min1 + rng1 * ((s - min0) / rng0));
          magnitude_ += fabsf((s - mid0) / hlf0);
        }
        
        magnitude_ /= float(numSamples_);
      }
      else
      {
        // temporary buffer for a row of samples:
        std::vector<TSample> tmp(numChannels_);
        
        for (; src < srcEnd; src += stride_, dst++)
        {
          memcpy(&tmp[0], src, stride_);
          
          float t0 = tmp[0];
          float s = std::min<float>(max0, fabsf(t0));
          float max = float(t0);
          
          for (std::size_t i = 1; i < numChannels_; i++)
          {
            float ti = tmp[i];
            float s0 = std::min<float>(max0, fabsf(ti));
            
            // store max amplitude only:
            if (s < s0)
            {
              s = s0;
              max = ti;
            }
          }
          
          // affine transform sample into local sample format:
          float t = (max - mid0) / hlf0;
          *dst = TScalar(max1 * t);
          magnitude_ += fabsf(t);
        }
        
        magnitude_ /= float(numSamples_);
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
            *dst = abs(src[0]) < abs(src[1]) ? src[1] : src[0];
          }
        }
      }
    }
    
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
      
      float mismatch = 0.0;
      float corr = 0.0;
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
      
      return corr * (1.0 / (1.0 + mismatch));
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
      static const float almostSilent = 3e-3f;
      if (fragment.magnitude_ < almostSilent && magnitude_ < almostSilent)
      {
        // both fragments are really quiet,
        // don't bother refining their alignment
        // since nobody will be able to notice anyway:
        return -drift;
      }
      
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
          const TScalar * xa = &fragment.pyramid_[level].front() + j;
          const TScalar * xb = &pyramid_[level].front();
          
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
      
#ifdef YAE_DEBUG_AUDIO_FRAGMENT_ALIGNMENT
      std::cerr << "best offset: " << bestOffset << std::endl
                << std::endl;
#endif
      
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
    
    // multi-resolution pyramid of mono waveform samples segment
    // stored in the TScalar sample format:
    std::vector<std::vector<TScalar> > pyramid_;
    
    // average sample amplitude:
    float magnitude_;
  };
  
  //----------------------------------------------------------------
  // TAudioFragment
  // 
  typedef AudioFragment TAudioFragment;
  
}


#endif // YAE_AUDIO_FRAGMENT_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Oct  2 11:50:06 MDT 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

//----------------------------------------------------------------
// __STDC_CONSTANT_MACROS
// 
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// system includes:
#include <vector>
#include <limits>
#include <utility>
#include <stdlib.h>
#include <string.h>

// yae includes:
#include <yaeAPI.h>


namespace yae
{

  //----------------------------------------------------------------
  // getRemixMatrix
  // 
  void
  getRemixMatrix(std::size_t srcChannels,
                 std::size_t dstChannels,
                 std::vector<double> & matrix)
  {
    std::size_t matrixSize = srcChannels * dstChannels;
    matrix.assign(matrixSize, 0.0);

    if (srcChannels == 1)
    {
      matrix.assign(matrixSize, 1.0);
    }
    else
    {
      std::size_t channels = std::min(srcChannels, dstChannels);
      for (std::size_t i = 0; i < channels; i++)
      {
        matrix[i * srcChannels + i] = 1.0;
      }
    }

    if (dstChannels == 2)
    {
      if (srcChannels == 3)
      {
        // assume 2.1 surround:
        matrix[0 * srcChannels + 2] = 0.71;
        matrix[1 * srcChannels + 2] = 0.71;
      }
      
      if (srcChannels == 4)
      {
        // assume 4.0 surround:
        matrix[0 * srcChannels + 2] = 0.71;
        matrix[1 * srcChannels + 3] = 0.71;
      }
      
      if (srcChannels > 4)
      {
        // add the front center channel:
        matrix[0 * srcChannels + 2] = 0.71;
        matrix[1 * srcChannels + 2] = 0.71;
      }
      
      if (srcChannels > 5)
      {
        // add Ls and Rs channels:
        matrix[0 * srcChannels + 3] = 0.71;
        matrix[1 * srcChannels + 4] = 0.71;
      }
      
      if (srcChannels == 7)
      {
        // add the rear center channel:
        matrix[0 * srcChannels + 5] = 1.0;
        matrix[1 * srcChannels + 5] = 1.0;
      }
      
      if (srcChannels == 8)
      {
        // add Ls and Rs channels:
        matrix[0 * srcChannels + 5] = 1.0;
        matrix[1 * srcChannels + 6] = 1.0;
      }
    }
  }

  //----------------------------------------------------------------
  // TRemix
  // 
  template <typename TData>
  void remix(const double * matrix,
             std::size_t numSamples,
             std::size_t srcStride,
             std::size_t srcChannels,
             const unsigned char * const * src,
             std::size_t dstStride,
             std::size_t dstChannels,
             unsigned char ** dst,
             double tMin = double(std::numeric_limits<TData>::min()),
             double tMax = double(std::numeric_limits<TData>::max()),
             std::size_t bytesPerSample = sizeof(TData))
  {
    for (std::size_t i = 0; i < numSamples; i++)
    {
      for (std::size_t j = 0; j < dstChannels; j++)
      {
        double t = 0.0;
        const double * scale = matrix + j * srcChannels;
        for (std::size_t k = 0; k < srcChannels; k++)
        {
          TData sample = 0;
          memcpy(&sample + sizeof(TData) - bytesPerSample,
                 src[k] + i * srcStride,
                 bytesPerSample);
          
          t += scale[k] * double(sample);
        }

        t = (t < tMin ? tMin : (t > tMax ? tMax : t));
        TData sample = TData(t);
        memcpy(dst[j] + i * dstStride,
               &sample + sizeof(TData) - bytesPerSample,
               bytesPerSample);
      }
    }
  }
  
  //----------------------------------------------------------------
  // remix
  // 
  void
  remix(std::size_t numSamples,
        TAudioSampleFormat sampleFormat,
        TAudioChannelFormat channelFormat,
        TAudioChannelLayout srcLayout,
        const unsigned char * src,
        TAudioChannelLayout dstLayout,
        unsigned char * dst,
        const double * channelMatrix)
  {
    bool isPlanar = (channelFormat == kAudioChannelsPlanar);
    std::size_t sampleSize = getBitsPerSample(sampleFormat) / 8;
    std::size_t srcChannels = getNumberOfChannels(srcLayout);
    std::size_t dstChannels = getNumberOfChannels(dstLayout);
    
    std::size_t srcPlanes = isPlanar ? srcChannels : 1;
    std::size_t dstPlanes = isPlanar ? dstChannels : 1;
    
    std::size_t srcStride = sampleSize * srcChannels / srcPlanes;
    std::size_t dstStride = sampleSize * dstChannels / dstPlanes;

    std::size_t srcPlaneStride = srcStride * numSamples;
    std::size_t dstPlaneStride = dstStride * numSamples;

    std::vector<const unsigned char *> srcChan(srcChannels, NULL);
    for (std::size_t i = 0; i < srcChannels; i++)
    {
      srcChan[i] = src + i * (isPlanar ? srcPlaneStride : sampleSize);
    }
    
    std::vector<unsigned char *> dstChan(dstChannels, NULL);
    for (std::size_t i = 0; i < dstChannels; i++)
    {
      dstChan[i] = dst + i * (isPlanar ? dstPlaneStride : sampleSize);
    }
    
    if (sampleFormat == kAudio8BitOffsetBinary)
    {
      remix<unsigned char>(channelMatrix,
                           numSamples,
                           srcStride,
                           srcChannels,
                           &srcChan[0],
                           dstStride,
                           dstChannels,
                           &dstChan[0]);
    }
    else if (sampleFormat == kAudio16BitBigEndian ||
             sampleFormat == kAudio16BitLittleEndian)
    {
      remix<short int>(channelMatrix,
                       numSamples,
                       srcStride,
                       srcChannels,
                       &srcChan[0],
                       dstStride,
                       dstChannels,
                       &dstChan[0]);
    }
    else if (sampleFormat == kAudio24BitLittleEndian)
    {
      remix<int>(channelMatrix,
                 numSamples,
                 srcStride,
                 srcChannels,
                 &srcChan[0],
                 dstStride,
                 dstChannels,
                 &dstChan[0],
                 -8388608.0, 8388607.0, 3);
    }
    else if (sampleFormat == kAudio32BitBigEndian ||
             sampleFormat == kAudio32BitLittleEndian)
    {
      remix<int>(channelMatrix,
                 numSamples,
                 srcStride,
                 srcChannels,
                 &srcChan[0],
                 dstStride,
                 dstChannels,
                 &dstChan[0]);
    }
    else if (sampleFormat == kAudio32BitFloat)
    {
      remix<float>(channelMatrix,
                   numSamples,
                   srcStride,
                   srcChannels,
                   &srcChan[0],
                   dstStride,
                   dstChannels,
                   &dstChan[0],
                   -1.0, 1.0);
    }
  }

}

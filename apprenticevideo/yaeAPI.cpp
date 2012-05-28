// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:18:35 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// std includes:
#include <stdlib.h>
#include <string.h>

// yae includes:
#include <yaeAPI.h>


namespace yae
{

  //----------------------------------------------------------------
  // TTime::TTime
  // 
  TTime::TTime():
    time_(0),
    base_(1001)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  // 
  TTime::TTime(int64 time, uint64 base):
    time_(time),
    base_(base)
  {}

  //----------------------------------------------------------------
  // TTime::operator +=
  // 
  TTime &
  TTime::operator += (const TTime & dt)
  {
    if (base_ == dt.base_)
    {
      time_ += dt.time_;
      return *this;
    }

    return operator += (dt.toSeconds());
  }
  
  //----------------------------------------------------------------
  // TTime::operator +
  // 
  TTime
  TTime::operator + (const TTime & dt) const
  {
    TTime t(*this);
    t += dt;
    return t;
  }
  
  //----------------------------------------------------------------
  // TTime::operator +
  // 
  TTime &
  TTime::operator += (double dtSec)
  {
    time_ += int64(dtSec * double(base_));
    return *this;
  }
  
  //----------------------------------------------------------------
  // TTime::operator +
  // 
  TTime
  TTime::operator + (double dtSec) const
  {
    TTime t(*this);
    t += dtSec;
    return t;
  }
  

  //----------------------------------------------------------------
  // getBitsPerSample
  // 
  unsigned int
  getBitsPerSample(TAudioSampleFormat sampleFormat)
  {
    switch (sampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return 8;
        
      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        return 16;
        
      case kAudio24BitLittleEndian:
        return 24;
        
      case kAudio32BitFloat:
      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        return 32;

      default:
        break;
    }
    
    YAE_ASSERT(false);
    return 0;
  }

  //----------------------------------------------------------------
  // getNumberOfChannels
  // 
  unsigned int
  getNumberOfChannels(TAudioChannelLayout channelLayout)
  {
    switch (channelLayout)
    {
      case kAudioMono:
        return 1;
        
      case kAudioStereo:
        return 2;
        
      case kAudio2Pt1:
        return 3;
        
      case kAudioQuad:
        return 4;
        
      case kAudio4Pt1:
        return 5;
        
      case kAudio5Pt1:
        return 6;

      case kAudio6Pt1:
        return 7;
        
      case kAudio7Pt1:
        return 8;

      default:
        break;
    }
    
    YAE_ASSERT(false);
    return 0;
  }
  
  //----------------------------------------------------------------
  // AudioTraits::AudioTraits
  // 
  AudioTraits::AudioTraits():
    sampleRate_(0),
    sampleFormat_(kAudioInvalidFormat),
    channelFormat_(kAudioChannelFormatInvalid),
    channelLayout_(kAudioChannelLayoutInvalid)
  {}
  
  //----------------------------------------------------------------
  // VideoTraits::VideoTraits
  // 
  VideoTraits::VideoTraits():
    frameRate_(0.0),
    pixelFormat_(kInvalidPixelFormat),
    encodedWidth_(0),
    encodedHeight_(0),
    offsetTop_(0),
    offsetLeft_(0),
    visibleWidth_(0),
    visibleHeight_(0),
    pixelAspectRatio_(1.0),
    isUpsideDown_(false)
  {}

  //----------------------------------------------------------------
  // ISampleBuffer::~ISampleBuffer
  // 
  ISampleBuffer::~ISampleBuffer()
  {}
  
  //----------------------------------------------------------------
  // ISampleBuffer::deallocator
  // 
  void
  ISampleBuffer::deallocator(ISampleBuffer * sb)
  {
    if (sb)
    {
      sb->destroy();
    }
  }
  
  //----------------------------------------------------------------
  // TSamplePlane::TSamplePlane
  // 
  TSamplePlane::TSamplePlane():
    data_(NULL),
    alignmentOffset_(0),
    rowBytes_(0),
    rows_(0),
    alignment_(0)
  {}

  //----------------------------------------------------------------
  // TSamplePlane::~TSamplePlane
  // 
  TSamplePlane::~TSamplePlane()
  {
    if (data_)
    {
      free(data_);
    }
  }
  
  //----------------------------------------------------------------
  // TSamplePlane::TSamplePlane
  // 
  TSamplePlane::TSamplePlane(const TSamplePlane & src):
    data_(NULL),
    alignmentOffset_(0),
    rowBytes_(0),
    rows_(0),
    alignment_(0)
  {
    *this = src;
  }

  //----------------------------------------------------------------
  // TSamplePlane::operator =
  // 
  TSamplePlane &
  TSamplePlane::operator = (const TSamplePlane & src)
  {
    YAE_ASSERT(this != &src);
    
    if (this != &src)
    {
      resize(src.rowBytes_, src.rows_, src.alignment_);
      memcpy(this->data(), src.data(), src.rowBytes_ * src.rows_);
    }
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // TSamplePlane::resize
  // 
  void
  TSamplePlane::resize(std::size_t rowBytes,
                       std::size_t rows,
                       std::size_t alignment)
  {
    std::size_t planeSize = (rowBytes * rows);
    
    if (planeSize)
    {
      data_ = (unsigned char *)realloc(data_, planeSize + alignment - 1);
    }
    else if (data_)
    {
      free(data_);
      data_ = NULL;
    }
    
    alignmentOffset_ = alignment ? std::size_t(data_) % alignment : 0;
    rowBytes_ = rowBytes;
    rows_ = rows;
    alignment_ = alignment;
  }
  

  //----------------------------------------------------------------
  // TSampleBuffer::TSampleBuffer
  // 
  TSampleBuffer::TSampleBuffer(std::size_t numSamplePlanes):
    plane_(numSamplePlanes)
  {}

  //----------------------------------------------------------------
  // TSampleBuffer::destroy
  // 
  void
  TSampleBuffer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // TSampleBuffer::samplePlanes
  // 
  std::size_t
  TSampleBuffer::samplePlanes() const
  {
    return plane_.size();
  }

  //----------------------------------------------------------------
  // TSampleBuffer::samples
  // 
  unsigned char *
  TSampleBuffer::samples(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].data() : NULL;
  }

  //----------------------------------------------------------------
  // TSampleBuffer::rowBytes
  // 
  std::size_t
  TSampleBuffer::rowBytes(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].rowBytes() : 0;
  }
  
  //----------------------------------------------------------------
  // TSampleBuffer::rows
  // 
  std::size_t
  TSampleBuffer::rows(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].rows() : 0;
  }

  //----------------------------------------------------------------
  // TSampleBuffer::resize
  // 
  void
  TSampleBuffer::resize(std::size_t samplePlane,
                        std::size_t rowBytes,
                        std::size_t rows,
                        std::size_t alignment)
  {
    YAE_ASSERT(samplePlane < plane_.size());
    if (samplePlane < plane_.size())
    {
      plane_[samplePlane].resize(rowBytes, rows, alignment);
    }
  }
}

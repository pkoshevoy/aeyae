// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:18:35 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

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
    colorFormat_(kInvalidColorFormat),
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
  // hasAlphaChannel
  // 
  bool
  hasAlphaChannel(TVideoColorFormat colorFormat)
  {
    switch (colorFormat)
    {
      case kColorFormatARGB:
      case kColorFormatBGRA:
      case kColorFormatI420Alpha:
        return true;

      default:
        break;
    }

    return false;
  }

  //----------------------------------------------------------------
  // isFormatPlanar
  // 
  bool
  isFormatPlanar(TVideoColorFormat colorFormat)
  {
    switch (colorFormat)
    {
      case kColorFormatI420:
      case kColorFormatI420Alpha:
      case kColorFormatYV12:
      case kColorFormatYUVJ420P:
      case kColorFormatYUVJ422P:
        return true;

      default:
        break;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // getBitsPerPixel
  // 
  unsigned int
  getBitsPerPixel(TVideoColorFormat colorFormat)
  {
    switch (colorFormat)
    {
      case kColorFormatI420:
      case kColorFormatYUVJ420P:
      case kColorFormatYV12:
        return 12;
        
      case kColorFormatI420Alpha:
        return 20;
        
      case kColorFormatYUYV:
      case kColorFormatUYVY:
      case kColorFormatYUVJ422P:
        return 16;
        
      case kColorFormatRGB:
      case kColorFormatBGR:
        return 24;
        
      case kColorFormatARGB:
      case kColorFormatBGRA:
        return 32;
        
      default:
        break;
    }

    assert(false);
    return 0;
  }
  
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed May 26 22:17:43 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_READER_H_
#define YAE_READER_H_

// yae includes:
#include <yaeAPI.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // IReader
  // 
  struct YAE_API IReader
  {
  protected:
    IReader() {}
    
    //! The de/structor is intentionally hidden, use destroy() method instead.
    //! This is necessary in order to avoid conflicting memory manager
    //! problems that arise on windows when various libs are linked to
    //! different versions of runtime library.  Each library uses its own
    //! memory manager, so allocating in one library call and deallocating
    //! in another library will not work.  This can be avoided by hiding
    //! the standard constructor/destructor and providing an explicit
    //! interface for de/allocating an object instance, thus ensuring that
    //! the same memory manager will perform de/allocation.
    virtual ~IReader() {}
    
  public:
    virtual void destroy() = 0;
    
    //! return a human readable name for this reader (preferably unique):
    virtual const char * getName() const = 0;
    
    //! open a resource specified by the resourcePath such as filepath or URL:
    virtual bool open(const char * resourcePathUTF8) = 0;
    
    //! close currently open resource:
    virtual void close() = 0;
    
    virtual std::size_t getNumberOfVideoTracks() const = 0;
    virtual std::size_t getNumberOfAudioTracks() const = 0;
    
    virtual std::size_t getSelectedVideoTrackIndex() const = 0;
    virtual std::size_t getSelectedAudioTrackIndex() const = 0;
    
    virtual bool selectVideoTrack(std::size_t i) = 0;
    virtual bool selectAudioTrack(std::size_t i) = 0;
    
    virtual const char * getSelectedVideoTrackName() const = 0;
    virtual const char * getSelectedAudioTrackName() const = 0;
    
    virtual bool getVideoDuration(TTime & t) const = 0;
    virtual bool getAudioDuration(TTime & t) const = 0;
    
    virtual bool getAudioTraits(AudioTraits & traits) const = 0;
    virtual bool getVideoTraits(VideoTraits & traits) const = 0;
    
    virtual void getVideoPosition(TTime & t) const = 0;
    virtual void getAudioPosition(TTime & t) const = 0;
    
    virtual bool setVideoPosition(const TTime & t) = 0;
    virtual bool setAudioPosition(const TTime & t) = 0;
    
    virtual bool readVideo(TVideoFrame & frame) = 0;
    virtual bool readAudio(TAudioFrame & frame) = 0;
  };
  
}


#endif // YAE_READER_H_

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
    virtual ~IReader() {}
    
  public:
    
    //! The de/structor is intentionally hidden, use destroy() method instead.
    //! This is necessary in order to avoid conflicting memory manager
    //! problems that arise on windows when various libs are linked to
    //! different versions of runtime library.  Each library uses its own
    //! memory manager, so allocating in one library call and deallocating
    //! in another library will not work.  This can be avoided by hiding
    //! the standard constructor/destructor and providing an explicit
    //! interface for de/allocating an object instance, thus ensuring that
    //! the same memory manager will perform de/allocation.
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
    
    virtual bool getVideoDuration(TTime & start, TTime & duration) const = 0;
    virtual bool getAudioDuration(TTime & start, TTime & duration) const = 0;
    
    virtual bool getAudioTraits(AudioTraits & traits) const = 0;
    virtual bool getVideoTraits(VideoTraits & traits) const = 0;
    
    //! force decoded audio/video frames to be in a particular format:
    virtual bool setAudioTraitsOverride(const AudioTraits & override) = 0;
    virtual bool setVideoTraitsOverride(const VideoTraits & override) = 0;

    virtual bool getAudioTraitsOverride(AudioTraits & override) const = 0;
    virtual bool getVideoTraitsOverride(VideoTraits & override) const = 0;

    //! set current position to a given value (or an earlier value nearby):
    virtual bool seek(double t) = 0;
    
    virtual bool readVideo(TVideoFramePtr & frame) = 0;
    virtual bool readAudio(TAudioFramePtr & frame) = 0;
    
    virtual bool threadStart() = 0;
    virtual bool threadStop() = 0;
    
    virtual void getPlaybackInterval(double & tIn, double & tOut) const = 0;
    virtual void setPlaybackIntervalStart(double timeIn) = 0;
    virtual void setPlaybackIntervalEnd(double timeOut) = 0;
    virtual void setPlaybackInterval(bool enabled) = 0;
    virtual void setPlaybackLooping(bool enabled) = 0;
  };
  
}


#endif // YAE_READER_H_

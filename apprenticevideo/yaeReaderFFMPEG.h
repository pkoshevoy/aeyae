// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:02:05 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_READER_FFMPEG_H_
#define YAE_READER_FFMPEG_H_

// system includes:
#include <vector>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>

// boost includes:
#include <boost/shared_ptr.hpp>


namespace yae
{

  //----------------------------------------------------------------
  // ReaderFFMPEG
  // 
  struct YAE_API ReaderFFMPEG : public IReader
  {
  private:
    //! intentionally disabled:
    ReaderFFMPEG(const ReaderFFMPEG & f);
    ReaderFFMPEG & operator = (const ReaderFFMPEG & f);
    
    //! private implementation details:
    class Private;
    Private * private_;
    
  protected:
    ReaderFFMPEG();
    virtual ~ReaderFFMPEG();
    
  public:
    virtual void destroy();
    
    //! return human-readable name for this reader:
    virtual const char * getName() const;
    
    virtual std::size_t getNumberOfVideoTracks() const;
    virtual std::size_t getNumberOfAudioTracks() const;
    
    virtual std::size_t getSelectedVideoTrackIndex() const;
    virtual std::size_t getSelectedAudioTrackIndex() const;
    
    virtual bool selectVideoTrack(std::size_t i);
    virtual bool selectAudioTrack(std::size_t i);
    
    virtual const char * getSelectedVideoTrackName() const;
    virtual const char * getSelectedAudioTrackName() const;
    
    virtual bool getVideoDuration(TTime & t) const;
    virtual bool getAudioDuration(TTime & t) const;
    
    virtual bool getAudioTraits(AudioTraits & traits) const;
    virtual bool getVideoTraits(VideoTraits & traits) const;
    
    virtual void getVideoPosition(TTime & t) const;
    virtual void getAudioPosition(TTime & t) const;
    
    virtual bool setVideoPosition(const TTime & t);
    virtual bool setAudioPosition(const TTime & t);
    
    virtual bool readVideo(TVideoFrame & frame);
    virtual bool readAudio(TAudioFrame & frame);
  };
  
}


#endif // YAE_READER_FFMPEG_H_

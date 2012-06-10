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


namespace fileUtf8
{
  //----------------------------------------------------------------
  // kProtocolName
  //
  extern const char * kProtocolName;
}


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
    Private * const private_;

  protected:
    ReaderFFMPEG();
    virtual ~ReaderFFMPEG();

  public:
    static ReaderFFMPEG * create();
    virtual void destroy();

    //! return a human readable name for this reader (preferably unique):
    virtual const char * getName() const;

    //! assemble a list of supported URL protocols:
    virtual bool getUrlProtocols(std::list<std::string> & protocols) const;

    //! open a resource specified by the resourcePath such as filepath or URL:
    virtual bool open(const char * resourcePathUTF8);

    //! close currently open resource:
    virtual void close();

    virtual std::size_t getNumberOfVideoTracks() const;
    virtual std::size_t getNumberOfAudioTracks() const;

    virtual std::size_t getSelectedVideoTrackIndex() const;
    virtual std::size_t getSelectedAudioTrackIndex() const;

    virtual bool selectVideoTrack(std::size_t i);
    virtual bool selectAudioTrack(std::size_t i);

    virtual const char * getSelectedVideoTrackName() const;
    virtual const char * getSelectedAudioTrackName() const;

    virtual bool getVideoDuration(TTime & start, TTime & duration) const;
    virtual bool getAudioDuration(TTime & start, TTime & duration) const;

    virtual bool getVideoTraits(VideoTraits & traits) const;
    virtual bool getAudioTraits(AudioTraits & traits) const;

    virtual bool setAudioTraitsOverride(const AudioTraits & override);
    virtual bool setVideoTraitsOverride(const VideoTraits & override);

    virtual bool getAudioTraitsOverride(AudioTraits & override) const;
    virtual bool getVideoTraitsOverride(VideoTraits & override) const;

    virtual bool seek(double t);

    virtual bool readVideo(TVideoFramePtr & frame, QueueWaitMgr * mgr = 0);
    virtual bool readAudio(TAudioFramePtr & frame, QueueWaitMgr * mgr = 0);

    virtual bool threadStart();
    virtual bool threadStop();

    virtual void getPlaybackInterval(double & timeIn, double & timeOut) const;
    virtual void setPlaybackIntervalStart(double timeIn);
    virtual void setPlaybackIntervalEnd(double timeOut);
    virtual void setPlaybackInterval(bool enabled);
    virtual void setPlaybackLooping(bool enabled);

    // these are used to speed up video decoding:
    virtual void skipLoopFilter(bool skip);
    virtual void skipNonReferenceFrames(bool skip);

    // this can be used to slow down audio if video decoding is too slow,
    // or it can be used to speed up audio to watch the movie faster:
    virtual bool setTempo(double tempo);
  };

}


#endif // YAE_READER_FFMPEG_H_

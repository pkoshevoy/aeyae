// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  5 21:57:57 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_INPUT_H_
#define YAE_AUDIO_RENDERER_INPUT_H_

// aeyae:
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"

// boost:
#ifndef Q_MOC_RUN
#include <boost/atomic.hpp>
#endif

// standard:
#include <string>


namespace yae
{
  //----------------------------------------------------------------
  // AudioRenderer
  //
  struct YAE_API AudioRendererInput
  {
    AudioRendererInput(SharedClock & sharedClock);

    //! begin rendering audio frames from a given reader:
    bool open(IReader * reader);

    //! terminate audio rendering:
    void stop();

    //! terminate audio rendering, discard current reader:
    void close();

    //! the initial state after open(...) must be paused;
    //! use this to resume or pause the rendering thread loop;
    void pause(bool paused);

  protected:
    // helper:
    void maybeReadOneFrame(IReader * reader,
                           TTime & framePosition,
                           TTime & packetPos);

  public:
    //! return the time of current audio frame, if any:
    TTime getCurrentTime() const;

    //! return the time of current audio frame, if any:
    TTime getPacketPos() const;

    //! this is used for single-frame stepping while playback is paused:
    void skipToTime(const TTime & t, IReader * reader);
    void skipForward(const TTime & dt, IReader * reader);

    //! pull a chunk of audio data from reader
    //! and copy into the given output buffer:
    void getData(void * data,
                 unsigned long samplesToRead,
                 int dstChannelCount,
                 bool dstPlanar);

    // audio source:
    IReader * reader_;

    // this is a tool for aborting a request for an audio frame
    // from the decoded frames queue, used to avoid a deadlock:
    QueueWaitMgr terminator_;

    // current audio frame:
    TAudioFramePtr audioFrame_;

    // bytes per sample:
    unsigned int sampleSize_;

    // number of samples already consumed from the current audio frame:
    std::size_t audioFrameOffset_;

    // expressed in seconds:
    double outputLatency_;

    // maintain (or synchronize to) this clock:
    SharedClock & clock_;

    // a flag indicating whether the renderer should be paused:
    boost::atomic<bool> pause_;
  };
}


#endif // YAE_AUDIO_RENDERER_INPUT_H_

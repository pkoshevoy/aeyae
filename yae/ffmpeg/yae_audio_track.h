// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_TRACK_H_
#define YAE_AUDIO_TRACK_H_

// aeyae:
#include "yae/ffmpeg/yae_audio_tempo_filter.h"
#include "yae/ffmpeg/yae_ffmpeg_audio_filter_graph.h"
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/thread/yae_queue.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS


namespace yae
{

  //----------------------------------------------------------------
  // TAudioFrameQueue
  //
  typedef Queue<TAudioFramePtr> TAudioFrameQueue;


  //----------------------------------------------------------------
  // AudioTrack
  //
  struct YAE_API AudioTrack : public Track
  {
    AudioTrack(Track * track);

    // virtual:
    ~AudioTrack();

    // virtual:
    bool initTraits();

    // virtual:
    AVCodecContext * open();

    // virtual:
    bool frameQueueWaitForConsumerToBlock(QueueWaitMgr * mgr = NULL)
    { return frameQueue_.waitIndefinitelyForConsumerToBlock(mgr); }

    // virtual:
    void frameQueueClear()
    { frameQueue_.clear(); }

    // virtual:
    bool decoderStartup();
    bool decoderShutdown();
    void handle(const AvFrm & decodedFrame);

    // virtual:
    bool threadStop();

    // flush and reset filter graph if native traits change during decoding:
    void noteNativeTraitsChanged();

    // audio traits, not overridden:
    bool getTraits(AudioTraits & traits) const;

    // use this for audio format conversion (sample rate, channels, etc...)
    bool setTraitsOverride(const AudioTraits & traits);
    bool getTraitsOverride(AudioTraits & traits) const;

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(const TSeekPosPtr & posIn,
                             const TSeekPosPtr & posOut,
                             bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(const TSeekPosPtr & seekPos,
                          bool dropPendingFrames);

    // adjust frame duration:
    bool setTempo(double tempo);

    TAudioFrameQueue frameQueue_;
    AudioTraits override_;
    AudioTraits native_;
    AudioTraits output_;

    // output sample buffer properties:
    int nativeChannels_;
    int outputChannels_;
    unsigned int nativeBytesPerSample_;
    unsigned int outputBytesPerSample_;

    TTime prevPTS_;
    bool hasPrevPTS_;
    uint64 prevNumSamples_;
    uint64 samplesDecoded_;

    // for adjusting audio frame duration:
    std::vector<unsigned char> tempoBuffer_;
    IAudioTempoFilter * tempoFilter_;

    AudioFilterGraph filterGraph_;
  };

  //----------------------------------------------------------------
  // AudioTrackPtr
  //
  typedef boost::shared_ptr<AudioTrack> AudioTrackPtr;


}


#endif // YAE_AUDIO_TRACK_H_

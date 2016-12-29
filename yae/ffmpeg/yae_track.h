// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TRACK_H_
#define YAE_TRACK_H_

// system includes:
#include <limits>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

// yae includes:
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // kMaxDouble
  //
  static const double kMaxDouble = std::numeric_limits<double>::max();

  //----------------------------------------------------------------
  // kMaxInt64
  //
  static const int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();

  //----------------------------------------------------------------
  // kMinInt64
  //
  static const int64_t kMinInt64 = std::numeric_limits<int64_t>::min();


  //----------------------------------------------------------------
  // kQueueSizeSmall
  //
  enum
  {
#if 1
    kQueueSizeSmall = 30,
    kQueueSizeMedium = 50,
    kQueueSizeLarge = 120
#else
    kQueueSizeSmall = 1,
    kQueueSizeMedium = 1,
    kQueueSizeLarge = 1
#endif
  };



  //----------------------------------------------------------------
  // startNewSequence
  //
  // push a special frame into frame queue to resetTimeCounters
  // down the line (the renderer):
  //
  template <typename FramePtr>
  static void
  startNewSequence(Queue<FramePtr> & frameQueue, bool dropPendingFrames)
  {
    typedef typename FramePtr::element_type Frame;
    FramePtr framePtr(new Frame());
    Frame & frame = *framePtr;
    frame.rendererHints_ = kRendererHintResetTimeCounters;
    if (dropPendingFrames)
    {
      frame.rendererHints_ |= kRendererHintDropPendingFrames;
    }

    frameQueue.startNewSequence(framePtr);
  }


  //----------------------------------------------------------------
  // Packet
  //
  struct YAE_API Packet : public AVPacket
  {
    Packet();
    ~Packet();

  private:
    // intentionally disabled:
    Packet(const Packet &);
    Packet & operator = (const Packet &);
  };

  //----------------------------------------------------------------
  // TPacketPtr
  //
  typedef boost::shared_ptr<Packet> TPacketPtr;

  //----------------------------------------------------------------
  // TPacketQueue
  //
  typedef Queue<TPacketPtr> TPacketQueue;


  //----------------------------------------------------------------
  // FrameWithAutoCleanup
  //
  struct YAE_API FrameWithAutoCleanup
  {
    FrameWithAutoCleanup();
    ~FrameWithAutoCleanup();

    inline AVFrame * get() const
    {
      return frame_;
    }

    AVFrame * reset();

  protected:
    AVFrame * frame_;

  private:
    // intentionally disabled:
    FrameWithAutoCleanup(const FrameWithAutoCleanup &);
    FrameWithAutoCleanup & operator = (const FrameWithAutoCleanup &);
  };


  //----------------------------------------------------------------
  // FrameAutoUnref
  //
  struct YAE_API FrameAutoUnref
  {
    FrameAutoUnref(AVFrame * frame);
    ~FrameAutoUnref();

    AVFrame * frame_;
  };


  //----------------------------------------------------------------
  // verify_pts
  //
  YAE_API bool verify_pts(bool hasPrevPTS,
                          const TTime & prevPTS,
                          const TTime & nextPTS,
                          const AVStream * stream,
                          const char * debugMessage = NULL);

  //----------------------------------------------------------------
  // find_best_decoder_for
  //
  // this will return a Nvidia CUVID or Intel QSV decoder when available
  //
  YAE_API const AVCodec *
  find_best_decoder_for(AVCodecID codecId);

  //----------------------------------------------------------------
  // Track
  //
  struct YAE_API Track
  {
    // NOTE: constructor does not open the stream:
    Track(AVFormatContext * context, AVStream * stream);

    // not-quiet a "move" constructor:
    Track(Track & track);

    // NOTE: destructor will close the stream:
    virtual ~Track();

    // open the stream for decoding:
    bool open();

    // close the stream:
    virtual void close();

    // get track name:
    const char * getName() const;
    const char * getLang() const;

    // accessor to stream index of this track within AVFormatContext:
    inline int streamIndex() const
    { return stream_->index; }

    // accessor to the codec context:
    inline AVCodecContext * codecContext() const
    { return codecContext_; }

    // accessor to the codec:
    inline const AVCodec * codec() const
    { return codec_; }

    // get track duration:
    bool getDuration(TTime & start, TTime & duration) const;

    // accessor to the packet queue:
    inline const TPacketQueue & packetQueue() const
    { return packetQueue_; }

    inline TPacketQueue & packetQueue()
    { return packetQueue_; }

    // decode a given packet:
    virtual bool decoderStartup()
    { return false; }

    virtual bool decoderShutdown()
    { return false; }

    virtual bool decode(const TPacketPtr & packetPtr)
    { return false; }

    // packet decoding thread:
    virtual void threadLoop() {}
    virtual bool threadStart();
    virtual bool threadStop();

    // adjust frame duration:
    virtual bool setTempo(double tempo);

  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);

  protected:
    // worker thread:
    Thread<Track> thread_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;
    AVStream * stream_;
    const AVCodec * codec_;
    AVCodecContext * codecContext_;
    TPacketQueue packetQueue_;

    double timeIn_;
    double timeOut_;
    bool playbackEnabled_;
    int64_t startTime_;

    // for adjusting frame duration (playback tempo scaling):
    mutable boost::mutex tempoMutex_;
    double tempo_;

  public:
    uint64_t discarded_;
  };

  //----------------------------------------------------------------
  // TrackPtr
  //
  typedef boost::shared_ptr<Track> TrackPtr;

}


#endif // YAE_TRACK_H_

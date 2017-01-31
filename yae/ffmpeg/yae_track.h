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
  // Rational
  //
  struct YAE_API Rational : public AVRational
  {
    Rational(int n = 0, int d = 1)
    {
      AVRational::num = n;
      AVRational::den = d;
    }
  };


  //----------------------------------------------------------------
  // AvPkt
  //
  struct YAE_API AvPkt : public AVPacket
  {
    AvPkt();
    AvPkt(const AvPkt & pkt);
    ~AvPkt();

    AvPkt & operator = (const AvPkt & pkt);

  private:
    // intentionally disabled:
    AvPkt(const AVPacket &);
    AvPkt & operator = (const AVPacket &);
  };

  //----------------------------------------------------------------
  // TPacketPtr
  //
  typedef boost::shared_ptr<AvPkt> TPacketPtr;

  //----------------------------------------------------------------
  // TPacketQueue
  //
  typedef Queue<TPacketPtr> TPacketQueue;


  //----------------------------------------------------------------
  // AvFrm
  //
  struct YAE_API AvFrm : public AVFrame
  {
    AvFrm();
    AvFrm(const AvFrm & frame);
    ~AvFrm();

    AvFrm & operator = (const AvFrm & frame);

  private:
    // intentionally disabled:
    AvFrm(const AVFrame &);
    AvFrm & operator = (const AVFrame &);
  };


  //----------------------------------------------------------------
  // AvCodecContextPtr
  //
  struct YAE_API AvCodecContextPtr : public boost::shared_ptr<AVCodecContext>
  {
    AvCodecContextPtr(AVCodecContext * ctx = NULL):
      boost::shared_ptr<AVCodecContext>(ctx, &AvCodecContextPtr::destroy)
    {}

    static void destroy(AVCodecContext * ctx);
  };


  //----------------------------------------------------------------
  // tryToOpen
  //
  YAE_API AvCodecContextPtr
  tryToOpen(const AVCodec * c,
            const AVCodecParameters * params = NULL,
            AVDictionary * opts = NULL);


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
  // this will return an instance of Nvidia CUVID decoder when available
  // or any decoder compatible with given codec parameters
  // and capable of decoding a given packet.
  //
  YAE_API AvCodecContextPtr
  find_best_decoder_for(const AVCodecParameters & params,
                        std::list<AvCodecContextPtr> & candidates);

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

    // initialize track traits, but do not open the decoder:
    virtual bool initTraits();

    // open the stream for decoding:
    virtual AVCodecContext * open();

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
    { return codecContext_.get(); }

    // get track duration:
    bool getDuration(TTime & start, TTime & duration) const;

    // accessor to the packet queue:
    inline const TPacketQueue & packetQueue() const
    { return packetQueue_; }

    inline TPacketQueue & packetQueue()
    { return packetQueue_; }

    // decoder spin-up/spin-down points:
    virtual bool decoderStartup()
    { return false; }

    virtual bool decoderShutdown()
    { return false; }

    // audio/video tracks will handled decoded frames differently,
    // but the interface is the same:
    virtual void handle(const AvFrm & decodedFrame)
    {}

    // packet decoding thread:
    virtual void threadLoop();
    virtual bool threadStart();
    virtual bool threadStop();

    // adjust frame duration:
    virtual bool setTempo(double tempo);

  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);

  protected:
    int decoderPull(AVCodecContext * ctx);
    int decode(AVCodecContext * ctx, const AvPkt & pkt);
    bool switchDecoder();
    void tryToSwitchDecoder(const std::string & name);

    // worker thread:
    Thread<Track> thread_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;
    AVStream * stream_;
    AvCodecContextPtr codecContext_;
    bool switchDecoderToRecommended_;
    std::list<AvCodecContextPtr> recommended_;
    std::list<AvCodecContextPtr> candidates_;
    std::list<TPacketPtr> packets_;
    uint64_t sent_;
    uint64_t received_;
    uint64_t errors_;
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

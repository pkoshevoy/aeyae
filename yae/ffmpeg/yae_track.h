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
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_video.h"


namespace yae
{
  // forward declarations:
  struct Demuxer;
  struct PacketBuffer;

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
  // AvPkt
  //
  struct YAE_API AvPkt
  {
    AvPkt(const AVPacket * pkt = NULL);
    AvPkt(const AvPkt & pkt);
    ~AvPkt();

    AvPkt & operator = (const AvPkt & pkt);

    inline const AVPacket & get() const
    { return *packet_; }

    inline AVPacket & get()
    { return *packet_; }

  protected:
    // the packet:
    AVPacket * packet_;

  public:
    // an indication of the origin on this packet:
    PacketBuffer * pbuffer_;
    Demuxer * demuxer_;
    int program_;
    std::string trackId_;
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
  // clone
  //
  // return a copy of this but with av_packet_clone(packet_),
  // so that modifying packet_.pts/dts of the clone
  // would not affect the original:
  //
  YAE_API TPacketPtr clone(const TPacketPtr & packet_ptr);


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

    // accessors to the global track id of this track.
    //
    // track id is composed of track type: a, v, or s
    // and global track index: i+
    //
    // examples: a:0, v:0, s:0, a:9, a:10, s:9, s:10
    //
    inline void setId(const std::string & id)
    { id_ = id; }

    inline const std::string & id() const
    { return id_; }

    // get track name:
    const char * getCodecName() const;
    const char * getName() const;
    const char * getLang() const;

    // accessor to stream index of this track within AVFormatContext:
    inline int streamIndex() const
    {
      YAE_ASSERT(stream_);
      return stream_ ? stream_->index : -1;
    }

    // accessor to the codec context:
    inline AVCodecContext * codecContext() const
    { return codecContext_.get(); }

    // get track duration:
    bool getDuration(TTime & start, TTime & duration) const;

    // decoder spin-up/spin-down points:
    virtual bool decoderStartup()
    { return false; }

    virtual bool decoderShutdown()
    { return false; }

    // audio/video tracks will handle decoded frames differently,
    // but the interface is the same:
    virtual void handle(const AvFrm & decodedFrame)
    {}

    // packet decoding thread:
    virtual void thread_loop();
    virtual bool threadStart();
    virtual bool threadStop();

    // helper:
    inline bool threadIsRunning() const
    { return thread_.isRunning(); }

    // adjust frame duration:
    virtual bool setTempo(double tempo);

    // accessors:
    inline const AVStream & stream() const
    { return *stream_; }

  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);

  protected:
    int decoderPull(AVCodecContext * ctx);
    int decode(AVCodecContext * ctx, const AvPkt & pkt);

  public:
    void decode(const TPacketPtr & packetPtr);
    void flush();

  protected:
    yae::AvBufferRef hw_device_ctx_;
    yae::AvBufferRef hw_frames_ctx_;

    // global track id:
    std::string id_;

    // worker thread:
    Thread<Track> thread_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;
    AVStream * stream_;
    AvCodecContextPtr codecContext_;
    uint64_t sent_;
    uint64_t received_;
    uint64_t errors_;

    double timeIn_;
    double timeOut_;
    bool playbackEnabled_;
    int64_t startTime_;

    // for adjusting frame duration (playback tempo scaling):
    mutable boost::mutex tempoMutex_;
    double tempo_;

  public:
    uint64_t discarded_;
    TPacketQueue packetQueue_;
  };

  //----------------------------------------------------------------
  // TrackPtr
  //
  typedef boost::shared_ptr<Track> TrackPtr;


  //----------------------------------------------------------------
  // PacketQueueCloseOnExit
  //
  struct YAE_API PacketQueueCloseOnExit
  {
    TrackPtr track_;

    PacketQueueCloseOnExit(TrackPtr track):
      track_(track)
    {
      if (track_ && track_->packetQueue_.isClosed())
      {
        track_->packetQueue_.open();
      }
    }

    ~PacketQueueCloseOnExit()
    {
      if (track_)
      {
        track_->packetQueue_.close();
      }
    }
  };


  //----------------------------------------------------------------
  // same_codec
  //
  YAE_API bool same_codec(const TrackPtr & a, const TrackPtr & b);


  //----------------------------------------------------------------
  // blockedOn
  //
  static inline bool
  blockedOn(const Track * a, const Track * b)
  {
    if (!a || !b)
    {
      return false;
    }

    bool blocked = (a->packetQueue_.producerIsBlocked() &&
                    b->packetQueue_.consumerIsBlocked());
    return blocked;
  }

}


#endif // YAE_TRACK_H_

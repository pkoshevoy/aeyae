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
#include "yae/utils/yae_time.h"
#include "yae/video/yae_video.h"


namespace yae
{
  // forward declarations:
  struct Demuxer;
  struct PacketBuffer;

  //----------------------------------------------------------------
  // kMaxInt64
  //
  static const int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();

  //----------------------------------------------------------------
  // kMinInt64
  //
  static const int64_t kMinInt64 = std::numeric_limits<int64_t>::min();

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
  // ISeekPos
  //
  struct YAE_API ISeekPos
  {
    virtual ~ISeekPos() {}
    virtual std::string to_str() const = 0;
    virtual bool lt(const TFrameBase & f, double dur = 0.0) const = 0;
    virtual bool gt(const TFrameBase & f, double dur = 0.0) const = 0;
    virtual std::string to_str(const TFrameBase & f, double dur) const = 0;
    virtual int seek(AVFormatContext * ctx, const AVStream * s) const = 0;
  };

  //----------------------------------------------------------------
  // TPosPtr
  //
  typedef yae::shared_ptr<ISeekPos> TSeekPosPtr;


  //----------------------------------------------------------------
  // TimePos
  //
  struct YAE_API TimePos : ISeekPos
  {
    TimePos(double sec);

    // virtual:
    std::string to_str() const;

    // virtual:
    bool lt(const TFrameBase & f, double dur = 0.0) const;
    bool gt(const TFrameBase & f, double dur = 0.0) const;

    // virtual:
    std::string to_str(const TFrameBase & f, double dur) const;

    // virtual:
    int seek(AVFormatContext * ctx, const AVStream * s) const;

    // seconds:
    double sec_;
  };

  //----------------------------------------------------------------
  // TTimePosPtr
  //
  typedef yae::shared_ptr<TimePos, ISeekPos> TTimePosPtr;


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
  // maybe_set_avopt
  //
  bool
  maybe_set_avopt(AVDictionary *& opts,
                  AVCodecContext * ctx,
                  const char * name,
                  const char * value);

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
    Track(AVFormatContext * context, AVStream * stream, bool hwdec);

    // not-quiet a "move" constructor:
    Track(Track * track);

    // NOTE: destructor will close the stream:
    virtual ~Track();

    // initialize track traits, but do not open the decoder:
    virtual bool initTraits();

    // open the stream for decoding:
    virtual AVCodecContext * open();

    virtual AVCodecContext *
    maybe_open(const AVCodec * codec,
               const AVCodecParameters & params,
               AVDictionary * opts);

    // close the stream:
    virtual void close();

    // helper:
    void maybe_reopen(bool hwdec);

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

    // NOTE: for MPEG-TS this corresponds to PID:
    inline int getStreamId() const
    { return stream_ ? stream_->id : std::numeric_limits<int>::max(); }

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

    inline bool packetQueueIsClosed() const
    { return packetQueue_.isClosed(); }

    void packetQueueOpen();
    void packetQueueClose();
    void packetQueueClear();

    // estimate packet ingest rate
    // and adjust Queue max size for 1s latency,
    // then try to add the packet to the packet queue:
    bool packetQueuePush(const TPacketPtr & packetPtr,
                         QueueWaitMgr * waitMgr = NULL);

    inline bool packetQueueProducerIsBlocked() const
    { return packetQueue_.producerIsBlocked(); }

    inline bool packetQueueConsumerIsBlocked() const
    { return packetQueue_.consumerIsBlocked(); }

    inline bool packetQueueWaitForConsumerToBlock(double sec)
    { return packetQueue_.waitForConsumerToBlock(sec); }

    inline bool packetQueueWaitForConsumerToBlock(QueueWaitMgr * mgr = NULL)
    { return packetQueue_.waitIndefinitelyForConsumerToBlock(mgr); }

    virtual bool frameQueueWaitForConsumerToBlock(QueueWaitMgr * mgr)
    { return true; }

    virtual void frameQueueClear()
    {}

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
    mutable boost::mutex packetRateMutex_;
    FramerateEstimator packetRateEstimator_;
    TPacketQueue packetQueue_;

    bool hwdec_;
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

    TSeekPosPtr posIn_;
    TSeekPosPtr posOut_;
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


  //----------------------------------------------------------------
  // PacketQueueCloseOnExit
  //
  struct YAE_API PacketQueueCloseOnExit
  {
    TrackPtr track_;

    PacketQueueCloseOnExit(TrackPtr track):
      track_(track)
    {
      if (track_ && track_->packetQueueIsClosed())
      {
        track_->packetQueueOpen();
      }
    }

    ~PacketQueueCloseOnExit()
    {
      if (track_)
      {
        track_->packetQueueClose();
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

    bool blocked = (a->packetQueueProducerIsBlocked() &&
                    b->packetQueueConsumerIsBlocked());
    return blocked;
  }

}


#endif // YAE_TRACK_H_

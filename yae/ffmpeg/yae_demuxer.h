// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DEMUXER_H_
#define YAE_DEMUXER_H_

// system includes:
#include <list>
#include <map>
#include <string>
#include <vector>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

// yae includes:
#include "yae/ffmpeg/yae_audio_track.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/video/yae_video.h"
#include "yae/video/yae_synchronous.h"


namespace yae
{

  //----------------------------------------------------------------
  // AvInputContextPtr
  //
  struct AvInputContextPtr : boost::shared_ptr<AVFormatContext>
  {
    AvInputContextPtr(AVFormatContext * ctx = NULL):
      boost::shared_ptr<AVFormatContext>(ctx, &AvInputContextPtr::destroy)
    {}

    static void destroy(AVFormatContext * ctx);
  };


  //----------------------------------------------------------------
  // AvOutputContextPtr
  //
  struct AvOutputContextPtr : boost::shared_ptr<AVFormatContext>
  {
    AvOutputContextPtr(AVFormatContext * ctx = NULL):
      boost::shared_ptr<AVFormatContext>(ctx, &AvOutputContextPtr::destroy)
    {}

    static void destroy(AVFormatContext * ctx);
  };


  //----------------------------------------------------------------
  // Demuxer
  //
  struct YAE_API Demuxer
  {
    Demuxer(std::size_t demuxer_index = 0,
            std::size_t track_offset = 0);
    ~Demuxer();

    bool open(const char * resourcePath);
    void close();

    bool isSeekable() const;

    int seekTo(int seekFlags, // AVSEEK_FLAG_* bitmask
               const TTime & seekTime,
               const std::string & trackId = std::string());

    // NOTE: this returns ffmpeg error code verbatim,
    // the caller must handle the error and retry as necessary:
    int demux(AvPkt & pkt);

    inline const std::vector<TProgramInfo> & programs() const
    { return programs_; }

    inline const std::vector<VideoTrackPtr> & videoTracks() const
    { return videoTracks_; }

    inline const std::vector<AudioTrackPtr> & audioTracks() const
    { return audioTracks_; }

    inline const std::vector<SubttTrackPtr> & subttTracks() const
    { return subttTracks_; }

    inline const std::map<int, TrackPtr> & tracks() const
    { return tracks_; }

    // lookup a track by global track id:
    TrackPtr getTrack(const std::string & trackId) const;

    inline bool has(const std::string & trackId) const
    { return !!this->getTrack(trackId); }

    // lookup a track by native ffmpeg stream index:
    TrackPtr getTrack(int streamIndex) const;

    // lookup program by native ffmpeg stream index:
    const TProgramInfo * getProgram(int streamIndex) const;

    void getVideoTrackInfo(std::size_t i, TTrackInfo & info) const;
    void getAudioTrackInfo(std::size_t i, TTrackInfo & info) const;

    TSubsFormat getSubttTrackInfo(std::size_t i, TTrackInfo & info) const;

    std::size_t countChapters() const;
    bool getChapterInfo(std::size_t i, TChapter & c) const;

    // get chapters, indexed by start time:
    void getChapters(std::map<TTime, TChapter> & chapters) const;

    // get metadata indexed by track id, and global metadata:
    void getMetadata(std::map<std::string, TDictionary> & track_meta,
                     TDictionary & metadata) const;

    // get programs indexed by program id:
    void getPrograms(std::map<int, TProgramInfo> & programs) const;

    // get decoders indexed by track id:
    void getDecoders(std::map<std::string, TrackPtr> & decoders) const;

    inline const std::vector<TAttachment> & attachments() const
    { return attachments_; }

    void requestDemuxerInterrupt();
    static int demuxerInterruptCallback(void * context);

    // accessors:
    inline const std::string & resourcePath() const
    { return resourcePath_; }

    inline const AVFormatContext & getFormatContext() const
    { return *(context_.get()); }

    inline std::size_t demuxer_index() const
    { return ix_; }

    inline std::size_t track_offset() const
    { return to_; }

  private:
    // intentionally disabled:
    Demuxer(const Demuxer &);
    Demuxer & operator = (const Demuxer &);

  protected:
    // a copy of the resource path passed to open(..):
    std::string resourcePath_;

    AvInputContextPtr context_;

    // demuxer index:
    std::size_t ix_;

    // track index offsets, to allow multiple demuxers
    // to output distiguishable packets of the same type
    // and the same local track index:
    //
    // global track index = local track index + track index offset
    //
    std::size_t to_;

    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    std::vector<SubttTrackPtr> subttTracks_;

    // same tracks as above, but indexed by native ffmpeg stream index:
    std::map<int, TrackPtr> tracks_;

    // map native ffmpeg stream_index to track id:
    std::map<int, std::string> trackId_;

    // map global track IDs to native ffmpeg stream index:
    std::map<std::string, int> streamIndex_;

    // a set of local audio, video, subtitle track indexes, for each program:
    std::vector<TProgramInfo> programs_;

    // lookup table for the local program index,
    // indexed by the native ffmpeg stream index:
    std::map<int, unsigned int> streamIndexToProgramIndex_;

    std::vector<TAttachment> attachments_;

    // this flag is observed from a callback passed to ffmpeg;
    // this is used to interrupt blocking ffmpeg APIs:
    bool interruptDemuxer_;
  };

  //----------------------------------------------------------------
  // TDemuxerPtr
  //
  typedef boost::shared_ptr<Demuxer> TDemuxerPtr;

  //----------------------------------------------------------------
  // open_demuxer
  //
  YAE_API TDemuxerPtr
  open_demuxer(const char * resourcePath, std::size_t track_offset = 0);

  //----------------------------------------------------------------
  // open_primary_and_aux_demuxers
  //
  // this will open a primary demuxer for the given path,
  // and any additional auxiliary demuxers for matching files
  //
  // example:
  // given /tmp/dr.flv this will open /tmp/dr.flv
  // and /tmp/dr.srt (if present)
  // and /tmp/dr.aac (if present)
  // and /tmp/dr.foo.avi (if present)
  //
  // all successfully opened demuxers are passed back via the src list,
  // and the primary demuxer is the first item in the src list
  //
  YAE_API bool
  open_primary_and_aux_demuxers(const std::string & filePath,
                                std::list<yae::TDemuxerPtr> & src);

  //----------------------------------------------------------------
  // get_dts
  //
  YAE_API bool
  get_dts(TTime & dts, const AVStream * stream, const AVPacket & pkt);

  //----------------------------------------------------------------
  // get_pts
  //
  YAE_API bool
  get_pts(TTime & pts, const AVStream * stream, const AVPacket & pkt);


  //----------------------------------------------------------------
  // ProgramBuffer
  //
  struct ProgramBuffer
  {
    // key:   ffmpeg native stream_index
    // value: a fifo list of buffered packets for that stream index
    typedef std::map<int, std::list<TPacketPtr> > TPackets;

    ProgramBuffer();

    void clear();

    // refill the buffer:
    void push(const TPacketPtr & pkt, const AVStream * stream);

    // select stream_index from which to pull the next packet:
    int choose(const AVFormatContext & ctx, TTime & dts_min) const;

    // lookup next packet and its DTS:
    TPacketPtr peek(const AVFormatContext & ctx,
                    TTime & dts_min,
                    int stream_index = -1) const;

    // remove next packet, pass back its AVStream:
    TPacketPtr get(const AVFormatContext & ctx,
                   AVStream *& src,
                   int stream_index = -1);

    // remove a given packet only if it is the front packet:
    bool pop(const TPacketPtr & pkt);

    // need to do this after a call to get which removes a packet:
    void update_duration(const AVFormatContext & ctx);

    // calculate average track duation, a sum of track durations
    // divided by number of tracks (only audio & video tracks):
    double avg_track_duration(const AVFormatContext & ctx) const;

    inline double duration() const
    { return (t0_ < t1_) ? (t1_ - t0_).sec() : 0.0; }

    inline bool empty() const
    { return num_packets_ < 1; }

    inline std::size_t num_tracks() const
    { return packets_.size(); }

    // accessors:
    inline std::size_t num_packets() const
    { return num_packets_; }

    inline const TPackets & packets() const
    { return packets_; }

  protected:
    TPackets packets_;
    std::size_t num_packets_;
    TTime t0_;
    TTime t1_;
    std::map<int, TTime> next_dts_;
  };

  //----------------------------------------------------------------
  // TProgramBufferPtr
  //
  typedef boost::shared_ptr<ProgramBuffer> TProgramBufferPtr;

  //----------------------------------------------------------------
  // PacketBuffer
  //
  struct PacketBuffer
  {
    PacketBuffer(const TDemuxerPtr & demuxer, double buffer_sec = 1.0);

    void get_metadata(std::map<std::string, TDictionary> & stream_metadata,
                      TDictionary & metadata) const;

    void get_decoders(std::map<std::string, TrackPtr> & decoders) const;
    void get_programs(std::map<int, TProgramInfo> & programs) const;

    // these are global, not per-program (avformat API doesn't it):
    void get_chapters(std::map<TTime, TChapter> & chapters) const;

    int seek(int seekFlags, // AVSEEK_FLAG_* bitmask
             const TTime & seekTime,
             const std::string & trackId = std::string());

    void clear();

    // refill the buffer:
    int populate();

    // select program and stream_index from which to pull the next packet:
    TProgramBufferPtr choose(TTime & dts_min, int & next_stream_index) const;

    // lookup next packet and its DTS:
    TPacketPtr peek(TTime & dts_min,
                    TProgramBufferPtr buffer = TProgramBufferPtr(),
                    int stream_index = -1) const;

    // remove next packet, pass back its AVStream:
    TPacketPtr get(AVStream *& src,
                   TProgramBufferPtr buffer = TProgramBufferPtr(),
                   int stream_index = -1);

    bool pop(const TPacketPtr & pkt);

    // accessors:
    inline const TDemuxerPtr & demuxer() const
    { return demuxer_; }

    // helpers:
    inline const AVFormatContext & context() const
    { return demuxer_->getFormatContext(); }

    AVStream * stream(const TPacketPtr & pkt) const;
    AVStream * stream(int stream_index) const;

  protected:
    TDemuxerPtr demuxer_;
    double buffer_sec_;
    bool gave_up_;

    // map native ffmpeg AVProgram id to ProgramBuffer:
    std::map<int, TProgramBufferPtr> program_;

    // map native ffmpeg stream_index to ProgramBuffer:
    std::map<int, TProgramBufferPtr> stream_;
  };


  //----------------------------------------------------------------
  // DemuxerSummary
  //
  struct YAE_API DemuxerSummary
  {
    void extend(const DemuxerSummary & s,
                const std::map<int, TTime> & prog_offset,
                double tolerance);

    // global metadata:
    TDictionary metadata_;

    // per-track metadata:
    std::map<std::string, TDictionary> trk_meta_;

    // streams, indexed by track id:
    std::map<std::string, const AVStream *> streams_;

    // decoders, indexed by track id:
    std::map<std::string, TrackPtr> decoders_;

    // chapters, indexed by start time:
    std::map<TTime, TChapter> chapters_;

    // program metadata, indexed by program id:
    std::map<int, TProgramInfo> programs_;

    // a mapping from program id to program timeline:
    std::map<int, Timeline> timeline_;

    // a mapping from video track id to framerate estimator:
    std::map<std::string, FramerateEstimator> fps_;

    // track_id and DTS of the first packet,
    // so we know what to pass to seek(..) to rewind:
    std::pair<std::string, TTime> rewind_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const DemuxerSummary & summary);


  //----------------------------------------------------------------
  // DemuxerInterface
  //
  struct YAE_API DemuxerInterface
  {
    virtual ~DemuxerInterface() {}

    virtual void populate() = 0;

    virtual int seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                     const TTime & seekTime,
                     const std::string & trackId = std::string()) = 0;

    // lookup front packet, pass back its AVStream:
    virtual TPacketPtr peek(AVStream *& src) const = 0;

    virtual void summarize(DemuxerSummary & summary,
                           double tolerance = 0.017) = 0;

    // NOTE: pkt must have originated from
    // an immediately prior peek call,
    // as in the get function below:
    bool pop(const TPacketPtr & pkt);

    // helper:
    TPacketPtr get(AVStream *& src);
  };

  //----------------------------------------------------------------
  // TDemuxerInterfacePtr
  //
  typedef boost::shared_ptr<DemuxerInterface> TDemuxerInterfacePtr;


  //----------------------------------------------------------------
  // summarize
  //
  // NOTE: this is always slow, because it demuxes the source
  // from start to finish to build the summary.
  //
  // DemuxerInterface::summarize is not necessarily as slow,
  // so use it instead.
  //
  YAE_API void
  summarize(DemuxerInterface & demuxer,
            DemuxerSummary & summary,
            double tolerance = 0.017);


  //----------------------------------------------------------------
  // DemuxerBuffer
  //
  struct YAE_API DemuxerBuffer : DemuxerInterface
  {
    DemuxerBuffer(const TDemuxerPtr & src, double buffer_sec = 1.0);

    virtual void populate();

    virtual int seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                     const TTime & seekTime,
                     const std::string & trackId = std::string());

    // lookup front packet, pass back its AVStream:
    virtual TPacketPtr peek(AVStream *& src) const;

    virtual void summarize(DemuxerSummary & summary,
                           double tolerance = 0.017);

  protected:
    PacketBuffer src_;
  };

  //----------------------------------------------------------------
  // ParallelDemuxer
  //
  struct YAE_API ParallelDemuxer : DemuxerInterface
  {
    void append(const TDemuxerInterfacePtr & src,
                const DemuxerSummary & summary);

    inline bool empty() const
    { return src_.empty(); }

    inline const std::list<TDemuxerInterfacePtr> & sources() const
    { return src_; }

    inline const std::list<DemuxerSummary> & summaries() const
    { return summary_; }

    virtual void populate();

    virtual int seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                     const TTime & seekTime,
                     const std::string & trackId = std::string());

    // lookup front packet, pass back its AVStream:
    virtual TPacketPtr peek(AVStream *& src) const;

    virtual void summarize(DemuxerSummary & summary,
                           double tolerance = 0.017);

  protected:
    std::list<TDemuxerInterfacePtr> src_;
    std::list<DemuxerSummary> summary_;
  };


  //----------------------------------------------------------------
  // analyze_timeline
  //
  YAE_API void
  analyze_timeline(DemuxerInterface & demuxer,
                   std::map<std::string, const AVStream *> & streams,
                   std::map<std::string, FramerateEstimator> & fps,
                   std::map<int, Timeline> & programs,
                   double tolerance = 0.017);

  //----------------------------------------------------------------
  // remux
  //
  YAE_API int
  remux(const char * output_path,
        const DemuxerSummary & summary,
        DemuxerInterface & demuxer);

  //----------------------------------------------------------------
  // SerialDemuxer
  //
  struct YAE_API SerialDemuxer : DemuxerInterface
  {
    void append(const TDemuxerInterfacePtr & src,
                const DemuxerSummary & summary);

    inline bool empty() const
    { return src_.empty(); }

    inline std::size_t num_sources() const
    { return src_.size(); }

    inline const std::vector<TDemuxerInterfacePtr> & sources() const
    { return src_; }

    inline const std::vector<DemuxerSummary> & summaries() const
    { return summary_; }

    virtual void populate();

    virtual int seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                     const TTime & seekTime,
                     const std::string & trackId = std::string());

    // lookup front packet, pass back its AVStream:
    virtual TPacketPtr peek(AVStream *& src) const;

    virtual void summarize(DemuxerSummary & summary,
                           double tolerance = 0.017);

  protected:
    // find the source corresponding to the given program/time:
    std::size_t find(const TTime & seek_time, int prog_id) const;

    std::vector<TDemuxerInterfacePtr> src_;
    std::vector<DemuxerSummary> summary_;
    std::map<int, std::vector<TTime> > t1_;
    std::map<int, std::vector<TTime> > offset_;
    std::map<std::string, int> prog_lut_;
    std::map<int, std::map<TTime, std::size_t> > t0_;
    mutable std::size_t curr_;
  };

  //----------------------------------------------------------------
  // save_keyframe
  //
  YAE_API bool
  save_keyframe(const std::string & path,
                const VideoTrackPtr & decoder_ptr,
                const TPacketPtr & packet_ptr,
                unsigned int envelope_w = 256,
                unsigned int envelope_h = 256,
                double source_dar = 0.0,
                double output_par = 1.0);

}


#endif // YAE_DEMUXER_H_

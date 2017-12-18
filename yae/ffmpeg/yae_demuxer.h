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
    Demuxer(std::size_t ato = 0,
            std::size_t vto = 0,
            std::size_t sto = 0);
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

    // lookup a track by native ffmpeg stream index:
    TrackPtr getTrack(int streamIndex) const;

    void getVideoTrackInfo(std::size_t i, TTrackInfo & info) const;
    void getAudioTrackInfo(std::size_t i, TTrackInfo & info) const;

    TSubsFormat getSubttTrackInfo(std::size_t i, TTrackInfo & info) const;

    std::size_t countChapters() const;
    bool getChapterInfo(std::size_t i, TChapter & c) const;

    inline const std::vector<TAttachment> & attachments() const
    { return attachments_; }

    void requestDemuxerInterrupt();
    static int demuxerInterruptCallback(void * context);

    // accessors:
    inline const std::string & resourcePath()
    { return resourcePath_; }

    inline const AVFormatContext & getFormatContext() const
    { return *(context_.get()); }

  private:
    // intentionally disabled:
    Demuxer(const Demuxer &);
    Demuxer & operator = (const Demuxer &);

  protected:
    // a copy of the resource path passed to open(..):
    std::string resourcePath_;

    AvInputContextPtr context_;

    // track index offsets, to allow multiple demuxers
    // to output distiguishable packets of the same type
    // and the same local track index:
    //
    // global track index = local track index + track index offset
    //
    std::size_t ato_;
    std::size_t vto_;
    std::size_t sto_;

    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    std::vector<SubttTrackPtr> subttTracks_;

    // same tracks as above, but indexed by native ffmpeg stream index:
    std::map<int, TrackPtr> tracks_;

    // map global track IDs to native ffmpeg stream index:
    std::map<std::string, int> trackIdToStreamIndex_;

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
  // PacketBuffer
  //
  struct PacketBuffer
  {
    PacketBuffer(const TDemuxerPtr & demuxer, double buffer_sec = 1.0);

    // refill the buffer:
    int populate();

    // select stream_index from which to pull the next packet:
    int choose(TTime & dts_min) const;

    // lookup next packet and its DTS:
    TPacketPtr peek(TTime & dts_min) const;

    // remove next packet, pass back its AVStream:
    TPacketPtr get(AVStream *& src);

  protected:
    typedef std::map<int, std::list<TPacketPtr> > TPackets;

    TDemuxerPtr demuxer_;
    double buffer_sec_;
    TPackets packets_;
    std::size_t num_packets_;
    TTime t0_;
    TTime t1_;
    std::map<int, TTime> next_dts_;
  };

  //----------------------------------------------------------------
  // DemuxerBuffer
  //
  struct YAE_API DemuxerBuffer
  {
    DemuxerBuffer(const std::list<TDemuxerPtr> & src);

    // remove next packet, pass back its AVStream:
    TPacketPtr get(AVStream *& src);

  protected:
    std::list<PacketBuffer> src_;
  };

}


#endif // YAE_DEMUXER_H_

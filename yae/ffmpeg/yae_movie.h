// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MOVIE_H_
#define YAE_MOVIE_H_

// aeyae:
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_audio_track.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"

// standard:
#include <list>
#include <map>
#include <string>
#include <vector>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// ffmpeg:
extern "C"
{
#include <libavformat/avformat.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // ProgramTracks
  //
  struct YAE_API ProgramTracks
  {
    std::map<int, std::list<VideoTrackPtr> > video_;
    std::map<int, std::list<AudioTrackPtr> > audio_;
    std::map<int, std::list<SubttTrackPtr> > subtt_;
  };

  //----------------------------------------------------------------
  // TProgramTracksLut
  //
  typedef std::map<int, ProgramTracks> TProgramTracksLut;


  //----------------------------------------------------------------
  // Movie
  //
  struct YAE_API Movie
  {
    Movie();

    // NOTE: destructor will close the movie:
    ~Movie();

    inline yae::TSettingGroup * settings()
    { return &settings_; }

    bool getUrlProtocols(std::list<std::string> & protocols) const;

  protected:
    VideoTrackPtr create_video_track(AVStream * stream);
    AudioTrackPtr create_audio_track(AVStream * stream);
    SubttTrackPtr create_subtt_track(AVStream * stream);

    bool extract_attachment(const AVStream * stream,
                            std::vector<TAttachment> & attachments);

    void get_program_info(std::vector<TProgramInfo> & program_infos,
                          std::map<int, int> & stream_ix_to_prog_ix);

    void prune_video_tracks(TProgramTracksLut & program_tracks_lut);

    void flatten_program_tracks(const TProgramTracksLut & program_tracks_lut,
                                std::vector<TProgramInfo> & program_infos,
                                std::vector<VideoTrackPtr> & video_tracks,
                                std::vector<AudioTrackPtr> & audio_tracks,
                                std::vector<SubttTrackPtr> & subtt_tracks,
                                std::map<int, int> & stream_ix_to_subtt_ix);

  public:
    std::size_t get_num_programs() const;
    std::size_t get_num_video_tracks() const;
    std::size_t get_num_audio_tracks() const;

    VideoTrackPtr get_video_track(std::size_t i) const;
    AudioTrackPtr get_audio_track(std::size_t i) const;

    inline VideoTrackPtr curr_video_track() const
    { return this->get_video_track(selectedVideoTrack_); }

    inline AudioTrackPtr curr_audio_track() const
    { return this->get_audio_track(selectedAudioTrack_); }

    bool open(const char * resourcePath, bool hwdec);
    bool find_anomalies();

  protected:
    void refresh();

  public:
    void close();

    inline uint64_t get_file_size() const
    { return file_size_; }

    inline int64_t get_packet_pos() const
    { return packet_pos_; }

    inline const char * getResourcePath() const
    { return resourcePath_.empty() ? NULL : resourcePath_.c_str(); }

    inline const char * getFormatName() const
    { return (context_ && context_->iformat) ? context_->iformat->name : ""; }

    inline std::size_t getSelectedVideoTrack() const
    { return selectedVideoTrack_; }

    inline std::size_t getSelectedAudioTrack() const
    { return selectedAudioTrack_; }

    bool getProgramInfo(std::size_t program_index,
                        TProgramInfo & program_info) const;

    bool getVideoTrackInfo(std::size_t video_track_index,
                           TTrackInfo & info,
                           VideoTraits & traits) const;

    bool getAudioTrackInfo(std::size_t audio_track_index,
                           TTrackInfo & info,
                           AudioTraits & traits) const;

    bool selectVideoTrack(std::size_t i);
    bool selectAudioTrack(std::size_t i);

    // this will read the file and push packets to decoding queues:
    void thread_loop();
    bool threadStart();
    bool threadStop();

    bool isSeekable() const;
    bool hasDuration() const;
    bool requestSeek(const TSeekPosPtr & pos);
    bool requestInfoRefresh();

    // for adjusting DTS/PTS timestamps:
    typedef void(*TAdjustTimestamps)(void *, AVFormatContext *, AVPacket *);
    void setAdjustTimestamps(TAdjustTimestamps cb, void * context);

  protected:
    int seekTo(const TSeekPosPtr & pos, bool dropPendingFrames);

  public:
    int rewind(const AudioTrackPtr & audioTrack,
               const VideoTrackPtr & videoTrack,
               bool seekToTimeIn = true);

    void setPlaybackIntervalStart(const TSeekPosPtr & posIn);
    void setPlaybackIntervalEnd(const TSeekPosPtr & posOut);
    void setPlaybackEnabled(bool enabled);
    void setPlaybackLooping(bool enabled);

    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    bool setTempo(double tempo);
    bool setDeinterlacing(bool enabled);

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    void setRenderCaptions(unsigned int cc);
    unsigned int getRenderCaptions() const;

    std::size_t subsCount() const;
    TSubsFormat subsInfo(std::size_t i, TTrackInfo & info) const;
    void setSubsRender(std::size_t i, bool render);
    bool getSubsRender(std::size_t i) const;

    SubtitlesTrack * subsLookup(unsigned int streamIndex);

    std::size_t countChapters() const;
    bool getChapterInfo(std::size_t i, TChapter & c) const;

    inline const std::vector<TAttachment> & attachments() const
    { return attachments_; }

    void requestMutex(boost::unique_lock<boost::timed_mutex> & lk);
    static int demuxerInterruptCallback(void * context);

    bool blockedOnVideo() const;
    bool blockedOnAudio() const;

    void setSharedClock(const SharedClock & clock);

    // this is mostly so we can notify the observer about MPEG-TS
    // program changes, ES codec changes, etc...
    void setEventObserver(const TEventObserverPtr & eo);

  private:
    // intentionally disabled:
    Movie(const Movie &);
    Movie & operator = (const Movie &);

  protected:
    // worker thread:
    Thread<Movie> thread_;
    mutable boost::timed_mutex mutex_;

    // output queue(s) deadlock avoidance mechanism:
    QueueWaitMgr outputTerminator_;

    // this one is only used to avoid a deadlock waiting
    // for audio renderer to empty out the frame queue
    // during frame stepping (when playback is disabled)
    QueueWaitMgr framestepTerminator_;

    std::string resourcePath_;
    AVFormatContext * context_;

    mutable boost::recursive_mutex data_mutex_;
    std::vector<TAttachment> attachments_;
    std::vector<TProgramInfo> program_infos_;
    std::vector<VideoTrackPtr> video_tracks_;
    std::vector<AudioTrackPtr> audio_tracks_;
    std::vector<SubttTrackPtr> subtt_tracks_;
    std::map<int, int> stream_ix_to_prog_ix_;
    std::map<int, int> stream_ix_to_subtt_ix_;

    // index of the selected video/audio track:
    boost::atomic<std::size_t> selectedVideoTrack_;
    boost::atomic<std::size_t> selectedAudioTrack_;

    // for thread-safe refresh()
    boost::atomic<bool> need_info_refresh_;

    // hw accelerated decoding is optional:
    bool hwdec_;

    // these are used to speed up video decoding:
    boost::atomic<bool> skipLoopFilter_;
    boost::atomic<bool> skipNonReferenceFrames_;

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    unsigned int enableClosedCaptions_;

    // allow packet DTS/PTS to be adjusted prior to being decoded:
    TAdjustTimestamps adjustTimestamps_;
    void * adjustTimestampsCtx_;

    // file size:
    uint64_t file_size_;

    // last known AVPacket.pos:
    int64_t packet_pos_;

    TSeekPosPtr posIn_;
    TSeekPosPtr posOut_;
    bool interruptDemuxer_;
    bool playbackEnabled_;
    bool looping_;

    bool mustStop_;
    TSeekPosPtr seekPos_;

    // shared clock used to synchronize the renderers:
    SharedClock clock_;

    // optional event observer:
    TEventObserverPtr eo_;

    // top-level settings group:
    yae::TSettingGroup settings_;
  };

}


#endif // YAE_MOVIE_H_

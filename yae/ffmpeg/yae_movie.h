// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MOVIE_H_
#define YAE_MOVIE_H_

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
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_audio_track.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"


namespace yae
{

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

    bool open(const char * resourcePath, bool hwdec);
    void close();

    inline const char * getResourcePath() const
    { return resourcePath_.empty() ? NULL : resourcePath_.c_str(); }

    inline const std::vector<TProgramInfo> & getPrograms() const
    { return programs_; }

    inline const std::vector<VideoTrackPtr> & getVideoTracks() const
    { return videoTracks_; }

    inline const std::vector<AudioTrackPtr> & getAudioTracks() const
    { return audioTracks_; }

    inline std::size_t getSelectedVideoTrack() const
    { return selectedVideoTrack_; }

    inline std::size_t getSelectedAudioTrack() const
    { return selectedAudioTrack_; }

    bool getVideoTrackInfo(std::size_t i, TTrackInfo & info) const;
    bool getAudioTrackInfo(std::size_t i, TTrackInfo & info) const;

    bool selectVideoTrack(std::size_t i);
    bool selectAudioTrack(std::size_t i);

    // this will read the file and push packets to decoding queues:
    void thread_loop();
    bool threadStart();
    bool threadStop();

    bool isSeekable() const;
    bool hasDuration() const;
    bool requestSeek(const TSeekPosPtr & pos);

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

    std::vector<TAttachment> attachments_;
    std::vector<TProgramInfo> programs_;
    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    std::vector<SubttTrackPtr> subs_;
    std::map<unsigned int, std::size_t> subsIdx_;
    std::map<int, int> streamIndexToProgramIndex_;

    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;

    // hw accelerated decoding is optional:
    bool hwdec_;

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    unsigned int enableClosedCaptions_;

    // allow packet DTS/PTS to be adjusted prior to being decoded:
    TAdjustTimestamps adjustTimestamps_;
    void * adjustTimestampsCtx_;

    // demuxer current position (DTS, stream index, and byte position):
    int dtsStreamIndex_;
    int64_t dtsBytePos_;
    int64_t dts_;

    TSeekPosPtr posIn_;
    TSeekPosPtr posOut_;
    bool interruptDemuxer_;
    bool playbackEnabled_;
    bool looping_;

    bool mustStop_;
    TSeekPosPtr seekPos_;

    // shared clock used to synchronize the renderers:
    SharedClock clock_;

    // top-level settings group:
    yae::TSettingGroup settings_;
  };

}


#endif // YAE_MOVIE_H_


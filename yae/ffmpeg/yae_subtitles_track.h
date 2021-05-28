// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SUBTITLES_TRACK_H_
#define YAE_SUBTITLES_TRACK_H_

// system includes:
#include <vector>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

// yae includes:
#include "yae/ffmpeg/yae_track.h"
#include "yae/thread/yae_queue.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // parse_ass_event_format
  //
  std::vector<std::string>
  parse_ass_event_format(const char * event_format,
                         bool drop_timing = false);

  //----------------------------------------------------------------
  // parse_ass_event
  //
  void
  parse_ass_event(const char * event,
                  const std::vector<std::string> & event_format,
                  std::map<std::string, std::string> & key_value);

  //----------------------------------------------------------------
  // find_ass_events_format
  //
  bool
  find_ass_events_format(const char * subtitle_header,
                         std::string & event_format);


  //----------------------------------------------------------------
  // TSubsPrivate
  //
  class YAE_API TSubsPrivate : public TSubsFrame::IPrivate
  {
    // virtual:
    ~TSubsPrivate();

  public:
    TSubsPrivate(const AVSubtitle & sub,
                 const unsigned char * subsHeader,
                 std::size_t subsHeaderSize);

    // virtual:
    void destroy();

    // virtual:
    std::size_t headerSize() const;

    // virtual:
    const unsigned char * header() const;

    // virtual:
    unsigned int numRects() const;

    // virtual:
    void getRect(unsigned int i, TSubsFrame::TRect & rect) const;

    // helper:
    static TSubtitleType getType(const AVSubtitleRect * r);

    AVSubtitle sub_;
    std::vector<unsigned char> header_;
  };

  //----------------------------------------------------------------
  // TSubsPrivatePtr
  //
  typedef boost::shared_ptr<TSubsPrivate> TSubsPrivatePtr;


  //----------------------------------------------------------------
  // TVobSubSpecs
  //
  struct YAE_API TVobSubSpecs
  {
    TVobSubSpecs();

    void init(const unsigned char * extraData, std::size_t size);

    // reference frame origin and dimensions:
    int x_;
    int y_;
    int w_;
    int h_;

    double scalex_;
    double scaley_;
    double alpha_;

    // color palette:
    std::vector<std::string> palette_;
  };

  //----------------------------------------------------------------
  // TSubsFrameQueue
  //
  typedef Queue<TSubsFrame> TSubsFrameQueue;

  //----------------------------------------------------------------
  // to_str
  //
  std::string to_str(const TSubsFrame & sf);

  //----------------------------------------------------------------
  // SubtitlesTrack
  //
  struct YAE_API SubtitlesTrack : public Track
  {
    SubtitlesTrack(AVStream * stream = NULL);
    ~SubtitlesTrack();

    void clear();

    // virtual:
    AVCodecContext * open();

    void close();

    void setInputEventFormat(const char * eventFormat);
    void setOutputEventFormat(const char * eventFormat);
    void addTimingEtc(TSubsFrame & sf);

    void fixupEndTime(double v1, TSubsFrame & prev, const TSubsFrame & next);
    void fixupEndTimes(double v1, const TSubsFrame & last);
    void expungeOldSubs(double v0);
    void get(double v0, double v1, std::list<TSubsFrame> & subs);
    void push(const TSubsFrame & sf, QueueWaitMgr * terminator);

  private:
    SubtitlesTrack(const SubtitlesTrack & given);
    SubtitlesTrack & operator = (const SubtitlesTrack & given);

  protected:
    // initialized in open():
    std::vector<std::string> outputEventFormat_;

    // this is to undo Matrosk ASS/SSA [Events] Format: changes
    // that (potentially) make decoded events incompatible with
    // AVCodecContext.subtitle_header and unusable with libass:
    std::vector<std::string> inputEventFormat_;

  public:
    bool render_;
    TSubsFormat format_;

    TIPlanarBufferPtr extraData_;
    TSubsFrameQueue queue_;
    std::list<TSubsFrame> active_;
    TSubsFrame last_;

    TVobSubSpecs vobsub_;
  };

  //----------------------------------------------------------------
  // SubttTrackPtr
  //
  typedef boost::shared_ptr<SubtitlesTrack> SubttTrackPtr;

}


#endif // YAE_SUBTITLES_TRACK_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 17 11:05:51 MDT 2016
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_REMUX_H_
#define YAE_REMUX_H_

// standard:
#include <list>
#include <string>

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/thread/yae_task_runner.h"


namespace yae
{

  //----------------------------------------------------------------
  // ClipInfo
  //
  struct YAE_API ClipInfo
  {
    ClipInfo(const std::string & source = std::string(),
             const std::string & track = std::string(),
             const std::string & t0 = std::string(),
             const std::string & t1 = std::string()):
      source_(source),
      track_(track),
      t0_(t0),
      t1_(t1)
    {}

    std::string source_;
    std::string track_;
    std::string t0_;
    std::string t1_;
  };

  //----------------------------------------------------------------
  // Clip
  //
  struct YAE_API Clip
  {
    Clip(const TDemuxerInterfacePtr & demuxer = TDemuxerInterfacePtr(),
         const std::string & track = std::string(),
         const Timespan & keep = Timespan());

    inline const Timeline::Track & get_track_timeline() const
    {
      return demuxer_->summary().get_track_timeline(track_);
    }

    // due to trimming/redacted track the initial track selection may become
    // invalid, so use this to get adjusted track_id and time span:
    bool get(const TDemuxerInterfacePtr & other_demuxer,
             std::string & track_id,
             Timespan & keep) const;

    bool change_track_id(const std::string & track_id);

    TDemuxerInterfacePtr demuxer_;
    std::string track_;

    // PTS timeline span to keep:
    Timespan keep_;

    mutable TTrimmedDemuxerPtr trimmed_;
  };

  //----------------------------------------------------------------
  // TClipPtr
  //
  typedef yae::shared_ptr<Clip> TClipPtr;

  //----------------------------------------------------------------
  // SetOfTracks
  //
  typedef std::set<std::string> SetOfTracks;

  //----------------------------------------------------------------
  // RemuxModel
  //
  struct YAE_API RemuxModel
  {
    TSerialDemuxerPtr make_serial_demuxer(bool unredacted = false) const;

    std::string to_json_str() const;

    static bool parse_json_str(const std::string & json_str,
                               std::set<std::string> & sources,
                               std::map<std::string, SetOfTracks> & redacted,
                               std::list<ClipInfo> & src_clips);

    void get_unredacted_track_ids(const std::string & src_name,
                                  std::set<std::string> & track_ids,
                                  const char * track_type = NULL) const;

    bool change_clip_track_id(const std::string & src_name,
                              const std::string & track_id);

    // sources, in order of appearance:
    std::list<std::string> sources_;

    // demuxer, indexed by source path:
    std::map<std::string, TDemuxerInterfacePtr> demuxer_;

    // source path, indexed by demuxer:
    std::map<TDemuxerInterfacePtr, std::string> source_;

    // redacted tracks, per source path:
    std::map<std::string, SetOfTracks> redacted_;

    // composition of the remuxed output:
    std::vector<TClipPtr> clips_;
  };

  //----------------------------------------------------------------
  // load
  //
  YAE_API TDemuxerInterfacePtr
  load(const std::set<std::string> & sources,
       const std::list<ClipInfo> & clips,
       // these are expressed in seconds:
       const double buffer_duration = 1.0,
       const double discont_tolerance = 0.017);


  namespace rx
  {

    //----------------------------------------------------------------
    // Loader
    //
    struct YAE_API Loader : yae::AsyncTaskQueue::Task
    {

      //----------------------------------------------------------------
      // IProgressObserver
      //
      struct YAE_API IProgressObserver
      {
        virtual ~IProgressObserver() {}

        virtual void began(const std::string & source) = 0;
        virtual void loaded(const std::string & source,
                            const TDemuxerInterfacePtr & demuxer,
                            const TClipPtr & clip) = 0;
        virtual void done() = 0;
      };

      //----------------------------------------------------------------
      // TProgressObserverPtr
      //
      typedef yae::shared_ptr<IProgressObserver> TProgressObserverPtr;

      //----------------------------------------------------------------
      // Loader
      //
      Loader(const std::map<std::string, TDemuxerInterfacePtr> & demuxers,
             const std::set<std::string> & sources,
             const std::list<ClipInfo> & src_clips,
             const TProgressObserverPtr & observer = TProgressObserverPtr()):
        demuxers_(demuxers),
        sources_(sources),
        src_clips_(src_clips),
        observer_(observer)
      {}

      // virtual:
      void run();

    protected:
      // helper, for loading a source on-demand:
      TDemuxerInterfacePtr get_demuxer(const std::string & source);

      // helpers:
      void load_sources();
      void load_source_clips();

      std::map<std::string, TDemuxerInterfacePtr> demuxers_;
      std::set<std::string> sources_;
      std::list<ClipInfo> src_clips_;
      TProgressObserverPtr observer_;
    };

  }

  //----------------------------------------------------------------
  // load_yaerx
  //
  YAE_API TSerialDemuxerPtr
  load_yaerx(const std::string & yaerx_path);

}


#endif // YAE_REMUX_H_

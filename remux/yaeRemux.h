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

// boost:
#include <boost/filesystem/path.hpp>

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/ffmpeg/yae_demuxer.h"


namespace yae
{

  //----------------------------------------------------------------
  // ClipInfo
  //
  struct ClipInfo
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
         const Timespan & keep = Timespan()):
      demuxer_(demuxer),
      track_(track),
      keep_(keep)
    {}

    inline const Timeline::Track & get_track_timeline() const
    {
      return demuxer_->summary().get_track_timeline(track_);
    }

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
  // RemuxModel
  //
  struct YAE_API RemuxModel
  {
    TSerialDemuxerPtr make_serial_demuxer() const;

    std::string to_json_str() const;

    static bool parse_json_str(const std::string & json_str,
                               std::set<std::string> & sources,
                               std::list<ClipInfo> & src_clips);

    // sources, in order of appearance:
    std::list<std::string> sources_;

    // demuxer, indexed by source path:
    std::map<std::string, TDemuxerInterfacePtr> demuxer_;

    // source path, indexed by demuxer:
    std::map<TDemuxerInterfacePtr, std::string> source_;

    // composition of the remuxed output:
    std::vector<TClipPtr> clips_;
  };

  //----------------------------------------------------------------
  // load
  //
  TDemuxerInterfacePtr
  load(const std::set<std::string> & sources,
       const std::list<ClipInfo> & clips,
       // these are expressed in seconds:
       const double buffer_duration = 1.0,
       const double discont_tolerance = 0.017);

  //----------------------------------------------------------------
  // demux
  //
  void
  demux(const TDemuxerInterfacePtr & demuxer,
        const std::string & output_path = std::string(),
        bool save_keyframes = false);
}


#endif // YAE_REMUX_H_

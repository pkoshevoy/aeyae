// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 17 11:05:51 MDT 2016
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_REPLAY_H_
#define YAE_REPLAY_H_

// standard:
#include <list>
#include <string>

// boost:
#include <boost/filesystem/path.hpp>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"


namespace yae
{

  //----------------------------------------------------------------
  // ClipInfo
  //
  struct ClipInfo
  {
    std::string track_;
    std::vector<std::string> t0_;
    std::vector<std::string> t1_;
  };

  //----------------------------------------------------------------
  // load
  //
  TDemuxerInterfacePtr
  load(DemuxerSummary & summary,
       const std::list<std::string> & sources,
       const std::map<std::string, ClipInfo> & clip,
       // these are expressed in seconds:
       const double buffer_duration = 1.0,
       const double discont_tolerance = 0.017);

  //----------------------------------------------------------------
  // demux
  //
  void
  demux(const TDemuxerInterfacePtr & demuxer,
        const DemuxerSummary & summary,
        const std::string & output_path = std::string(),
        bool save_keyframes = false);
}


#endif // YAE_REPLAY_H_

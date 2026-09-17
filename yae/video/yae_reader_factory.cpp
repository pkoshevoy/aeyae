// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Aug 23 12:46:54 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_demuxer_reader.h"
#include "yae/ffmpeg/yae_live_reader.h"
#include "yae/ffmpeg/yae_reader_ffmpeg.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader_factory.h"
#include "yae/video/yae_recording.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// namespace shortcut:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // ReaderFactory::create
  //
  IReaderPtr
  ReaderFactory::create(const std::string & resource_path_utf8) const
  {
    IReaderPtr reader_ptr;

    if (maybe_yaetv_recording(resource_path_utf8))
    {
      reader_ptr.reset(LiveReader::create());
    }
#if 1
    else if (al::iends_with(resource_path_utf8, ".yaerx") ||
             al::iends_with(resource_path_utf8, ".m2ts") ||
             al::iends_with(resource_path_utf8, ".mpeg") ||
             al::iends_with(resource_path_utf8, ".mpg") ||
             al::iends_with(resource_path_utf8, ".m2t") ||
             al::iends_with(resource_path_utf8, ".ts"))

    {
      reader_ptr.reset(DemuxerReader::create());
    }
#endif
    else
    {
      reader_ptr.reset(ReaderFFMPEG::create());
    }

    return reader_ptr;
  }

}

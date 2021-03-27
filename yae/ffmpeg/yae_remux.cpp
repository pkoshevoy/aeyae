// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 17 11:05:51 MDT 2016
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <string>

// boost:
#include <boost/filesystem/path.hpp>

// jsoncpp:
#include "json/json.h"

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/ffmpeg/yae_remux.h"
#include "yae/utils/yae_json.h"

// namespace shortcuts:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // Clip::get
  //
  bool
  Clip::get(const TDemuxerInterfacePtr & other_demuxer,
            std::string & track_id,
            Timespan & keep) const
  {
    const DemuxerSummary & other_summary = other_demuxer->summary();
    if (other_summary.has_track(track_))
    {
      track_id = track_;
      keep = keep_;
      return true;
    }

    // find an alternative track:
    track_id = other_summary.suggest_clip_track_id();
    if (track_id.empty())
    {
      keep = Timespan();
      return false;
    }

    const DemuxerSummary & demuxer_summary = demuxer_->summary();
    TTime dt = demuxer_summary.get_timeline_diff(track_id, track_);

    // adjust time span:
    keep = keep_;
    keep += dt;
    return true;
  }

  //----------------------------------------------------------------
  // RemuxModel::make_serial_demuxer
  //
  TSerialDemuxerPtr
  RemuxModel::make_serial_demuxer(bool unredacted) const
  {
    std::string prev_fn;
    std::string prev_track;
    std::ostringstream oss;

    TSerialDemuxerPtr serial_demuxer(new SerialDemuxer());

    // avoid thread-safety issues by using clones to populate
    // the serial demuxer, not the originals (which may be accessed
    // from the UI thread to fetch thumbnails, etc...):
    std::map<TDemuxerInterfacePtr, TDemuxerInterfacePtr> clones;

    for (std::vector<TClipPtr>::const_iterator
           i = clips_.begin(); i != clips_.end(); ++i)
    {
      const Clip & clip = *(*i);
      std::string clip_track = clip.track_;
      Timespan clip_keep = clip.keep_;

      TDemuxerInterfacePtr & clone = clones[clip.demuxer_];
      if (!clone)
      {
        clone.reset(clip.demuxer_->clone());

        if (!unredacted)
        {
          // adjust for redacted tracks:
          const std::string & src_name = yae::at(source_, clip.demuxer_);
          std::map<std::string, SetOfTracks>::const_iterator
            found = redacted_.find(src_name);

          if (found != redacted_.end())
          {
            const SetOfTracks & redacted = found->second;
            TRedactedDemuxerPtr redacted_demuxer(new RedactedDemuxer(clone));
            redacted_demuxer->set_redacted(redacted);
            redacted_demuxer->update_summary();

            if (!clip.get(redacted_demuxer, clip_track, clip_keep))
            {
              clone.reset();
              continue;
            }

            clone = redacted_demuxer;
          }
        }
      }

      clip.trimmed_.reset(new TrimmedDemuxer(clone, clip_track));
      clip.trimmed_->set_pts_span(clip_keep);

      std::string fn = yae::at(source_, clip.demuxer_);
      if (fn != prev_fn)
      {
        oss << " \"" << fn << "\"";
        prev_fn = fn;
      }

      if (clip_track != prev_track)
      {
        oss << " -track " << clip_track;
        prev_track = clip_track;
      }

      const Timeline::Track & track =
        clip.demuxer_->summary().get_track_timeline(clip_track);

      if (clip_keep.t0_ > track.pts_.front() ||
          clip_keep.t1_ < track.pts_.back())
      {
        oss << " -t"
            << " " << clip_keep.t0_.to_hhmmss_ms()
            << " " << clip_keep.t1_.to_hhmmss_ms();
      }

      // summarize clip demuxer:
      clip.trimmed_->update_summary();
      serial_demuxer->append(clip.trimmed_);
    }

    // summarize serial demuxer:
    serial_demuxer->update_summary();

#ifndef NDEBUG
    av_log(NULL, AV_LOG_WARNING, "args: %s", oss.str().c_str());
#endif

    return serial_demuxer;
  }

  //----------------------------------------------------------------
  // RemuxModel::to_json_str
  //
  std::string
  RemuxModel::to_json_str() const
  {
    Json::Value jv_clips;
    for (std::vector<TClipPtr>::const_iterator
           i = clips_.begin(); i != clips_.end(); ++i)
    {
      const Clip & clip = *(*i);
      std::string source = yae::at(source_, clip.demuxer_);

      Json::Value jv_clip;
      jv_clip["source"] = source;
      jv_clip["track"] = clip.track_;

      const Timeline::Track & track =
        clip.demuxer_->summary().get_track_timeline(clip.track_);

      if (clip.keep_.t0_ > track.pts_.front() ||
          clip.keep_.t1_ < track.pts_.back())
      {
        Json::Value jv_keep;
        jv_keep["t0"] = clip.keep_.t0_.to_hhmmss_ms();
        jv_keep["t1"] = clip.keep_.t1_.to_hhmmss_ms();
        jv_clip["keep"] = jv_keep;
      }

      jv_clips.append(jv_clip);
    }

    Json::Value jv_aeyae;
    jv_aeyae["doctype"] = "remux";
    jv_aeyae["revision"] = YAE_REVISION;
    jv_aeyae["timestamp"] = YAE_REVISION_TIMESTAMP;

    Json::Value jv_doc;
    jv_doc["aeyae"] = jv_aeyae;
    jv_doc["clips"] = jv_clips;

    if (!redacted_.empty())
    {
      yae::save(jv_doc["redacted"], redacted_);
    }

    return Json::StyledWriter().write(jv_doc);
  }

  //----------------------------------------------------------------
  // RemuxModel::load_json_str
  //
  bool
  RemuxModel::parse_json_str(const std::string & json_str,
                             std::set<std::string> & sources,
                             std::map<std::string, SetOfTracks> & redacted,
                             std::list<ClipInfo> & src_clips)
  {
    Json::Value jv_doc;
    Json::Reader reader;
    if (!reader.parse(json_str, jv_doc))
    {
      return false;
    }

    if (!(jv_doc.isMember("aeyae") && jv_doc.isMember("clips")))
    {
      return false;
    }

    if (jv_doc["aeyae"].get("doctype", std::string()).asString() != "remux")
    {
      return false;
    }

    Json::Value clips = jv_doc["clips"];
    if (!clips.isArray())
    {
      return false;
    }

    Json::ArrayIndex n = clips.size();
    for (Json::ArrayIndex i = 0; i < n; i++)
    {
      Json::Value jv_clip = clips[i];

      ClipInfo clip;
      clip.source_ = jv_clip["source"].asString();
      clip.track_ = jv_clip["track"].asString();

      if (jv_clip.isMember("keep"))
      {
        Json::Value jv_keep = jv_clip["keep"];
        clip.t0_ = jv_keep["t0"].asString();
        clip.t1_ = jv_keep["t1"].asString();
      }

      sources.insert(clip.source_);
      src_clips.push_back(clip);
    }

    if (jv_doc.isMember("redacted"))
    {
      Json::Value jv_redacted = jv_doc["redacted"];
      yae::load(jv_redacted, redacted);
    }

    return true;
  }


  //----------------------------------------------------------------
  // load
  //
  TDemuxerInterfacePtr
  load(const std::set<std::string> & sources,
       const std::map<std::string, SetOfTracks> & redacted,
       const std::list<ClipInfo> & clips,
       // these are expressed in seconds:
       const double buffer_duration,
       const double discont_tolerance)
  {
    std::map<std::string, TDemuxerInterfacePtr> source_demuxers;

    for (std::set<std::string>::const_iterator i = sources.begin();
         i != sources.end(); ++i)
    {
      const std::string & filePath = *i;

      std::list<TDemuxerPtr> demuxers;
      if (!open_primary_and_aux_demuxers(filePath, demuxers))
      {
        // failed to open the primary resource:
        av_log(NULL, AV_LOG_WARNING,
               "failed to open %s, skipping...",
               filePath.c_str());
        continue;
      }

      TParallelDemuxerPtr parallel_demuxer(new ParallelDemuxer());

      // wrap each demuxer in a DemuxerBuffer, build a summary:
      for (std::list<TDemuxerPtr>::const_iterator
             i = demuxers.begin(); i != demuxers.end(); ++i)
      {
        const TDemuxerPtr & demuxer = *i;

        TDemuxerInterfacePtr
          buffer(new DemuxerBuffer(demuxer, buffer_duration));

        buffer->update_summary(discont_tolerance);
        parallel_demuxer->append(buffer);
      }

      // summarize the demuxer:
      parallel_demuxer->update_summary(discont_tolerance);

      SetOfTracks redacted_tracks = yae::get(redacted, filePath);
      if (redacted_tracks.empty())
      {
        source_demuxers[filePath] = parallel_demuxer;
      }
      else
      {
        TRedactedDemuxerPtr redacted_demuxer;
        redacted_demuxer.reset(new RedactedDemuxer(parallel_demuxer));
        redacted_demuxer->set_redacted(redacted_tracks);
        redacted_demuxer->update_summary(discont_tolerance);
        source_demuxers[filePath] = redacted_demuxer;
      }
    }

    TSerialDemuxerPtr serial_demuxer(new SerialDemuxer());

    for (std::list<ClipInfo>::const_iterator
           i = clips.begin(); i != clips.end(); ++i)
    {
      const ClipInfo & trim = *i;

      const TDemuxerInterfacePtr & demuxer =
        yae::at(source_demuxers, trim.source_);

      std::string track_id =
        trim.track_.empty() ? std::string("v:000") : trim.track_;

      if (!al::starts_with(track_id, "v:"))
      {
        // not a video track:
        continue;
      }
      const DemuxerSummary & summary = demuxer->summary();

      if (!yae::has(summary.decoders_, track_id))
      {
        // no such track:
        continue;
      }

      const Timeline::Track & track = summary.get_track_timeline(track_id);
      Timespan keep(track.pts_.front(), track.pts_.back());

      const FramerateEstimator & fe = yae::at(summary.fps_, track_id);
      double fps = fe.best_guess();

      if (!trim.t0_.empty() &&
          !parse_time(keep.t0_, trim.t0_.c_str(), NULL, NULL, fps))
      {
        av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t0_.c_str());
      }

      if (!trim.t1_.empty() &&
          !parse_time(keep.t1_, trim.t1_.c_str(), NULL, NULL, fps))
      {
        av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t1_.c_str());
      }

      TTrimmedDemuxerPtr clip_demuxer(new TrimmedDemuxer());
      clip_demuxer->trim(demuxer, track_id, keep);

      // summarize clip demuxer:
      clip_demuxer->update_summary(discont_tolerance);
      serial_demuxer->append(clip_demuxer);
    }

    if (serial_demuxer->empty())
    {
      av_log(NULL, AV_LOG_ERROR, "failed to open any input files, gave up");
      return TDemuxerInterfacePtr();
    }

    // unwrap serial demuxer if there is just 1 source:
    if (serial_demuxer->num_sources() == 1)
    {
      return serial_demuxer->sources().front();
    }

    serial_demuxer->update_summary(discont_tolerance);
    return serial_demuxer;
  }


  //----------------------------------------------------------------
  // rx::Loader::run
  //
  void
  rx::Loader::run()
  {
    try
    {
      if (src_clips_.empty())
      {
        this->load_sources();
      }
      else
      {
        this->load_source_clips();
      }
    }
    catch (const std::exception & e)
    {
      yae_wlog("rx::Loader::run exception: %s", e.what());
    }
    catch (...)
    {
      yae_wlog("rx::Loader::run unknown exception");
    }

    if (observer_)
    {
      observer_->done();
    }
  }

  //----------------------------------------------------------------
  // rx::Loader::get_demuxer
  //
  TDemuxerInterfacePtr
  rx::Loader::get_demuxer(const std::string & source)
  {
    if (!yae::has(demuxers_, source))
    {
      if (observer_)
      {
        observer_->began(source);
      }

      std::list<TDemuxerPtr> demuxers;
      if (!open_primary_and_aux_demuxers(source, demuxers))
      {
        // failed to open the primary resource:
       yae_wlog("failed to open %s, skipping...",
               source.c_str());
        return TDemuxerInterfacePtr();
      }

      TParallelDemuxerPtr parallel_demuxer(new ParallelDemuxer());

      // these are expressed in seconds:
      static const double buffer_duration = 1.0;
      static const double discont_tolerance = 0.1;

      // wrap each demuxer in a DemuxerBuffer, build a summary:
      for (std::list<TDemuxerPtr>::const_iterator
             i = demuxers.begin(); i != demuxers.end(); ++i)
      {
        const TDemuxerPtr & demuxer = *i;

        TDemuxerInterfacePtr
          buffer(new DemuxerBuffer(demuxer, buffer_duration));

        buffer->update_summary(discont_tolerance);
        parallel_demuxer->append(buffer);
      }

      // summarize the demuxer:
      parallel_demuxer->update_summary(discont_tolerance);

      SetOfTracks redacted_tracks = yae::get(redacted_, source);
      if (redacted_tracks.empty())
      {
        demuxers_[source] = parallel_demuxer;
      }
      else
      {
        TRedactedDemuxerPtr redacted_demuxer;
        redacted_demuxer.reset(new RedactedDemuxer(parallel_demuxer));
        redacted_demuxer->set_redacted(redacted_tracks);
        redacted_demuxer->update_summary(discont_tolerance);
        demuxers_[source] = redacted_demuxer;
      }
    }

    return demuxers_[source];
  }

  //----------------------------------------------------------------
  // rx::Loader::load_sources
  //
  void
  rx::Loader::load_sources()
  {
    for (std::set<std::string>::const_iterator
           i = sources_.begin(); i != sources_.end(); ++i)
    {
      try
      {
        const std::string & source = *i;

        TDemuxerInterfacePtr demuxer = get_demuxer(source);
        if (!demuxer)
        {
          continue;
        }

        // shortcut:
        const DemuxerSummary & summary = demuxer->summary();
        std::string track_id = summary.suggest_clip_track_id();
        if (track_id.empty())
        {
          continue;
        }

        if (yae::has(summary.decoders_, track_id))
        {
          const Timeline::Track & track = summary.get_track_timeline(track_id);
          Timespan keep(track.pts_.front(), track.pts_.back());
          TClipPtr clip(new Clip(demuxer, track_id, keep));

          if (observer_)
          {
            observer_->loaded(source, demuxer, clip);
          }
        }
      }
      catch (const std::exception & e)
      {
       yae_wlog("rx::Loader::load_sources exception: %s", e.what());
      }
      catch (...)
      {
       yae_wlog("rx::Loader::load_sources unknown exception");
      }
    }
  }

  //----------------------------------------------------------------
  // rx::Loader::load_source_clips
  //
  void
  rx::Loader::load_source_clips()
  {
    for (std::list<ClipInfo>::const_iterator
           i = src_clips_.begin(); i != src_clips_.end(); ++i)
    {
      try
      {
        const ClipInfo & trim = *i;

        TDemuxerInterfacePtr demuxer = get_demuxer(trim.source_);
        if (!demuxer)
        {
          // failed to demux:
          continue;
        }

        const DemuxerSummary & summary = demuxer->summary();
        std::string track_id = trim.track_;

        if (track_id.empty())
        {
          track_id = summary.suggest_clip_track_id();
        }

        if (!yae::has(summary.decoders_, track_id))
        {
          // no such track:
          continue;
        }

        const Timeline::Track & track = summary.get_track_timeline(track_id);
        Timespan keep(track.pts_.front(), track.pts_.back());

        if (!trim.t0_.empty() &&
            !parse_time(keep.t0_, trim.t0_.c_str()))
        {
          yae_elog("failed to parse %s", trim.t0_.c_str());
        }

        if (!trim.t1_.empty() &&
            !parse_time(keep.t1_, trim.t1_.c_str()))
        {
          yae_elog("failed to parse %s", trim.t1_.c_str());
        }

        if (observer_)
        {
          TClipPtr clip(new Clip(demuxer, track_id, keep));
          observer_->loaded(trim.source_, demuxer, clip);
        }
      }
      catch (const std::exception & e)
      {
       yae_wlog("rx::Loader::load_source_clips exception: %s", e.what());
      }
      catch (...)
      {
       yae_wlog("rx::Loader::load_source_clips unknown exception");
      }
    }
  }

  //----------------------------------------------------------------
  // ProgressObserver
  //
  struct ProgressObserver : rx::Loader::IProgressObserver
  {
    ProgressObserver(RemuxModel & model):
      model_(model)
    {}

    // virtual:
    void began(const std::string & source)
    {
      (void)source;
    }

    // virtual:
    void loaded(const std::string & source,
                const TDemuxerInterfacePtr & demuxer,
                const TClipPtr & clip)
    {
      model_.sources_.push_back(source);
      model_.demuxer_[source] = demuxer;
      model_.source_[demuxer] = source;
      model_.clips_.push_back(clip);
    }

    // virtual:
    void done()
    {}

    RemuxModel & model_;
  };

  //----------------------------------------------------------------
  // load_yaerx
  //
  TSerialDemuxerPtr
  load_yaerx(const std::string & filename)
  {
    std::string json_str = TOpenFile(filename.c_str(), "rb").read();

    std::set<std::string> sources;
    std::map<std::string, SetOfTracks> redacted;
    std::list<ClipInfo> src_clips;
    if (!RemuxModel::parse_json_str(json_str, sources, redacted, src_clips))
    {
      return TSerialDemuxerPtr();
    }

    RemuxModel model;
    {
      rx::Loader::TProgressObserverPtr observer(new ProgressObserver(model));
      rx::Loader loader(model.demuxer_,
                        sources,
                        redacted,
                        src_clips,
                        observer);
      loader.run();
    }

    TSerialDemuxerPtr serial_demuxer = model.make_serial_demuxer();
    return serial_demuxer;
  }

}

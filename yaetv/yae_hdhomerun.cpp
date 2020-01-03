// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <iomanip>
#include <iostream>
#include <list>
#include <stdio.h>
#include <vector>

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#endif

// ffmpeg:
extern "C" {
#include <libavformat/avio.h>
}

// hdhomerun:
#include <hdhomerun.h>

// jsoncpp:
#include "json/json.h"

// yae:
#include "yae/api/yae_log.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_mpeg_ts.h"

// epg:
#include "yae_hdhomerun.h"
#include "yae_signal_handler.h"


// namespace shortcut:
namespace fs = boost::filesystem;



//----------------------------------------------------------------
// hdhomerun_device_deallocator
//
struct hdhomerun_device_deallocator
{
  inline static void destroy(hdhomerun_device_t * hd)
  {
    if (hd)
    {
      hdhomerun_device_destroy(hd);
    }
  }
};

//----------------------------------------------------------------
// hdhomerun_devptr_t
//
typedef yae::shared_ptr<hdhomerun_device_t,
                        hdhomerun_device_t,
                        hdhomerun_device_deallocator> hdhomerun_devptr_t;


//----------------------------------------------------------------
// hdhomerun_debug_deallocator
//
struct hdhomerun_debug_deallocator
{
  inline static void destroy(hdhomerun_debug_t * dbg)
  {
    if (dbg)
    {
      hdhomerun_debug_destroy(dbg);
    }
  }
};

//----------------------------------------------------------------
// hdhomerun_dbgptr_t
//
typedef yae::shared_ptr<hdhomerun_debug_t,
                        hdhomerun_debug_t,
                        hdhomerun_debug_deallocator> hdhomerun_dbgptr_t;


namespace yae
{

  //----------------------------------------------------------------
  // LockTuner
  //
  struct LockTuner
  {
    hdhomerun_devptr_t hd_ptr_;
    std::string lockkey_path_;

    LockTuner(const hdhomerun_devptr_t & hd_ptr = hdhomerun_devptr_t(),
              const std::string & lockkey_dir = std::string());
    ~LockTuner();

    void lock(const hdhomerun_devptr_t & hd_ptr,
              const std::string & lockkey_dir);
    void unlock();
  };

  //----------------------------------------------------------------
  // LockTuner::LockTuner
  //
  LockTuner::LockTuner(const hdhomerun_devptr_t & hd_ptr,
                       const std::string & lockkey_dir)
  {
    lock(hd_ptr, lockkey_dir);
  }

  //----------------------------------------------------------------
  // LockTuner::~LockTuner
  //
  LockTuner::~LockTuner()
  {
    unlock();
  }

  //----------------------------------------------------------------
  // LockTuner::lock
  //
  void
  LockTuner::lock(const hdhomerun_devptr_t & hd_ptr,
                  const std::string & lockkey_dir)
  {
    if (hd_ptr.get() == hd_ptr_.get())
    {
      return;
    }

    unlock();

    hdhomerun_device_t & hd = *hd_ptr;
    std::string name = hdhomerun_device_get_name(&hd);
    lockkey_path_ = (fs::path(lockkey_dir) / (name + ".lockkey")).string();

    yae::TOpenFile lock_file;
    if (lock_file.open(lockkey_path_, "rb"))
    {
      lock_file.close();
      hdhomerun_device_tuner_lockkey_force(&hd);
      yae::remove_utf8(lockkey_path_);
    }

    char * ret_error = NULL;
    if (hdhomerun_device_tuner_lockkey_request(&hd, &ret_error) <= 0)
    {
      YAE_THROW("failed to lock tuner: %s%s",
                name.c_str(),
                ret_error ? ret_error : "");
    }

    lock_file.open(lockkey_path_, "wb");
    hd_ptr_ = hd_ptr;
  }

  //----------------------------------------------------------------
  // LockTuner::unlock
  //
  void
  LockTuner::unlock()
  {
    if (hd_ptr_)
    {
      hdhomerun_device_t & hd = *hd_ptr_;
      hdhomerun_device_tuner_lockkey_release(&hd);
      yae::remove_utf8(lockkey_path_);
      hd_ptr_.reset();
    }
  }


  //----------------------------------------------------------------
  // HDHomeRun::Session::Private
  //
  struct HDHomeRun::Session::Private
  {
    Private():
      stop_(TTime::now())
    {}

    inline bool expired() const
    {
      TTime t = TTime::now();
      return stop_ <= t;
    }

    inline void extend(const TTime & t)
    {
      if (stop_ < t)
      {
        stop_ = t;
      }
    }

    inline void finish()
    {
      stop_ = TTime::now();
    }

    TTime stop_;
    LockTuner lock_tuner_;
    hdhomerun_devptr_t hd_ptr_;
    std::string tuner_name_;
  };


  //----------------------------------------------------------------
  // HDHomeRun::Session::Session
  //
  HDHomeRun::Session::Session():
    private_(new HDHomeRun::Session::Private())
  {}

  //----------------------------------------------------------------
  // HDHomeRun::Session::~Session
  //
  HDHomeRun::Session::~Session()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Session::tuner_name
  //
  const std::string &
  HDHomeRun::Session::tuner_name() const
  {
    return private_->tuner_name_;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Session::expired
  //
  bool
  HDHomeRun::Session::expired() const
  {
    return private_->expired();
  }

  //----------------------------------------------------------------
  // HDHomeRun::Session::extend
  //
  void
  HDHomeRun::Session::extend(const TTime & t)
  {
    private_->extend(t);
  }

  //----------------------------------------------------------------
  // HDHomeRun::Session::finish
  //
  void
  HDHomeRun::Session::finish()
  {
    private_->finish();
  }


  //----------------------------------------------------------------
  // HDHomeRun::Private
  //
  struct HDHomeRun::Private
  {
    Private(const std::string & cache_dir);

    void discover_tuners(std::list<std::string> & tuners);
    bool init(const std::string & tuner_name);

    bool get_channels(std::map<uint32_t, std::string> & chan_freq) const;
    bool get_channels(const std::string & freq, TChannels & channels) const;
    bool get_channels(std::map<std::string, TChannels> & freq_channels) const;

    HDHomeRun::TSessionPtr open_session();
    HDHomeRun::TSessionPtr open_session(const std::string & tuner_name);
    HDHomeRun::TSessionPtr open_session(const std::string & tuner_name,
                                        const hdhomerun_devptr_t & hd_ptr);

    bool scan_channels(const HDHomeRun::TSessionPtr & session_ptr,
                       const IAssert & keep_going);

    void tune_to(const HDHomeRun::TSessionPtr & session_ptr,
                 const uint32_t frequency);

    void capture(yae::weak_ptr<IStream> stream_ptr,
                 const HDHomeRun::TSessionPtr & session_ptr,
                 const std::string & frequency);

    mutable boost::mutex mutex_;
    std::vector<struct hdhomerun_discover_device_t> devices_;
    std::map<std::string, hdhomerun_devptr_t> tuners_;
    Json::Value tuner_cache_;
    std::string cache_dir_;

    // keep track of existing sessions, but don't extend their lifetime:
    std::map<std::string, yae::weak_ptr<HDHomeRun::Session> > sessions_;

    // for HDHomeRun debug logging:
    hdhomerun_dbgptr_t dbg_;
  };


  //----------------------------------------------------------------
  // HDHomeRun::Private::Private
  //
  HDHomeRun::Private::Private(const std::string & cache_dir):
    devices_(64),
    cache_dir_(cache_dir)
  {
    YAE_THROW_IF(!yae::mkdir_p(cache_dir_));

    dbg_.reset(hdhomerun_debug_create());
    hdhomerun_debug_set_prefix(dbg_.get(), "yaetv-hdhd");

    std::string dbg_path = (fs::path(cache_dir) / "yaetv-hdhr.log").string();
    hdhomerun_debug_set_filename(dbg_.get(), dbg_path.c_str());

    hdhomerun_debug_enable(dbg_.get());

    std::string ts = unix_epoch_time_to_localtime_str(TTime::now().get(1));
    hdhomerun_debug_printf(dbg_.get(), "FIXME: pkoshevoy: %s", ts.c_str());
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::discover_tuners
  //
  void
  HDHomeRun::Private::discover_tuners(std::list<std::string> & tuners)
  {
    YAE_ASSERT(tuners.empty());

    // discover HDHomeRun devices:
    int num_found =
      hdhomerun_discover_find_devices_custom_v2
      (0, // target_ip, 0 to auto-detect IP address(es)
       HDHOMERUN_DEVICE_TYPE_TUNER,
       HDHOMERUN_DEVICE_ID_WILDCARD,
       &(devices_[0]),
       devices_.size()); // max devices

    for (int i = 0; i < num_found; i++)
    {
      const hdhomerun_discover_device_t & found = devices_[i];
      yae_ilog("hdhomerun device %08X found at %u.%u.%u.%u\n",
               (unsigned int)found.device_id,
               (unsigned int)(found.ip_addr >> 24) & 0x0FF,
               (unsigned int)(found.ip_addr >> 16) & 0x0FF,
               (unsigned int)(found.ip_addr >> 8) & 0x0FF,
               (unsigned int)(found.ip_addr >> 0) & 0x0FF);

      for (int tuner = 0; tuner < found.tuner_count; tuner++)
      {
        hdhomerun_devptr_t hd_ptr(hdhomerun_device_create(found.device_id,
                                                          found.ip_addr,
                                                          tuner,
                                                          NULL));

        if (!hd_ptr)
        {
          continue;
        }

        hdhomerun_device_t & hd = *hd_ptr;
        std::string name = hdhomerun_device_get_name(&hd);
        tuners_[name] = hd_ptr;

        std::string model = hdhomerun_device_get_model_str(&hd);
        uint32_t target_addr = hdhomerun_device_get_local_machine_addr(&hd);
        yae_ilog("%s, id: %s, target addr: %u.%u.%u.%u",
                 model.c_str(),
                 name.c_str(),
                 (unsigned int)(target_addr >> 24) & 0x0FF,
                 (unsigned int)(target_addr >> 16) & 0x0FF,
                 (unsigned int)(target_addr >> 8) & 0x0FF,
                 (unsigned int)(target_addr >> 0) & 0x0FF);
        tuners.push_back(name);
      }
    }
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::init
  //
  bool
  HDHomeRun::Private::init(const std::string & tuner_name)
  {
    std::map<std::string, hdhomerun_devptr_t>::const_iterator
      found = tuners_.find(tuner_name);
    YAE_THROW_IF(found == tuners_.end());

    std::string cache_path = (fs::path(cache_dir_) / tuner_name).string();
    Json::Value & tuner_cache = tuner_cache_[tuner_name];

    // load from cache:
    if (yae::TOpenFile(cache_path.c_str(), "rb").load(tuner_cache))
    {
      return true;
    }

    const hdhomerun_devptr_t & hd_ptr = found->second;
    HDHomeRun::TSessionPtr session_ptr = open_session(tuner_name, hd_ptr);
    DontStop dont_stop;
    return scan_channels(session_ptr, dont_stop);
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::get_channels
  //
  bool
  HDHomeRun::Private::
  get_channels(std::map<uint32_t, std::string> & chan_freq) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    for (Json::Value::const_iterator i = tuner_cache_.begin();
         i != tuner_cache_.end(); ++i)
    {
      try
      {
        std::string tuner_name = i.key().asString();
        const Json::Value & tuner_info = *i;
        const Json::Value & frequencies = tuner_info["frequencies"];

        for (Json::Value::const_iterator j = frequencies.begin();
             j != frequencies.end(); ++j)
        {
          std::string frequency = j.key().asString();
          const Json::Value & info = *j;
          const Json::Value & programs = info["programs"];
          for (Json::Value::const_iterator k = programs.begin();
               k != programs.end(); ++k)
          {
            const Json::Value & program = *k;
            const uint32_t ch_num =
              yae::mpeg_ts::channel_number(program["virtual_major"].asUInt(),
                                           program["virtual_minor"].asUInt());
            chan_freq[ch_num] = frequency;
          }
        }

        return true;
      }
      catch (...)
      {}
    }

    return false;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::get_channels
  //
  bool
  HDHomeRun::Private::
  get_channels(const std::string & frequency, TChannels & channels) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    for (Json::Value::const_iterator i = tuner_cache_.begin();
         i != tuner_cache_.end(); ++i)
    {
      try
      {
        std::string tuner_name = i.key().asString();
        const Json::Value & tuner_info = *i;
        const Json::Value & frequencies = tuner_info["frequencies"];

        if (!frequencies.isMember(frequency))
        {
          continue;
        }

        const Json::Value & info = frequencies[frequency];
        const Json::Value & programs = info["programs"];
        for (Json::Value::const_iterator k = programs.begin();
             k != programs.end(); ++k)
        {
          const Json::Value & program = *k;
          uint16_t major = uint16_t(program["virtual_major"].asUInt());
          uint16_t minor = uint16_t(program["virtual_minor"].asUInt());
          std::string name = program["name"].asString();
          channels[major][minor] = name;
        }

        return true;
      }
      catch (...)
      {}
    }

    return false;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::get_channels
  //
  bool
  HDHomeRun::Private::
  get_channels(std::map<std::string, TChannels> & freq_channels) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    for (Json::Value::const_iterator i = tuner_cache_.begin();
         i != tuner_cache_.end(); ++i)
    {
      try
      {
        std::string tuner_name = i.key().asString();
        const Json::Value & tuner_info = *i;
        const Json::Value & frequencies = tuner_info["frequencies"];

        for (Json::Value::const_iterator j = frequencies.begin();
             j != frequencies.end(); ++j)
        {
          std::string frequency = j.key().asString();
          TChannels channels;
          const Json::Value & info = *j;
          const Json::Value & programs = info["programs"];
          for (Json::Value::const_iterator k = programs.begin();
               k != programs.end(); ++k)
          {
            const Json::Value & program = *k;
            uint16_t major = uint16_t(program["virtual_major"].asUInt());
            uint16_t minor = uint16_t(program["virtual_minor"].asUInt());
            std::string name = program["name"].asString();
            channels[major][minor] = name;
          }

          if (!channels.empty())
          {
            freq_channels[frequency].swap(channels);
          }
        }

        return true;
      }
      catch (...)
      {}
    }

    return false;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session()
  {
    HDHomeRun::TSessionPtr session;
    for (std::map<std::string, hdhomerun_devptr_t>::reverse_iterator
           i = tuners_.rbegin(); i != tuners_.rend() && !session; ++i)
    {
      const std::string & tuner_name = i->first;
      const hdhomerun_devptr_t & hd_ptr = i->second;
      session = open_session(tuner_name, hd_ptr);
    }
    return session;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session(const std::string & tuner_name)
  {
    HDHomeRun::TSessionPtr session;
    std::map<std::string, hdhomerun_devptr_t>::const_iterator
      found = tuners_.find(tuner_name);
    if (found != tuners_.end())
    {
      const hdhomerun_devptr_t & hd_ptr = found->second;
      session = open_session(tuner_name, hd_ptr);
    }
    return session;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session(const std::string & tuner_name,
                                   const hdhomerun_devptr_t & hd_ptr)
  {
    HDHomeRun::TSessionPtr session_ptr;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      session_ptr = sessions_[tuner_name].lock();

      if (session_ptr)
      {
        // we should not attempt sharing an existing session,
        // and hopefully tuner locking would prevent it anyway:
        return HDHomeRun::TSessionPtr();
      }

      session_ptr.reset(new HDHomeRun::Session());
      sessions_[tuner_name] = session_ptr;
    }

    try
    {
      hdhomerun_device_t * hd = hd_ptr.get();

      // clear stale lock:
      try { LockTuner lock(hd_ptr, cache_dir_); } catch (...) {}

      char * owner = NULL;
      if (hdhomerun_device_get_tuner_lockkey_owner(hd, &owner) != 1)
      {
        yae_wlog("hdhomerun_device_get_tuner_lockkey_owner failed for %s",
                 tuner_name.c_str());
        return HDHomeRun::TSessionPtr();
      }

      if (strcmp(owner, "none") != 0)
      {
        // tuner belongs to another process, ignore:
        yae_wlog("skipping tuner %s, current owner: %s",
                 tuner_name.c_str(),
                 owner);
        return HDHomeRun::TSessionPtr();;
      }

      char * status_str = NULL;
      hdhomerun_tuner_status_t status;
      if (hdhomerun_device_get_tuner_status(hd,
                                            &status_str,
                                            &status) == 1)
      {
        yae_ilog("%s, status: %s", tuner_name.c_str(), status_str);
      }

      HDHomeRun::Session::Private & session = *(session_ptr->private_);
      session.lock_tuner_.lock(hd_ptr, cache_dir_);
      session.tuner_name_ = tuner_name;
      session.hd_ptr_ = hd_ptr;
      return session_ptr;
    }
    catch (const std::exception & e)
    {
      yae_wlog("failed to lock tuner %s: %s",
               tuner_name.c_str(),
               e.what());
    }
    catch (...)
    {
      yae_wlog("failed to configure tuner %s: unexpected exception",
               tuner_name.c_str());
    }

    return HDHomeRun::TSessionPtr();
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::scan_channels
  //
  bool
  HDHomeRun::Private::scan_channels(const HDHomeRun::TSessionPtr & session_ptr,
                                    const IAssert & keep_going)
  {
    if (!session_ptr)
    {
      return false;
    }

    // shortcuts:
    HDHomeRun::Session::Private & session = *(session_ptr->private_);
    const std::string & tuner_name = session.tuner_name_;
    hdhomerun_device_t * hd = session.hd_ptr_.get();

    YAE_ASSERT(yae::mkdir_p(cache_dir_));

    std::string cache_path = (fs::path(cache_dir_) / tuner_name).string();
    Json::Value tuner_cache;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      tuner_cache = tuner_cache_[tuner_name];
    }

    // check the cache:
    {
      int64_t now = yae::TTime::now().get(1);
      int64_t timestamp = tuner_cache.get("timestamp", 0).asInt64();
      int64_t elapsed = now - timestamp;
      int64_t threshold = 10 * 24 * 60 * 60;

      if (threshold < elapsed)
      {
        // cache is too old, purge it:
        yae_wlog("%s channel scan cache expired", cache_path.c_str());
        tuner_cache.clear();
      }
    }

    if (!tuner_cache.empty())
    {
      int64_t ts = tuner_cache.get("timestamp", 0).asInt64();

      struct tm localtime;
      yae::unix_epoch_time_to_localtime(ts, localtime);
      std::string date = yae::to_yyyymmdd_hhmmss(localtime);

      yae_ilog("%s skipping channel scan, using cache from %s",
               tuner_name.c_str(),
               date.c_str());
      return true;
    }

    try
    {
      hdhomerun_device_set_tuner_target(hd, "none");

      char * channelmap = NULL;
      if (hdhomerun_device_get_tuner_channelmap(hd, &channelmap) <= 0)
      {
        YAE_THROW("failed to query channelmap from device");
      }

      const char * scan_group =
        hdhomerun_channelmap_get_channelmap_scan_group(channelmap);

      if (!scan_group)
      {
        YAE_THROW("unknown channelmap '%s'", channelmap);
      }

      if (hdhomerun_device_channelscan_init(hd, scan_group) <= 0)
      {
        YAE_THROW("failed to initialize channel scan: %s", scan_group);
      }

      while (keep_going())
      {
        struct hdhomerun_channelscan_result_t result;
        int ret = hdhomerun_device_channelscan_advance(hd, &result);
        if (ret <= 0)
        {
          yae_wlog("hdhomerun_device_channelscan_advance: %i", ret);
          break;
        }

        yae_ilog("SCANNING: %u (%s)", result.frequency, result.channel_str);
        tune_to(session_ptr, result.frequency);

        struct hdhomerun_tuner_status_t status;
        ret = hdhomerun_device_wait_for_lock(hd, &status);
        if (ret < 0)
        {
          yae_wlog("hdhomerun_device_wait_for_lock: %i", ret);
        }

        ret = hdhomerun_device_channelscan_detect(hd, &result);
        if (ret < 0)
        {
          yae_wlog("hdhomerun_device_channelscan_detect: %i", ret);
          // break;
          continue;
        }

        if (ret == 0)
        {
          continue;
        }

        yae_ilog("LOCK: %s (ss=%u snq=%u seq=%u)",
                 result.status.lock_str,
                 result.status.signal_strength,
                 result.status.signal_to_noise_quality,
                 result.status.symbol_error_quality);

        if (result.transport_stream_id_detected)
        {
          yae_ilog("TSID: 0x%04X", result.transport_stream_id);
        }

        if (result.original_network_id_detected)
        {
          yae_ilog("ONID: 0x%04X", result.original_network_id);
        }

        if (!result.program_count)
        {
          continue;
        }

        Json::Value v;
        v["channel_str"] = result.channel_str;
        v["channelmap"] = result.channelmap;
        v["frequency"] = result.frequency;

        v["original_network_id"] = result.original_network_id;
        v["original_network_id_detected"] =
          result.original_network_id_detected;

        v["transport_stream_id"] = result.transport_stream_id;
        v["transport_stream_id_detected"] =
          result.transport_stream_id_detected;

        v["program_count"] = result.program_count;
        Json::Value & programs = v["programs"];

        // status:
        {
          Json::Value s;
          s["channel"] = result.status.channel;
          s["lock_str"] = result.status.lock_str;
          s["lock_supported"] = result.status.lock_supported;
          s["lock_unsupported"] = result.status.lock_unsupported;

          s["signal_present"] = result.status.signal_present;
          s["signal_strength"] = result.status.signal_strength;

          s["signal_to_noise_quality"] =
            result.status.signal_to_noise_quality;

          s["symbol_error_quality"] =
            result.status.symbol_error_quality;

          v["status"] = s;
        }

        for (int j = 0; j < result.program_count; j++)
        {
          const hdhomerun_channelscan_program_t & program =
            result.programs[j];

          yae_ilog("PROGRAM %s", program.program_str);

          std::string key = yae::strfmt("%02i.%i %s",
                                        program.virtual_major,
                                        program.virtual_minor,
                                        program.name);
          Json::Value p;

          p["program_number"] = program.program_number;
          p["virtual_major"] = program.virtual_major;
          p["virtual_minor"] = program.virtual_minor;
          p["name"] = program.name;
          p["type"] = program.type;

          programs.append(p);

          Json::Value & channels = tuner_cache["channels"];
          Json::Value z;
          z["frequency"] = result.frequency;
          z["program"] = p;

          channels[key] = z;
        }

        Json::Value & frequencies = tuner_cache["frequencies"];
        frequencies[yae::to_text(result.frequency)] = v;
      }
    }
    catch (const std::exception & e)
    {
      yae_wlog("failed to configure tuner %s: %s",
               tuner_name.c_str(),
               e.what());
      return false;
    }
    catch (...)
    {
      yae_wlog("failed to configure tuner %s: unexpected exception",
               tuner_name.c_str());
      return false;
    }

    if (!tuner_cache.empty())
    {
      tuner_cache["timestamp"] = Json::Int64(yae::TTime::now().get(1));
      YAE_ASSERT(yae::TOpenFile(cache_path.c_str(), "wb").save(tuner_cache));

      boost::unique_lock<boost::mutex> lock(mutex_);
      tuner_cache_[tuner_name] = tuner_cache;
    }

    return true;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::tune_to
  //
  void
  HDHomeRun::Private::tune_to(const HDHomeRun::TSessionPtr & session_ptr,
                              const uint32_t frequency)
  {
    // shortcuts:
    HDHomeRun::Session::Private & session = *(session_ptr->private_);
    hdhomerun_device_t * hd = session.hd_ptr_.get();
    unsigned int tuner = hdhomerun_device_get_tuner(hd);

    std::string channel = yae::strfmt("auto:%" PRIu32 "", + frequency);
    std::string param = yae::strfmt("/tuner%i/channel", tuner);
    char * error = NULL;

    if (hdhomerun_device_set_var(hd,
                                 param.c_str(),
                                 channel.c_str(),
                                 NULL,
                                 &error) <= 0)
    {
      YAE_THROW("failed to set channel, error: %s", error);
    }
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::capture
  //
  void
  HDHomeRun::Private::capture(yae::weak_ptr<IStream> stream_weak_ptr,
                              const HDHomeRun::TSessionPtr & session_ptr,
                              const std::string & frequency)
  {
    // shortcuts:
    HDHomeRun::Session::Private & session = *(session_ptr->private_);
    const std::string & tuner_name = session.tuner_name_;
    hdhomerun_device_t * hd = session.hd_ptr_.get();

    try
    {
      Json::Value programs;
      Json::Value status;
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        const Json::Value & info =
          tuner_cache_[tuner_name]["frequencies"][frequency];
        programs = info.get("programs", Json::Value());
        status = info.get("status", Json::Value());
      }

      std::string channel = status.get("channel", "").asString();
      unsigned int tuner = hdhomerun_device_get_tuner(hd);
      std::string param = yae::strfmt("/tuner%i/channel", tuner);
      char * error = NULL;

      if (hdhomerun_device_set_var(hd,
                                   param.c_str(),
                                   channel.c_str(),
                                   NULL,
                                   &error) <= 0)
      {
        YAE_THROW("failed to set channel, error: %s", error);
      }

      if (hdhomerun_device_stream_start(hd) <= 0)
      {
        YAE_THROW("failed to start stream for %s", channel.c_str());
      }

      std::string channels_txt;
      {
        std::ostringstream oss;
        const char * separator = "";
        for (Json::Value::iterator k = programs.begin();
             k != programs.end(); ++k)
        {
          Json::Value prog = *k;
          std::string prog_name = prog["name"].asString();
          uint32_t major = prog["virtual_major"].asUInt();
          uint32_t minor = prog["virtual_minor"].asUInt();
          oss << separator << major << '.' << minor << ' ' << prog_name;
          separator = ", ";
        }
        channels_txt = oss.str().c_str();
      }

      yae_wlog("%s %sHz: capturing %s",
               session.tuner_name_.c_str(),
               frequency.c_str(),
               channels_txt.c_str());

      while (true)
      {
        yae::shared_ptr<IStream> stream_ptr = stream_weak_ptr.lock();
        if (!stream_ptr)
        {
          return;
        }

        IStream & stream = *stream_ptr;
        if (!stream.is_open())
        {
          return;
        }

#ifdef _WIN32
        // indicate activity to suppress auto sleep mode:
        SetThreadExecutionState(ES_SYSTEM_REQUIRED);
#endif

        static const std::size_t buffer_size_1s = VIDEO_DATA_BUFFER_SIZE_1S;
        std::size_t buffer_size = 0;
        uint8_t * buffer = hdhomerun_device_stream_recv(hd,
                                                        buffer_size_1s,
                                                        &buffer_size);

        if (buffer && !stream.push(buffer, buffer_size))
        {
          return;
        }

        if (session.expired())
        {
          return;
        }

        if (!buffer)
        {
          msleep_approx(64);
        }
      }
    }
    catch (const std::exception & e)
    {
      yae_wlog("%s %sHz: stream failed: %s",
               tuner_name.c_str(),
               frequency.c_str(),
               e.what());
    }
    catch (...)
    {
      yae_wlog("%s %sHz: stream failed: unexpected exception",
               tuner_name.c_str(),
               frequency.c_str());
    }
  }


  //----------------------------------------------------------------
  // HDHomeRun::HDHomeRun
  //
  HDHomeRun::HDHomeRun(const std::string & cache_dir):
    private_(new HDHomeRun::Private(cache_dir))
  {}

  //----------------------------------------------------------------
  // HDHomeRun::~HDHomeRun
  //
  HDHomeRun::~HDHomeRun()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // HDHomeRun::discover_tuners
  //
  void
  HDHomeRun::discover_tuners(std::list<std::string> & tuners)
  {
    private_->discover_tuners(tuners);
  }

  //----------------------------------------------------------------
  // HDHomeRun::init
  //
  bool
  HDHomeRun::init(const std::string & tuner_name)
  {
    return private_->init(tuner_name);
  }

  //----------------------------------------------------------------
  // HDHomeRun::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::open_session()
  {
    return private_->open_session();
  }

  //----------------------------------------------------------------
  // HDHomeRun::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::open_session(const std::string & tuner)
  {
    return private_->open_session(tuner);
  }

  //----------------------------------------------------------------
  // HDHomeRun::scan_channels
  //
  bool
  HDHomeRun::scan_channels(HDHomeRun::TSessionPtr session_ptr,
                           const IAssert & keep_going)
  {
    return private_->scan_channels(session_ptr, keep_going);
  }

  //----------------------------------------------------------------
  // HDHomeRun::capture
  //
  void
  HDHomeRun::capture(const HDHomeRun::TSessionPtr session_ptr,
                     yae::weak_ptr<IStream> stream_ptr,
                     const std::string & frequency)
  {
    private_->capture(stream_ptr, session_ptr, frequency);
  }

  //----------------------------------------------------------------
  // HDHomeRun::get_channels
  //
  bool
  HDHomeRun::get_channels(std::map<uint32_t, std::string> & ch_freq) const
  {
    return private_->get_channels(ch_freq);
  }

  //----------------------------------------------------------------
  // HDHomeRun::get_channels
  //
  bool
  HDHomeRun::get_channels(const std::string & freq, TChannels & chans) const
  {
    return private_->get_channels(freq, chans);
  }

  //----------------------------------------------------------------
  // HDHomeRun::get_channels
  //
  bool
  HDHomeRun::get_channels(std::map<std::string, TChannels> & channels) const
  {
    return private_->get_channels(channels);
  }
}

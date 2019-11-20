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



namespace yae
{
  //----------------------------------------------------------------
  // LockTuner
  //
  struct LockTuner
  {
    hdhomerun_devptr_t hd_ptr_;
    std::string lockkey_path_;

    //----------------------------------------------------------------
    // LockTuner
    //
    LockTuner(hdhomerun_devptr_t hd_ptr)
    {
      lock(hd_ptr);
    }

    //----------------------------------------------------------------
    // ~LockTuner
    //
    ~LockTuner()
    {
      unlock();
    }

    //----------------------------------------------------------------
    // lock
    //
    void lock(hdhomerun_devptr_t hd_ptr)
    {
      if (hd_ptr == hd_ptr_)
      {
        return;
      }

      unlock();

      hdhomerun_device_t & hd = *hd_ptr;
      std::string name = hdhomerun_device_get_name(&hd);

      std::string yaepg_dir = yae::get_user_folder_path(".yaepg");
      lockkey_path_ = (fs::path(yaepg_dir) / (name + ".lockkey")).string();

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
    // unlock
    //
    void unlock()
    {
      if (hd_ptr_)
      {
        hdhomerun_device_t & hd = *hd_ptr_;
        hdhomerun_device_tuner_lockkey_release(&hd);
        yae::remove_utf8(lockkey_path_);
      }
    }
  };


  //----------------------------------------------------------------
  // HDHomeRun::Private
  //
  struct HDHomeRun::Private
  {
    Private();

    void capture_all();

    std::vector<struct hdhomerun_discover_device_t> devices_;
    std::map<std::string, hdhomerun_devptr_t> tuners_;
    Json::Value tuner_cache_;
  };


  //----------------------------------------------------------------
  // HDHomeRun::Private::Private
  //
  HDHomeRun::Private::Private():
    devices_(64)
  {
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
      yae_dlog("hdhomerun device %08X found at %u.%u.%u.%u\n",
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
        const char * name = hdhomerun_device_get_name(&hd);
        const char * model = hdhomerun_device_get_model_str(&hd);
        uint32_t target_addr = hdhomerun_device_get_local_machine_addr(&hd);

        yae_dlog("%s, id: %s, target addr: %u.%u.%u.%u",
                 model,
                 name,
                 (unsigned int)(target_addr >> 24) & 0x0FF,
                 (unsigned int)(target_addr >> 16) & 0x0FF,
                 (unsigned int)(target_addr >> 8) & 0x0FF,
                 (unsigned int)(target_addr >> 0) & 0x0FF);

        // clear stale lock:
        try { LockTuner lock(hd_ptr); } catch (...) {}

        char * owner_str = NULL;
        if (hdhomerun_device_get_tuner_lockkey_owner(&hd, &owner_str) != 1)
        {
          yae_wlog("hdhomerun_device_get_tuner_lockkey_owner failed for %s",
                   name);
          continue;
        }

        if (strcmp(owner_str, "none") != 0)
        {
          // tuner belongs to another process, ignore:
          yae_wlog("skipping tuner %s, current owner: %s", name, owner_str);
          continue;
        }

        char * status_str = NULL;
        hdhomerun_tuner_status_t status = { 0 };
        if (hdhomerun_device_get_tuner_status(&hd,
                                              &status_str,
                                              &status) == 1)
        {
          yae_dlog("\tstatus: %s", status_str);
        }

        std::string yaepg_dir = yae::get_user_folder_path(".yaepg");
        YAE_ASSERT(yae::mkdir_p(yaepg_dir));

        std::string cache_path = (fs::path(yaepg_dir) / name).string();
        Json::Value & tuner_cache = tuner_cache_[name];

        // load from cache
        {
          yae::TOpenFile cache_file(cache_path.c_str(), "rb");
          if (cache_file.is_open())
          {
            std::string document = cache_file.read();
            if (Json::Reader().parse(document, tuner_cache))
            {
              int64_t now = yae::TTime::now().get(1);
              int64_t timestamp = tuner_cache.get("timestamp", 0).asInt64();
              int64_t elapsed = now - timestamp;
              int64_t threshold = 10 * 24 * 60 * 60;
              if (threshold < elapsed)
              {
                // cache is too old, purge it:
                yae_wlog("%s cache expired", cache_path.c_str());
                tuner_cache.clear();
              }
            }
          }
        }

        if (!tuner_cache.empty())
        {
          yae_wlog("using tuner cache %s", name);
        }
        else
        {
          // probably need to configure this tuner, or something:
          yae_wlog("scanning %s", name);

          try
          {
            LockTuner lock_tuner(hd_ptr);

            hdhomerun_device_set_tuner_target(&hd, "none");

            char * channelmap = NULL;
            if (hdhomerun_device_get_tuner_channelmap(&hd, &channelmap) <= 0)
            {
              YAE_THROW("failed to query channelmap from device");
            }

            const char * scan_group =
              hdhomerun_channelmap_get_channelmap_scan_group(channelmap);

            if (!scan_group)
            {
              YAE_THROW("unknown channelmap '%s'", channelmap);
            }

            if (hdhomerun_device_channelscan_init(&hd, scan_group) <= 0)
            {
              YAE_THROW("failed to initialize channel scan: %s", scan_group);
            }

            while (!signal_handler_received_sigpipe() &&
                   !signal_handler_received_sigint())
            {
              struct hdhomerun_channelscan_result_t result;
              int ret = hdhomerun_device_channelscan_advance(&hd, &result);
              if (ret <= 0)
              {
                break;
              }

              yae_dlog("SCANNING: %u (%s)",
                       (unsigned int)(result.frequency),
                       result.channel_str);

              ret = hdhomerun_device_channelscan_detect(&hd, &result);
              if (ret < 0)
              {
                break;
              }

              if (ret == 0)
              {
                continue;
              }

              yae_dlog("LOCK: %s (ss=%u snq=%u seq=%u)",
                       result.status.lock_str,
                       result.status.signal_strength,
                       result.status.signal_to_noise_quality,
                       result.status.symbol_error_quality);

              if (result.transport_stream_id_detected)
              {
                yae_dlog("TSID: 0x%04X", result.transport_stream_id);
              }

              if (result.original_network_id_detected)
              {
                yae_dlog("ONID: 0x%04X", result.original_network_id);
              }

              if (result.program_count)
              {
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

                  yae_dlog("PROGRAM %s", program.program_str);

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
          }
          catch (const std::exception & e)
          {
            yae_wlog("failed to configure tuner %s: %s", name, e.what());
            continue;
          }
          catch (...)
          {
            yae_wlog("failed to configure tuner %s: unexpected exception",
                     name);
            continue;
          }

          if (!tuner_cache.empty())
          {
            yae::TOpenFile cache_file(cache_path.c_str(), "wb");
            if (cache_file.is_open())
            {
              tuner_cache["timestamp"] = Json::Int64(yae::TTime::now().get(1));
              std::ostringstream oss;
              Json::StyledStreamWriter().write(oss, tuner_cache);
              cache_file.write(oss.str());
            }
          }
        }

        tuners_[std::string(name)] = hd_ptr;
      }
    }
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::capture_all
  //
  void
  HDHomeRun::Private::capture_all()
  {
    int err = 0;

    for (std::map<std::string, hdhomerun_devptr_t>::reverse_iterator
           i = tuners_.rbegin(); i != tuners_.rend(); ++i)
    {
      const std::string & name = i->first;
      hdhomerun_devptr_t hd_ptr = i->second;
      try
      {
        LockTuner lock_tuner(hd_ptr);
        hdhomerun_device_t * hd = hd_ptr.get();
        unsigned int tuner = hdhomerun_device_get_tuner(hd);

        Json::Value frequencies = tuner_cache_[name]["frequencies"];

        for (Json::Value::iterator j = frequencies.begin();
             j != frequencies.end(); ++j)
        {
          std::string frequency = j.key().asString();
          Json::Value programs = (*j)["programs"];
          Json::Value status = (*j)["status"];
          std::string channel = status["channel"].asString();

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

          // FIXME:
          std::string capture_path =
            (fs::path("/tmp") / (frequency + ".ts")).string();

          yae::TOpenFile capture(capture_path, "wb");
          YAE_THROW_IF(!capture.is_open());

          if (hdhomerun_device_stream_start(hd) <= 0)
          {
            YAE_THROW("failed to start stream for %s", channel.c_str());
          }

          yae::TTime sample_duration(30, 1);
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
          yae_wlog("%s %sHz, capturing %ss sample: %s",
                   name.c_str(),
                   frequency.c_str(),
                   sample_duration.to_short_txt().c_str(),
                   channels_txt.c_str());

          yae::TTime t_stop = yae::TTime::now() + sample_duration;
          while (!signal_handler_received_sigpipe() &&
                 !signal_handler_received_sigint())
          {
            size_t buffer_size = 0;
            uint8_t * buffer =
              hdhomerun_device_stream_recv(hd,
                                           VIDEO_DATA_BUFFER_SIZE_1S,
                                           &buffer_size);
            if (!buffer)
            {
              msleep_approx(64);
              continue;
            }

            capture.write(buffer, buffer_size);

            yae::TTime t = yae::TTime::now();
            if (t >= t_stop)
            {
              break;
            }
          }
        }

        break;
      }
      catch (const std::exception & e)
      {
        yae_wlog("failed to configure tuner %s: %s", name.c_str(), e.what());
        continue;
      }
      catch (...)
      {
        yae_wlog("failed to configure tuner %s: unexpected exception",
                 name.c_str());
        continue;
      }
    }
  }


  //----------------------------------------------------------------
  // HDHomeRun::HDHomeRun
  //
  HDHomeRun::HDHomeRun():
    private_(new HDHomeRun::Private())
  {}

  //----------------------------------------------------------------
  // HDHomeRun::~HDHomeRun
  //
  HDHomeRun::~HDHomeRun()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // HDHomeRun::capture_all
  //
  void
  HDHomeRun::capture_all()
  {
    private_->capture_all();
  }

}

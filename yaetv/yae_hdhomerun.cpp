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
  // TunerDevice::~TunerDevice
  //
  TunerDevice::~TunerDevice()
  {}


  //----------------------------------------------------------------
  // TunerStatus::TunerStatus
  //
  TunerStatus::TunerStatus(bool signal_present,
                           uint32_t signal_strength,
                           uint32_t signal_to_noise_quality,
                           uint32_t symbol_error_quality):
    signal_present_(signal_present),
    signal_strength_(signal_strength),
    signal_to_noise_quality_(signal_to_noise_quality),
    symbol_error_quality_(symbol_error_quality)
  {}


  //----------------------------------------------------------------
  // HDHomeRunDevice
  //
  struct HDHomeRunDevice : TunerDevice
  {
    HDHomeRunDevice(const hdhomerun_discover_device_t & device):
      device_(device)
    {}

    // virtual:
    int num_tuners() const
    { return device_.tuner_count; }

    // virtual:
    std::string name() const
    { return yae::strfmt("%08X", device_.device_id); }

    std::string tuner_name(int tuner_index) const
    { return yae::strfmt("%08X-%u", device_.device_id, tuner_index); }

    hdhomerun_discover_device_t device_;
  };


  //----------------------------------------------------------------
  // TunerChannel::TunerChannel
  //
  TunerChannel::TunerChannel(uint16_t ch_number,
                             uint32_t frequency):
    ch_number_(ch_number),
    frequency_(frequency)
  {}


  //----------------------------------------------------------------
  // TunerRef
  //
  struct TunerRef
  {
    TunerRef(uint32_t device_index = 0, uint32_t tuner_index = 0):
      device_(device_index),
      tuner_(tuner_index)
    {}

    uint32_t device_;
    uint32_t tuner_;
  };

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
      stop_(TTime::now()),
      exclusive_(false)
    {
      memset(&status_, 0, sizeof(status_));
    }

    void save_timesheet()
    {
      std::string timesheet = timesheet_.to_str();
      TTime t = TTime::now();
      std::string fn = strfmt("timesheet-hdhr.%s.log", tuner_name_.c_str());
      fn = sanitize_filename_utf8(fn);
      fn = (fs::path(yae::get_temp_dir_utf8()) / fn).string();
      yae::TOpenFile(fn, "ab").write(timesheet);
    }

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

    inline void set_tuner_status(const hdhomerun_tuner_status_t & status)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
#if 0
      yae_ilog("%p %s: SET status: signal strength %u",
               this,
               tuner_name_.c_str(),
               status.signal_strength);
#endif
      status_ = status;
    }

    inline void get_tuner_status(TunerStatus & tuner_status) const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
#if 0
      yae_ilog("%p %s: GET status: signal strength %u",
               this,
               tuner_name_.c_str(),
               status_.signal_strength);
#endif
      tuner_status.signal_present_ = status_.signal_present;
      tuner_status.signal_strength_ = status_.signal_strength;
      tuner_status.signal_to_noise_quality_ = status_.signal_to_noise_quality;
      tuner_status.symbol_error_quality_ = status_.symbol_error_quality;
    }

    TTime stop_;
    LockTuner lock_tuner_;
    hdhomerun_devptr_t hd_ptr_;
    std::string device_name_;
    std::string tuner_name_;
    bool exclusive_;

    // for profiling:
    mutable yae::Timesheet timesheet_;

  protected:
    mutable boost::mutex mutex_;
    hdhomerun_tuner_status_t status_;
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
  // HDHomeRun::Session::device_name
  //
  const std::string &
  HDHomeRun::Session::device_name() const
  {
    return private_->device_name_;
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
  // HDHomeRun::Session::exclusive
  //
  bool
  HDHomeRun::Session::exclusive() const
  {
    return private_->exclusive_;
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
  // HDHomeRun::Session::get_tuner_status
  //
  void
  HDHomeRun::Session::get_tuner_status(TunerStatus & tuner_status) const
  {
    private_->get_tuner_status(tuner_status);
  }


  //----------------------------------------------------------------
  // HDHomeRun::Private
  //
  struct HDHomeRun::Private
  {
    Private(const std::string & cache_dir);

    void discover_devices(std::list<TunerDevicePtr> & devices);

    void get_channel_list(std::list<TunerChannel> & channels,
                          const char * channel_map) const;

    HDHomeRun::TSessionPtr open_session(const std::set<std::string> & tuners,
                                        uint32_t frequency = 0);
    HDHomeRun::TSessionPtr open_session(const std::string & tuner_name,
                                        bool exclusive = false);
    HDHomeRun::TSessionPtr open_session(const std::string & tuner_name,
                                        const hdhomerun_devptr_t & hd_ptr,
                                        uint32_t frequency = 0,
                                        bool exclusive = false);

    void tune_to(const HDHomeRun::TSessionPtr & session_ptr,
                 const uint32_t frequency);

    bool wait_for_lock(const HDHomeRun::TSessionPtr & session_ptr,
                       TunerStatus & status);

    void capture(yae::weak_ptr<IStream> stream_ptr,
                 const HDHomeRun::TSessionPtr & session_ptr,
                 const std::string & frequency);

    mutable boost::mutex mutex_;
    std::vector<struct hdhomerun_discover_device_t> devices_;
    std::map<std::string, TunerRef> tuners_;
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
    hdhomerun_debug_set_prefix(dbg_.get(), "yaetv-hdhr");

    std::string dbg_path = (fs::path(cache_dir) / "yaetv-hdhr.log").string();
    hdhomerun_debug_set_filename(dbg_.get(), dbg_path.c_str());

    hdhomerun_debug_enable(dbg_.get());

    std::string ts = unix_epoch_time_to_localtime_str(TTime::now().get(1));
    hdhomerun_debug_printf(dbg_.get(), "start: %s", ts.c_str());
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::discover_tuners
  //
  void
  HDHomeRun::Private::discover_devices(std::list<TunerDevicePtr> & devices)
  {
    YAE_ASSERT(devices.empty());

    // discover HDHomeRun devices:
    boost::unique_lock<boost::mutex> lock(mutex_);

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
      yae_ilog("%p hdhomerun device %08X found at %u.%u.%u.%u\n",
               this,
               (unsigned int)found.device_id,
               (unsigned int)(found.ip_addr >> 24) & 0x0FF,
               (unsigned int)(found.ip_addr >> 16) & 0x0FF,
               (unsigned int)(found.ip_addr >> 8) & 0x0FF,
               (unsigned int)(found.ip_addr >> 0) & 0x0FF);

      devices.push_back(TunerDevicePtr(new HDHomeRunDevice(found)));

      for (int tuner = 0; tuner < found.tuner_count; tuner++)
      {
        std::string name = yae::strfmt("%08X-%u", found.device_id, tuner);
        tuners_[name] = TunerRef(i, tuner);
      }
    }
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::get_channel_list
  //
  void
  HDHomeRun::Private::get_channel_list(std::list<TunerChannel> & channels,
                                       const char * channel_map) const
  {
    struct hdhomerun_channel_list_t * channel_list =
      hdhomerun_channel_list_create(channel_map);

    struct hdhomerun_channel_entry_t * iter =
      hdhomerun_channel_list_first(channel_list);

    while (iter)
    {
      channels.push_back(TunerChannel());
      TunerChannel & channel = channels.back();
      channel.ch_number_ = hdhomerun_channel_entry_channel_number(iter);
      channel.frequency_ = hdhomerun_channel_entry_frequency(iter);
      channel.name_ = hdhomerun_channel_entry_name(iter);
      iter = hdhomerun_channel_list_next(channel_list, iter);
    }

    hdhomerun_channel_list_destroy(channel_list);
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session(const std::set<std::string> & tuners,
                                   uint32_t frequency)
  {
    std::map<std::string, TunerRef> all_tuners;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      all_tuners = tuners_;
    }

    HDHomeRun::TSessionPtr session;
    for (std::map<std::string, TunerRef>::reverse_iterator
           i = all_tuners.rbegin(); i != all_tuners.rend() && !session; ++i)
    {
      const std::string & tuner_name = i->first;
      if (!yae::has(tuners, tuner_name))
      {
        continue;
      }

      const TunerRef & tuner = i->second;
      hdhomerun_devptr_t hd_ptr;
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        const hdhomerun_discover_device_t & device = devices_[tuner.device_];
        hd_ptr.reset(hdhomerun_device_create(device.device_id,
                                             device.ip_addr,
                                             tuner.tuner_,
                                             dbg_.get()));
      }

      if (hd_ptr)
      {
        session = open_session(tuner_name, hd_ptr, frequency);
      }
    }

    return session;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session(const std::string & tuner_name,
                                   bool exclusive)
  {
    std::map<std::string, TunerRef> all_tuners;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      all_tuners = tuners_;
    }

    HDHomeRun::TSessionPtr session;
    std::map<std::string, TunerRef>::const_iterator
      found = all_tuners.find(tuner_name);
    if (found != all_tuners.end())
    {
      const TunerRef & tuner = found->second;
      hdhomerun_devptr_t hd_ptr;
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        const hdhomerun_discover_device_t & device = devices_[tuner.device_];
        hd_ptr.reset(hdhomerun_device_create(device.device_id,
                                             device.ip_addr,
                                             tuner.tuner_,
                                             dbg_.get()));
      }

      if (hd_ptr)
      {
        session = open_session(tuner_name, hd_ptr, 0, exclusive);
      }
    }

    return session;
  }

  //----------------------------------------------------------------
  // HDHomeRun::Private::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::Private::open_session(const std::string & tuner_name,
                                   const hdhomerun_devptr_t & hd_ptr,
                                   uint32_t frequency,
                                   bool exclusive)
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
        yae_wlog("%p hdhomerun_device_get_tuner_lockkey_owner failed for %s",
                 this,
                 tuner_name.c_str());
        return HDHomeRun::TSessionPtr();
      }

      if (strcmp(owner, "none") != 0)
      {
        // tuner belongs to another process, ignore:
        yae_wlog("%p skipping tuner %s, current owner: %s",
                 this,
                 tuner_name.c_str(),
                 owner);
        return HDHomeRun::TSessionPtr();;
      }

      HDHomeRun::Session::Private & session = *(session_ptr->private_);
      session.lock_tuner_.lock(hd_ptr, cache_dir_);
      session.device_name_ =
        yae::strfmt("%08X", hdhomerun_device_get_device_id(hd));
      session.tuner_name_ = tuner_name;
      session.hd_ptr_ = hd_ptr;
      session.exclusive_ = exclusive;

      if (frequency)
      {
        tune_to(session_ptr, frequency);
      }

      return session_ptr;
    }
    catch (const std::exception & e)
    {
      yae_wlog("%p failed to lock tuner %s: %s",
               this,
               tuner_name.c_str(),
               e.what());
    }
    catch (...)
    {
      yae_wlog("%p failed to configure tuner %s: unexpected exception",
               this,
               tuner_name.c_str());
    }

    return HDHomeRun::TSessionPtr();
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
      YAE_THROW("%p failed to set channel, error: %s", this, error);
    }
  }

  //----------------------------------------------------------------
  // HDHomeRun::wait_for_lock
  //
  bool
  HDHomeRun::Private::wait_for_lock(const HDHomeRun::TSessionPtr & session_ptr,
                                    TunerStatus & tuner_status)
  {
    // shortcuts:
    HDHomeRun::Session::Private & session = *(session_ptr->private_);
    hdhomerun_device_t * hd = session.hd_ptr_.get();

    struct hdhomerun_tuner_status_t status;
    int ret = hdhomerun_device_wait_for_lock(hd, &status);
    if (ret < 0)
    {
      yae_wlog("%p hdhomerun_device_wait_for_lock: %i", this, ret);
    }

    tuner_status.signal_present_ = status.signal_present;
    tuner_status.signal_strength_ = status.signal_strength;
    tuner_status.signal_to_noise_quality_ = status.signal_to_noise_quality;
    tuner_status.symbol_error_quality_ = status.symbol_error_quality;
    return tuner_status.signal_present_;
  }

  //----------------------------------------------------------------
  // StopStream
  //
  struct StopStream
  {
    StopStream(const HDHomeRun::TSessionPtr & session_ptr):
      session_ptr_(session_ptr)
    {}

    ~StopStream()
    {
      if (session_ptr_)
      {
        HDHomeRun::Session::Private & session = *(session_ptr_->private_);

        hdhomerun_device_t * hd = session.hd_ptr_.get();
        if (hd)
        {
          hdhomerun_device_stream_stop(hd);
        }

        session.save_timesheet();
      }
    }

    HDHomeRun::TSessionPtr session_ptr_;
  };

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
    const std::string & device_name = session.device_name_;
    const std::string & tuner_name = session.tuner_name_;
    hdhomerun_device_t * hd = session.hd_ptr_.get();
    yae::shared_ptr<StopStream> stop_stream;

    try
    {
      std::string channel = "auto:" + frequency;
      unsigned int tuner = hdhomerun_device_get_tuner(hd);
      std::string param = yae::strfmt("/tuner%i/channel", tuner);
      char * error = NULL;
#if 0
      static bool failed_once = false;
      if (frequency == "557000000" && !failed_once)
      {
        failed_once = true;
        YAE_THROW("FIXME: pkoshevoy: simulating a tuner lock failure, once");
      }
#endif
      if (hdhomerun_device_set_var(hd,
                                   param.c_str(),
                                   channel.c_str(),
                                   NULL,
                                   &error) <= 0)
      {
        YAE_THROW("%p failed to set channel, error: %s", this, error);
      }

      if (hdhomerun_device_stream_start(hd) <= 0)
      {
        YAE_THROW("%p failed to start stream for %s", this, channel.c_str());
      }
      else
      {
        stop_stream.reset(new StopStream(session_ptr));
      }

      char * status_str = NULL;
      hdhomerun_tuner_status_t status;
      if (hdhomerun_device_get_tuner_status(hd,
                                            &status_str,
                                            &status) == 1)
      {
        yae_ilog("%p %s, status: %s", this, tuner_name.c_str(), status_str);
        session.set_tuner_status(status);
      }

      yae_ilog("%p %s %sHz: capturing",
               this,
               session.tuner_name_.c_str(),
               frequency.c_str());

      TTime time_of_last_status = TTime::now();
      TTime time_of_last_packet = TTime::now();

      while (true)
      {
        yae::shared_ptr<IStream> stream_ptr = stream_weak_ptr.lock();
        if (!stream_ptr)
        {
          yae_ilog("%p break capture: !stream_ptr", this);
          break;
        }

        IStream & stream = *stream_ptr;
        if (!stream.is_open())
        {
          yae_ilog("%p break capture: !stream.is_open()", this);
          break;
        }

#ifdef _WIN32
        // indicate activity to suppress auto sleep mode:
        SetThreadExecutionState(ES_SYSTEM_REQUIRED);
#endif

        static const std::size_t buffer_size_1s = VIDEO_DATA_BUFFER_SIZE_1S;
        std::size_t buffer_size = 0;
        uint8_t * buffer = NULL;
        {
          yae::Timesheet::Probe probe(session.timesheet_,
                                      "HDHomeRun::Private::capture",
                                      "hdhomerun_device_stream_recv");

          buffer = hdhomerun_device_stream_recv(hd,
                                                buffer_size_1s,
                                                &buffer_size);
        }

        TTime now = TTime::now();
        double time_since_last_status = (now - time_of_last_status).sec();
        if (time_since_last_status > 1.0)
        {
          int ret = hdhomerun_device_get_tuner_status(hd,
                                                      &status_str,
                                                      &status);
          if (ret <= 0)
          {
            yae_elog("%p %s, hdhomerun_device_get_tuner_status: %i",
                     this,
                     tuner_name.c_str(),
                     ret);
            break;
          }

          // update tuner status for current session:
          session.set_tuner_status(status);
          time_of_last_status = now;
        }

        if (buffer)
        {
          yae::Timesheet::Probe probe(session.timesheet_,
                                      "HDHomeRun::Private::capture",
                                      "stream.push");

          if (!stream.push(buffer, buffer_size))
          {
            break;
          }

          time_of_last_packet = now;
        }

        if (session.expired())
        {
          break;
        }

        if (!buffer)
        {
          double time_since_last_packet = (now - time_of_last_packet).sec();
          if (time_since_last_packet > 1.0)
          {
            yae_elog("%p %s %sHz: no data received within %f sec, "
                     "giving up now",
                     this,
                     tuner_name.c_str(),
                     frequency.c_str(),
                     time_since_last_packet);
            break;
          }

          msleep_approx(64);
        }
      }
    }
    catch (const std::exception & e)
    {
      yae_wlog("%p %s %sHz: stream failed: %s",
               this,
               tuner_name.c_str(),
               frequency.c_str(),
               e.what());
    }
    catch (...)
    {
      yae_wlog("%p %s %sHz: stream failed: unexpected exception",
               this,
               tuner_name.c_str(),
               frequency.c_str());
    }

    yae::shared_ptr<IStream> stream_ptr = stream_weak_ptr.lock();
    if (stream_ptr)
    {
      stream_ptr->close();
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
  // HDHomeRun::discover_devices
  //
  void
  HDHomeRun::discover_devices(std::list<TunerDevicePtr> & devices)
  {
    private_->discover_devices(devices);
  }

  //----------------------------------------------------------------
  // HDHomeRun::get_channel_list
  //
  void
  HDHomeRun::get_channel_list(std::list<TunerChannel> & channels,
                              const char * channel_map) const
  {
    private_->get_channel_list(channels, channel_map);
  }

  //----------------------------------------------------------------
  // HDHomeRun::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::open_session(const std::set<std::string> & enabled_tuners,
                          uint32_t frequency)
  {
    return private_->open_session(enabled_tuners, frequency);
  }

  //----------------------------------------------------------------
  // HDHomeRun::open_session
  //
  HDHomeRun::TSessionPtr
  HDHomeRun::open_session(const std::string & tuner, bool exclusive)
  {
    return private_->open_session(tuner, exclusive);
  }

  //----------------------------------------------------------------
  // HDHomeRun::tune_to
  //
  bool
  HDHomeRun::tune_to(const HDHomeRun::TSessionPtr & session_ptr,
                     const uint32_t frequency,
                     TunerStatus & status)
  {
    try
    {
      private_->tune_to(session_ptr, frequency);
      return private_->wait_for_lock(session_ptr, status);
    }
    catch (const std::exception & e)
    {}
    catch (...)
    {}

    return false;
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

}

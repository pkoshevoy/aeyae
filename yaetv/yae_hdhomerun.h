// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_HDHOMERUN_H_
#define YAE_HDHOMERUN_H_

// standard:
#include <stdexcept>

// aeyae:
#include "yae/utils/yae_time.h"
#include "yae/video/yae_mpeg_ts.h"


namespace yae
{

  //----------------------------------------------------------------
  // IAssert
  //
  struct IAssert
  {
    virtual ~IAssert() {}
    virtual bool eval() const = 0;

    inline bool operator()() const
    {
      if (!this->eval())
      {
        throw std::runtime_error("false predicate");
      }

      return true;
    }
  };


  //----------------------------------------------------------------
  // DontStop
  //
  struct DontStop : IAssert
  {
    DontStop():
      stop_(false)
    {}

    virtual bool eval() const
    { return !stop_; }

    bool stop_;
  };


  //----------------------------------------------------------------
  // IStream
  //
  struct IStream
  {
    virtual ~IStream();
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    // return value will be interpreted as follows:
    //
    //  true  -- keep going
    //  false -- stop
    //
    virtual bool push(const void * data, std::size_t size) = 0;
  };


  //----------------------------------------------------------------
  // TunerDevice
  //
  struct TunerDevice
  {
    virtual ~TunerDevice();
    virtual int num_tuners() const = 0;
    virtual std::string name() const = 0;
    virtual std::string tuner_name(int tuner_index) const = 0;
  };

  //----------------------------------------------------------------
  // TunerDevicePtr
  //
  typedef yae::shared_ptr<TunerDevice> TunerDevicePtr;


  //----------------------------------------------------------------
  // TunerStatus
  //
  struct TunerStatus
  {
    TunerStatus(bool signal_present = false,
                uint32_t signal_strength = 0,
                uint32_t signal_to_noise_quality = 0,
                uint32_t symbol_error_quality = 0);

    bool signal_present_;
    uint32_t signal_strength_;
    uint32_t signal_to_noise_quality_;
    uint32_t symbol_error_quality_;
  };


  //----------------------------------------------------------------
  // TunerChannel
  //
  struct TunerChannel
  {
    TunerChannel(uint16_t ch_number = 0,
                 uint32_t frequency = 0);

    inline std::string frequency_str() const
    { return boost::lexical_cast<std::string>(frequency_); }

    uint16_t ch_number_;
    uint32_t frequency_;
    std::string name_;
  };


  //----------------------------------------------------------------
  // HDHomeRun
  //
  struct HDHomeRun
  {
    HDHomeRun(const std::string & cache_dir);
    ~HDHomeRun();

    void discover_devices(std::list<TunerDevicePtr> & devices);

    void get_channel_list(std::list<TunerChannel> & channels,
                          const char * channel_map) const;

    //----------------------------------------------------------------
    // Session
    //
    struct Session
    {
      struct Private;
      Private * private_;

      Session();
      ~Session();

      const std::string & device_name() const;
      const std::string & tuner_name() const;

      bool exclusive() const;
      bool expired() const;
      void extend(const TTime & t);
      void finish();
      void get_tuner_status(TunerStatus & tuner_status) const;

    protected:
      Session(const Session &);
      Session & operator = (const Session &);
    };

    //----------------------------------------------------------------
    // TSessionPtr
    //
    typedef yae::shared_ptr<Session> TSessionPtr;

    // grab any tuner (if available), capable of tuning to a given frequency:
    TSessionPtr open_session(const std::set<std::string> & enabled_tuners,
                             uint32_t frequency = 0);

    // grab a specific tuner, if available:
    TSessionPtr open_session(const std::string & tuner,

                             // for channel scanning session
                             // should not be shared:
                             bool exclusive_session = false);

    bool tune_to(const HDHomeRun::TSessionPtr & session_ptr,
                 const uint32_t frequency,
                 TunerStatus & status);

    void capture(TSessionPtr session_ptr,
                 yae::weak_ptr<IStream> stream_ptr,
                 const std::string & frequency);

  protected:
    // intentionally disabled:
    HDHomeRun(const HDHomeRun &);
    HDHomeRun & operator = (const HDHomeRun &);

    struct Private;
    Private * private_;
  };

}


#endif // YAE_HDHOMERUN_H_

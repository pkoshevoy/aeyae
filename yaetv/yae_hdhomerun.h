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
    virtual ~IStream() {}

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
  // TChannelNames
  //
  // channel names indexed by channel_minor
  //
  typedef std::map<uint16_t, std::string> TChannelNames;

  //----------------------------------------------------------------
  // TChannels
  //
  // indexed by channel_major
  //
  typedef std::map<uint16_t, TChannelNames> TChannels;


  //----------------------------------------------------------------
  // HDHomeRun
  //
  struct HDHomeRun
  {
    HDHomeRun(const std::string & cache_dir);
    ~HDHomeRun();

    void discover_tuners(std::list<std::string> & tuners);
    bool init(const std::string & tuner_name);

    //----------------------------------------------------------------
    // Session
    //
    struct Session
    {
      struct Private;
      Private * private_;

      Session();
      ~Session();

      const std::string & tuner_name() const;

      bool expired() const;
      void extend(const TTime & t);
      void finish();

    protected:
      Session(const Session &);
      Session & operator = (const Session &);
    };

    //----------------------------------------------------------------
    // TSessionPtr
    //
    typedef yae::shared_ptr<Session> TSessionPtr;

    // grab any tuner, if available:
    TSessionPtr open_session();

    // grab a specific tuner, if available:
    TSessionPtr open_session(const std::string & tuner);

    bool scan_channels(TSessionPtr session_ptr,
                       const IAssert & keep_going);

    void capture(TSessionPtr session_ptr,
                 yae::weak_ptr<IStream> stream_ptr,
                 const std::string & frequency);

    // fill in the major.minor -> frequency lookup table:
    bool get_channels(std::map<uint32_t, std::string> & chan_freq) const;
    bool get_channels(const std::string & freq, TChannels & channels) const;
    bool get_channels(std::map<std::string, TChannels> & channels) const;

    // helper:
    uint16_t get_channel_major(const std::string & frequency) const;

  protected:
    // intentionally disabled:
    HDHomeRun(const HDHomeRun &);
    HDHomeRun & operator = (const HDHomeRun &);

    struct Private;
    Private * private_;
  };

}


#endif // YAE_HDHOMERUN_H_

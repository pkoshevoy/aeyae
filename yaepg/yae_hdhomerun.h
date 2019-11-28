// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_HDHOMERUN_H_
#define YAE_HDHOMERUN_H_

// yae includes:
#include "yae/utils/yae_time.h"
#include "yae/video/yae_mpeg_ts.h"


namespace yae
{

  //----------------------------------------------------------------
  // ICapture
  //
  struct ICapture
  {
    virtual ~ICapture() {}

    //----------------------------------------------------------------
    // TResponse
    //
    typedef enum
    {
      STOP_E = 0,
      MORE_E = 1,
    } TResponse;

    virtual TResponse
    push(const std::string & tuner_name,
         const std::string & frequency,
         const void * data,
         std::size_t size) = 0;
  };

  //----------------------------------------------------------------
  // TCapturePtr
  //
  typedef yae::shared_ptr<ICapture> TCapturePtr;


  //----------------------------------------------------------------
  // HDHomeRun
  //
  struct HDHomeRun
  {
    HDHomeRun();
    ~HDHomeRun();

    // fill in the major.minor -> frequency lookup table:
    bool get_channels(std::map<uint32_t, std::string> & chan_freq) const;

    bool capture_all(const yae::TTime & duration,
                     const TCapturePtr & callback);

    bool capture(const std::string & frequency,
                 const TCapturePtr & callback,
                 const yae::TTime & duration = yae::TTime(0, 0));

  protected:
    // intentionally disabled:
    HDHomeRun(const HDHomeRun &);
    HDHomeRun & operator = (const HDHomeRun &);

    struct Private;
    Private * private_;
  };

}


#endif // YAE_HDHOMERUN_H_

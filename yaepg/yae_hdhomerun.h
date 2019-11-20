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


namespace yae
{

  //----------------------------------------------------------------
  // HDHomeRun
  //
  struct HDHomeRun
  {
    HDHomeRun();
    ~HDHomeRun();

    typedef void(*TCallback)(void * context,
                             const std::string & name,
                             const std::string & frequency,
                             const void * data,
                             std::size_t size);

    void capture_all(const yae::TTime & duration,
                     TCallback callback,
                     void * context);

  protected:
    // intentionally disabled:
    HDHomeRun(const HDHomeRun &);
    HDHomeRun & operator = (const HDHomeRun &);

    struct Private;
    Private * private_;
  };

}


#endif // YAE_HDHOMERUN_H_

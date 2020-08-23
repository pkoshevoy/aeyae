// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 22 17:35:04 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ISTREAM_H_
#define YAE_ISTREAM_H_

// aeyae:
#include "yae/api/yae_api.h"


namespace yae
{

  //----------------------------------------------------------------
  // IStream
  //
  struct YAE_API IStream
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

}


#endif // YAE_ISTREAM_H_

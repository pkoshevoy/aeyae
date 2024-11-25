// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Aug 23 12:43:11 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_READER_FACTORY_H_
#define YAE_READER_FACTORY_H_

// aeyae:
#include "yae/video/yae_reader.h"

// standard:
#include <string>


namespace yae
{

  //----------------------------------------------------------------
  // ReaderFactory
  //
  struct YAE_API ReaderFactory
  {
    virtual ~ReaderFactory() {}
    virtual IReaderPtr create(const std::string & resource_path_utf8) const;
  };

  //----------------------------------------------------------------
  // TReaderFactoryPtr
  //
  typedef yae::shared_ptr<ReaderFactory> TReaderFactoryPtr;

}


#endif // YAE_READER_FACTORY_H_

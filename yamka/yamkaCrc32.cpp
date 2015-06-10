// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:13:21 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaCrc32.h>

// boost includes:
#include <boost/crc.hpp>


namespace Yamka
{

  //----------------------------------------------------------------
  // Crc32::Private
  //
  class Crc32::Private
  {
  public:
    boost::crc_32_type checksum_;
  };

  //----------------------------------------------------------------
  // Crc32::Crc32
  //
  Crc32::Crc32():
    private_(new Crc32::Private)
  {}

  //----------------------------------------------------------------
  // Crc32::~Crc32
  //
  Crc32::~Crc32()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // Crc32::compute
  //
  void
  Crc32::compute(const void * bytes, std::size_t numBytes)
  {
    private_->checksum_.process_bytes(bytes, numBytes);
  }

  //----------------------------------------------------------------
  // Crc32::checksum
  //
  unsigned int
  Crc32::checksum() const
  {
    unsigned int crc32 = private_->checksum_.checksum();
    return crc32;
  }
}

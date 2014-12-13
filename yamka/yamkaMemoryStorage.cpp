// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat, Dec 13, 2014 10:07:01 AM
// Copyright : Bill Brimley, Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <sstream>

// yamka includes:
#include <yamkaMemoryStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // ConstFileInMemory::ConstFileInMemory
  //
  ConstFileInMemory::ConstFileInMemory(const unsigned char * data,
                                       std::size_t size):
    data_(data),
    size_(size),
    posn_(0)
  {}

  //----------------------------------------------------------------
  // ConstFileInMemory::load
  //
  bool
  ConstFileInMemory::load(void * dst, std::size_t numBytes)
  {
    if (peek(dst, numBytes) == numBytes)
    {
      posn_ += numBytes;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ConstFileInMemory::peek
  //
  std::size_t
  ConstFileInMemory::peek(void * dst, std::size_t numBytes) const
  {
    std::size_t nbytes =
      (posn_ + numBytes <= size_) ?
      numBytes :
      (std::size_t)(size_ - posn_);

    memcpy(dst, data_ + posn_, nbytes);
    return nbytes;
  }

  //----------------------------------------------------------------
  // ConstFileInMemory::calcCrc32
  //
  bool
  ConstFileInMemory::calcCrc32(uint64 seekToPosition,
                               uint64 numBytesToRead,
                               Crc32 & computeCrc32) const
  {
    if (!numBytesToRead)
    {
      return true;
    }

    if (seekToPosition + numBytesToRead < size_)
    {
      const unsigned char * dataPtr = data_ + seekToPosition;
      computeCrc32.compute(dataPtr, (std::size_t)numBytesToRead);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ConstFileInMemory::seek
  //
  bool
  ConstFileInMemory::seek(TFileOffset offset, TPositionReference relativeTo)
  {
    if (relativeTo == kAbsolutePosition &&
        offset >= 0 &&
        offset <= size_)
    {
      posn_ = offset;
      return true;
    }

    if (relativeTo == kRelativeToCurrent)
    {
      return ConstFileInMemory::seek(posn_ + offset, kAbsolutePosition);
    }

    if (relativeTo == kOffsetFromEnd)
    {
      return ConstFileInMemory::seek(size_ - offset, kAbsolutePosition);
    }

    return false;
  }

  //----------------------------------------------------------------
  // FileInMemory::FileInMemory
  //
  FileInMemory::FileInMemory(unsigned char * data, std::size_t size):
    ConstFileInMemory(data, size)
  {}

  //----------------------------------------------------------------
  // FileInMemory::save
  //
  bool
  FileInMemory::save(const void * src, std::size_t numBytes)
  {
    if (posn_ + numBytes <= size_)
    {
      unsigned char * data = const_cast<unsigned char *>(data_);
      memcpy(data + posn_, src, numBytes);
      posn_ += numBytes;
      return true;
    }

    return false;
  }

}

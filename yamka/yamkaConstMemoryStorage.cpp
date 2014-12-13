// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat, Dec 13, 2014 10:07:01 AM
// Copyright : Bill Brimley, Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <sstream>

// yamka includes:
#include <yamkaConstMemoryStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // FileInMemory::FileInMemory
  //
  FileInMemory::FileInMemory(const unsigned char * data, std::size_t size):
    data_(data),
    size_(size),
    posn_(0)
  {}

  //----------------------------------------------------------------
  // FileInMemory::load
  //
  bool
  FileInMemory::load(void * dst, std::size_t numBytes)
  {
    if (peek(dst, numBytes) == numBytes)
    {
      posn_ += numBytes;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // FileInMemory::peek
  //
  std::size_t
  FileInMemory::peek(void * dst, std::size_t numBytes) const
  {
    std::size_t nbytes =
      (posn_ + numBytes <= size_) ?
      numBytes :
      (std::size_t)(size_ - posn_);

    memcpy(dst, data_ + posn_, nbytes);
    return nbytes;
  }

  //----------------------------------------------------------------
  // FileInMemory::calcCrc32
  //
  bool
  FileInMemory::calcCrc32(uint64 seekToPosition,
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
  // FileInMemory::seek
  //
  bool
  FileInMemory::seek(TFileOffset offset, TPositionReference relativeTo)
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
      return FileInMemory::seek(posn_ + offset, kAbsolutePosition);
    }

    if (relativeTo == kOffsetFromEnd)
    {
      return FileInMemory::seek(size_ - offset, kAbsolutePosition);
    }

    return false;
  }


  //----------------------------------------------------------------
  // ConstMemoryStorage::ConstMemoryStorage
  //
  ConstMemoryStorage::ConstMemoryStorage(const unsigned char * data,
                                         std::size_t size):
    file_(data, size)
  {}

  //----------------------------------------------------------------
  // ConstMemoryStorage::receipt
  //
  IStorage::IReceiptPtr
  ConstMemoryStorage::receipt() const
  {
    return IStorage::IReceiptPtr(new Receipt(file_));
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::load
  //
  IStorage::IReceiptPtr
  ConstMemoryStorage::load(unsigned char * data, std::size_t size)
  {
    IStorage::IReceiptPtr receipt(new Receipt(file_));
    if (!file_.load(data, size))
    {
      return IStorage::IReceiptPtr();
    }

    receipt->add(size);
    return receipt;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::peek
  //
  std::size_t
  ConstMemoryStorage::peek(unsigned char * data, std::size_t size)
  {
    return file_.peek(data, size);
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::skip
  //
  uint64
  ConstMemoryStorage::skip(uint64 numBytes)
  {
    if (!file_.seek(numBytes, kRelativeToCurrent))
    {
      return 0;
    }

    return numBytes;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::seekTo
  //
  void
  ConstMemoryStorage::seekTo(uint64 absolutePosition)
  {
    if (!file_.seek(TFileOffset(absolutePosition), kAbsolutePosition))
    {
      std::ostringstream oss;
      oss << "ConstMemoryStorage::seekTo(" << absolutePosition << ") failed";

      throw std::runtime_error(oss.str());
    }

    assert(absolutePosition == receipt()->position());
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::Receipt
  //
  ConstMemoryStorage::Receipt::Receipt(const FileInMemory & file):
    file_(file),
    addr_(file.posn_),
    numBytes_(0)
  {}

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::Receipt
  //
  ConstMemoryStorage::Receipt::Receipt(const FileInMemory & file,
                                       uint64 addr,
                                       uint64 numBytes):
    file_(file),
    addr_(addr),
    numBytes_(0)
  {
    assert(addr_ + numBytes_ <= file_.size_);
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::position
  //
  uint64
  ConstMemoryStorage::Receipt::position() const
  {
    return addr_;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::numBytes
  //
  uint64
  ConstMemoryStorage::Receipt::numBytes() const
  {
    return numBytes_;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::setNumBytes
  //
  ConstMemoryStorage::Receipt &
  ConstMemoryStorage::Receipt::setNumBytes(uint64 numBytes)
  {
    assert(addr_ + numBytes <= file_.size_);
    numBytes_ = numBytes;
    return *this;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::add
  //
  ConstMemoryStorage::Receipt &
  ConstMemoryStorage::Receipt::add(uint64 numBytes)
  {
    assert(addr_ + numBytes_ + numBytes <= file_.size_);
    numBytes_ += numBytes;
    return *this;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::load
  //
  bool
  ConstMemoryStorage::Receipt::load(unsigned char * data)
  {
    memcpy(data, file_.data_ + file_.posn_, (std::size_t)numBytes_);
    return true;
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::calcCrc32
  //
  bool
  ConstMemoryStorage::Receipt::calcCrc32(Crc32 & computeCrc32,
                                         const IReceiptPtr & skip)
  {
    return Yamka::calcCrc32<FileInMemory>(file_, this, skip, computeCrc32);
  }

  //----------------------------------------------------------------
  // ConstMemoryStorage::Receipt::receipt
  //
  IStorage::IReceiptPtr
  ConstMemoryStorage::Receipt::receipt(uint64 offset, uint64 size) const
  {
    if (offset + size <= numBytes_)
    {
      return IStorage::IReceiptPtr(new Receipt(file_, addr_ + offset, size));
    }

    assert(false);
    return IStorage::IReceiptPtr();
  }

}

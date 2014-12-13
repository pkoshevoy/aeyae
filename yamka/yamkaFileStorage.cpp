// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 20:02:27 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <sstream>

// yamka includes:
#include <yamkaFileStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // FileStorage::FileStorage
  //
  FileStorage::FileStorage(const std::string & pathUTF8,
                           File::AccessMode fileMode):
    file_(pathUTF8, fileMode)
  {}

  //----------------------------------------------------------------
  // FileStorage::receipt
  //
  IStorage::IReceiptPtr
  FileStorage::receipt() const
  {
    return IStorage::IReceiptPtr(new Receipt(file_));
  }

  //----------------------------------------------------------------
  // FileStorage::save
  //
  // NOTE: IStorage::save always appends at the end of the file.
  //
  IStorage::IReceiptPtr
  FileStorage::save(const unsigned char * data, std::size_t size)
  {
    IStorage::IReceiptPtr receipt(new Receipt(file_));
    if (!file_.save(data, size))
    {
      return IStorage::IReceiptPtr();
    }

    receipt->add(size);
    return receipt;
  }

  //----------------------------------------------------------------
  // FileStorage::load
  //
  IStorage::IReceiptPtr
  FileStorage::load(unsigned char * data, std::size_t size)
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
  // FileStorage::peek
  //
  std::size_t
  FileStorage::peek(unsigned char * data, std::size_t size)
  {
    return file_.peek(data, size);
  }

  //----------------------------------------------------------------
  // FileStorage::skip
  //
  uint64
  FileStorage::skip(uint64 numBytes)
  {
    if (!file_.seek(numBytes, kRelativeToCurrent))
    {
      assert(false);
      return 0;
    }

    return numBytes;
  }

  //----------------------------------------------------------------
  // FileStorage::seekTo
  //
  void
  FileStorage::seekTo(uint64 absolutePosition)
  {
    if (!file_.seek(absolutePosition, kAbsolutePosition))
    {
      std::ostringstream oss;
      oss << "FileStorage::seekTo(" << absolutePosition << ") failed";

      throw std::runtime_error(oss.str());
    }

    assert(absolutePosition == receipt()->position());
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::Receipt
  //
  FileStorage::Receipt::Receipt(const File & file):
    file_(file),
    addr_(file.absolutePosition()),
    numBytes_(0)
  {}

  //----------------------------------------------------------------
  // FileStorage::Receipt::position
  //
  uint64
  FileStorage::Receipt::position() const
  {
    return addr_;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::numBytes
  //
  uint64
  FileStorage::Receipt::numBytes() const
  {
    return numBytes_;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::setNumBytes
  //
  FileStorage::Receipt &
  FileStorage::Receipt::setNumBytes(uint64 numBytes)
  {
    numBytes_ = numBytes;
    return *this;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::add
  //
  FileStorage::Receipt &
  FileStorage::Receipt::add(uint64 numBytes)
  {
    numBytes_ += numBytes;
    return *this;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::save
  //
  bool
  FileStorage::Receipt::save(const unsigned char * data, std::size_t size)
  {
    try
    {
      File::Seek temp(file_, addr_);
      if (file_.save(data, size))
      {
        numBytes_ = std::max<uint64>(numBytes_, size);
        return true;
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::load
  //
  bool
  FileStorage::Receipt::load(unsigned char * data)
  {
    try
    {
      File::Seek temp(file_, addr_);
      if (file_.load(data, (std::size_t)numBytes_))
      {
        return true;
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::calcCrc32
  //
  bool
  FileStorage::Receipt::calcCrc32(Crc32 & computeCrc32,
                                  const IReceiptPtr & skip)
  {
    return Yamka::calcCrc32<File>(file_, this, skip, computeCrc32);
  }

  //----------------------------------------------------------------
  // FileStorage::Receipt::receipt
  //
  IStorage::IReceiptPtr
  FileStorage::Receipt::receipt(uint64 offset, uint64 size) const
  {
    if (offset + size > numBytes_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }

    FileStorage::Receipt * r = new FileStorage::Receipt(file_);
    r->addr_ = addr_ + offset;
    r->numBytes_ = size;
    return IStorage::IReceiptPtr(r);
  }

}

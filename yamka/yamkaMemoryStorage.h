// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat, Dec 13, 2014 10:07:01 AM
// Copyright : Bill Brimley, Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_MEMORY_STORAGE_H_
#define YAMKA_MEMORY_STORAGE_H_

// system includes:
#include <assert.h>
#include <sstream>

// yamka includes:
#include <yamkaIStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // ConstFileInMemory
  //
  struct ConstFileInMemory
  {
    //----------------------------------------------------------------
    // TDataPtr
    //
    typedef const unsigned char * TDataPtr;

    //----------------------------------------------------------------
    // TSeek
    //
    typedef Seek<ConstFileInMemory> TSeek;

    ConstFileInMemory(const unsigned char * data, std::size_t size);

    // read at current file position a specified number of bytes
    // into the destination buffer:
    bool load(void * dst, std::size_t numBytes);

    // not supported here:
    inline bool save(const void * src, std::size_t numBytes)
    { return false; }

    // unlike load/save/read/write peek does not change the current
    // file position; otherwise peek behaves like load and
    // returns the number of bytes successfully loaded:
    std::size_t peek(void * dst, std::size_t numBytes) const;

    // calculate CRC-32 checksum over a region of this file:
    bool calcCrc32(uint64 seekToPosition,
                   uint64 numBytesToRead,
                   Crc32 & computeCrc32) const;

    // seek to a specified file position:
    bool seek(TFileOffset offset, TPositionReference relativeTo);

    // helper:
    inline bool seekTo(TFileOffset absolutePosition)
    { return seek(absolutePosition, kAbsolutePosition); }

    // return current absolute file position:
    inline TFileOffset absolutePosition() const
    { return posn_; }

    // accessor:
    inline TDataPtr data() const
    { return data_; }

    // accessor:
    inline std::size_t size() const
    { return size_; }

  protected:
    const unsigned char * data_;
    const std::size_t size_;
    TFileOffset posn_;
  };

  //----------------------------------------------------------------
  // FileInMemory
  //
  struct FileInMemory : public ConstFileInMemory
  {
    //----------------------------------------------------------------
    // TDataPtr
    //
    typedef unsigned char * TDataPtr;

    FileInMemory(unsigned char * data, std::size_t size);

    // write out at current file position a specified number of bytes
    // from the source buffer:
    bool save(const void * src, std::size_t numBytes);

    // accessor:
    inline TDataPtr data() const
    { return const_cast<TDataPtr>(data_); }
  };

  //----------------------------------------------------------------
  // ConstMemoryStorage
  //
  template <typename TFile>
  struct Storage : public IStorage
  {
    Storage(typename TFile::TDataPtr data, std::size_t size):
      file_(data, size)
    {}

    // virtual:
    IReceiptPtr receipt() const
    { return IStorage::IReceiptPtr(new Receipt(const_cast<TFile &>(file_))); }

    // virtual:
    IReceiptPtr load(unsigned char * data, std::size_t size)
    {
      IStorage::IReceiptPtr receipt(new Receipt(file_));
      if (!file_.load(data, size))
      {
        return IStorage::IReceiptPtr();
      }

      receipt->add(size);
      return receipt;
    }

    // virtual:
    IReceiptPtr save(const unsigned char * data, std::size_t size)
    {
      IStorage::IReceiptPtr receipt(new Receipt(file_));
      if (!file_.save(data, size))
      {
        return IStorage::IReceiptPtr();
      }

      receipt->add(size);
      return receipt;
    }

    // virtual:
    std::size_t peek(unsigned char * data, std::size_t size)
    {
      return file_.peek(data, size);
    }

    // virtual:
    uint64 skip(uint64 numBytes)
    {
      if (!file_.seek(numBytes, kRelativeToCurrent))
      {
        return 0;
      }

      return numBytes;
    }

    // virtual:
    void seekTo(uint64 absolutePosition)
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
    // Receipt
    //
    struct Receipt : public IReceipt
    {
      Receipt(TFile & file):
         file_(file),
         addr_(file.absolutePosition()),
         numBytes_(0)
      {}

      Receipt(TFile & file, uint64 addr, uint64 numBytes):
        file_(file),
        addr_(addr),
        numBytes_(0)
      {
        assert(addr_ + numBytes_ <= file_.size());
      }

      // virtual:
      uint64 position() const
      { return addr_; }

      // virtual:
      uint64 numBytes() const
      { return numBytes_; }

      // virtual:
      Receipt & setNumBytes(uint64 numBytes)
      {
        assert(addr_ + numBytes <= file_.size());
        numBytes_ = numBytes;
        return *this;
      }

      // virtual:
      Receipt & add(uint64 numBytes)
      {
        assert(addr_ + numBytes_ + numBytes <= file_.size());
        numBytes_ += numBytes;
        return *this;
      }

      // virtual:
      bool load(unsigned char * data)
      {
        TFile::TSeek temp(file_, addr_);
        return file_.load(data, (std::size_t)numBytes_);
      }

      // virtual:
      bool save(const unsigned char * data, std::size_t size)
      {
        TFile::TSeek temp(file_, addr_);
        return file_.save(data, size);
      }

      // virtual:
      bool calcCrc32(Crc32 & computeCrc32, const IReceiptPtr & skip)
      { return Yamka::calcCrc32<TFile>(file_, this, skip, computeCrc32); }

      // virtual:
      IReceiptPtr receipt(uint64 offset, uint64 size) const
      {
        if (offset + size <= numBytes_)
        {
          return IStorage::IReceiptPtr(new Receipt(file_,
                                                   addr_ + offset,
                                                   size));
        }

        assert(false);
        return IStorage::IReceiptPtr();
      }

    protected:
      TFile & file_;
      uint64 addr_;
      uint64 numBytes_;
    };

  protected:
    // storage:
    TFile file_;
  };

  //----------------------------------------------------------------
  // TConstMemoryStorage
  //
  typedef Storage<ConstFileInMemory> TConstMemoryStorage;

  //----------------------------------------------------------------
  // TMemoryStorage
  //
  typedef Storage<FileInMemory> TMemoryStorage;

}


#endif // YAMKA_MEMORY_STORAGE_H_

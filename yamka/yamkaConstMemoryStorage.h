// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat, Dec 13, 2014 10:07:01 AM
// Copyright : Bill Brimley, Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_CONST_MEMORY_STORAGE_H_
#define YAMKA_CONST_MEMORY_STORAGE_H_

// yamka includes:
#include <yamkaIStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // FileInMemory
  //
  struct FileInMemory
  {
    FileInMemory(const unsigned char * data, std::size_t size);

    // read at current file position a specified number of bytes
    // into the destination buffer:
    bool load(void * dst, std::size_t numBytes);

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

    // return current absolute file position:
    inline TFileOffset absolutePosition() const
    { return posn_; }

    const unsigned char * data_;
    const std::size_t size_;
    TFileOffset posn_;
  };

  //----------------------------------------------------------------
  // ConstMemoryStorage
  //
  struct ConstMemoryStorage : public IStorage
  {
    ConstMemoryStorage(const unsigned char * data = NULL,
                       std::size_t size = 0);

    // virtual:
    IReceiptPtr receipt() const;

    // virtual: not supported here:
    IReceiptPtr save(const unsigned char * data, std::size_t size)
    { return IStorage::IReceiptPtr(); }

    // virtual:
    IReceiptPtr load(unsigned char * data, std::size_t size);
    std::size_t peek(unsigned char * data, std::size_t size);
    uint64      skip(uint64 numBytes);
    void        seekTo(uint64 absolutePosition);

    //----------------------------------------------------------------
    // Receipt
    //
    struct Receipt : public IReceipt
    {
      Receipt(const FileInMemory & file);
      Receipt(const FileInMemory & file, uint64 addr, uint64 numBytes);

      // virtual:
      uint64 position() const;

      // virtual:
      uint64 numBytes() const;

      // virtual:
      Receipt & setNumBytes(uint64 numBytes);

      // virtual:
      Receipt & add(uint64 numBytes);

      // virtual: not supported here:
      bool save(const unsigned char * data, std::size_t size)
      { return false; }

      // virtual:
      bool load(unsigned char * data);

      // virtual:
      bool calcCrc32(Crc32 & computeCrc32, const IReceiptPtr & receiptSkip);

      // virtual:
      IReceiptPtr receipt(uint64 offset, uint64 size) const;

    protected:
      const FileInMemory & file_;
      uint64 addr_;
      uint64 numBytes_;
    };

  protected:

    // storage:
    FileInMemory file_;
  };

}


#endif // YAMKA_CONST_MEMORY_STORAGE_H_

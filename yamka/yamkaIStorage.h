// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 12:47:08 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_ISTORAGE_H_
#define YAMKA_ISTORAGE_H_

// yamka includes:
#include <yamkaCrc32.h>
#include <yamkaStdInt.h>
#include <yamkaSharedPtr.h>
#include <yamkaFile.h>


namespace Yamka
{
  // forward declarations:
  struct HodgePodge;

  //----------------------------------------------------------------
  // IStorage
  //
  // NOTE: storage is assumed to be not thread safe, multiple threads
  // should not attempt to access the same storage simultaneously
  // and expect well-defined results.
  //
  struct IStorage
  {
    virtual ~IStorage() {}

    // forward declaration:
    struct IReceipt;

    //----------------------------------------------------------------
    // IReceiptPtr
    //
    typedef TSharedPtr<IReceipt> IReceiptPtr;

    //----------------------------------------------------------------
    // IReceipt
    //
    struct IReceipt
    {
      virtual ~IReceipt() {}

      // NOTE: position interpretation is implementation specific:
      virtual uint64 position() const = 0;

      // return number of stored bytes for this receipt:
      virtual uint64 numBytes() const = 0;

      // set number of stored bytes for this receipt:
      virtual IReceipt & setNumBytes(uint64 numBytes) = 0;

      // increase number of stored bytes for this receipt:
      virtual IReceipt & add(uint64 numBytes) = 0;

      // return false if load/save fails:
      virtual bool save(const unsigned char * data, std::size_t size) = 0;
      virtual bool load(unsigned char * data) = 0;

      // compute CRC-32 checksum on data covered by this receipt,
      // skip data in region specied by receiptSkip:
      virtual bool calcCrc32(Crc32 & computeCrc32,
                             const IReceiptPtr & receiptSkip) = 0;

      // increase number of stored bytes for this receipt
      // by adding number of stored bytes in a given receipt:
      virtual IReceipt & operator += (const IReceiptPtr & receipt);

      // create a receipt for a contiguous region of data
      // contained within this receipt:
      virtual IReceiptPtr receipt(uint64 offset, uint64 size) const = 0;

      // piece-wise load data referenced by this receipt and save it
      // to the given storage, return resulting storage receipt:
      virtual IReceiptPtr saveTo(IStorage & storage,
                                 std::size_t maxChunkSize = 4096) const;
    };

    //----------------------------------------------------------------
    // Seek
    //
    struct Seek
    {
      // save current seek position:
      Seek(IStorage & storage);

      // if required, then restore saved seek position:
      ~Seek();

      // call this to disable restoring the previous file position:
      void doNotRestore();

      // call this to enable restoring the previous file position:
      void doRestore();

      // call this to immediately restore the previous file position:
      void restorePosition();

      // accessor to the absolute storage position
      // at the moment a Seek instance was created:
      uint64 absolutePosition() const;

      // synonyms:
      inline void enable()
      { doRestore(); }

      inline void disable()
      { doNotRestore(); }

    private:
      Seek(const Seek &);
      Seek & operator = (const Seek &);

      IStorage & storage_;
      uint64 savedPosition_;
      bool restoreOnExit_;
    };

    // If a storage implementation does not actually load/save
    // any data it should override this to return true.
    // A NULL storage implementation is useful for file layout optimization.
    virtual bool isNullStorage() const
    { return false; }

    // get a receipt for the current storage position:
    virtual IReceiptPtr receipt() const = 0;

    // NOTE: IStorage::save always appends at the end of the file:
    virtual IReceiptPtr save(const unsigned char * data, std::size_t size) = 0;

    // NOTE: IStorage::load always reads from current storage position:
    virtual IReceiptPtr load(unsigned char * data, std::size_t size) = 0;

    // NOTE: IStorage::peek does not change current storage position,
    // returns number of bytes loaded:
    virtual std::size_t peek(unsigned char * data, std::size_t size) = 0;

    // NOTE: seeking is not guaranteed to be supported, not all subclasses
    // provide a meaningful implementation, default implementation will
    // throw a runtime exception; seeking will also throw a runtime
    // exception if the call fails for any other reason:
    virtual void seekTo(uint64 absolutePosition);

    // NOTE: IStorage::skip always skips from current storage position,
    // returns number of bytes skipped:
    virtual uint64 skip(uint64 numBytes) = 0;

    // NOTE: this is the same as skip(numBytes) above, except this function
    // returns a storage receipt for the storage location that was skipped:
    IReceiptPtr skipWithReceipt(uint64 numBytes);
  };

  //----------------------------------------------------------------
  // NullStorage
  //
  // Helper for estimating saved element positions without actually
  // saving any element data
  //
  struct NullStorage : public IStorage
  {
    NullStorage(uint64 currentPostion = 0);

    // virtual:
    bool isNullStorage() const;

    // virtual:
    IReceiptPtr receipt() const;

    // virtual:
    IReceiptPtr save(const unsigned char * data, std::size_t size);

    // virtual: not supported for null-storage:
    IReceiptPtr load(unsigned char * data, std::size_t size);

    // virtual: not supported for null-storage:
    std::size_t peek(unsigned char * data, std::size_t size);

    // virtual:
    uint64 skip(uint64 numBytes);

    //----------------------------------------------------------------
    // Receipt
    //
    struct Receipt : public IReceipt
    {
      Receipt(uint64 addr, uint64 numBytes = 0);

      // virtual:
      uint64 position() const;

      // virtual:
      uint64 numBytes() const;

      // virtual:
      Receipt & setNumBytes(uint64 numBytes);

      // virtual:
      Receipt & add(uint64 numBytes);

      // virtual: not supported for null-storage:
      bool save(const unsigned char * data, std::size_t size);
      bool load(unsigned char * data);
      bool calcCrc32(Crc32 & computeCrc32, const IReceiptPtr & receiptSkip);

      // virtual:
      IReceiptPtr receipt(uint64 offset, uint64 size) const;

    protected:
      uint64 addr_;
      uint64 numBytes_;
    };

    uint64 currentPosition_;
  };

  //----------------------------------------------------------------
  // MemoryStorage
  //
  // Non-contiguous storage of binary data in memory
  //
  // NOTE: position, seeking, peeking and loading data are meaningless
  // to non-contiguous memory storage, and are not supported.
  //
  // Stored data may be accessed again only via storage receipt.
  //
  struct MemoryStorage : public IStorage
  {
    typedef std::vector<unsigned char> TStorage;
    typedef TSharedPtr<TStorage> TStoragePtr;

    static MemoryStorage Instance;

    // virtual:
    IReceiptPtr receipt() const;

    // virtual:
    IReceiptPtr save(const unsigned char * data, std::size_t size);

    // virtual: not supported for non-contiguous memory storage:
    IReceiptPtr load(unsigned char * data, std::size_t size);

    // virtual: not supported for non-contiguous memory storage:
    std::size_t peek(unsigned char * data, std::size_t size);

    // virtual: not supported for non-contiguous memory storage:
    uint64 skip(uint64 numBytes);

    //----------------------------------------------------------------
    // Receipt
    //
    struct Receipt : public IReceipt
    {
      Receipt(const TStoragePtr & data,
              std::size_t position = 0,
              std::size_t numBytes = 0);

      // virtual:
      uint64 position() const;

      // virtual:
      uint64 numBytes() const;

      // virtual:
      Receipt & setNumBytes(uint64 numBytes);

      // virtual:
      Receipt & add(uint64 numBytes);

      // virtual:
      bool save(const unsigned char * data, std::size_t size);

      // virtual:
      bool load(unsigned char * data);

      // virtual: not supported for non-contiguous memory storage:
      bool calcCrc32(Crc32 & computeCrc32, const IReceiptPtr & receiptSkip);

      // virtual:
      IReceiptPtr receipt(uint64 offset, uint64 size) const;

    protected:
      TStoragePtr bytesPtr_;
      std::size_t position_;
      std::size_t numBytes_;
    };
  };

  //----------------------------------------------------------------
  // MemReceipt
  //
  struct MemReceipt : public IStorage::IReceipt
  {
    MemReceipt(void * addr = NULL, std::size_t numBytes = 0);

    // virtual:
    uint64 position() const;

    // virtual:
    uint64 numBytes() const;

    // virtual: use at your own risk:
    MemReceipt & setNumBytes(uint64 numBytes);

    // virtual: use at your own risk:
    MemReceipt & add(uint64 numBytes);

    // virtual: this will fail if data is larger than this receipt
    bool save(const unsigned char * data, std::size_t size);

    // virtual: use at your own rist:
    bool load(unsigned char * data);

    // virtual: not supported
    bool calcCrc32(Crc32 & computeCrc32,
                   const IStorage::IReceiptPtr & receiptSkip);

    // virtual: use at your own risk:
    IStorage::IReceiptPtr receipt(uint64 offset, uint64 size) const;

  protected:
    unsigned char * addr_;
    std::size_t numBytes_;
  };

  //----------------------------------------------------------------
  // ConstMemReceipt
  //
  struct ConstMemReceipt : public MemReceipt
  {
    ConstMemReceipt(const void * addr = NULL, std::size_t numBytes = 0);

    // virtual: this is not allowed for const memory:
    bool save(const unsigned char * data, std::size_t size);

    // virtual: use at your own risk:
    IStorage::IReceiptPtr receipt(uint64 offset, uint64 size) const;
  };

  //----------------------------------------------------------------
  // receiptForMemory
  //
  extern IStorage::IReceiptPtr
  receiptForMemory(void * data, std::size_t size);

  //----------------------------------------------------------------
  // receiptForConstMemory
  //
  extern IStorage::IReceiptPtr
  receiptForConstMemory(const void * data, std::size_t size);

}


#endif // YAMKA_ISTORAGE_H_

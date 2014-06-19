// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 17:10:52 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_BYTES_H_
#define YAMKA_BYTES_H_

// yamka includes:
#include <yamkaStdInt.h>
#include <yamkaIStorage.h>
#include <yamkaFile.h>

// system includes:
#include <deque>
#include <vector>
#include <iostream>


namespace Yamka
{

  //----------------------------------------------------------------
  // serialize
  //
  extern std::ostream &
  serialize(std::ostream & os, const unsigned char * data, std::size_t size);

  //----------------------------------------------------------------
  // TByteVec
  //
  typedef std::vector<unsigned char> TByteVec;

  //----------------------------------------------------------------
  // TEightByteBuffer
  //
  struct TEightByteBuffer
  {
    TEightByteBuffer(unsigned int n = 0):
      n_(n)
    {}

    inline TEightByteBuffer reverseByteOrder() const
    {
      TEightByteBuffer buf(n_);

      for (unsigned int i = 0; i < n_; i++)
      {
        buf.v_[n_ - i - 1] = v_[i];
      }

      return buf;
    }

    unsigned char v_[8];
    unsigned int n_;
  };

  //----------------------------------------------------------------
  // append
  //
  inline void
  append(TByteVec & v, const unsigned char * data, std::size_t size)
  {
    v.insert(v.end(), data, data + size);
  }

  //----------------------------------------------------------------
  // append
  //
  inline void
  append(TByteVec & v, const TEightByteBuffer & buf)
  {
    v.insert(v.end(), &(buf.v_[0]), &(buf.v_[0]) + buf.n_);
  }

  //----------------------------------------------------------------
  // append
  //
  inline void
  append(TByteVec & v, const TByteVec & data)
  {
    if (!data.empty())
    {
      v.insert(v.end(), &(data[0]), &(data[0]) + data.size());
    }
  }

  //----------------------------------------------------------------
  // operator <<
  //
  inline TByteVec &
  operator << (TByteVec & v, const TEightByteBuffer & buf)
  {
    append(v, buf);
    return v;
  }

  //----------------------------------------------------------------
  // operator <<
  //
  inline TByteVec &
  operator << (TByteVec & v, const TByteVec & data)
  {
    append(v, data);
    return v;
  }

  //----------------------------------------------------------------
  // operator <<
  //
  inline TByteVec &
  operator << (TByteVec & v, unsigned char byte)
  {
    v.push_back(byte);
    return v;
  }

  //----------------------------------------------------------------
  // operator <<
  //
  inline std::ostream &
  operator << (std::ostream & os, const TByteVec & bytes)
  {
    return bytes.empty() ? os : serialize(os, &(bytes[0]), bytes.size());
  }

  //----------------------------------------------------------------
  // operator <<
  //
  inline std::ostream &
  operator << (std::ostream & os, const TEightByteBuffer & buf)
  {
    return buf.n_ ? serialize(os, buf.v_, buf.n_) : os;
  }

  //----------------------------------------------------------------
  // save
  //
  inline bool
  save(File & f, const TByteVec & vec)
  {
    return vec.empty() ? true : f.save(&(vec[0]), vec.size());
  }

  //----------------------------------------------------------------
  // load
  //
  inline bool
  load(File & f, TByteVec & vec)
  {
    return vec.empty() ? true : f.load(&(vec[0]), vec.size());
  }

  //----------------------------------------------------------------
  // save
  //
  inline IStorage::IReceiptPtr
  save(IStorage & s, const TEightByteBuffer & buf)
  {
    return buf.n_ ? s.save(buf.v_, buf.n_) : s.receipt();
  }

  //----------------------------------------------------------------
  // save
  //
  inline IStorage::IReceiptPtr
  save(IStorage & s, const TByteVec & vec)
  {
    return vec.empty() ? s.receipt() : s.save(&(vec[0]), vec.size());
  }

  //----------------------------------------------------------------
  // load
  //
  inline IStorage::IReceiptPtr
  load(IStorage & s, TByteVec & vec)
  {
    return vec.empty() ? s.receipt() : s.load(&(vec[0]), vec.size());
  }

  //----------------------------------------------------------------
  // save
  //
  inline bool
  save(IStorage::IReceipt * receipt, const TByteVec & vec)
  {
    std::size_t nb = vec.size();
    return nb ? receipt->save(&(vec[0]), nb) : receipt->save(NULL, 0);
  }

  //----------------------------------------------------------------
  // load
  //
  inline bool
  load(IStorage::IReceipt * receipt, TByteVec & vec)
  {
    std::size_t nb = (std::size_t)(receipt->numBytes());
    vec.resize(nb);
    return nb ? receipt->load(&(vec[0])) : true;
  }

  //----------------------------------------------------------------
  // receiptForMemory
  //
  inline IStorage::IReceiptPtr
  receiptForMemory(TByteVec & vec)
  {
    std::size_t size = vec.size();
    unsigned char * data = size ? &(vec[0]) : NULL;
    return receiptForMemory(data, size);
  }

  //----------------------------------------------------------------
  // receiptForConstMemory
  //
  inline IStorage::IReceiptPtr
  receiptForConstMemory(const TByteVec & vec)
  {
    std::size_t size = vec.size();
    const unsigned char * data = size ? &(vec[0]) : NULL;
    return receiptForConstMemory(data, size);
  }

  //----------------------------------------------------------------
  // vsizeDecode
  //
  // NOTE: if the decoded value equals vsizeUnknown[vsizeSize]
  //       then uintMax[8] will be returned
  //
  inline uint64
  vsizeDecode(const TEightByteBuffer & buf, uint64 & vsizeSize)
  {
    return vsizeDecode(buf.v_, vsizeSize);
  }

  //----------------------------------------------------------------
  // vsizeEncode
  //
  inline TEightByteBuffer
  vsizeEncode(uint64 vsize, uint64 nbytes)
  {
    TEightByteBuffer buf;
    buf.n_ = (unsigned int)nbytes;
    vsizeEncode(vsize, buf.v_, buf.n_);
    return buf;
  }

  //----------------------------------------------------------------
  // vsizeEncode
  //
  inline TEightByteBuffer
  vsizeEncode(uint64 vsize)
  {
    TEightByteBuffer buf;
    buf.n_ = vsizeEncode(vsize, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // vsizeSignedDecode
  //
  inline int64
  vsizeSignedDecode(const TEightByteBuffer & buf, uint64 & vsizeSize)
  {
    return vsizeSignedDecode(buf.v_, vsizeSize);
  }

  //----------------------------------------------------------------
  // vsizeSignedEncode
  //
  inline TEightByteBuffer
  vsizeSignedEncode(int64 vsize)
  {
    TEightByteBuffer buf;
    buf.n_ = vsizeSignedEncode(vsize, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // uintEncode
  //
  inline TEightByteBuffer &
  uintEncode(uint64 ui, TEightByteBuffer & buf, uint64 nbytes)
  {
    buf.n_ = (unsigned int)nbytes;
    uintEncode(ui, buf.v_, buf.n_);
    return buf;
  }

  //----------------------------------------------------------------
  // uintEncode
  //
  inline TEightByteBuffer uintEncode(uint64 ui, uint64 nbytes)
  {
    TEightByteBuffer buf;
    buf.n_ = (unsigned int)nbytes;
    uintEncode(ui, buf.v_, buf.n_);
    return buf;
  }

  //----------------------------------------------------------------
  // uintEncode
  //
  inline TEightByteBuffer &
  uintEncode(uint64 ui, TEightByteBuffer & buf)
  {
    buf.n_ = uintEncode(ui, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // uintEncode
  //
  inline TEightByteBuffer uintEncode(uint64 ui)
  {
    TEightByteBuffer buf;
    buf.n_ = uintEncode(ui, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // intEncode
  //
  inline TEightByteBuffer &
  intEncode(int64 si, TEightByteBuffer & buf, uint64 nbytes)
  {
    buf.n_ = (unsigned int)nbytes;
    intEncode(si, buf.v_, buf.n_);
    return buf;
  }

  //----------------------------------------------------------------
  // intEncode
  //
  inline TEightByteBuffer intEncode(int64 si, uint64 nbytes)
  {
    TEightByteBuffer buf;
    buf.n_ = (unsigned int)nbytes;
    intEncode(si, buf.v_, buf.n_);
    return buf;
  }

  //----------------------------------------------------------------
  // intEncode
  //
  inline TEightByteBuffer &
  intEncode(int64 si, TEightByteBuffer & buf)
  {
    buf.n_ = intEncode(si, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // intEncode
  //
  inline TEightByteBuffer intEncode(int64 si)
  {
    TEightByteBuffer buf;
    buf.n_ = intEncode(si, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // floatEncode
  //
  inline TEightByteBuffer floatEncode(float f)
  {
    TEightByteBuffer buf;
    buf.n_ = floatEncode(f, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // floatDecode
  //
  inline float
  floatDecode(const TEightByteBuffer & buf)
  {
    return floatDecode(buf.v_);
  }

  //----------------------------------------------------------------
  // doubleEncode
  //
  inline TEightByteBuffer doubleEncode(double f)
  {
    TEightByteBuffer buf;
    buf.n_ = doubleEncode(f, buf.v_);
    return buf;
  }

  //----------------------------------------------------------------
  // doubleDecode
  //
  inline double
  doubleDecode(const TEightByteBuffer & buf)
  {
    return doubleDecode(buf.v_);
  }

  //----------------------------------------------------------------
  // Indent
  //
  namespace Indent
  {
    struct More
    {
      More(unsigned int & indentation);
      ~More();

      unsigned int & indentation_;
    };

    extern unsigned int depth_;
  }

  //----------------------------------------------------------------
  // indent
  //
  struct indent
  {
    indent(unsigned int depth = 0);

    const unsigned int depth_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  extern std::ostream &
  operator << (std::ostream & s, const indent & ind);
}


#endif // YAMKA_BYTES_H_

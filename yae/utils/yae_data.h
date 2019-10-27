// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Oct 25 21:23:14 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DATA_H_
#define YAE_DATA_H_

// system includes:
#include <inttypes.h>
#include <string>
#include <string.h>
#include <vector>

// yae includes:
#include "yae/api/yae_api.h"
#include "yae/api/yae_shared_ptr.h"


namespace yae
{

  //----------------------------------------------------------------
  // IBuffer
  //
  struct YAE_API IBuffer
  {
    virtual ~IBuffer() {}
    virtual unsigned char * get() const = 0;
    virtual std::size_t size() const = 0;
    virtual void truncate(std::size_t new_size) = 0;
  };

  //----------------------------------------------------------------
  // TBufferPtr
  //
  typedef yae::shared_ptr<IBuffer> TBufferPtr;


  //----------------------------------------------------------------
  // Buffer
  //
  struct YAE_API Buffer : IBuffer
  {
    Buffer(std::size_t size):
      data_(size)
    {}

    // virtual:
    unsigned char * get() const
    { return data_.empty() ? NULL : &(data_[0]); }

    // virtual:
    std::size_t size() const
    { return data_.size(); }

    // virtual:
    void truncate(std::size_t size)
    {
      YAE_THROW_IF(data_.size() < size);
      data_.resize(size);
    }

  protected:
    mutable std::vector<unsigned char> data_;
  };


  //----------------------------------------------------------------
  // ExtBuffer
  //
  struct YAE_API ExtBuffer : IBuffer
  {
    ExtBuffer(unsigned char * data, std::size_t size):
      data_(data),
      size_(size)
    {}

    // virtual:
    unsigned char * get() const
    { return data_; }

    // virtual:
    std::size_t size() const
    { return size_; }

    // virtual:
    void truncate(std::size_t size)
    {
      YAE_THROW_IF(size_ < size);
      size_ = size;
    }

  protected:
    unsigned char * data_;
    std::size_t size_;
  };


  //----------------------------------------------------------------
  // SubBuffer
  //
  struct YAE_API SubBuffer : IBuffer
  {
    SubBuffer(const TBufferPtr & data, std::size_t addr, std::size_t size):
      data_(data),
      addr_(addr),
      size_(size)
    {}

    // virtual:
    unsigned char * get() const
    { return data_->get() + addr_; }

    // virtual:
    std::size_t size() const
    { return size_; }

    // virtual:
    void truncate(std::size_t size)
    {
      YAE_THROW_IF(size_ < size);
      size_ = size;
    }

  protected:
    TBufferPtr data_;
    std::size_t addr_;
    std::size_t size_;
  };


  //----------------------------------------------------------------
  // Data
  //
  struct YAE_API Data
  {
    explicit Data(std::size_t size):
      data_(new Buffer(size))
    {}

    // NOTE: this copies the data:
    Data(const void * data, std::size_t size)
    {
      assign(data, size);
    }

    // NOTE: this does not copy the data:
    Data(const TBufferPtr & data = TBufferPtr()):
      data_(data)
    {}

    inline void clear()
    { data_.reset(); }

    inline void set(const TBufferPtr & data)
    { data_ = data; }

    inline bool empty() const
    { return data_ ? !(data_->size()) : true; }

    inline unsigned char * alloc(std::size_t size)
    {
      if (size)
      {
        data_.reset(new Buffer(size));
        return data_->get();
      }

      data_.reset();
      return NULL;
    }

    inline unsigned char * allocz(std::size_t size)
    {
      unsigned char * data = alloc(size);

      if (data)
      {
        ::memset(data, 0, size);
      }

      return data;
    }

    inline unsigned char * resize(std::size_t size)
    { return this->resize<unsigned char>(size); }

    template <typename TData>
    inline TData *
    resize(std::size_t n)
    {
      if (n)
      {
        std::size_t size = n * sizeof(TData);
        data_.reset(new Buffer(size));
        return (TData *)(data_->get());
      }

      data_.reset();
      return NULL;
    }

    // NOTE: this copies the data:
    template <typename TData>
    inline TData *
    assign(const TData * data, std::size_t n)
    {
      if (data && n)
      {
        std::size_t size = n * sizeof(TData);
        TBufferPtr buf(new Buffer(size));
        memcpy(buf->get(), data, size);
        data_ = buf;
        return (TData *)(data_->get());
      }

      data_.reset();
      return NULL;
    }

    // NOTE: this does not copy the data:
    inline void
    shallow_ref(void * data, std::size_t size)
    {
      TBufferPtr buf;

      if (data && size)
      {
        buf.reset(new ExtBuffer((unsigned char *)data, size));
      }

      data_ = buf;
    }

    template <typename TData>
    inline TData * assign(const std::vector<TData> & data)
    { return assign<TData>(data.empty() ? NULL : &data[0], data.size()); }

    inline char * assign(const std::string & data)
    { return assign<char>(data.empty() ? NULL : data.c_str(), data.size()); }

    inline void * assign(const void * data, std::size_t size)
    { return (void *)assign<uint8_t>((const uint8_t *)data, size); }

    inline void truncate(std::size_t z)
    {
      if (z)
      {
        YAE_THROW_IF(!data_);
        data_->truncate(z);
      }
      else
      {
        data_.reset();
      }
    }

    inline std::size_t size() const
    { return data_ ? data_->size() : 0; }

    inline unsigned char * get() const
    { return data_ ? data_->get() : NULL; }

    inline unsigned char * end() const
    { return data_ ? (data_->get() + data_->size()) : NULL; }

    template <typename TData>
    inline TData * get() const
    { return data_ ? (TData *)(data_->get()) : NULL; }

    template <typename TData>
    inline TData * end() const
    { return data_ ? (TData *)(data_->get()) + num<TData>() : NULL; }

    template <typename TData>
    inline TData & get(std::size_t i) const
    {
      std::size_t z = size();
      std::size_t j = i * sizeof(TData);
      YAE_THROW_IF(z <= j);
      return *(TData *)(data_->get() + j);
    }

    template <typename TData>
    inline std::size_t num() const
    {
      std::size_t z = size();
      YAE_ASSERT(z % sizeof(TData) == 0);
      std::size_t n = z / sizeof(TData);
      return n;
    }

    inline void memset(unsigned char v)
    {
      std::size_t z = size();
      if (z)
      {
        unsigned char * data = data_->get();
        ::memset(data, v, z);
      }
    }

    inline std::string to_str() const
    {
      if (!empty())
      {
        const char * str = (const char *)(data_->get());
        const char * end = str + data_->size();
        return std::string(str, end);
      }

      return std::string();
    }

    inline void swap(Data & other)
    { std::swap(data_, other.data_); }

    // user-defined conversions
    // https://en.cppreference.com/w/cpp/language/cast_operator

    inline operator bool() const
    { return size() != 0; }

    inline operator TBufferPtr() const
    { return data_; }

    template <typename TOffset>
    inline unsigned char * operator + (TOffset i) const
    {
      YAE_ASSERT(i < size());
      return data_->get() + i;
    }

    inline unsigned char & operator[](std::size_t i)
    {
      YAE_ASSERT(i < size());
      return data_->get()[i];
    }

    inline const unsigned char & operator[](std::size_t i) const
    {
      YAE_ASSERT(i < size());
      return data_->get()[i];
    }

    inline operator void * () const
    { return data_ ? data_->get() : NULL; }

    template <typename TData>
    inline operator TData * ()
    { return data_ ? (TData *)(data_->get()) : NULL; }

    template <typename TData>
    inline operator const TData * () const
    { return data_ ? (const TData *)(data_->get()) : NULL; }

  protected:
    TBufferPtr data_;
  };

  //----------------------------------------------------------------
  // Bitstream
  //
  struct YAE_API Bitstream
  {
    Bitstream(const TBufferPtr & data):
      data_(data),
      position_(0),
      end_(data ? (data->size() << 3) : 0)
    {}

    // set current bitstream position:
    inline void seek(std::size_t bit_position)
    {
      YAE_ASSERT(bit_position <= end_);
      YAE_THROW_IF(end_ < bit_position);
      position_ = bit_position;
    }

    inline void skip(int num_bits)
    { seek(position_ + num_bits); }

    inline void skip_bytes(std::size_t bytes)
    { seek(position_ + (bytes << 3)); }

    inline bool has_enough_bits(std::size_t num_bits) const
    {
      std::size_t need = position_ + num_bits;
      return need <= end_;
    }

    inline bool has_enough_bytes(std::size_t num_bytes) const
    {
      std::size_t need = num_bytes + (position_ >> 3);
      return data_ ? need <= data_->size() : !num_bytes;
    }

    // read a given number of bits and advance current bitstream
    // position accordingly.
    //
    // NOTE: this converts from big-endian integer in the bitstream
    // to a native-endian integer.
    //
    // We can read up to 64 bits at a time. Attempting to read
    // more than 64 bits is considered an abuse of the API.
    // Use skip(bits), or seek(pos), or skip_bytes(n),
    // instead to skip past bits without reading them.
    //
    uint64_t read_bits(int num_bits);

    // same as above, but preserves current bitstream position:
    uint64_t peek_bits(int num_bits);

    //----------------------------------------------------------------
    // Bits
    //
    struct Bits
    {
      inline explicit Bits(const int64_t & data):
        data_(data)
      {}

      template <typename TData>
      inline operator TData() const
      { return TData(data_); }

      template <typename TData>
      inline TData operator & (const TData & mask) const
      { return TData(data_ & mask); }

      uint64_t data_;
    };

    inline Bits read(int num_bits)
    { return Bits(read_bits(num_bits)); }

    template <typename TData>
    inline TData read(int num_bits)
    { return TData(read_bits(num_bits)); }

    TBufferPtr read_bytes(std::size_t bytes);

    void write_bits(std::size_t num_bits, uint64_t bits);
    void write_bytes(const void * data, std::size_t size);

    template <typename TData>
    inline void write(std::size_t num_bits, TData data)
    { write_bits(num_bits, uint64_t(data)); }

    // https://en.wikipedia.org/wiki/Exponential-Golomb_coding
    uint64_t read_bits_ue();
    int64_t read_bits_se();

    void write_bits_ue(uint64_t v);
    void write_bits_se(int64_t v);

    // helpers:
    inline void skip_until_byte_aligned()
    {
      std::size_t misaligned = position_ & 0x7;
      if (misaligned)
      {
        skip(8 - misaligned);
      }
    }

    inline void pad_until_byte_aligned()
    {
      std::size_t misaligned = position_ & 0x7;
      if (misaligned)
      {
        write(0, 8 - misaligned);
      }
    }

    inline std::size_t position() const
    { return position_; }

  protected:
    TBufferPtr data_;
    std::size_t position_;
    std::size_t end_;
  };

}


#endif // YAE_DATA_H_

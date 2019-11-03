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
    virtual ~IBuffer();

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
    Buffer(std::size_t size);

    // virtual:
    unsigned char * get() const;

    // virtual:
    std::size_t size() const;

    // virtual:
    void truncate(std::size_t size);

  protected:
    mutable std::vector<unsigned char> data_;
  };


  //----------------------------------------------------------------
  // ExtBuffer
  //
  struct YAE_API ExtBuffer : IBuffer
  {
    ExtBuffer(void * data, std::size_t size);

    // virtual:
    unsigned char * get() const;

    // virtual:
    std::size_t size() const;

    // virtual:
    void truncate(std::size_t size);

  protected:
    unsigned char * data_;
    std::size_t size_;
  };


  //----------------------------------------------------------------
  // SubBuffer
  //
  struct YAE_API SubBuffer : IBuffer
  {
    SubBuffer(const TBufferPtr & data, std::size_t addr, std::size_t size);

    // virtual:
    unsigned char * get() const;

    // virtual:
    std::size_t size() const;

    // virtual:
    void truncate(std::size_t size);

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
        return static_cast<TData *>(data_->get());
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
        return reinterpret_cast<TData *>(data_->get());
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
        buf.reset(new ExtBuffer(data, size));
      }

      data_ = buf;
    }

    template <typename TData>
    inline TData * assign(const std::vector<TData> & data)
    { return assign<TData>(data.empty() ? NULL : &data[0], data.size()); }

    inline char * assign(const std::string & data)
    { return assign<char>(data.empty() ? NULL : data.c_str(), data.size()); }

    inline void * assign(const void * data, std::size_t size)
    {
      const unsigned char * v = reinterpret_cast<const unsigned char *>(data);
      return assign<unsigned char>(v, size);
    }

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

    inline TBufferPtr get(std::size_t addr, std::size_t size) const
    { return TBufferPtr(new SubBuffer(data_, addr, size)); }

    inline unsigned char * get() const
    { return data_ ? data_->get() : NULL; }

    inline unsigned char * end() const
    { return data_ ? (data_->get() + data_->size()) : NULL; }

    template <typename TData>
    inline TData * get() const
    { return data_ ? reinterpret_cast<TData *>(data_->get()) : NULL; }

    template <typename TData>
    inline TData * end() const
    { return data_ ? get<TData>() + num<TData>() : NULL; }

    template <typename TData>
    inline TData & get(std::size_t i) const
    {
      std::size_t z = size();
      std::size_t j = i * sizeof(TData);
      YAE_THROW_IF(z <= j);
      return *reinterpret_cast<TData *>(data_->get() + j);
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
        const char * str = reinterpret_cast<const char *>(data_->get());
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
    { return data_ ? static_cast<TData *>(data_->get()) : NULL; }

    template <typename TData>
    inline operator const TData * () const
    { return data_ ? static_cast<const TData *>(data_->get()) : NULL; }

  protected:
    TBufferPtr data_;
  };


  //----------------------------------------------------------------
  // IBitstream
  //
  struct YAE_API IBitstream
  {
    IBitstream(std::size_t end):
      position_(0),
      end_(end)
    {}

    virtual ~IBitstream() {}

    inline bool at_end() const
    { return position_ == end_; }

    // set current bitstream position:
    inline void seek(std::size_t bit_position)
    {
      YAE_ASSERT(bit_position <= end_);
      YAE_THROW_IF(end_ < bit_position);
      position_ = bit_position;
    }

    inline void skip(std::size_t num_bits)
    { seek(position_ + num_bits); }

    inline void skip_bytes(std::size_t bytes)
    { seek(position_ + (bytes << 3)); }

    inline bool has_enough_bits(std::size_t num_bits) const
    {
      std::size_t need = position_ + num_bits;
      return need <= end_;
    }

    inline bool has_enough_bytes(std::size_t num_bytes) const
    { return has_enough_bits(num_bytes << 3); }

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
    virtual uint64_t read_bits(std::size_t num_bits) = 0;

    // same as above, but preserves current bitstream position:
    inline uint64_t peek_bits(std::size_t num_bits)
    {
      std::size_t pos = position_;
      uint64_t b = read_bits(num_bits);
      position_ = pos;
      return b;
    }

    inline bool next_bits(std::size_t num_bits, uint64_t expected)
    {
      if (has_enough_bits(num_bits))
      {
        uint64_t actual = peek_bits(num_bits);
        return expected == actual;
      }

      return false;
    }

    //----------------------------------------------------------------
    // Bits
    //
    struct Bits
    {
      inline explicit Bits(const uint64_t & data):
        data_(data)
      {}

      template <typename TData>
      inline operator TData() const
      { return static_cast<TData>(data_); }

      template <typename TData>
      inline TData operator & (const TData & mask) const
      { return static_cast<TData>(data_ & mask); }

      uint64_t data_;
    };

    inline Bits read(std::size_t num_bits)
    { return Bits(this->read_bits(num_bits)); }

    template <typename TData>
    inline TData read(std::size_t num_bits)
    { return TData(this->read_bits(num_bits)); }

    virtual TBufferPtr read_bytes(std::size_t num_bytes) = 0;

    virtual void write_bits(std::size_t num_bits, uint64_t bits) = 0;
    virtual void write_bytes(const void * data, std::size_t num_bytes) = 0;

    template <typename TData>
    inline void write(std::size_t num_bits, TData data)
    { this->write_bits(num_bits, uint64_t(data)); }

    // https://en.wikipedia.org/wiki/Exponential-Golomb_coding
    virtual uint64_t read_bits_ue();
    int64_t read_bits_se();

    void write_bits_ue(uint64_t v);
    void write_bits_se(int64_t v);

    // helpers:
    inline bool is_byte_aligned()
    {
      std::size_t misaligned = position_ & 0x7;
      return !misaligned;
    }

    inline void skip_until_byte_aligned()
    {
      std::size_t misaligned = position_ & 0x7;
      if (misaligned)
      {
        this->skip(8 - misaligned);
      }
    }

    inline void pad_until_byte_aligned()
    {
      std::size_t misaligned = position_ & 0x7;
      if (misaligned)
      {
        this->write(0, 8 - misaligned);
      }
    }

    inline std::size_t position() const
    { return position_; }

    inline TBufferPtr read_remaining_bytes()
    {
      std::size_t remaining_bits = end_ - position_;
      YAE_ASSERT((remaining_bits & 0x7) == 0)
      return this->read_bytes(remaining_bits >> 3);
    }

  protected:
    std::size_t position_;
    std::size_t end_;
  };


  //----------------------------------------------------------------
  // NullBitstream
  //
  // /dev/null bitstream
  //
  struct YAE_API NullBitstream : IBitstream
  {
    NullBitstream(std::size_t end = std::numeric_limits<std::size_t>::max());

    // virtual:
    uint64_t read_bits(std::size_t num_bits);

    // virtual:
    TBufferPtr read_bytes(std::size_t num_bytes);

    // virtual:
    void write_bits(std::size_t num_bits, uint64_t bits);

    // virtual:
    void write_bytes(const void * data, std::size_t num_bytes);

    // virtual: not supported for null bitstream, will throw an exception:
    uint64_t read_bits_ue();
  };


  //----------------------------------------------------------------
  // Bitstream
  //
  struct YAE_API Bitstream : IBitstream
  {
    Bitstream(const TBufferPtr & data);

    // virtual:
    uint64_t read_bits(std::size_t num_bits);

    // virtual:
    TBufferPtr read_bytes(std::size_t bytes);

    // virtual:
    void write_bits(std::size_t num_bits, uint64_t bits);

    // virtual:
    void write_bytes(const void * data, std::size_t num_bytes);

  protected:
    TBufferPtr data_;
  };


  namespace bitstream
  {
    //----------------------------------------------------------------
    // IPayload
    //
    struct YAE_API IPayload
    {
      virtual ~IPayload() {}

      // payload bitstream size, expressed in bits
      // calculated atomatically using NullBitstream
      virtual std::size_t size() const;

      virtual void save(IBitstream & bs) const = 0;
      virtual bool load(IBitstream & bs) = 0;
    };
  }


  //----------------------------------------------------------------
  // Bit
  //
  template <std::size_t nbits, uint64_t default_value = 0>
  struct YAE_API Bit : bitstream::IPayload
  {
    Bit(uint64_t value = default_value):
      data_(value)
    {}

    // virtual:
    std::size_t size() const
    { return nbits; }

    // virtual:
    void save(IBitstream & bs) const
    { bs.write_bits(nbits, data_); }

    // virtual:
    bool load(IBitstream & bs)
    {
      if (!bs.has_enough_bits(nbits))
      {
        return false;
      }

      data_ = bs.read_bits(nbits);
      return true;
    }

    inline Bit & operator = (uint64_t value)
    {
      data_ = value;
      return *this;
    }

    uint64_t data_;
  };


  //----------------------------------------------------------------
  // NBit
  //
  struct YAE_API NBit : bitstream::IPayload
  {
    NBit(std::size_t nbits, uint64_t data = 0):
      nbits_(nbits),
      data_(data)
    {}

    // virtual:
    std::size_t size() const;

    // virtual:
    void save(IBitstream & bs) const;

    // virtual:
    bool load(IBitstream & bs);

    inline NBit & operator = (uint64_t value)
    {
      data_ = value;
      return *this;
    }

    std::size_t nbits_;
    uint64_t data_;
  };

}


#endif // YAE_DATA_H_
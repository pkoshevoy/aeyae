// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Oct 25 21:23:14 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae:
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // IBuffer::~IBuffer
  //
  IBuffer::~IBuffer()
  {}


  //----------------------------------------------------------------
  // Buffer::Buffer
  //
  Buffer::Buffer(std::size_t size):
    data_(size)
  {}

  //----------------------------------------------------------------
  // Buffer::get
  //
  unsigned char *
  Buffer::get() const
  {
    return data_.empty() ? NULL : &(data_[0]);
  }

  //----------------------------------------------------------------
  // Buffer::size
  //
  std::size_t
  Buffer::size() const
  {
    return data_.size();
  }

  //----------------------------------------------------------------
  // Buffer::truncate
  //
  void
  Buffer::truncate(std::size_t size)
  {
    YAE_THROW_IF(data_.size() < size);
    data_.resize(size);
  }


  //----------------------------------------------------------------
  // ExtBuffer::ExtBuffer
  //
  ExtBuffer::ExtBuffer(void * data, std::size_t size):
    data_(static_cast<unsigned char *>(data)),
    size_(size)
  {}

  //----------------------------------------------------------------
  // ExtBuffer::get
  //
  unsigned char *
  ExtBuffer::get() const
  {
    return data_;
  }

  //----------------------------------------------------------------
  // ExtBuffer::size
  //
  std::size_t
  ExtBuffer::size() const
  {
    return size_;
  }

  //----------------------------------------------------------------
  // ExtBuffer::truncate
  //
  void
  ExtBuffer::truncate(std::size_t size)
  {
    YAE_THROW_IF(size_ < size);
    size_ = size;
  }


  //----------------------------------------------------------------
  // SubBuffer::SubBuffer
  //
  SubBuffer::SubBuffer(const TBufferPtr & data,
                       std::size_t addr,
                       std::size_t size):
    data_(data),
    addr_(addr),
    size_(size)
  {}

  //----------------------------------------------------------------
  // SubBuffer::get
  //
  unsigned char *
  SubBuffer::get() const
  {
    return data_->get() + addr_;
  }

  //----------------------------------------------------------------
  // SubBuffer::size
  //
  std::size_t
  SubBuffer::size() const
  {
    return size_;
  }

  //----------------------------------------------------------------
  // SubBuffer::truncate
  //
  void
  SubBuffer::truncate(std::size_t size)
  {
    YAE_THROW_IF(size_ < size);
    size_ = size;
  }


  //----------------------------------------------------------------
  // Data::load_hex
  //
  Data &
  Data::load_hex(const char * hex_str, std::size_t hex_len)
  {
    if (!hex_len)
    {
      hex_len = strlen(hex_str);
    }

    std::size_t num_bytes = (hex_len + 1) / 2;
    this->resize(num_bytes);
    yae::load_hex(this->get(), this->size(), hex_str, hex_len);
    return *this;
  }

  //----------------------------------------------------------------
  // Data::load_hex
  //
  Data &
  Data::load_hex(const std::string & hex_str)
  {
    std::size_t len = hex_str.size();
    const char * src = len ? &hex_str[0] : "";
    return this->load_hex(src, len);
  }

  //----------------------------------------------------------------
  // Data::to_hex
  //
  std::string
  Data::to_hex() const
  {
    return yae::to_hex(this->get(), this->size());
  }

  //----------------------------------------------------------------
  // IBitstream::read_pascal_string
  //
  std::size_t
  IBitstream::read_pascal_string(std::string & str)
  {
    YAE_ASSERT((position_ % 8) == 0);
    uint8_t num_bytes = this->read<uint8_t>();
    std::size_t num_read = this->read_string(str, num_bytes);
    return num_read + 1;
  }

  //----------------------------------------------------------------
  // IBitstream::read_string
  //
  std::size_t
  IBitstream::read_string(std::string & str, std::size_t num_bytes)
  {
    YAE_ASSERT((position_ % 8) == 0);

    std::size_t end_pos = position_ + (num_bytes << 3);
    YAE_ASSERT(end_pos <= end_);
    end_pos = std::min(end_pos, end_);

    return this->read_string_until(str, end_pos);
  }

  //----------------------------------------------------------------
  // IBitstream::read_string_until
  //
  std::size_t
  IBitstream::read_string_until(std::string & str, std::size_t end_pos)
  {
    YAE_ASSERT((position_ % 8) == 0);
    YAE_ASSERT(end_pos <= end_);
    end_pos = std::min(end_pos, end_);

    std::size_t n = 0;
    while (position_ < end_pos)
    {
      char c = this->read<char>(8);
      str.push_back(c);
      n += 1;
    }

    return n;
  }

  //----------------------------------------------------------------
  // IBitstream::read_string_until_null
  //
  std::size_t
  IBitstream::read_string_until_null(std::string & str, std::size_t end_pos)
  {
    YAE_ASSERT((position_ % 8) == 0);
    YAE_ASSERT(end_pos <= end_);
    end_pos = std::min(end_pos, end_);

    std::size_t n = 0;
    while (position_ < end_pos)
    {
      char c = this->read<char>(8);
      if (!c)
      {
        break;
      }

      str.push_back(c);
      n += 1;
    }

    return n;
  }

  //----------------------------------------------------------------
  // IBitstream::read_bits_ue
  //
  // ISO/IEC 14496-10, section 9.1, Parsing process for Exp-Golomb codes
  //
  uint64_t
  IBitstream::read_bits_ue()
  {
    std::size_t starting_bit_position = position_;

    // skip the leading zero bits:
    while (!read_bits(1))
    {}

    // count the number of leading zero bits:
    std::size_t leading_zero_bits = position_ - starting_bit_position - 1;

    uint64_t ue = (1 << leading_zero_bits) - 1;
    if (leading_zero_bits)
    {
      ue += read_bits(leading_zero_bits);
    }

    return ue;
  }

  //----------------------------------------------------------------
  // IBitstream::read_bits_se
  //
  // ISO/IEC 14496-10, section 9.1.1, Parsing process for Exp-Golomb codes
  //
  int64_t
  IBitstream::read_bits_se()
  {
    uint64_t ue = read_bits_ue();
    int64_t se = (ue & 0x1) ? int64_t(1 + (ue >> 1)) : -int64_t(ue >> 1);
    return se;
  }

  //----------------------------------------------------------------
  // IBitstream::write_bits_ue
  //
  // ISO/IEC 14496-10, section 9.1, Parsing process for Exp-Golomb codes
  //
  void
  IBitstream::write_bits_ue(uint64_t v)
  {
    // https://en.wikipedia.org/wiki/Exponential-Golomb_coding
    uint64_t v1 = v + 1;
    unsigned int w = yae::bitmask_width(v1);
    write_bits(w - 1, 0);
    write_bits(w, v1);
  }

  //----------------------------------------------------------------
  // IBitstream::write_bits_se
  //
  // ISO/IEC 14496-10, section 9.1.1, Parsing process for Exp-Golomb codes
  //
  void
  IBitstream::write_bits_se(int64_t v)
  {
    uint64_t u = (v > 0) ? (uint64_t(v << 1) - 1) : (uint64_t(-v) << 1);
    write_bits_ue(u);
  }


  //----------------------------------------------------------------
  // SetEnd::SetEnd
  //
  SetEnd::SetEnd(IBitstream & bin, std::size_t new_end):
    bin_(bin),
    end_(bin.end())
  {
    YAE_THROW_IF(end_ < new_end);
    bin_.set_end(new_end);
  }

  //----------------------------------------------------------------
  // SetEnd::~SetEnd
  //
  SetEnd::~SetEnd()
  {
    // restore previous end position:
    bin_.set_end(end_);
  }


  //----------------------------------------------------------------
  // NullBitstream::NullBitstream
  //
  NullBitstream::NullBitstream(std::size_t end):
    IBitstream(end)
  {}

  //----------------------------------------------------------------
  // NullBitstream::read_bits
  //
  uint64_t
  NullBitstream::read_bits(std::size_t num_bits)
  {
    YAE_THROW_IF(64 < num_bits);
    YAE_THROW_IF(!IBitstream::has_enough_bits(num_bits));

    IBitstream::position_ += num_bits;
    return 0;
  }

  //----------------------------------------------------------------
  // NullBitstream::read_bytes
  //
  TBufferPtr
  NullBitstream::read_bytes(std::size_t num_bytes)
  {
    YAE_THROW_IF(!has_enough_bytes(num_bytes));
    YAE_EXPECT(IBitstream::is_byte_aligned());

    Data data;
    data.allocz(num_bytes);

    TBufferPtr buffer = data;
    return buffer;
  }

  //----------------------------------------------------------------
  // NullBitstream::write_bits
  //
  void
  NullBitstream::write_bits(std::size_t num_bits, uint64_t bits)
  {
    YAE_THROW_IF(64 < num_bits);
    YAE_THROW_IF(!IBitstream::has_enough_bits(num_bits));

    (void)bits;
    IBitstream::position_ += num_bits;
  }

  //----------------------------------------------------------------
  // NullBitstream::write_bytes
  //
  void
  NullBitstream::write_bytes(const void * data, std::size_t num_bytes)
  {
    YAE_THROW_IF(!IBitstream::has_enough_bytes(num_bytes));
    YAE_EXPECT(IBitstream::is_byte_aligned());

    (void)data;
    IBitstream::position_ += (num_bytes << 3);
  }

  //----------------------------------------------------------------
  // NullBitstream::read_bits_ue
  //
  uint64_t
  NullBitstream::read_bits_ue()
  {
    throw std::runtime_error("NullBitstream::read_bits_ue is not supported");
    return 0;
  }


  //----------------------------------------------------------------
  // Bitstream::Bitstream
  //
  Bitstream::Bitstream(const TBufferPtr & data):
    IBitstream(data ? (data->size() << 3) : 0),
    data_(data)
  {}

  //----------------------------------------------------------------
  // Bitstream::read_bits
  //
  uint64_t
  Bitstream::read_bits(std::size_t num_bits)
  {
    YAE_ASSERT(num_bits <= 64 && IBitstream::has_enough_bits(num_bits));
    YAE_THROW_IF(64 < num_bits);
    YAE_THROW_IF(!IBitstream::has_enough_bits(num_bits));

    // output value:
    uint64_t v = 0;

    // starting byte position:
    std::size_t y0 = IBitstream::position_ >> 3;

    // starting bit offset relative to starting byte position:
    std::size_t i0 = IBitstream::position_ & 0x7;
    std::size_t n0 = 8 - i0;

    // mask-off leading bits and trailing bits:
    unsigned char m0 = static_cast<unsigned char>(~(0xFF << n0));

    // shortcut to bytes:
    unsigned char * b = data_->get() + y0;

    v |= ((*b) & m0);

    if (i0 + num_bits < 8)
    {
      v >>= (n0 - num_bits);
      IBitstream::position_ += num_bits;
      return v;
    }

    IBitstream::position_ += n0;
    num_bits -= n0;
    b++;

    while (8 <= num_bits)
    {
      num_bits -= 8;
      IBitstream::position_ += 8;
      v <<= 8;
      v |= (*b);
      b++;
    }

    if (num_bits)
    {
      v <<= num_bits;
      IBitstream::position_ += num_bits;
      std::size_t n1 = 8 - num_bits;
      v |= ((*b) >> n1);
    }

    return v;
  }

  //----------------------------------------------------------------
  // Bitstream::read_bytes
  //
  TBufferPtr
  Bitstream::read_bytes(std::size_t num_bytes)
  {
    TBufferPtr data;
    if (!num_bytes)
    {
      return data;
    }

    YAE_ASSERT(IBitstream::has_enough_bytes(num_bytes));
    YAE_THROW_IF(!IBitstream::has_enough_bytes(num_bytes));
    YAE_EXPECT(IBitstream::is_byte_aligned());

    if (IBitstream::is_byte_aligned())
    {
      std::size_t addr = IBitstream::position_ >> 3;
      data.reset(new SubBuffer(data_, addr, num_bytes));
      IBitstream::position_ += (num_bytes << 3);
    }
    else
    {
      // slow:
      data.reset(new Buffer(num_bytes));
      unsigned char * b = data->get();
      for (std::size_t i = 0; i < num_bytes; i++)
      {
        *b = read(8);
        ++b;
      }
    }

    return data;
  }

  //----------------------------------------------------------------
  // Bitstream::write_bits
  //
  void
  Bitstream::write_bits(std::size_t num_bits, uint64_t v)
  {
    YAE_THROW_IF(64 < num_bits);
    YAE_THROW_IF(!IBitstream::has_enough_bits(num_bits));

    // starting byte position:
    std::size_t y0 = IBitstream::position_ >> 3;

    // starting bit offset relative to starting byte position:
    std::size_t i0 = IBitstream::position_ & 0x7;
    std::size_t n0 = 8 - i0;

    // shortcut to bytes:
    unsigned char * b = data_->get() + y0;

    if (i0 + num_bits < 8)
    {
      unsigned char m = static_cast<unsigned char>(~(0xFF << num_bits));
      *b &= ~(m << (n0 - num_bits));
      *b |= (v << (n0 - num_bits));
      IBitstream::position_ += num_bits;
      return;
    }

    unsigned char m = static_cast<unsigned char>(0xFF << n0);
    num_bits -= n0;
    IBitstream::position_ += n0;
    *b &= m; 
    *b |= (v >> num_bits);
    b++;

    while (8 <= num_bits)
    {
      num_bits -= 8;
      IBitstream::position_ += 8;
      *b = static_cast<unsigned char>(v >> num_bits);
      b++;
    }

    if (num_bits)
    {
      IBitstream::position_ += num_bits;
      std::size_t n1 = 8 - num_bits;
      unsigned char m = static_cast<unsigned char>(~(0xFF << num_bits));
      *b &= ~(m << n1);
      *b |= ((v & m) << n1);
    }
  }

  //----------------------------------------------------------------
  // Bitstream::write_bytes
  //
  void
  Bitstream::write_bytes(const void * data, std::size_t num_bytes)
  {
    YAE_THROW_IF(!IBitstream::has_enough_bytes(num_bytes));
    YAE_EXPECT(IBitstream::is_byte_aligned());

    if (IBitstream::is_byte_aligned())
    {
      std::size_t addr = IBitstream::position_ >> 3;
      unsigned char * b = data_->get() + addr;
      memcpy(b, data, num_bytes);
      IBitstream::position_ += (num_bytes << 3);
    }
    else
    {
      // slow:
      const unsigned char * b = static_cast<const unsigned char *>(data);
      const unsigned char * end = b + num_bytes;
      while (b < end)
      {
        write_bits(8, *b);
        ++b;
      }
    }
  }


  namespace bitstream
  {
    //----------------------------------------------------------------
    // IPayload::size
    //
    std::size_t
    IPayload::size() const
    {
      yae::NullBitstream null_bitstream;
      save(null_bitstream);
      return null_bitstream.position();
    }


    //----------------------------------------------------------------
    // ByteAlignment::save
    //
    void
    ByteAlignment::save(IBitstream & bin) const
    {
      bin.pad_until_byte_aligned();
    }

    //----------------------------------------------------------------
    // ByteAlignment::load
    //
    bool
    ByteAlignment::load(IBitstream & bin)
    {
      std::size_t misaligned = bin.position() & 0x7;
      if (!misaligned)
      {
        return true;
      }

      std::size_t padding = 8 - misaligned;
      if (!bin.has_enough_bits(padding))
      {
        return false;
      }

      bin.skip_until_byte_aligned();
      return true;
    }
  }


  //----------------------------------------------------------------
  // NBit::size
  //
  std::size_t
  NBit::size() const
  {
    return nbits_;
  }

  //----------------------------------------------------------------
  // NBit::save
  //
  void
  NBit::save(IBitstream & bs) const
  {
    bs.write_bits(nbits_, data_);
  }

  //----------------------------------------------------------------
  // NBit::load
  //
  bool
  NBit::load(IBitstream & bs)
  {
    if (!bs.has_enough_bits(nbits_))
    {
      return false;
    }

    data_ = bs.read_bits(nbits_);
    return true;
  }


  //----------------------------------------------------------------
  // load_as_utf8
  //
  void
  load_as_utf8(std::string & output, IBitstream & bin, std::size_t end_pos)
  {
    if (bin.at_end())
    {
      return;
    }

    if (bin.peek<uint16_t>(16) == 0xFEFF)
    {
      // big endian UTF-16:
      while (bin.position() + 16 <= end_pos)
      {
        Data wc = bin.read_bytes(2);
        yae::utf16be_to_utf8(wc.get(), wc.end(), output);

        // check for NULL terminator:
        if (wc[0] == 0 && wc[1] == 0)
        {
          break;
        }
      }
    }
    else if (bin.peek<uint16_t>(16) == 0xFFFE)
    {
      // little endian UTF-16:
      while (bin.position() + 16 <= end_pos)
      {
        Data wc = bin.read_bytes(2);
        yae::utf16le_to_utf8(wc.get(), wc.end(), output);

        // check for NULL terminator:
        if (wc[0] == 0 && wc[1] == 0)
        {
          break;
        }
      }
    }
    else // UTF-8
    {
      bin.read_string_until_null(output, end_pos);
    }
  }

}

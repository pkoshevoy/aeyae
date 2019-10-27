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
  // Bitstream::read_bits
  //
  uint64_t
  Bitstream::read_bits(int num_bits)
  {
    YAE_THROW_IF(64 < num_bits);
    YAE_THROW_IF(!has_enough_bits(num_bits));

    // output value:
    uint64_t v = 0;

    // starting byte position:
    std::size_t y0 = position_ >> 3;

    // starting bit offset relative to starting byte position:
    std::size_t i0 = position_ & 0x7;
    std::size_t n0 = 8 - i0;

    // mask-off leading bits and trailing bits:
    unsigned char m0 = ~(0xFF << n0);

    // shortcut to bytes:
    unsigned char * b = data_->get() + y0;

    v |= ((*b) & m0);

    if (i0 + num_bits < 8)
    {
      v >>= (n0 - num_bits);
      position_ += num_bits;
      return v;
    }

    position_ += n0;
    num_bits -= n0;
    b++;

    while (8 <= num_bits)
    {
      num_bits -= 8;
      position_ += 8;
      v <<= 8;
      v |= (*b);
      b++;
    }

    if (num_bits)
    {
      v <<= num_bits;
      position_ += num_bits;
      std::size_t n1 = 8 - num_bits;
      v |= ((*b) >> n1);
    }

    return v;
  }

  //----------------------------------------------------------------
  // Bitstream::peek_bits
  //
  uint64_t
  Bitstream::peek_bits(int num_bits)
  {
    std::size_t current_position = position_;
    uint64_t v = read_bits(num_bits);
    position_ = current_position;
    return v;
  }

  //----------------------------------------------------------------
  // Bitstream::read_bytes
  //
  TBufferPtr
  Bitstream::read_bytes(std::size_t num_bytes)
  {
    YAE_THROW_IF(!has_enough_bytes(num_bytes));
    YAE_EXPECT(!(position_ & 0x7));

    TBufferPtr data;

    if (!(position_ & 0x7))
    {
      std::size_t addr = position_ >> 3;
      data.reset(new SubBuffer(data_, addr, num_bytes));
      position_ += (num_bytes << 3);
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
    YAE_THROW_IF(!has_enough_bits(num_bits));

    // starting byte position:
    std::size_t y0 = position_ >> 3;

    // starting bit offset relative to starting byte position:
    std::size_t i0 = position_ & 0x7;
    std::size_t n0 = 8 - i0;

    // shortcut to bytes:
    unsigned char * b = data_->get() + y0;

    if (i0 + num_bits < 8)
    {
      unsigned char m = ~(0xFF << num_bits);
      *b &= ~(m << (n0 - num_bits));
      *b |= (v << (n0 - num_bits));
      position_ += num_bits;
      return;
    }

    unsigned char m = 0xFF << n0;
    num_bits -= n0;
    position_ += n0;
    *b &= m; 
    *b |= (v >> num_bits);
    b++;

    while (8 <= num_bits)
    {
      num_bits -= 8;
      position_ += 8;
      *b = (v >> num_bits);
      b++;
    }

    if (num_bits)
    {
      position_ += num_bits;
      std::size_t n1 = 8 - num_bits;
      unsigned char m = ~(0xFF << num_bits);
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
    YAE_THROW_IF(!has_enough_bytes(num_bytes));
    YAE_EXPECT(!(position_ & 0x7));

    if (!(position_ & 0x7))
    {
      std::size_t addr = position_ >> 3;
      unsigned char * b = data_->get() + addr;
      memcpy(b, data, num_bytes);
      position_ += (num_bytes << 3);
    }
    else
    {
      // slow:
      const unsigned char * b = (unsigned char *)data;
      const unsigned char * end = b + num_bytes;
      while (b < end)
      {
        write_bits(8, *b);
        ++b;
      }
    }
  }


  //----------------------------------------------------------------
  // Bitstream::read_bits_ue
  //
  // ISO/IEC 14496-10, section 9.1, Parsing process for Exp-Golomb codes
  //
  uint64_t
  Bitstream::read_bits_ue()
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
  // Bitstream::read_bits_se
  //
  // ISO/IEC 14496-10, section 9.1.1, Parsing process for Exp-Golomb codes
  //
  int64_t
  Bitstream::read_bits_se()
  {
    uint64_t ue = read_bits_ue();
    int64_t se = (ue & 0x1) ? int64_t((ue + 1) >> 1) : -int64_t(ue >> 1);
    return se;
  }

  //----------------------------------------------------------------
  // Bitstream::write_bits_ue
  //
  // ISO/IEC 14496-10, section 9.1, Parsing process for Exp-Golomb codes
  //
  void
  Bitstream::write_bits_ue(uint64_t v)
  {
    // https://en.wikipedia.org/wiki/Exponential-Golomb_coding
    uint64_t v1 = v + 1;
    unsigned int w = yae::bitmask_width(v1);
    write_bits(w - 1, 0);
    write_bits(w, v1);
  }

  //----------------------------------------------------------------
  // Bitstream::write_bits_se
  //
  // ISO/IEC 14496-10, section 9.1.1, Parsing process for Exp-Golomb codes
  //
  void
  Bitstream::write_bits_se(int64_t v)
  {
    uint64_t u = (v > 0) ? ((v - 1) << 1) : ((-v) << 1);
    write_bits_ue(u);
  }

}

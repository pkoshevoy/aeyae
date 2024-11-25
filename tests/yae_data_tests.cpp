// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 26 21:35:18 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_data.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/test/unit_test.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// shortcut:
using namespace yae;


struct FooFoo
{
  double param_;
  bool flag_;
  int one_;
  int two_;
};

BOOST_AUTO_TEST_CASE(yae_data)
{
  FooFoo foo[3] = { 0 };

  TBufferPtr ext_buffer(new ExtBuffer(&foo[0], sizeof(foo)));
  Data data(ext_buffer);

  BOOST_CHECK_EQUAL(3, data.num<FooFoo>());
  BOOST_CHECK_EQUAL(&foo[0], &data.get<FooFoo>(0));
  BOOST_CHECK_EQUAL(&foo[1], &data.get<FooFoo>(1));
  BOOST_CHECK_EQUAL(&foo[2], &data.get<FooFoo>(2));
}

BOOST_AUTO_TEST_CASE(yae_bitstream_read)
{
  unsigned char buffer[8] = {
    0x01, 0x23, 0x45, 0x67,
    0x89, 0xAB, 0xCD, 0xEF
  };

  TBufferPtr data(new ExtBuffer(buffer, sizeof(buffer)));
  Bitstream bits(data);

  BOOST_CHECK_EQUAL(0, bits.position());
  int b4 = bits.read(4);
  BOOST_CHECK_EQUAL(0x00, b4);

  BOOST_CHECK_EQUAL(4, bits.position());
  int b8 = bits.read(8);
  BOOST_CHECK_EQUAL(0x12, b8);

  BOOST_CHECK_EQUAL(12, bits.position());
  bits.skip(2);

  BOOST_CHECK_EQUAL(14, bits.position());
  int b3 = bits.read(3);
  BOOST_CHECK_EQUAL(6, b3);

  BOOST_CHECK_EQUAL(17, bits.position());
  int b16 = bits.read(16);
  BOOST_CHECK_EQUAL(35535, b16);

  BOOST_CHECK_EQUAL(33, bits.position());
  int b1 = bits.read(1);
  BOOST_CHECK_EQUAL(0, b1);

  BOOST_CHECK_EQUAL(34, bits.position());
  b1 = bits.read(1);
  BOOST_CHECK_EQUAL(0, b1);

  BOOST_CHECK_EQUAL(35, bits.position());
  b1 = bits.read(1);
  BOOST_CHECK_EQUAL(0, b1);

  BOOST_CHECK_EQUAL(36, bits.position());
  b1 = bits.read(1);
  BOOST_CHECK_EQUAL(1, b1);

  BOOST_CHECK_EQUAL(37, bits.position());
  b4 = bits.read(4);
  BOOST_CHECK_EQUAL(3, b4);

  BOOST_CHECK_EQUAL(41, bits.position());
  int b2 = bits.read(2);
  BOOST_CHECK_EQUAL(1, b2);

  BOOST_CHECK_EQUAL(43, bits.position());
  int b21 = bits.read(21);
  BOOST_CHECK_EQUAL(0x0BCDEF, b21);
  BOOST_CHECK_EQUAL(64, bits.position());
}

BOOST_AUTO_TEST_CASE(yae_bitstream_write)
{
  unsigned char b[8] = { 0 };

  TBufferPtr data(new ExtBuffer(b, sizeof(b)));
  Bitstream bits(data);

  BOOST_CHECK_EQUAL(0, bits.position());
  bits.write(4, 0x00);
  BOOST_CHECK_EQUAL(0x00, b[0]);

  BOOST_CHECK_EQUAL(4, bits.position());
  bits.write(8, 0x12);
  BOOST_CHECK_EQUAL(0x01, b[0]);
  BOOST_CHECK_EQUAL(0x20, b[1]);

  BOOST_CHECK_EQUAL(12, bits.position());
  bits.skip(2);

  BOOST_CHECK_EQUAL(14, bits.position());
  bits.write(3, 6);
  BOOST_CHECK_EQUAL(0x23, b[1]);

  BOOST_CHECK_EQUAL(17, bits.position());
  bits.write(16, (0x4567 << 1) + 1);
  BOOST_CHECK_EQUAL(0x45, b[2]);
  BOOST_CHECK_EQUAL(0x67, b[3]);
  BOOST_CHECK_EQUAL(0x80, b[4]);

  BOOST_CHECK_EQUAL(33, bits.position());
  bits.write(1, 0);
  BOOST_CHECK_EQUAL(0x80, b[4]);

  BOOST_CHECK_EQUAL(34, bits.position());
  bits.write(1, 0);
  BOOST_CHECK_EQUAL(0x80, b[4]);

  BOOST_CHECK_EQUAL(35, bits.position());
  bits.write(1, 0);
  BOOST_CHECK_EQUAL(0x80, b[4]);

  BOOST_CHECK_EQUAL(36, bits.position());
  bits.write(1, 1);
  BOOST_CHECK_EQUAL(0x88, b[4]);

  BOOST_CHECK_EQUAL(37, bits.position());
  bits.write(4, 3);
  BOOST_CHECK_EQUAL(0x89, b[4]);
  BOOST_CHECK_EQUAL(0x80, b[5]);

  BOOST_CHECK_EQUAL(41, bits.position());
  bits.write(2, 1);
  BOOST_CHECK_EQUAL(0xA0, b[5]);

  BOOST_CHECK_EQUAL(43, bits.position());
  bits.write(21, 0x0BCDEF);
  BOOST_CHECK_EQUAL(0xAB, b[5]);
  BOOST_CHECK_EQUAL(0xCD, b[6]);
  BOOST_CHECK_EQUAL(0xEF, b[7]);
  BOOST_CHECK_EQUAL(64, bits.position());
}

BOOST_AUTO_TEST_CASE(yae_bitstream_read_write_bytes)
{
  unsigned char b[8] = { 0 };

  TBufferPtr data(new ExtBuffer(b, sizeof(b)));
  Bitstream bits(data);

  BOOST_CHECK_EQUAL(0, bits.position());
  bits.write_bytes("Hello", 5);
  BOOST_CHECK_EQUAL('H', b[0]);

  BOOST_CHECK_EQUAL(40, bits.position());
  bits.seek(0);
  BOOST_CHECK_EQUAL(0, bits.position());
  std::string hello = Data(bits.read_bytes(5)).to_str();
  BOOST_CHECK_EQUAL("Hello", hello);

  bits.seek(0);
  bits.write(4, 0);
  bits.seek(4);
  bits.write_bytes("World", 5);
  BOOST_CHECK_EQUAL(44, bits.position());
  BOOST_CHECK_EQUAL('W' >> 4, b[0]);

  bits.seek(4);
  BOOST_CHECK_EQUAL(4, bits.position());
  std::string world = Data(bits.read_bytes(5)).to_str();
  BOOST_CHECK_EQUAL("World", world);
}

BOOST_AUTO_TEST_CASE(yae_bitstream_read_write_exp_golomb)
{
  unsigned char b[17] = { 0 };

  TBufferPtr data(new ExtBuffer(b, sizeof(b)));
  Bitstream bits(data);

  std::size_t p0 = bits.position();
  bits.write_bits_ue(123);

  std::size_t p1 = bits.position();
  bits.write_bits_se(-456);

  std::size_t p2 = bits.position();
  bits.write_bits_se(789);

  std::size_t p3 = bits.position();
  bits.seek(p0);
  BOOST_CHECK_EQUAL(123, bits.read_bits_ue());
  BOOST_CHECK_EQUAL(p1, bits.position());

  BOOST_CHECK_EQUAL(-456, bits.read_bits_se());
  BOOST_CHECK_EQUAL(p2, bits.position());

  BOOST_CHECK_EQUAL(789, bits.read_bits_se());
  BOOST_CHECK_EQUAL(p3, bits.position());
}


BOOST_AUTO_TEST_CASE(yae_bitstream_payload)
{
  NullBitstream null_bitstream;

  Bit<5> b5;
  b5 = 31;
  BOOST_CHECK_EQUAL(5, b5.size());

  NBit bv(3, 7);
  BOOST_CHECK_EQUAL(3, bv.size());

  b5.save(null_bitstream);
  BOOST_CHECK_EQUAL(5, null_bitstream.position());

  bv.save(null_bitstream);
  BOOST_CHECK_EQUAL(8, null_bitstream.position());
}

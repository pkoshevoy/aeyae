// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_bit_tree.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2003/01/21 17:11
// Copyright    : (C) 2003
// License      : MIT
// Description  : A structure for order O(log(n)) lookup of
//                arbitrary memory pointers.

// local includes:
#include "utils/the_bit_tree.hxx"

// system includes:
#include <iostream>
#include <iomanip>


//----------------------------------------------------------------
// the_bit_tree_node_t::node_size
// 
const unsigned int
the_bit_tree_node_t::node_size[] =
{
  0,   // 0 bits
  2,   // 1 bit
  4,   // 2 bits
  8,   // 3 bits
  16,  // 4 bits
  32,  // 5 bits
  64,  // 6 bits
  128, // 7 bits
  256  // 8 bits
};

//----------------------------------------------------------------
// the_bit_tree_node_t::node_mask
// 
const unsigned int
the_bit_tree_node_t::node_mask[] =
{
  0x00, // 0 bits
  0x01, // 1 bit
  0x03, // 2 bits
  0x07, // 3 bits
  0x0F, // 4 bits
  0x1F, // 5 bits
  0x3F, // 6 bits
  0x7F, // 7 bits
  0xFF  // 8 bits
};

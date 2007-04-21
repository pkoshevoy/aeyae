// File         : the_bit_tree.cxx
// Author       : Paul A. Koshevoy
// Created      : 
// Copyright    : (C) 2003
// License      : GPL.
// Description  : 

// local includes:
#include "utils/the_bit_tree.hxx"

// system includes:
#include <iostream>
#include <iomanip>


//----------------------------------------------------------------
// the_bit_tree_node_t::node_size
// 
const uint64_t
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
const uint64_t
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

/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


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

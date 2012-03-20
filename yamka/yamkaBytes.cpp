// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 17:24:08 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaBytes.h>

// system includes:
#include <iomanip>
#include <assert.h>


namespace Yamka
{
  //----------------------------------------------------------------
  // serialize
  // 
  std::ostream &
  serialize(std::ostream & os, const unsigned char * data, std::size_t size)
  {
    os << std::hex
       << std::uppercase;
    
    const unsigned char * end = data + size;
    for (const unsigned char * i = data; i < end; ++i)
    {
      const unsigned char & byte = *i;
      os << std::setw(2)
         << std::setfill('0')
         << int(byte);
    }
    
    os << std::dec;
    return os;
  }
  
  
  namespace Indent
  {
    
    //----------------------------------------------------------------
    // More::More
    // 
    More::More(unsigned int & indentation):
      indentation_(indentation)
    {
      ++indentation_;
    }
    
    //----------------------------------------------------------------
    // More::~More
    // 
    More::~More()
    {
      --indentation_;
    };

    //----------------------------------------------------------------
    // depth_
    // 
    unsigned int depth_ = 0;
  }
  
  
  //----------------------------------------------------------------
  // indent::indent
  // 
  indent::indent(unsigned int depth):
    depth_(Indent::depth_ + depth)
  {}
  
  
  //----------------------------------------------------------------
  // operator <<
  // 
  std::ostream &
  operator << (std::ostream & s, const indent & ind)
  {
    static const char * tab = "        \0";
    for (unsigned int i = 0; i < ind.depth_ / 8; i++)
    {
      s << tab;
    }
    
    const char * trailing_spaces = tab + (8 - ind.depth_ % 8);
    s << trailing_spaces;
    
    return s;
  }
  
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:06:08 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_CRC32_H_
#define YAMKA_CRC32_H_

// system includes:
#include <vector>


namespace Yamka
{

  //----------------------------------------------------------------
  // Crc32
  // 
  struct Crc32
  {
    Crc32();
    ~Crc32();
    
    // process data to compute the checksum:
    void compute(const void * bytes, std::size_t numBytes);
    
    // process data to compute the checksum:
    inline void compute(const std::vector<unsigned char> & bytes)
    {
      if (!bytes.empty())
      {
        compute(&bytes[0], bytes.size());
      }
    }
    
    // accessor to the current checksum:
    unsigned int checksum() const;
    
  private:
    // intentionally disabled:
    Crc32(const Crc32 &);
    Crc32 & operator = (const Crc32);
    
    // private implementation details:
    class Private;
    Private * private_;
  };
}


#endif // YAMKA_CRC32_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:31:46 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaIStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // IStorage::IReceipt::operator +=
  // 
  IStorage::IReceipt &
  IStorage::IReceipt::operator += (const IReceiptPtr & receipt)
  {
    if (!receipt)
    {
      return *this;
    }
    
    return add(receipt->numBytes());
  }
  
}

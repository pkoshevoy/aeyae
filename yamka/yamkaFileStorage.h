// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 12:48:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_FILE_STORAGE_H_
#define YAMKA_FILE_STORAGE_H_

// yamka includes:
#include <yamkaIStorage.h>
#include <yamkaFile.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // FileStorage
  // 
  struct FileStorage : public IStorage
  {
    FileStorage(const std::string & pathUTF8 = std::string(),
                File::AccessMode fileMode = File::READ_WRITE);
    
    // virtual:
    IReceiptPtr receipt() const;
    
    // virtual:
    IReceiptPtr save(const Bytes & data);
    IReceiptPtr load(Bytes & data);

    //----------------------------------------------------------------
    // Receipt
    // 
    struct Receipt : public IReceipt
    {
      Receipt(const File & file);
      
      // virtual:
      bool save(const Bytes & data);
      bool load(Bytes & data);
      
    protected:
      File file_;
      File::TOff addr_;
    };
    
    // file used to store data:
    File file_;
  };
  
}


#endif // YAMKA_FILE_STORAGE_H_

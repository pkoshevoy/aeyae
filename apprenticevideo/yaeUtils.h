// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:34:13 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_H_
#define YAE_UTILS_H_

// yae includes:
#include <yaeAPI.h>


namespace yae
{
  
  YAE_API int
  fileOpenUtf8(const char * filenameUTF8, int accessMode, int permissions);
  
  YAE_API int64_t
  fileSeek64(int fd, int64_t offset, int whence);
  
  YAE_API int64_t
  fileSize64(int fd);
  
  //----------------------------------------------------------------
  // TOpenHere
  // 
  // open an instance of TOpenable in the constructor,
  // close it in the destructor:
  // 
  template <typename TOpenable>
  struct TOpenHere
  {
    TOpenHere(TOpenable & something):
      something_(something)
    {
      something_.open();
    }
    
    ~TOpenHere()
    {
      something_.close();
    }
    
  protected:
    TOpenable & something_;
  };
  
}


#endif // YAE_UTILS_H_

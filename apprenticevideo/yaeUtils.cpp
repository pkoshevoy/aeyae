// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:37:50 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

//----------------------------------------------------------------
// __STDC_CONSTANT_MACROS
// 
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// system includes:
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

// yae includes:
#include <yaeAPI.h>
#include <yaeUtils.h>

namespace yae
{
  
  //----------------------------------------------------------------
  // fileOpenUtf8
  // 
  int
  fileOpenUtf8(const char * filenameUTF8, int accessMode, int permissions)
  {
#ifdef _WIN32
    accessMode |= O_BINARY;
    
    size_t wcsSize =
      MultiByteToWideChar(CP_UTF8, // source string encoding
                          0,       // flags (precomposed, composite, etc...)
                          filenameUTF8, // source string
                          -1,      // source string size
                          NULL,    // output string destination buffer
                          0);      // output string destination buffer size
    
    wchar_t * filenameUTF16 = (wchar_t *)malloc(sizeof(wchar_t) *
                                                (wcsSize + 1));
    MultiByteToWideChar(CP_UTF8,
                        0,
                        filenameUTF8,
                        -1,
                        filenameUTF16,
                        wcsSize);
    
    int fd = _wopen(filenameUTF16, accessMode, permissions);
    free(filenameUTF16);
    
#else
    
    int fd = open(filenameUTF8, accessMode, permissions);
#endif
    
    return fd;
  }
  
  //----------------------------------------------------------------
  // fileSeek64
  // 
  int64_t
  fileSeek64(int fd, int64_t offset, int whence)
  {
#ifdef _WIN32
    __int64 pos = _lseeki64(fd, offset, whence);
#elif defined(__APPLE__)
    off_t pos = lseek(fd, offset, whence);
#else
    off64_t pos = lseek64(fd, offset, whence);
#endif
    
    return pos;
  }
  
  //----------------------------------------------------------------
  // fileSize64
  // 
  int64_t
  fileSize64(int fd)
  {
#ifdef _WIN32
    struct _stati64 st;
    __int64 ret = _fstati64(fd, &st);
#else
    struct stat st;
    int ret = fstat(fd, &st);
#endif
    
    if (ret < 0)
    {
      return ret;
    }
    
    return st.st_size;
  }
  
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : texture_data.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 3 18:27:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a texture data helper class

// local includes:
#include "image/texture_data.hxx"
#include "utils/the_exception.hxx"

// system includes:
#include <sstream>

//----------------------------------------------------------------
// _1MB
// 
static const size_t _1MB = 1024 * 1024;

//----------------------------------------------------------------
// DEBUG_TEXTURE_DATA
// 
// #define DEBUG_TEXTURE_DATA

#ifdef DEBUG_TEXTURE_DATA
#include "utils/the_indentation.hxx"
#include <iostream>
#include <QMutex>

static QMutex mutex;
using std::cerr;
using std::endl;

static size_t total = 0;
#endif // DEBUG_TEXTURE_DATA

//----------------------------------------------------------------
// texture_data_t::texture_data_t
// 
texture_data_t::texture_data_t(size_t bytes)
{
  data_ = malloc(bytes);
  size_ = bytes;
  
  if (data_ == NULL)
  {
    std::ostringstream os;
    os << "could not allocate the texture data buffer of "
       << (size_ / _1MB) << "MB" << std::endl;
    the_exception_t e(os.str().c_str());
    throw e;
  }
  
#ifdef DEBUG_TEXTURE_DATA
  QMutexLocker locker(&mutex);
  total += size_;
  cerr << this << " -- malloced " << size_ / _1MB << "MB"
       << ", total allocated: " << total / _1MB << "MB" << endl;
#endif // DEBUG_TEXTURE_DATA
}

//----------------------------------------------------------------
// texture_data_t::~texture_data_t
// 
texture_data_t::~texture_data_t()
{
  free(data_);
  
#ifdef DEBUG_TEXTURE_DATA
  QMutexLocker locker(&mutex);
  total -= size_;
  cerr << this << " -- released " << size_ / _1MB << "MB"
       << ", total allocated: " << total / _1MB << "MB" << endl;
  // put_breakpoint_here();
#endif // DEBUG_TEXTURE_DATA
}

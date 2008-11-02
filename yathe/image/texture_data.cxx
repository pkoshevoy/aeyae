// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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

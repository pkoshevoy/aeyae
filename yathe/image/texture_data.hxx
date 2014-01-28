// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : texture_data.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 3 18:27:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a texture data helper class

#ifndef TEXTURE_DATA_HXX_
#define TEXTURE_DATA_HXX_

// system includes:
#include <stdlib.h>
#include <assert.h>


//----------------------------------------------------------------
// texture_data_t
//
class texture_data_t
{
public:
  texture_data_t(size_t bytes);
  ~texture_data_t();

  // size of this buffer (in bytes):
  inline const size_t & size() const
  { return size_; }

  // byte accessors:
  inline const unsigned char & operator[] (const size_t & i) const
  {
    assert(i < size_);
    return *(data() + i);
  }

  inline unsigned char & operator[] (const size_t & i)
  {
    assert(i < size_);
    return *(data() + i);
  }

  // buffer accessors:
  inline const unsigned char * data() const
  { return (const unsigned char *)(data_); }

  inline unsigned char * data()
  { return (unsigned char *)(data_); }

  inline operator const void * () const
  { return data_; }

private:
  texture_data_t(const texture_data_t &);
  texture_data_t & operator = (const texture_data_t &);

  void * data_;
  size_t size_;
};


//----------------------------------------------------------------
// const_texture_data_t
//
class const_texture_data_t
{
public:
  const_texture_data_t(const void * data):
    data_(data)
  {}

  inline operator const void * () const
  { return data_; }

  inline unsigned char * data()
  { return (unsigned char *)(data_); }

  const void * data_;
};


#endif // TEXTURE_DATA_HXX_

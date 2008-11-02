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


// File         : the_domain_array.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A data array that may be accessed with negative
//                indeces, provided they map into the
//                [lower boundary, upper boundary] range.

#ifndef THE_DOMAIN_ARRAY_HXX_
#define THE_DOMAIN_ARRAY_HXX_

// system includes:
#include <vector>
#include <assert.h>


//----------------------------------------------------------------
// the_domain_array_t
// 
// This is a one-dimmensional array with predefined boundaries.
// It may be helpful when implementing certain mathematical
// formulas where indicies are negative:
template<class T>
class the_domain_array_t
{
public:
  // default constructor:
  the_domain_array_t():
    lower_boundary_(1),
    upper_boundary_(0)
  {}
  
  // constructor with predefined array boundaries:
  the_domain_array_t(int lower_boundary, int upper_boundary):
    lower_boundary_(1),
    upper_boundary_(0)
  {
    set_domain(lower_boundary, upper_boundary);
  }
  
  // copy constructor:
  the_domain_array_t(const the_domain_array_t<T> & array):
    lower_boundary_(1),
    upper_boundary_(0)
  {
    *this = array;
  }
  
  // destructor:
  ~the_domain_array_t() {}
  
  // move the array boundaries:
  void set_domain(int lower_boundary, int upper_boundary)
  {
    // resize as necessary:
    unsigned int new_size = upper_boundary - lower_boundary + 1;
    
    if (new_size == 0)
    {
      array_.clear();
    }
    else if (size() != new_size)
    {
      array_.resize(new_size);
    }
    
    // move the boundaries:
    lower_boundary_ = lower_boundary;
    upper_boundary_ = upper_boundary;
  }
  
  // move the lower boundary of an array:
  void set_lower_boundary(int boundary)
  {
    int diff = upper_boundary_ - lower_boundary_;
    lower_boundary_ = boundary;
    upper_boundary_ = lower_boundary_ + diff;
  }
  
  // array boundary accessors:
  inline int lower_boundary() const { return lower_boundary_; }
  inline int upper_boundary() const { return upper_boundary_; }
  
  // the size of the array:
  inline unsigned int size() const
  { return (unsigned int)(upper_boundary_ - lower_boundary_ + 1); }
  
  // non-const array element accessor:
  inline T & operator [] (int i)
  {
    assert((i >= lower_boundary_) && (i <= upper_boundary_));
    return array_[zero_base_index(i)];
  }
  
  // const array element accessor:
  inline const T & operator [] (int i) const
  {
    assert((i >= lower_boundary_) && (i <= upper_boundary_));
    return array_[zero_base_index(i)];
  }
  
  // assignment operator:
  the_domain_array_t<T> &
  operator = (const the_domain_array_t<T> & array)
  {
    if (this == &array) return *this;
    
    set_domain(array.lower_boundary_, array.upper_boundary_);
    unsigned int domain_size = size();
    for (unsigned int i = 0; i < domain_size; i++)
    {
      array_[i] = array.array_[i];
    }
    
    return *this;
  }
  
  // the "add-to-this" operator:
  the_domain_array_t<T> &
  operator += (const the_domain_array_t<T> & array)
  {
    *this = the_domain_array_t<T>(*this) + array;
    return *this;
  }
  
  // addition operator:
  const the_domain_array_t<T>
  operator + (const the_domain_array_t<T> & array) const
  {
    the_domain_array_t<T> sum(*this);
    sum += array;
    return sum;
  }
  
  // dump this array to the standard out, prefix it with boundaries:
  ostream & dump(ostream & s) const
  {
    int domain_size = size();
    s << lower_boundary_ << ':' << upper_boundary_ << endl << '{' << endl;
    for (int i = 0; i < domain_size; i++)
    {
      s << ' ' << array_[i] << endl;
    }
    s << '}' << endl;
    return s;
  }
  
  // raw data accessor:
  inline std::vector<T> & data()
  { return array_; }
  
  inline const std::vector<T> & data() const
  { return array_; }
  
private:
  inline unsigned int zero_base_index(int i) const
  { return (unsigned int)(i - lower_boundary_); }
  
  std::vector<T> array_;
  int lower_boundary_;
  int upper_boundary_;
};


//----------------------------------------------------------------
// operator <<
//
template <class T>
ostream &
operator << (ostream & s, const the_domain_array_t<T> & array)
{
  return array.dump(s);
}


#endif // THE_DOMAIN_ARRAY_HXX_

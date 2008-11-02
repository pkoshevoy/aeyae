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


// File         : the_array.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A templated container class.

#ifndef THE_ARRAY_HXX_
#define THE_ARRAY_HXX_

// system includes:
#include <iostream>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

// forward declarations:
template<class T> class the_array_t;


//----------------------------------------------------------------
// the_array_ref_t
// 
template<class T>
class the_array_ref_t
{
public:
  the_array_ref_t(the_array_t<T> & array, const unsigned int & index = 0):
    array_(array),
    index_(index)
  {}
  
  inline the_array_ref_t<T> & operator << (const T & elem)
  {
    assert(index_ != array_.size());
    array_[index_++] = elem;
    return *this;
  }
  
private:
  // reference to the array:
  the_array_t<T> & array_;
  
  // current index into the array:
  unsigned int index_;
};


//----------------------------------------------------------------
// the_array_t
// 
template<class T>
class the_array_t
{
public:
  // default constructor:
  the_array_t():
    data_(NULL),
    size_(0)
  {}
  
  // constructor with a predefined array size:
  the_array_t(const unsigned int & init_size):
    data_(NULL),
    size_(0)
  { resize(init_size); }
  
  // constructor from a standard C-style array:
  the_array_t(const T * data, const unsigned int & size):
    data_(NULL),
    size_(0)
  {
    resize(size);
    for (unsigned int i = 0; i < size; i++)
    {
      data_[i] = data[i];
    }
  }
  
  // copy constructor:
  the_array_t(const the_array_t<T> & a):
    data_(NULL),
    size_(0)
  { *this = a; }
  
  // destructor:
  ~the_array_t()
  {
    delete [] data_;
    data_ = NULL;
  }
  
  // resize the array (all contents will be destroyed):
  void resize(const unsigned int & new_size)
  {
    if (new_size == size_) return;
    
    delete [] data_;
    size_ = new_size;
    data_ = new T [size_];
  }
  
  // assignment operator:
  the_array_t<T> & operator = (const the_array_t<T> & array)
  {
    if (&array == this) return *this;
    
    delete [] data_;
    size_ = array.size_;
    data_ = new T [size_];
    for (unsigned int i = 0; i < size_; i++)
    {
      data_[i] = array.data_[i];
    }
    
    return *this;
  }
  
  // this is usefull for filling-in the array:
  the_array_ref_t<T> operator << (const T & elem)
  {
    data_[0] = elem;
    return the_array_ref_t<T>(*this, 1);
  }
  
  // addition operator:
  the_array_t<T> operator + (const the_array_t<T> & a) const
  {
    the_array_t<T> result(*this);
    result += a;
    return result;
  }
  
  // add-to-this operator:
  the_array_t<T> & operator += (const the_array_t<T> & a)
  {
    unsigned int new_size = size_ + a.size_;
    T * new_data = new T [new_size];
    
    for (unsigned int i = 0; i < size_; i++)
    {
      new_data[i] = data_[i];
    }
    
    for (unsigned int j = 0; j < a.size_; j++)
    {
      new_data[j + size_] = a.data_[j];
    }
    
    delete [] data_;
    data_ = new_data;
    size_ = new_size;
    
    return *this;
  }
  
  // non-const accessors:
  inline T & elem(const unsigned int & i)
  {
    assert(i < size_);
    return data_[i];
  }
  
  inline T & operator [] (const unsigned int & i)
  { return elem(i); }
  
  // const accessors:
  inline const T & elem(const unsigned int & i) const
  {
    assert(i < size_);
    return data_[i];
  }
  
  inline const T & operator [] (const unsigned int & i) const
  { return elem(i); }
  
  // the size of this array:
  inline const unsigned int & size() const
  { return size_; }
  
  // return either first or last index into the array:
  inline unsigned int end_index(bool last) const
  {
    if (last == false) return 0;
    return size_ - 1;
  }
  
  // return either first or last element in the array:
  inline const T & end_elem(bool last) const
  { return elem(end_index(last)); }
  
  inline T & end_elem(bool last)
  { return elem(end_index(last)); }
  
  // return the index of the first occurrence of a given element in the array:
  unsigned int index_of(const T & element) const
  {
    for (unsigned int i = 0; i < size_; i++)
    {
      if (!(elem(i) == element)) continue;
      return i;
    }
    
    return UINT_MAX;
  }
  
  // check whether this array contains a given element:
  inline bool has(const T & element) const
  { return index_of(element) != UINT_MAX; }
  
  // for debugging, dumps this list into a stream:
  void dump(std::ostream & strm) const
  {
    strm << "the_array_t(" << (void *)this << ") {\n";
    for (unsigned int i = 0; i < size_; i++)
    {
      strm << data_[i] << std::endl;
    }
    strm << '}';
  }
  
  // raw data accessors:
  inline T * data()
  { return const_cast<T *>(data_); }
  
  inline const T * data() const
  { return const_cast<T *>(data_); }
  
  // make room for a new element to be inserted into the array
  // at a specified index, grow the array:
  the_array_t<T> & make_room(const unsigned int & insertion_index)
  {
    unsigned int new_size = size_ + 1;
    T * new_data = new T [new_size];
    
    for (unsigned int i = 0; i < insertion_index; i++)
    {
      new_data[i] = data_[i];
    }
    
    for (unsigned int j = insertion_index; j < size_; j++)
    {
      new_data[j + 1] = data_[j];
    }
    
    delete [] data_;
    data_ = new_data;
    size_ = new_size;
    
    return *this;
  }
  
  // insert a new element into the array at a specified index, grow the array:
  the_array_t<T> & insert(const unsigned int & insertion_index, const T & elem)
  {
    make_room(insertion_index);
    data_[insertion_index] = elem;
    return *this;
  }
  
  // remove an element from the array at a specified index, shrink the array:
  the_array_t<T> & remove(const unsigned int & removal_index, T & elem)
  {
    if (removal_index >= size_) return *this;
    
    unsigned int new_size = size_ - 1;
    T * new_data = new T [new_size];
    
    for (unsigned int i = 0; i < removal_index; i++)
    {
      new_data[i] = data_[i];
    }
    
    elem = data_[removal_index];
    
    for (unsigned int j = removal_index + 1; j < size_; j++)
    {
      new_data[j - 1] = data_[j];
    }
    
    delete [] data_;
    data_ = new_data;
    size_ = new_size;
    
    return *this;
  }
  
  // shrink the array by removing an element at a specified index:
  inline the_array_t<T> & remove(const unsigned int & removal_index)
  {
    T elem;
    return remove(removal_index, elem);
  }
  
private:  
  // this is counterintuitive, because it changes the size of the array:
  the_array_t<T> & operator = (unsigned int size);
  
protected:
  T * data_;
  unsigned int size_;
};

//----------------------------------------------------------------
// operator +
// 
// expand an array by appending a new element on the right:
// 
template <class T>
extern the_array_t<T>
operator + (const the_array_t<T> & a, const T & b)
{
  the_array_t<T> c(1);
  c[0] = b;
  return a + c;
}

//----------------------------------------------------------------
// operator +
// 
// expand an array by appending a new element on the left:
// 
template <class T>
extern the_array_t<T>
operator + (const T & a, const the_array_t<T> & b)
{
  the_array_t<T> c(1);
  c[0] = a;
  return c + b;
}

//----------------------------------------------------------------
// operator <<
// 
template <class T>
std::ostream &
operator << (std::ostream & s, const the_array_t<T> & a)
{
  a.dump(s);
  return s;
}

//----------------------------------------------------------------
// resize
// 
template <class T>
void
resize(the_array_t< the_array_t<T> > & a,
       const unsigned int & rows,
       const unsigned int & cols)
{
  a.resize(rows);
  for (unsigned int i = 0; i < rows; i++)
  {
    a[i].resize(cols);
  }
}


#endif // THE_ARRAY_HXX_

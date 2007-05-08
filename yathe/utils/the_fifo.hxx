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


// File         : the_fifo.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sat Oct 30 16:20:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A fixed size first-in first-out queue.

#ifndef THE_FIFO_HXX_
#define THE_FIFO_HXX_

// system includes:
#include <vector>
#include <assert.h>


//----------------------------------------------------------------
// the_fifo_t
// 
template <class data_t>
class the_fifo_t
{
public:
  the_fifo_t(const unsigned int & size = 0):
    data_(size),
    head_(0),
    size_(0)
  {}
  
  // accessors to the element last added to the fifo (last in):
  inline const data_t & head() const
  { return data_[head_]; }
  
  inline data_t & head()
  { return data_[head_]; }
  
  // accessors to the element first added to the fifo (first in):
  inline const data_t & tail() const
  {
    const unsigned int & cap = capacity();
    return data_[(head_ + size_ - 1) % cap];
  }
  
  inline data_t & tail()
  {
    const unsigned int & cap = capacity();
    return data_[(head_ + size_ - 1) % cap];
  }
  
  // accessors to the fifo queue elements (0 is most recently added):
  inline const data_t & operator [] (const unsigned int & i) const
  {
    const unsigned int & cap = capacity();
    assert(i < size_);
    return data_[(head_ + i) % cap];
  }
  
  inline data_t & operator [] (const unsigned int & i)
  {
    const unsigned int & cap = capacity();
    assert(i < size_);
    return data_[(head_ + i) % cap];
  }
  
  // shift the fifo queue by one (most recent becomes 2nd most recent):
  inline void shift()
  {
    const unsigned int & cap = capacity();
    head_ = (head_ + (cap - 1)) % cap;
    size_ = (size_ + 1) - (size_ + 1) / (cap + 1);
  }
  
  // push a new element into the queue:
  inline the_fifo_t<data_t> & operator << (const data_t & d)
  {
    shift();
    data_[head_] = d;
    return *this;
  }
  
  // raw data accessor:
  inline const std::vector<data_t> & data() const
  { return data_; }
  
  // accessor to the current number of elements in the fifo queue:
  inline const unsigned int & size() const
  { return size_; }
  
  // the maximum number of elements this fifo queue can store:
  inline const unsigned int capacity() const
  { return data_.size(); }
  
  // for debugging, dumps this list into a stream:
  void dump(std::ostream & strm, unsigned int indent = 0) const
  {
    strm << "the_fifo_t<data_t>(" << (void *)this << ") {\n"
	 << "head_ = " << head_ << std::endl
	 << "size_ = " << size_ << std::endl
	 << "data_ = " << std::endl
	 << data_ << std::endl
	 << '}' << std::endl;
  }
  
private:
  // the fifo elements:
  std::vector<data_t> data_;
  
  // index of the most recently added queue element (last in):
  unsigned int head_;
  
  // at most this will be equal to the maximum capacity of the queue:
  unsigned int size_;
};

//----------------------------------------------------------------
// operator <<
// 
template <class data_t>
std::ostream &
operator << (std::ostream & s, const the_fifo_t<data_t> & fifo)
{
  fifo.dump(s);
  return s;
}


#endif // THE_FIFO_HXX_

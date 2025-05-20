// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Nov 29 22:28:10 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FIFO_H_
#define YAE_FIFO_H_

// standard:
#include <list>


namespace yae
{

  //----------------------------------------------------------------
  // fifo
  //
  // A first-in-first-out queue -- once size reaches maximum capacity
  // adding a new item drops the oldest item.
  //
  template <typename TData>
  struct fifo
  {
    typedef TData value_type;

    fifo(std::size_t capacity):
      capacity_(capacity),
      size_(0)
    {}

    inline void clear()
    {
      size_ = 0;
      data_.clear();
    }

    inline void push(const TData & v)
    {
      if (size_ < capacity_)
      {
        size_++;
      }
      else
      {
        data_.pop_front();
      }

      data_.push_back(v);
    }

    inline bool pop(TData & v)
    {
      if (data_.empty())
      {
        return false;
      }

      v = data_.front();
      data_.pop_front();
      size_--;
      return true;
    }

    inline TData pop()
    {
      YAE_ASSERT(!data_.empty());
      TData v = data_.front();
      data_.pop_front();
      size_--;
      return v;
    }

    inline bool empty() const
    { return data_.empty(); }

    inline bool full() const
    { return size_ == capacity_; }

    inline std::size_t capacity() const
    { return capacity_; }

    inline void set_capacity(std::size_t capacity)
    { capacity_ = capacity; }

    inline std::size_t size() const
    { return size_; }

    inline const TData & front() const
    { return data_.front(); }

    inline const TData & back() const
    { return data_.back(); }

    inline bool has(const TData & data) const
    { return yae::has(data_, data); }

  protected:
    std::list<TData> data_;
    std::size_t capacity_;
    std::size_t size_;
  };

}


#endif // YAE_FIFO_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Nov 24 11:08:19 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae:
#include "yae/thread/yae_ring_buffer.h"


namespace yae
{

  //----------------------------------------------------------------
  // RingBuffer::RingBuffer
  //
  RingBuffer::RingBuffer(std::size_t capacity):
    size_(0),
    head_(0),
    tail_(0)
  {
    open(capacity);
  }

  //----------------------------------------------------------------
  // RingBuffer::~RingBuffer
  //
  RingBuffer::~RingBuffer()
  {
    close();
  }

  //----------------------------------------------------------------
  // RingBuffer::open
  //
  void
  RingBuffer::open(std::size_t capacity)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (capacity != data_.size())
    {
      head_ = 0;
      tail_ = 0;
      size_ = 0;
      data_.resize(capacity);
    }

    open_ = data_.size() > 0;
    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // RingBuffer::close
  //
  void
  RingBuffer::close()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    open_ = false;
    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // RingBuffer::is_open
  //
  bool
  RingBuffer::is_open() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return open_;
  }

  //----------------------------------------------------------------
  // RingBuffer::push
  //
  std::size_t
  RingBuffer::push(const void * data, std::size_t data_size)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    const unsigned char * src = static_cast<const unsigned char *>(data);
    const unsigned char * end = src + data_size;
    const std::size_t capacity = data_.size();

    std::size_t done = 0;
    while (open_ && src < end)
    {
      std::size_t todo = (end - src);
      std::size_t size = 0;

      if (tail_ < head_)
      {
        size = std::min(head_ - tail_, todo);
      }
      else if (size_ < capacity)
      {
        size = std::min(capacity - tail_, todo);
      }

      if (size)
      {
        memcpy(&(data_[tail_]), src, size);
        tail_ = (tail_ + size) % capacity;
        src += size;
        done += size;
        size_ += size;
        cond_.notify_all();
      }
      else
      {
        cond_.wait(lock);
      }
    }

    return done;
  }

  //----------------------------------------------------------------
  // RingBuffer::pull
  //
  std::size_t
  RingBuffer::pull(void * data, std::size_t data_size)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    unsigned char * dst = static_cast<unsigned char *>(data);
    unsigned char * end = dst + data_size;
    const std::size_t capacity = data_.size();

    std::size_t done = 0;
    while (dst < end)
    {
      std::size_t todo = (end - dst);
      std::size_t size = 0;

      if (head_ < tail_)
      {
        size = std::min(tail_ - head_, todo);
      }
      else if (size_)
      {
        size = std::min(capacity - head_, todo);
      }

      if (size)
      {
        YAE_ASSERT(size <= size_);
        memcpy(dst, &(data_[head_]), size);
        head_ = (head_ + size) % capacity;
        dst += size;
        done += size;
        size_ -= size;
        cond_.notify_all();
      }
      else if (open_)
      {
        cond_.wait(lock);
      }
      else
      {
        break;
      }
    }

    return done;
  }

}

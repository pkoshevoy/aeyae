// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Nov 24 11:08:19 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_RING_BUFFER_H_
#define YAE_RING_BUFFER_H_

// standard:
#include <vector>

// boost:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// yae:
#include "../utils/yae_data.h"


namespace yae
{

  //----------------------------------------------------------------
  // RingBuffer
  //
  struct YAE_API RingBuffer
  {
    RingBuffer(std::size_t capacity);
    ~RingBuffer();

    void open(std::size_t capacity = 0);
    void close();
    bool is_open() const;

    std::size_t push(const void * data, std::size_t size);
    std::size_t pull(void * data, std::size_t size);

    double occupancy() const;

  private:
    // intentionally disabled:
    RingBuffer(const RingBuffer &);
    RingBuffer & operator = (const RingBuffer &);

  protected:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;

    std::vector<unsigned char> data_;
    std::size_t size_;
    std::size_t head_;
    std::size_t tail_;
    bool open_;
  };

}


#endif // YAE_RING_BUFFER_H_

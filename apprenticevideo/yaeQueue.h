// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:32:01 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_QUEUE_H_
#define YAE_QUEUE_H_

// system includes:
#include <set>
#include <list>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // Queue
  // 
  // Thread-safe queue
  // 
  template <typename TData>
  struct Queue
  {
    typedef Queue<TData> TSelf;
    typedef bool(*TSortFunc)(const TData &, const TData &);
    typedef std::list<TData> TSequence;
    
    Queue(std::size_t maxSize = 50):
      closed_(true),
      consumerIsBlocked_(false),
      size_(0),
      maxSize_(maxSize),
      sortFunc_(0)
    {}
    
    ~Queue()
    {
      close();
      
      try
      {
        boost::this_thread::disable_interruption disableInterruption;
        boost::lock_guard<boost::mutex> lock(mutex_);

        while (!sequences_.empty())
        {
          TSequence & sequence = sequences_.front();
          while (!sequence.empty())
          {
            sequence.pop_front();
            size_--;
          }
          sequences_.pop_front();
        }
        
        // sanity check:
        YAE_ASSERT(size_ == 0);
      }
      catch (...)
      {}
    }

    void setSortFunc(TSortFunc sortFunc)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      sortFunc_ = sortFunc;
    }
    
    void setMaxSize(std::size_t maxSize)
    {
      // change max queue size:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (maxSize_ == maxSize)
        {
          // same size, nothing changed:
          return;
        }
        
        maxSize_ = maxSize;
      }
      
      cond_.notify_all();
    }
    
    std::size_t getMaxSize() const
    {
      // get max queue size:
      boost::lock_guard<boost::mutex> lock(mutex_);
      return maxSize_;
    }
    
    // check whether the Queue is empty:
    bool isEmpty() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return !size_;
    }
    
    // check whether the queue is closed:
    bool isClosed() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return closed_;
    }
    
    // close the queue, abort any pending push/pop/etc... calls:
    void close()
    {
      // close the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (closed_)
        {
          // already closed:
          return;
        }
        
#if 0 // ndef NDEBUG
        std::cerr << this << " close " << std::endl;
#endif
        closed_ = true;
      }
      
      cond_.notify_all();
    }
    
    // open the queue allowing push/pop/etc... calls:
    void open()
    {
      // open the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (!closed_)
        {
          // already open:
          return;
        }
        
#if 0 // ndef NDEBUG
        std::cerr << this << " open " << std::endl;
#endif
        sequences_.clear();
        size_ = 0;
        closed_ = false;
      }
      
      cond_.notify_all();
    }
    
    // remove all data from the queue:
    bool clear()
    {
      try
      {
        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          sequences_.clear();
          size_ = 0;
        }
        
        cond_.notify_all();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }
    
    // push data into the queue:
    bool push(const TData & newData)
    {
      try
      {
        // add to queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && size_ >= maxSize_)
          {
#if 0 // ndef NDEBUG
            std::cerr << this << " push wait, size " << size_ << std::endl;
#endif
            cond_.wait(lock);
          }
          
          if (size_ >= maxSize_)
          {
            return false;
          }

          insert(newData);
          size_++;
          
#if 0 // ndef NDEBUG
          std::cerr << this << " push done" << size_ << std::endl;
#endif
        }
        
        cond_.notify_all();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }

    // remove data from the queue:
    bool pop(TData & data)
    {
      try
      {
        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && !size_)
          {
#if 0 // ndef NDEBUG
            std::cerr << this << " pop wait, size " << size_ << std::endl;
#endif
            consumerIsBlocked_ = true;
            cond_.notify_all();
            cond_.wait(lock);
          }
          
          consumerIsBlocked_ = false;
          if (!size_)
          {
#if 0 // ndef NDEBUG
            std::cerr << this << " queue is empty, closed: "
                      << closed_ << std::endl;
#endif
            return false;
          }

          while (sequences_.front().empty())
          {
            sequences_.pop_front();
          }
          
          TSequence & sequence = sequences_.front();
          data = sequence.back();
          sequence.pop_back();
          size_--;
          
          if (sequence.empty())
          {
            sequences_.pop_front();
          }
          
#if 0 // ndef NDEBUG
          std::cerr << this << " pop done, size " << std::endl;
#endif
        }
        
        cond_.notify_all();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }
    
    bool waitForConsumerToBlock()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      while (!closed_ && !consumerIsBlocked_)
      {
        cond_.wait(lock);
      }
      
      return consumerIsBlocked_;
    }

    void startNewSequence(const TData & sequenceEndData)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (sequences_.empty())
      {
        sequences_.push_back(TSequence());
      }
      
      TSequence & sequence = sequences_.back();
      sequence.push_front(sequenceEndData);
      size_++;
      sequences_.push_back(TSequence());
      cond_.notify_all();
    }

  protected:
    
    // push data into the queue:
    void insert(const TData & newData)
    {
      if (sequences_.empty())
      {
        sequences_.push_back(TSequence());
      }
      
      TSequence & sequence = sequences_.back();
      if (sequence.empty() || !sortFunc_)
      {
        sequence.push_front(newData);
        return;
      }

      // keep the queue sorted:
      for (typename std::list<TData>::iterator i = sequence.begin();
           i != sequence.end(); ++i)
      {
        if (sortFunc_(newData, *i))
        {
          sequence.insert(i, newData);
          return;
        }
      }
      
      sequence.push_back(newData);
    }
    
    bool closed_;
    bool consumerIsBlocked_;
    std::list<TSequence> sequences_;
    std::size_t size_;
    std::size_t maxSize_;
    TSortFunc sortFunc_;

  public:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;
  };
  
}


#endif // YAE_QUEUE_H_

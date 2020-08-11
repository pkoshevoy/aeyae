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
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// yae includes:
#include "../video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // QueueWaitMgr
  //
  struct QueueWaitMgr
  {
    QueueWaitMgr():
      cond_(NULL),
      wait_(true)
    {}

    void waitingFor(boost::condition_variable * cond)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      cond_ = cond;
    }

    void stopWaiting(bool stop = true)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      wait_ = !stop;

      if (cond_)
      {
        cond_->notify_all();
      }
    }

    inline bool keepWaiting() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return wait_;
    }

  protected:
    mutable boost::mutex mutex_;
    boost::condition_variable * cond_;
    bool wait_;
  };

  //----------------------------------------------------------------
  // QueueWaitTerminator
  //
  struct QueueWaitTerminator
  {
    QueueWaitTerminator(QueueWaitMgr * waitMgr,
                        boost::condition_variable * cond):
      waitMgr_(waitMgr)
    {
      if (waitMgr_)
      {
        waitMgr_->waitingFor(cond);
      }
    }

    ~QueueWaitTerminator()
    {
      if (waitMgr_)
      {
        waitMgr_->waitingFor(NULL);
      }
    }

    inline bool keepWaiting() const
    {
      return waitMgr_ ? waitMgr_->keepWaiting() : true;
    }

  private:
    QueueWaitMgr * waitMgr_;
  };


  //----------------------------------------------------------------
  // TemporaryValueOverride
  //
  template <typename TValue>
  struct TemporaryValueOverride
  {
    TemporaryValueOverride(TValue & value, const TValue & temporaryValue):
      restore_(value),
      value_(value)
    {
      value_ = temporaryValue;
    }

    ~TemporaryValueOverride()
    {
      value_ = restore_;
    }

  private:
    TemporaryValueOverride(const TemporaryValueOverride &);
    TemporaryValueOverride & operator = (const TemporaryValueOverride &);

    const TValue restore_;
    TValue & value_;
  };


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

    Queue(std::size_t maxSize = 1):
      closed_(true),
      producerIsBlocked_(false),
      consumerIsBlocked_(false),
      size_(0),
      maxSize_(maxSize),
      sortFunc_(0)
    {}

    ~Queue()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      sequences_.clear();
      size_ = 0;
      closed_ = true;
    }

    void setSortFunc(TSortFunc sortFunc)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      sortFunc_ = sortFunc;
    }

    void setMaxSizeUnlimited()
    {
      // change max queue size:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        maxSize_ = std::numeric_limits<std::size_t>::max();
      }

      cond_.notify_all();
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
    bool close()
    {
      // close the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (closed_)
        {
          // already closed:
          return true;
        }

#if 0 // ndef NDEBUG
        yae_debug << this << " close";
#endif
        closed_ = true;
      }

      cond_.notify_all();
      return true;
    }

    // open the queue allowing push/pop/etc... calls:
    bool open()
    {
      // open the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (!closed_)
        {
          // already open:
          return true;
        }

#if 0 // ndef NDEBUG
        yae_debug << this << " open";
#endif
        sequences_.clear();
        size_ = 0;
        closed_ = false;
      }

      cond_.notify_all();
      return true;
    }

    // remove all data from the queue:
    bool clear()
    {
      try
      {
        // remove from queue:
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
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
    bool push(const TData & newData, QueueWaitMgr * waitMgr = NULL)
    {
      try
      {
        QueueWaitTerminator terminator(waitMgr, &cond_);

        // add to queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && size_ >= maxSize_ && terminator.keepWaiting())
          {
#if 0 // ndef NDEBUG
            yae_debug << this << " push wait, size " << size_;
#endif
            TemporaryValueOverride<bool> temp(producerIsBlocked_, true);
            cond_.wait(lock);
          }

          if (size_ >= maxSize_)
          {
            return false;
          }

          insert(newData);
          size_++;

#if 0 // ndef NDEBUG
          yae_debug << this << " push done" << size_;
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
    bool pop(TData & data,
             QueueWaitMgr * waitMgr = NULL,
             bool waitForData = true)
    {
      try
      {
        QueueWaitTerminator terminator(waitMgr, &cond_);

        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && !size_ && waitForData && terminator.keepWaiting())
          {
#if 0 // ndef NDEBUG
            yae_debug << this << " pop wait, size " << size_;
#endif
            TemporaryValueOverride<bool> temp(consumerIsBlocked_, true);
            cond_.notify_all();
            cond_.wait(lock);
          }

          if (!size_)
          {
#if 0 // ndef NDEBUG
            yae_debug << this << " queue is empty, closed: " << closed_;
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
          yae_debug << this << " pop done, size";
#endif
        }

        cond_.notify_all();
        return true;
      }
      catch (...)
      {}

      return false;
    }

    // remove items from the queue if they satisfy predicate conditions:
    template <typename TPredicate>
    bool get(const TPredicate & predicate,
             std::list<TData> & found,
             QueueWaitMgr * waitMgr = NULL)
    {
      try
      {
        QueueWaitTerminator terminator(waitMgr, &cond_);

        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          for (typename std::list<TSequence>::iterator
                 i = sequences_.begin(); i != sequences_.end(); )
          {
            TSequence & sequence = *i;

            for (typename std::list<TData>::iterator j = sequence.begin();
                 j != sequence.end(); )
            {
              const TData & data = *j;
              if (predicate(data))
              {
                found.push_back(data);
                j = sequence.erase(j);
                size_--;
              }
              else
              {
                ++j;
              }
            }

            if (sequence.empty())
            {
              i = sequences_.erase(i);
            }
            else
            {
              ++i;
            }
          }
        }

        cond_.notify_all();
        return true;
      }
      catch (...)
      {}

      return false;
    }

    // take a peek at the top of the queue:
    bool peek(TData & data, QueueWaitMgr * waitMgr = NULL)
    {
      try
      {
        QueueWaitTerminator terminator(waitMgr, &cond_);

        // peek at the queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          if (closed_ || !size_)
          {
            return false;
          }

          for (typename std::list<TSequence>::const_iterator
                 i = sequences_.begin(); i != sequences_.end(); ++i)
          {
            const TSequence & sequence = *i;
            if (sequence.empty())
            {
              continue;
            }

            data = sequence.back();
          }
        }

        return true;
      }
      catch (...)
      {}

      return false;
    }

    bool waitIndefinitelyForConsumerToBlock(QueueWaitMgr * waitMgr = NULL)
    {
      QueueWaitTerminator terminator(waitMgr, &cond_);

      boost::unique_lock<boost::mutex> lock(mutex_);
      while (!closed_ && !(consumerIsBlocked_ && !size_) &&
             terminator.keepWaiting())
      {
        cond_.wait(lock);
      }

      return consumerIsBlocked_;
    }

    bool waitForConsumerToBlock(double secToWait)
    {
      boost::system_time whenToGiveUp(boost::get_system_time());
      whenToGiveUp += boost::posix_time::microseconds(long(secToWait * 1e+6));

      boost::unique_lock<boost::mutex> lock(mutex_);
      while (!closed_ && !(consumerIsBlocked_ && !size_))
      {
        if (!cond_.timed_wait(lock, whenToGiveUp))
        {
          boost::system_time now(boost::get_system_time());
          if (whenToGiveUp <= now)
          {
            break;
          }
        }
      }

      return consumerIsBlocked_ || closed_;
    }

    bool producerIsBlocked() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return producerIsBlocked_;
    }

    bool consumerIsBlocked() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return consumerIsBlocked_;
    }

    void startNewSequence(const TData & sequenceEndData)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
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
    bool producerIsBlocked_;
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

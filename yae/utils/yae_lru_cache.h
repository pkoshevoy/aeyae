// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan 14 11:05:30 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LRU_CACHE_H_
#define YAE_LRU_CACHE_H_

// standard:
#include <map>

// boost:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // LRUCache
  //
  // a thread safe cache
  //
  template <typename TKey, typename TValue>
  struct LRUCache
  {

    //----------------------------------------------------------------
    // TCache
    //
    typedef LRUCache<TKey, TValue> TCache;

    //----------------------------------------------------------------
    // LRUCache
    //
    LRUCache():
      capacity_(0),
      referenced_(0),
      unreferenced_(0)
    {}

    //----------------------------------------------------------------
    // capacity
    //
    inline std::size_t capacity() const
    {
      return capacity_;
    }

    //----------------------------------------------------------------
    // set_capacity
    //
    void
    set_capacity(std::size_t capacity)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      capacity_ = capacity;

      while (capacity_ < referenced_ + unreferenced_)
      {
        purge_one(lock);
      }
    }

    //----------------------------------------------------------------
    // Ref
    //
    struct Ref
    {
      friend TCache;

      ~Ref()
      {
        cache_.release(key_, value_);
      }

      inline const TKey & key() const
      { return key_; }

      inline const TValue & value() const
      { return value_; }

    private:
      Ref(TCache & cache, const TKey & k, const TValue & v):
        cache_(cache),
        key_(k),
        value_(v)
      {}

      // intentionally disabled:
      Ref(const Ref &);
      Ref & operator = (const Ref &);

      TCache & cache_;
      TKey key_;
      TValue value_;
    };

    //----------------------------------------------------------------
    // TRefPtr
    //
    typedef boost::shared_ptr<Ref> TRefPtr;

    //----------------------------------------------------------------
    // Cache
    //
    struct Cache
    {
      std::list<TValue> referenced_;
      std::list<TValue> unreferenced_;
    };

    //----------------------------------------------------------------
    // TFactoryCallback
    //
    typedef bool(*TFactoryCallback)(void *, const TKey &, TValue &);

    // Return an existing unreferenced cached value, or create
    // a new cached value on demand via the supplied callback:
    //
    // NOTE: if the cache is full this will wait for an
    // existing referenced instance to become unrefernced:
    //
    TRefPtr
    get(const TKey & key, TFactoryCallback assign = NULL, void * ctx = NULL)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      while (referenced_ >= capacity_)
      {
        cond_.wait(lock);
      }

      typename std::map<TKey, Cache>::iterator found = cache_.find(key);
      if (found != cache_.end())
      {
        Cache & cache = found->second;
        if (!cache.unreferenced_.empty())
        {
          TValue value = cache.unreferenced_.front();
          yae::remove_one(lru_, key);

          cache.referenced_.push_back(value);
          referenced_++;

          cache.unreferenced_.pop_front();
          unreferenced_--;

          return TCache::TRefPtr(new TCache::Ref(*this, key, value));
        }
      }

      if (assign)
      {
        // make sure there is room for a new cached value instance:
        if (capacity_ <= referenced_ + unreferenced_)
        {
          purge_one(lock);
        }

        // add a placeholder and call the callback to give it value:
        cache_[key].referenced_.push_back(TValue());
        TValue & value = cache_[key].referenced_.back();

        try
        {
          if (assign(ctx, key, value))
          {
            TCache::TRefPtr ref(new TCache::Ref(*this, key, value));
            referenced_++;
            return ref;
          }
        }
        catch (...)
        {
          // assignment failed:
          cache_[key].referenced_.pop_back();
          throw;
        }

        // assignment failed:
        cache_[key].referenced_.pop_back();
      }

      return TCache::TRefPtr();
    }

    //----------------------------------------------------------------
    // purge_one
    //
    void
    purge_one(boost::unique_lock<boost::mutex> & lock)
    {
      // must purge an unused instance:
      while (!unreferenced_)
      {
        cond_.wait(lock);
      }

      for (typename std::list<TKey>::iterator
             i = lru_.begin(); i != lru_.end(); ++i)
      {
        const TKey & key = *i;
        Cache & cache = cache_[key];

        if (!cache.unreferenced_.empty())
        {
          cache.unreferenced_.pop_front();
          unreferenced_--;
          lru_.erase(i);
          break;
        }
      }
    }

  protected:
    friend TCache::Ref;

    //----------------------------------------------------------------
    // release
    //
    void
    release(const TKey & key, const TValue & value)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);

      Cache & cache = cache_[key];
      typename std::list<TValue>::iterator found =
        std::find(cache.referenced_.begin(), cache.referenced_.end(), value);
      YAE_ASSERT(found != cache.referenced_.end());

      if (found != cache.referenced_.end())
      {
        lru_.push_back(key);

        cache.unreferenced_.push_back(value);
        unreferenced_++;

        cache.referenced_.erase(found);
        referenced_--;

        cond_.notify_all();
      }
    }

    // intentionally disabled:
    LRUCache(const LRUCache &);
    LRUCache & operator = (const LRUCache &);

    mutable boost::mutex mutex_;
    boost::condition_variable cond_;

    // cache capacity (hard limit):
    std::size_t capacity_;

    // number of cached values that are externally referenced:
    std::size_t referenced_;

    // number of cached instances that are unreferenced
    // and can be reused or replaced:
    std::size_t unreferenced_;

    // referenced and unreferenced values:
    std::map<TKey, Cache> cache_;

    // least recently used at the front, most recently used at the back:
    std::list<TKey> lru_;
  };

}


#endif // YAE_LRU_CACHE_H_

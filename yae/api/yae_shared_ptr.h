// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SHARED_PTR_H_
#define YAE_SHARED_PTR_H_

// system includes:
#include <algorithm>
#include <cassert>
#include <iostream>

// atomics:
#if defined(__APPLE__) && defined(_ARCH_PPC) && (__GNUC__ == 4)
# include <bits/atomicity.h>
# define YAE_EXCHANGE_AND_ADD __gnu_cxx::__exchange_and_add
# define YAE_ATOMIC_ADD       __gnu_cxx::__atomic_add
#elif defined(__GNUC__) && __GNUC_MINOR__ >= 8
# include <ext/atomicity.h>
# define YAE_EXCHANGE_AND_ADD __gnu_cxx::__exchange_and_add_dispatch
# define YAE_ATOMIC_ADD       __gnu_cxx::__atomic_add_dispatch
#else
# define YAE_USE_BOOST_ATOMICS
# ifndef Q_MOC_RUN
#  include <boost/atomic.hpp>
# endif
#endif

// aeyae:
#include "../api/yae_api.h"


namespace yae
{
  //----------------------------------------------------------------
  // ref_count_base
  //
  struct ref_count_base
  {
    ref_count_base():
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_(NULL),
#endif
      shared_(0),
      weak_(0)
    {}

    virtual ~ref_count_base() {}

#if __cplusplus < 201103L
  private:
    ref_count_base(const ref_count_base &);
    ref_count_base & operator = (const ref_count_base &);
  public:
#else
    ref_count_base(ref_count_base &&) = delete;
    ref_count_base(const ref_count_base &) = delete;
    ref_count_base & operator = (ref_count_base &&) = delete;
    ref_count_base & operator = (const ref_count_base &) = delete;
#endif

    inline std::size_t shared() const YAE_NOEXCEPT
    { return static_cast<std::size_t>(shared_); }

    inline std::size_t weak() const YAE_NOEXCEPT
    { return static_cast<std::size_t>(weak_); }

    inline void increment_shared() YAE_NOEXCEPT
    {
#ifdef YAE_USE_BOOST_ATOMICS
      ++shared_;
#else
      YAE_ATOMIC_ADD(&shared_, 1);
#endif

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      if (footprint_ && footprint_->name() == "yae::AsyncTaskQueue::Task")
      {
        footprint_->capture_backtrace();
      }
#endif
    }

    inline void decrement_shared() YAE_NOEXCEPT
    {
      assert(shared_ > 0);

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      if (footprint_ && footprint_->name() == "yae::AsyncTaskQueue::Task")
      {
        footprint_->capture_backtrace();
      }
#endif

#ifdef YAE_USE_BOOST_ATOMICS
      std::size_t num_shared = --shared_;
#else
      _Atomic_word num_shared = YAE_EXCHANGE_AND_ADD(&shared_, -1) - 1;
#endif
      react_if_no_longer_referenced(static_cast<std::size_t>(num_shared),
                                    static_cast<std::size_t>(weak_));
    }

    inline void increment_weak() YAE_NOEXCEPT
    {
#ifdef YAE_USE_BOOST_ATOMICS
      ++weak_;
#else
      YAE_ATOMIC_ADD(&weak_, 1);
#endif

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      if (footprint_ && footprint_->name() == "yae::AsyncTaskQueue::Task")
      {
        footprint_->capture_backtrace();
      }
#endif
    }

    inline void decrement_weak() YAE_NOEXCEPT
    {
      assert(weak_ > 0);

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      if (footprint_ && footprint_->name() == "yae::AsyncTaskQueue::Task")
      {
        footprint_->capture_backtrace();
      }
#endif

#ifdef YAE_USE_BOOST_ATOMICS
      std::size_t num_weak = --weak_;
#else
      _Atomic_word num_weak = YAE_EXCHANGE_AND_ADD(&weak_, -1) - 1;
#endif
      react_if_no_longer_referenced(static_cast<std::size_t>(shared_),
                                    static_cast<std::size_t>(num_weak));
    }

    virtual void destroy_data_ptr() YAE_NOEXCEPT = 0;

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
    TFootprint * footprint_;
#endif

  protected:
    inline void
    react_if_no_longer_referenced(std::size_t num_shared,
                                  std::size_t num_weak) YAE_NOEXCEPT
    {
      if (!num_shared)
      {
        destroy_data_ptr();

        if (!num_weak)
        {
          delete this;
        }
      }
    }

#ifdef YAE_USE_BOOST_ATOMICS
    boost::atomic<std::size_t> shared_;
    boost::atomic<std::size_t> weak_;
#else
    _Atomic_word shared_;
    _Atomic_word weak_;
#endif
  };

  //----------------------------------------------------------------
  // ref_count
  //
  template <typename TBase>
  struct ref_count : public ref_count_base
  {
    typedef TBase base_type;

    ref_count():
      ref_count_base(),
      ptr_(NULL)
    {}

    virtual ~ref_count()
    {
      assert(!ptr_);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      assert(!footprint_);
#endif
    }

#if __cplusplus < 201103L
  private:
    ref_count(const ref_count &);
    ref_count & operator = (const ref_count &);
  public:
#else
    ref_count(ref_count &&) = delete;
    ref_count(const ref_count &) = delete;
    ref_count & operator = (ref_count &&) = delete;
    ref_count & operator = (const ref_count &) = delete;
#endif

    TBase * ptr_;
  };

  //----------------------------------------------------------------
  // default_deallocatorr
  //
  struct default_deallocator
  {
    template <typename TData>
    inline static
    void destroy(TData * data_ptr)
    {
      delete data_ptr;
    }
  };

  //----------------------------------------------------------------
  // call_destroy
  //
  struct call_destroy
  {
    template <typename TData>
    inline static
    void destroy(TData * data_ptr)
    {
      if (data_ptr)
      {
        data_ptr->destroy();
      }
    }
  };

  //----------------------------------------------------------------
  // weak_ptr
  //
  template <typename TData, typename TBase, typename TDeallocator>
  class weak_ptr;

  //----------------------------------------------------------------
  // shared_ptr
  //
  template <typename TData,
            typename TBase = TData,
            typename TDeallocator = default_deallocator>
  class shared_ptr
  {
    template <typename TCast,
              typename TCastBase,
              typename TCastDeallocator> friend class weak_ptr;

    template <typename TCast,
              typename TCastBase,
              typename TCastDeallocator> friend class shared_ptr;

    //----------------------------------------------------------------
    // ref_counter
    //
    struct ref_counter : public ref_count<TBase>
    {
      virtual ~ref_counter()
      { destroy_data_ptr(); }

#if __cplusplus < 201103L
    private:
      ref_counter(const ref_counter &);
      ref_counter & operator = (const ref_counter &);
    public:
      ref_counter() {}
#else
      ref_counter() = default;
      ref_counter(ref_counter &&) = delete;
      ref_counter(const ref_counter &) = delete;
      ref_counter & operator = (ref_counter &&) = delete;
      ref_counter & operator = (const ref_counter &) = delete;
#endif

      virtual void destroy_data_ptr() YAE_NOEXCEPT YAE_OVERRIDE
      {
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
        delete ref_count<TBase>::footprint_;
        ref_count<TBase>::footprint_ = NULL;
#endif
        TData * data_ptr = static_cast<TData *>(ref_count<TBase>::ptr_);
        ref_count<TBase>::ptr_ = NULL;
        TDeallocator::destroy(data_ptr);
      }
    };

    ref_count<TBase> * ref_counter_;

    // this is the mechanism for obtaining a shared_ptr from a weak_ptr
    explicit shared_ptr(ref_count<TBase> * shared_ref_counter):
      ref_counter_(shared_ref_counter)
    {
      ref_counter_->increment_shared();
    }

  public:
    typedef TBase base_type;
    typedef TData element_type;
    typedef TDeallocator deallocator_type;
    typedef weak_ptr<TData, TBase, TDeallocator> weak_ptr_type;
    typedef shared_ptr<TData, TBase, TDeallocator> shared_ptr_type;

    shared_ptr():
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_shared();
    }

    explicit shared_ptr(TData * data_ptr):
      ref_counter_(new ref_counter())
    {
      ref_counter_->ptr_ = static_cast<TBase *>(data_ptr);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      ref_counter_->footprint_ = data_ptr ? TFootprint::create<TData>() : NULL;
#endif
      ref_counter_->increment_shared();
    }

    shared_ptr(const shared_ptr & from_ptr):
      ref_counter_(from_ptr.ref_counter_)
    {
      ref_counter_->increment_shared();
    }

#if __cplusplus >= 201103L
    shared_ptr(shared_ptr&& from_ptr):
      ref_counter_(from_ptr.ref_counter_)
    {
      ref_counter_->increment_shared();
    }
#endif

    template <typename TFrom>
    shared_ptr(const shared_ptr<TFrom, TBase, TDeallocator> & from_ptr):
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_shared();
      this->operator=(from_ptr.template cast<TData>());
    }

    ~shared_ptr()
    { ref_counter_->decrement_shared(); }

    shared_ptr & operator = (const shared_ptr & from_ptr) YAE_NOEXCEPT
    {
      if (ref_counter_ != from_ptr.ref_counter_)
      {
        ref_counter_->decrement_shared();
        ref_counter_ = from_ptr.ref_counter_;
        ref_counter_->increment_shared();
      }

      return (*this);
    }

    inline std::size_t use_count() const YAE_NOEXCEPT
    { return ref_counter_->shared(); }

    inline operator bool() const YAE_NOEXCEPT
    { return this->get() != NULL; }

    inline operator const void * () const YAE_NOEXCEPT
    { return this->get(); }

    inline bool unique() const YAE_NOEXCEPT
    { return this->use_count() == 1; }

    inline void reset(TData * data_ptr = NULL)
    { this->operator=(shared_ptr(data_ptr)); }

    inline void swap(shared_ptr & ptr) YAE_NOEXCEPT
    { std::swap(ref_counter_, ptr.ref_counter_); }

    inline operator std::size_t () const YAE_NOEXCEPT
    { return reinterpret_cast<std::size_t>(get()); }

    inline bool operator == (std::size_t value) const YAE_NOEXCEPT
    { return (get() == reinterpret_cast<TData *>(value)); }

    inline bool operator != (std::size_t value) const YAE_NOEXCEPT
    { return (!operator == (value)); }

    inline bool operator == (int value) const YAE_NOEXCEPT
    { return (operator == (std::size_t(value))); }

    inline bool operator != (int value) const YAE_NOEXCEPT
    { return (!operator == (std::size_t(value))); }

    inline bool operator == (const shared_ptr & other) const YAE_NOEXCEPT
    { return (ref_counter_ == other.ref_counter_); }

    inline bool operator != (const shared_ptr & other) const YAE_NOEXCEPT
    { return !(operator == (other)); }

    inline TData * get() const YAE_NOEXCEPT
    { return static_cast<TData *>(ref_counter_->ptr_); }

    inline operator TData * () const YAE_NOEXCEPT
    { return get(); }

    inline operator unsigned char * () const YAE_NOEXCEPT
    { return reinterpret_cast<unsigned char *>(get()); }

    inline TData * operator -> () const YAE_NOEXCEPT
    { return get(); }

    inline TData & operator * () const YAE_NOEXCEPT
    { return *get(); }

    inline bool operator < (const shared_ptr & other) const YAE_NOEXCEPT
    { return ref_counter_ < other.ref_counter_; }

    // shared pointer dynamic cast method:
    template <typename TCast>
    shared_ptr<TCast, TBase, TDeallocator> cast() const
    {
      shared_ptr<TCast, TBase, TDeallocator> cast_ptr;

      TData * data = static_cast<TData *>(ref_counter_->ptr_);
      TCast * cast = dynamic_cast<TCast *>(data);

      if (cast)
      {
        delete cast_ptr.ref_counter_;
        cast_ptr.ref_counter_ = ref_counter_;
        cast_ptr.ref_counter_->increment_shared();
      }

      return cast_ptr;
    }
  };

  //----------------------------------------------------------------
  // operator <<
  //
  template <typename TChar,
            typename TBasicOstreamTraits,
            typename TData,
            typename TBase,
            typename TDeallocator>
  std::basic_ostream<TChar, TBasicOstreamTraits> &
  operator << (std::basic_ostream<TChar, TBasicOstreamTraits> & os,
               const shared_ptr<TData, TBase, TDeallocator> & ptr)
  {
    return os << ptr.get();
  }


  //----------------------------------------------------------------
  // weak_ptr
  //
  template <typename TData,
            typename TBase = TData,
            typename TDeallocator = default_deallocator>
  class weak_ptr
  {
    template <typename TCast,
              typename TCastBase,
              typename TCastDeallocator> friend class weak_ptr;

    template <typename TCast,
              typename TCastBase,
              typename TCastDeallocator> friend class shared_ptr;

    ref_count<TBase> * ref_counter_;

  public:
    typedef shared_ptr<TData, TBase, TDeallocator> shared_ptr_type;
    typedef typename shared_ptr_type::ref_counter ref_counter;
    typedef weak_ptr<TData, TBase, TDeallocator> weak_ptr_type;

    typedef TBase base_type;
    typedef TData element_type;
    typedef TDeallocator deallocator_type;

    weak_ptr():
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_weak();
    }

    weak_ptr(const weak_ptr & from_ptr):
      ref_counter_(from_ptr.ref_counter_)
    {
      ref_counter_->increment_weak();
    }

    template <typename TFrom>
    weak_ptr(const shared_ptr<TFrom, TBase, TDeallocator> & from_ptr):
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_weak();
      this->operator=(from_ptr.template cast<TData>());
    }

    template <typename TFrom>
    weak_ptr(const weak_ptr<TFrom, TBase, TDeallocator> & from_ptr):
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_weak();
      this->operator=(from_ptr.template cast<TData>());
    }

    ~weak_ptr()
    { ref_counter_->decrement_weak(); }

    weak_ptr & operator = (const weak_ptr & from_ptr) YAE_NOEXCEPT
    {
      if (ref_counter_ != from_ptr.ref_counter_)
      {
        ref_counter_->decrement_weak();
        ref_counter_ = from_ptr.ref_counter_;
        ref_counter_->increment_weak();
      }

      return (*this);
    }

    weak_ptr & operator = (const shared_ptr_type & from_ptr) YAE_NOEXCEPT
    {
      if (ref_counter_ != from_ptr.ref_counter_)
      {
        ref_counter_->decrement_weak();
        ref_counter_ = from_ptr.ref_counter_;
        ref_counter_->increment_weak();
      }

      return (*this);
    }

    template <typename TFrom>
    weak_ptr &
    operator = (const weak_ptr<TFrom, TBase, TDeallocator> & ptr) YAE_NOEXCEPT
    { return this->operator=(ptr.template cast<TData>()); }

    template <typename TFrom>
    weak_ptr &
    operator = (const shared_ptr<TFrom, TBase, TDeallocator> & ptr) YAE_NOEXCEPT
    { return this->operator=(ptr.template cast<TData>()); }

    inline bool operator == (const weak_ptr & other) const YAE_NOEXCEPT
    { return ref_counter_ == other.ref_counter_; }

    inline bool operator != (const weak_ptr & other) const YAE_NOEXCEPT
    { return ref_counter_ != other.ref_counter_; }

    inline bool operator < (const weak_ptr & other) const YAE_NOEXCEPT
    { return ref_counter_ < other.ref_counter_; }

    inline std::size_t use_count() const
    { return ref_counter_->shared(); }

    inline bool expired() const YAE_NOEXCEPT
    { return !ref_counter_->shared(); }

    inline void swap(weak_ptr & ptr) YAE_NOEXCEPT
    { std::swap(ref_counter_, ptr.ref_counter_); }

    inline void reset() YAE_NOEXCEPT
    { this->operator=(weak_ptr()); }

    inline shared_ptr_type lock() const
    { return shared_ptr_type(ref_counter_); }

    template <typename TCast>
    weak_ptr<TCast, TBase, TDeallocator> cast() const
    {
      weak_ptr<TCast, TBase, TDeallocator> cast_ptr;

      TData * data = static_cast<TData *>(ref_counter_->ptr_);
      TCast * cast = dynamic_cast<TCast *>(data);

      if (cast)
      {
        delete cast_ptr.ref_counter_;
        cast_ptr.ref_counter_ = ref_counter_;
        cast_ptr.ref_counter_->increment_weak();
      }

      return cast_ptr;
    }
  };


  //----------------------------------------------------------------
  // default_copier
  //
  struct default_copier
  {
    template <typename TData>
    inline static
    TData * copy(const TData * src)
    {
      return src ? new TData(*src) : NULL;
    }
  };

  //----------------------------------------------------------------
  // optional
  //
  template <typename TData,
            typename TBase = TData,
            typename TCopier = default_copier,
            typename TDeallocator = default_deallocator>
  class optional
  {
    template <typename TCast,
              typename TCastBase,
              typename TCastCopier,
              typename TCastDeallocator> friend class optional;

#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
    TFootprint * footprint_;
#endif

    TBase * ptr_;

  public:
    typedef TBase base_type;
    typedef TData element_type;
    typedef TDeallocator deallocator_type;
    typedef optional<TData, TBase, TDeallocator> optional_type;

    optional():
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_(NULL),
#endif
      ptr_(NULL)
    {}

    explicit optional(TData * data_ptr):
      ptr_(data_ptr)
    {
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
    }

    optional(const TData & data):
      ptr_(TCopier::copy(&data))
    {
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_ = TFootprint::create<TData>();
#endif
    }

    optional(const optional & other):
      ptr_(TCopier::copy(other.ptr_))
    {
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
    }

#if __cplusplus >= 201103L
    optional(optional&& other):
      ptr_(NULL)
    {
      this->swap(other);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
    }
#endif

    template <typename TFrom>
    optional(const optional<TFrom, TBase, TCopier, TDeallocator> & other):
      ptr_(TCopier::copy(other.template cast<TData>()))
    {
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
    }

    ~optional()
    {
      TDeallocator::destroy(ptr_);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      delete footprint_;
#endif
    }

    optional & operator = (const optional & other) YAE_NOEXCEPT
    {
      if (ptr_ != other.ptr_)
      {
        TDeallocator::destroy(ptr_);
        ptr_ = TCopier::copy(other.ptr_);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
        delete footprint_;
        footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
      }

      return (*this);
    }

    template <typename TFrom>
    optional & operator = (const optional<TFrom, TBase, TCopier, TDeallocator> &
                           other) YAE_NOEXCEPT
    {
      if (ptr_ != other.ptr_)
      {
        TDeallocator::destroy(ptr_);
        ptr_ = TCopier::copy(other.template cast<TData>());
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
        delete footprint_;
        footprint_ = ptr_ ? TFootprint::create<TData>() : NULL;
#endif
      }

      return *this;
    }

    inline operator bool() const YAE_NOEXCEPT
    { return ptr_; }

    inline operator const void * () const YAE_NOEXCEPT
    { return ptr_; }

    inline void reset(TData * data_ptr = NULL)
    { this->operator=(optional(data_ptr)); }

    inline void reset(const TData & data)
    { this->operator=(optional(data)); }

    inline void swap(optional & other) YAE_NOEXCEPT
    {
      std::swap(ptr_, other.ptr_);
#if defined(YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS)
      std::swap(footprint_, other.footprint_);
#endif
    }

    inline operator std::size_t () const YAE_NOEXCEPT
    { return reinterpret_cast<std::size_t>(ptr_); }

    inline bool operator == (std::size_t value) const YAE_NOEXCEPT
    { return (ptr_ == reinterpret_cast<TData *>(value)); }

    inline bool operator != (std::size_t value) const YAE_NOEXCEPT
    { return (!operator == (value)); }

    inline bool operator == (int value) const YAE_NOEXCEPT
    { return (operator == (std::size_t(value))); }

    inline bool operator != (int value) const YAE_NOEXCEPT
    { return (!operator == (std::size_t(value))); }

    template <typename TFrom>
    inline bool
    operator == (const optional<TFrom, TBase, TCopier, TDeallocator> &
                 other) const YAE_NOEXCEPT
    {
      return (ptr_ && other.ptr_ ?
              *get() == *other.get() :
              ptr_ == other.ptr_);
    }

    template <typename TFrom>
    inline bool
    operator != (const optional<TFrom, TBase, TCopier, TDeallocator> &
                 other) const YAE_NOEXCEPT
    { return !(operator == (other)); }

    template <typename TFrom>
    inline bool
    operator < (const optional<TFrom, TBase, TCopier, TDeallocator> &
                other) const YAE_NOEXCEPT
    {
      return (ptr_ && other.ptr_ ?
              *get() < *other.get() :
              ptr_ < other.ptr_);
    }

    inline TData * get() const YAE_NOEXCEPT
    { return static_cast<TData *>(ptr_); }

    inline operator TData * () const YAE_NOEXCEPT
    { return get(); }

    inline operator unsigned char * () const YAE_NOEXCEPT
    { return reinterpret_cast<unsigned char *>(get()); }

    inline TData * operator -> () const YAE_NOEXCEPT
    { return get(); }

    inline TData & operator * () const YAE_NOEXCEPT
    { return *get(); }

    // shared pointer dynamic cast method:
    template <typename TCast>
    TCast * cast() const
    {
      TData * data = static_cast<TData *>(ptr_);
      TCast * cast = dynamic_cast<TCast *>(data);
      return cast;
    }
  };

  //----------------------------------------------------------------
  // operator <<
  //
  template <typename TChar,
            typename TBasicOstreamTraits,
            typename TData,
            typename TBase,
            typename TDeallocator>
  std::basic_ostream<TChar, TBasicOstreamTraits> &
  operator << (std::basic_ostream<TChar, TBasicOstreamTraits> & os,
               const optional<TData, TBase, TDeallocator> & value)
  {
    if (value)
    {
      os << *value;
    }

    return os;
  }

}


#endif // YAE_SHARED_PTR_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef AEYAE_SHARED_PTR_HXX_
#define AEYAE_SHARED_PTR_HXX_

// system includes:
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>


namespace yae
{
  //----------------------------------------------------------------
  // ref_count_base
  //
  struct ref_count_base
  {
    ref_count_base(): shared_(0), weak_(0) {}
    virtual ~ref_count_base() = default;

    ref_count_base(ref_count_base &&) = delete;
    ref_count_base(const ref_count_base &) = delete;
    ref_count_base & operator = (ref_count_base &&) = delete;
    ref_count_base & operator = (const ref_count_base &) = delete;

    inline std::size_t shared() const noexcept
    { return shared_; }

    inline std::size_t weak() const noexcept
    { return weak_; }

    inline std::size_t increment_shared() noexcept
    { return ++shared_; }

    inline std::size_t decrement_shared() noexcept
    {
      std::size_t num_shared = --shared_;
      react_if_no_longer_referenced(num_shared, weak_);
      return num_shared;
    }

    inline std::size_t increment_weak() noexcept
    { return ++weak_; }

    inline std::size_t decrement_weak() noexcept
    {
      std::size_t num_weak = --weak_;
      react_if_no_longer_referenced(shared_, num_weak);
      return num_weak;
    }

    virtual void destroy_data_ptr() noexcept = 0;

  protected:
    inline void
    react_if_no_longer_referenced(std::size_t num_shared,
                                  std::size_t num_weak) noexcept
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

    std::atomic<std::size_t> shared_;
    std::atomic<std::size_t> weak_;
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
      ptr_(nullptr)
    {}

    virtual ~ref_count()
    { assert(!ptr_); }

    ref_count(ref_count &&) = delete;
    ref_count(const ref_count &) = delete;
    ref_count & operator = (ref_count &&) = delete;
    ref_count & operator = (const ref_count &) = delete;

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
      ref_counter() = default;

      virtual ~ref_counter()
      { destroy_data_ptr(); }

      ref_counter(ref_counter &&) = delete;
      ref_counter(const ref_counter &) = delete;
      ref_counter & operator = (ref_counter &&) = delete;
      ref_counter & operator = (const ref_counter &) = delete;

      virtual void destroy_data_ptr() noexcept override
      {
        TData * data_ptr = static_cast<TData *>(ref_count<TBase>::ptr_);
        TDeallocator::destroy(data_ptr);
        ref_count<TBase>::ptr_ = nullptr;
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
      ref_counter_->increment_shared();
    }

    shared_ptr(const shared_ptr & from_ptr):
      ref_counter_(from_ptr.ref_counter_)
    {
      ref_counter_->increment_shared();
    }

    shared_ptr(shared_ptr&& from_ptr):
      ref_counter_(from_ptr.ref_counter_)
    {
      ref_counter_->increment_shared();
    }

    template <typename TFrom>
    shared_ptr(const shared_ptr<TFrom, TBase, TDeallocator> & from_ptr):
      ref_counter_(new ref_counter())
    {
      ref_counter_->increment_shared();
      this->operator=(from_ptr.template cast<TData>());
    }

    ~shared_ptr()
    { ref_counter_->decrement_shared(); }

    shared_ptr & operator = (const shared_ptr & from_ptr) noexcept
    {
      if (ref_counter_ != from_ptr.ref_counter_)
      {
        ref_counter_->decrement_shared();
        ref_counter_ = from_ptr.ref_counter_;
        ref_counter_->increment_shared();
      }

      return (*this);
    }

    inline std::size_t use_count() const noexcept
    { return ref_counter_->shared(); }

    inline operator bool() const noexcept
    { return this->get() != nullptr; }

    inline operator const void * () const noexcept
    { return this->get(); }

    inline bool unique() const noexcept
    { return this->use_count() == 1; }

    inline void reset(TData * data_ptr = nullptr)
    { this->operator=(shared_ptr(data_ptr)); }

    inline void swap(shared_ptr & ptr) noexcept
    { std::swap(ref_counter_, ptr.ref_counter_); }

    inline operator std::size_t () const noexcept
    { return reinterpret_cast<std::size_t>(get()); }

    inline bool operator == (std::size_t value) const noexcept
    { return (get() == reinterpret_cast<TData *>(value)); }

    inline bool	operator != (std::size_t value) const noexcept
    { return (!operator == (value)); }

    inline bool	operator == (int value) const noexcept
    { return (operator == (std::size_t(value))); }

    inline bool	operator != (int value) const noexcept
    { return (!operator == (std::size_t(value))); }

    inline bool operator == (const shared_ptr & other) const noexcept
    { return (get() == other.get()); }

    inline bool operator != (const shared_ptr & other) const noexcept
    { return !(operator == (other)); }

    inline TData * get() const noexcept
    { return static_cast<TData *>(ref_counter_->ptr_); }

    inline operator TData * () const noexcept
    { return get(); }

    inline operator unsigned char * () const noexcept
    { return reinterpret_cast<unsigned char *>(get()); }

    inline TData * operator -> () const noexcept
    { return get(); }

    inline TData & operator * () const noexcept
    { return *get(); }

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

    weak_ptr & operator = (const weak_ptr & from_ptr) noexcept
    {
      if (ref_counter_ != from_ptr.ref_counter_)
      {
        ref_counter_->decrement_weak();
        ref_counter_ = from_ptr.ref_counter_;
        ref_counter_->increment_weak();
      }

      return (*this);
    }

    weak_ptr & operator = (const shared_ptr_type & from_ptr) noexcept
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
    operator = (const weak_ptr<TFrom, TBase, TDeallocator> & ptr) noexcept
    { return this->operator=(ptr.template cast<TData>()); }

    template <typename TFrom>
    weak_ptr &
    operator = (const shared_ptr<TFrom, TBase, TDeallocator> & ptr) noexcept
    { return this->operator=(ptr.template cast<TData>()); }

    inline std::size_t use_count() const
    { return ref_counter_->shared(); }

    inline bool expired() const noexcept
    { return !ref_counter_->shared(); }

    inline void swap(weak_ptr & ptr) noexcept
    { std::swap(ref_counter_, ptr.ref_counter_); }

    inline void reset() noexcept
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
}


#endif // AEYAE_SHARED_PTR_HXX_

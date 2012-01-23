// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 21 16:36:49 MST 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_SHARED_PTR_H_
#define YAMKA_SHARED_PTR_H_

// system includes:
#include <assert.h>
#include <cstddef>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // TRefCount
  // 
  // NOTE: This reference counter implementation is not thread safe,
  // increment/decrement are not atomic function calls.
  // 
  template <typename TBase>
  struct TRefCount
  {
    TRefCount():
      p_(NULL),
      n_(0)
    {}

    ~TRefCount()
    {
      delete p_;
    }

    inline void increment()
    {
      ++n_;
    }

    inline void decrement()
    {
      if (!(--n_))
      {
        delete this;
      }
    }

    TBase * p_;
    std::size_t n_;
  };

  //----------------------------------------------------------------
  // TSharedPtr
  // 
  template <typename TData, typename TBase = TData>
  struct TSharedPtr
  {
    template <typename TCast, typename TCastBase> friend struct TSharedPtr;

    typedef TSharedPtr<TData, TBase> TSelf;
    typedef TBase base_type;
    typedef TData value_type;
    
    TSharedPtr():
      r_(new TRefCount<TBase>())
    {
      r_->increment();
    }

    // This constructor is marked explicit in order to disallow
    // automatic conversion from raw pointer to shared pointer,
    // otherwise the following can happen:
    // 
    // int * rawPtr = new int;
    // 
    // Give ownership of rawPtr to a shared ptr A
    // TSharedPtr<int> A(rawPtr);
    // 
    // Here shared pointer B also points to rawPtr,
    // but the reference count is not shared with shared pointer A
    // TSharedPtr<int> B = (int *)(A);
    // 
    explicit TSharedPtr(TData * rawPtr):
      r_(new TRefCount<TBase>())
    {
      r_->p_ = rawPtr;
      r_->increment();
    }

    TSharedPtr(const TSelf & from):
      r_(from.r_)
    {
      r_->increment();
    }

    template <typename TFrom>
    TSharedPtr(const TSharedPtr<TFrom, TBase> & from):
      r_(new TRefCount<TBase>())
    {
      r_->increment();
      *this = from.cast<TData>();
    }

    ~TSharedPtr()
    {
      r_->decrement();
    }
    
    template <typename TCast>
    TSharedPtr<TCast, TBase> cast() const
    {
      TData * data = static_cast<TData *>(r_->p_);
      TCast * cast = dynamic_cast<TCast *>(data);
      TSharedPtr<TCast, TBase> castPtr;
      if (cast)
      {
        delete castPtr.r_;
        castPtr.r_ = r_;
        castPtr.r_->increment();
      }
      
      return castPtr;
    }

    inline TSelf & operator = (const TSelf & from)
    {
      if (r_ != from.r_)
      {
        r_->decrement();
        r_ = from.r_;
        r_->increment();
      }
      
      return (*this);
    }

    inline void setToNull()
    {
      *this = TSelf();
    }
    
    inline void setTo(TData * newData)
    {
      reset(newData);
    }
    
    inline void reset(TData * newData)
    {
      *this = TSelf(newData);
    }
    
    inline operator std::size_t () const
    {
      std::size_t value = reinterpret_cast<std::size_t>(get());
      return value;
    }
    
    inline bool operator == (std::size_t value) const
    {
      TData * data = get();
      return (data == reinterpret_cast<TData *>(value));
    }
    
    inline bool	operator != (std::size_t value) const
    {
      return (!operator == (value));
    }
    
    inline bool	operator == (int value) const
    {
      return (operator == (std::size_t(value)));
    }
    
    inline bool	operator != (int value) const
    {
      return (!operator == (std::size_t(value)));
    }
    
    inline bool operator == (const TSelf & compPtr) const
    {
      return (get() == compPtr.get());
    }
    
    inline bool operator != (const TSelf & compPtr) const
    {
      return !(operator == (compPtr));
    }
    
    inline TData * get () const
    {
      TData * data = static_cast<TData *>(r_->p_);
      return data;
    }
    
    inline operator TData * () const
    {
      return get();
    }
    
    inline operator unsigned char * () const
    {
      return reinterpret_cast<unsigned char *>(get());
    }
    
    inline TData * operator -> () const
    {
      return get();
    }
    
    inline TData & operator * () const
    {
      return *get();
    }
    
  protected:
    TRefCount<TBase> * r_;
  };
}


#endif // YAMKA_SHARED_PTR_H_

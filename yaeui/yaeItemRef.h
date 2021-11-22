// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_REF_H_
#define YAE_ITEM_REF_H_

// standard libraries:
#include <stdexcept>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// yae includes:
#include "yae/api/yae_api.h"

// local interfaces:
#include "yaeProperty.h"
#include "yaeExpression.h"


namespace yae
{

  //----------------------------------------------------------------
  // DataRefCaching
  //
  enum DataRefCaching
  {
    kDisableCaching = 2,
    kEnableCaching = 4,
  };

  //----------------------------------------------------------------
  // DataRef
  //
  template <typename data_t>
  struct DataRef
  {

    //----------------------------------------------------------------
    // TData
    //
    typedef data_t TData;

    //----------------------------------------------------------------
    // value_type
    //
    typedef data_t value_type;

    //----------------------------------------------------------------
    // TDataRef
    //
    typedef DataRef<data_t> TDataRef;

    //----------------------------------------------------------------
    // TDataProperties
    //
    typedef IProperties<data_t> TDataProperties;

    //----------------------------------------------------------------
    // TExpression
    //
    typedef Expression<data_t> TExpression;

    //----------------------------------------------------------------
    // IDataSrc
    //
    struct IDataSrc
    {
      virtual ~IDataSrc() {}

      virtual const TData & get() const = 0;
      virtual void get(TData & data) const = 0;

      virtual bool is_memo() const { return false; }
      virtual bool is_data() const { return false; }
      virtual bool is_prop() const { return false; }
      virtual bool is_expr() const { return false; }
      virtual bool is_dref() const { return false; }
    };

    //----------------------------------------------------------------
    // IRef
    //
    struct IRef
    {
      virtual ~IRef() {}

      struct Copy
      {
        template <typename TRef>
        inline static TRef * copy(const TRef * ref)
        { return ref ? ref->copy() : NULL; }
      };

      virtual IRef * copy() const = 0;
      virtual IRef * make_const_ref() const = 0;
      virtual IRef * make_cached_ref() const = 0;

      virtual const IDataSrc & src() const = 0;
      virtual const TData & get_value() const = 0;

      virtual bool is_cacheable() const = 0;
      virtual bool is_cached() const = 0;
      virtual void cache(const TData &) const = 0;
      virtual void uncache() const = 0;

      template <typename TSrc>
      inline const TSrc * get() const
      {
        return dynamic_cast<const TSrc *>(&src());
      }
    };

    // forward declaration:
    template <typename TDataSrc> struct CachedRef;

    //----------------------------------------------------------------
    // ConstRef
    //
    template <typename TDataSrc>
    struct ConstRef : IRef
    {
      ConstRef(const TDataSrc & src = TDataSrc()):
        src_(src)
      {}

      // virtual:
      inline ConstRef<TDataSrc> * copy() const
      { return new ConstRef<TDataSrc>(*this); }

      inline ConstRef<TDataSrc> * make_const_ref() const
      { return copy(); }

      inline CachedRef<TDataSrc> * make_cached_ref() const
      { return new CachedRef<TDataSrc>(src_); }

      // virtual:
      const IDataSrc & src() const { return src_; }

      // virtual:
      inline const TData & get_value() const
      { return src_.get(); }

      // virtual:
      bool is_cacheable() const { return false; }
      bool is_cached() const { return false; }
      void cache(const TData &) const {}
      void uncache() const {}

      TDataSrc src_;
    };

    //----------------------------------------------------------------
    // CachedRef
    //
    template <typename TDataSrc>
    struct CachedRef : IRef
    {
      CachedRef(const TDataSrc & src = TDataSrc()):
        visited_(false),
        cached_(false),
        src_(src)
      {}

      // virtual:
      inline CachedRef<TDataSrc> * copy() const
      { return new CachedRef(*this); }

      inline CachedRef<TDataSrc> * make_cached_ref() const
      { return copy(); }

      inline ConstRef<TDataSrc> * make_const_ref() const
      { return new ConstRef<TDataSrc>(src_); }

      // virtual:
      inline const IDataSrc & src() const { return src_; }

      // virtual:
      inline bool is_cacheable() const { return true; }
      inline bool is_cached() const { return cached_; }

      // virtual: cache an externally computed value:
      inline void cache(const TData & value) const
      {
        cached_ = true;
        value_ = value;
      }

      // virtual: discard the cache:
      inline void uncache() const
      {
        visited_ = false;
        cached_ = false;
      }

      // virtual:
      const TData & get_value() const
      {
        if (cached_)
        {
          return value_;
        }

        if (visited_)
        {
          // cycle detected:
          YAE_ASSERT(false);
          throw std::runtime_error("property reference cycle detected");
        }

        // NOTE: reference cycles can not be detected
        //       for items with disabled caching:
        visited_ = true;
        src_.get(value_);
        cached_ = true;
        return value_;
      }

      // reference state:
      mutable bool visited_;
      mutable bool cached_;
      mutable TData value_;

      TDataSrc src_;
    };



    //----------------------------------------------------------------
    // MemoSrc
    //
    // this is to make DataRef::cache work for invalid DataRefs.
    // used by Item::width() and Item::height() to cache implicitly
    // computed width and height.
    //
    struct MemoSrc : IDataSrc
    {
      MemoSrc(const TData & data):
        data_(data)
      {}

      // virtual:
      inline bool is_memo() const { return true; }

      // virtual:
      inline const TData & get() const { return data_; }

      // virtual:
      inline void get(TData & data) const { data = data_; }

    protected:
      TData data_;
    };

    //----------------------------------------------------------------
    // ConstDataSrc
    //
    struct ConstDataSrc : IDataSrc
    {
      ConstDataSrc(const TData & data):
        data_(data)
      {}

      // virtual:
      inline bool is_data() const { return true; }

      // virtual:
      inline const TData & get() const { return data_; }

      // virtual:
      inline void get(TData & data) const { data = data_; }

    protected:
      TData data_;
    };

    //----------------------------------------------------------------
    // PropDataSrc
    //
    struct PropDataSrc : IDataSrc
    {
      PropDataSrc(const TDataProperties & properties, Property property):
        properties_(&properties),
        property_(property)
      {}

      // virtual:
      inline bool is_prop() const
      { return true; }

      // virtual:
      inline const TData & get() const
      {
        properties_->get(property_, data_);
        return data_;
      }

      // virtual:
      inline void get(TData & data) const
      { properties_->get(property_, data); }

    protected:
      // reference properties:
      const TDataProperties * properties_;
      const Property property_;
      mutable TData data_;
    };

    //----------------------------------------------------------------
    // ExprDataSrc
    //
    struct ExprDataSrc : IDataSrc
    {
      ExprDataSrc(TExpression * expr):
        expr_(expr)
      {}

      // virtual:
      inline bool is_expr() const
      { return true; }

      // virtual:
      inline const TData & get() const
      {
        expr_->evaluate(data_);
        return data_;
      }

      // virtual:
      inline void get(TData & data) const
      { expr_->evaluate(data); }

      // accessor:
      template <typename TExpr>
      inline yae::shared_ptr<TExpr, TExpression> get() const
      { return yae::shared_ptr<TExpr, TExpression>(expr_); }

    protected:
      yae::shared_ptr<TExpression> expr_;
      mutable TData data_;
    };

    //----------------------------------------------------------------
    // DataRefSrc
    //
    struct DataRefSrc : IDataSrc
    {
      DataRefSrc(const DataRef & dref):
        dref_(&dref)
      {
        if (!dref.isValid())
        {
          YAE_ASSERT(false);
          throw std::runtime_error("reference to an invalid data reference");
        }
      }

      // virtual:
      inline bool is_dref() const
      { return true; }

      // virtual:
      inline const TData & get() const
      { return dref_->get(); }

      // virtual:
      inline void get(TData & data) const
      { data = dref_->get(); }

      // accessor:
      inline const DataRef * dref() const
      { return dref_; }

    protected:
      const DataRef * dref_;
    };


    inline void reset()
    { private_.reset(); }

    // reference setters:
    template <typename TDataSrc>
    inline void set(const TDataSrc & data_src, DataRefCaching caching)
    {
      if (caching == kEnableCaching)
      {
        private_.reset(new CachedRef<TDataSrc>(data_src));
      }
      else
      {
        private_.reset(new ConstRef<TDataSrc>(data_src));
      }
    }

    inline void set(const TData & value)
    { private_.reset(new ConstRef<ConstDataSrc>((ConstDataSrc(value)))); }

    inline void set(const TDataRef & dref)
    { private_.reset(new ConstRef<DataRefSrc>((DataRefSrc(dref)))); }

    inline void set(const TDataProperties & properties,
                    Property property,
                    DataRefCaching caching = kEnableCaching)
    {
      PropDataSrc data_src(properties, property);
      set<PropDataSrc>(data_src, caching);
    }

    inline void set(TExpression * expression,
                    DataRefCaching caching = kEnableCaching)
    {
      ExprDataSrc data_src(expression);
      set<ExprDataSrc>(data_src, caching);
    }

    // for explicit caching of an externally computed value
    // on an undefined (invalid) DataRef:
    inline void cache(const TData & value) const
    {
      YAE_ASSERT(!private_ || private_->src().is_memo());
      private_.reset(new ConstRef<MemoSrc>((MemoSrc(value))));
    }


    // constructors:
    DataRef() {}
    explicit DataRef(const TData & data) { TDataRef::set(data); }


    // constructor helpers:
    inline static TDataRef
    constant(const TData & data)
    {
      TDataRef ref;
      ref.set(data);
      return ref;
    }

    inline static TDataRef
    reference(const TDataRef & other)
    {
      TDataRef ref;
      ref.set(other);
      return ref;
    }

    inline static TDataRef
    reference(const TDataProperties & properties,
              Property property,
              DataRefCaching caching = kEnableCaching)
    {
      TDataRef ref;
      ref.set(properties, property, caching);
      return ref;
    }

    inline static TDataRef
    expression(TExpression * expr,
               DataRefCaching caching = kEnableCaching)
    {
      TDataRef ref;
      ref.set(expr, caching);
      return ref;
    }



    // check whether this property reference is valid:
    inline bool isValid() const
    { return private_ && !private_->src().is_memo(); }

    inline bool isCacheable() const
    { return private_ && private_->is_cacheable(); }

    inline bool isCached() const
    { return private_ && private_->is_cached(); }

    // caching is used to avoid re-calculating the same property:
    inline void uncache() const
    {
      if (isValid())
      {
        private_->uncache();
      }
      else
      {
        private_.reset();
      }
    }

    inline void disableCaching()
    {
      YAE_ASSERT(private_);
      if (private_ && private_->is_cacheable())
      {
        private_.reset(private_->make_const_ref());
      }
    }

    inline void enableCaching()
    {
      YAE_ASSERT(private_);
      if (private_ && !private_->is_cacheable())
      {
        private_.reset(private_->make_cached_ref());
      }
    }

    inline const TData & get() const
    {
      YAE_ASSERT(private_);
      return private_->get_value();
    }

    template <typename TExpr>
    inline yae::shared_ptr<TExpr, TExpression>
    get_expr() const
    {
      const ExprDataSrc * src =
        private_ ? (private_->template get<ExprDataSrc>()) : NULL;
      return src ? src->template get<TExpr>() :
        yae::shared_ptr<TExpr, TExpression>();
    }

    inline bool refers_to(const TDataRef & dref) const
    {
      const DataRefSrc * src =
        private_ ? (private_->template get<DataRefSrc>()) : NULL;
      return src ? (src->dref() == &dref) : false;
    }

    inline TDataRef & operator = (const TData & data)
    {
      set(data);
      return *this;
    }

    // implementation details:
    typedef yae::optional<typename TDataRef::IRef,
                          typename TDataRef::IRef,
                          typename TDataRef::IRef::Copy> TOptionalRef;

    mutable TOptionalRef private_;
  };



  //----------------------------------------------------------------
  // ItemRef
  //
  struct ItemRef : DataRef<double>
  {
    typedef DataRef<double> TBase;
    using TBase::set;
    using TBase::operator=;

    // constructors:
    ItemRef() {}
    explicit ItemRef(double data) { TBase::set(data); }

    //----------------------------------------------------------------
    // Affine
    //
    template <typename TDataSrc>
    struct Affine : IDataSrc
    {
      Affine(const TDataSrc & src, double scale, double translate):
        src_(src),
        scale_(scale),
        translate_(translate)
      {}

      // virtual:
      inline const TData & get() const
      {
        data_ = src_.get();
        data_ *= scale_;
        data_ += translate_;
        return data_;
      }

      // virtual:
      inline void get(TData & data) const
      {
        src_.get(data);
        data *= scale_;
        data += translate_;
      }

      // virtual:
      inline bool is_memo() const { return src_.is_memo(); }
      inline bool is_data() const { return src_.is_data(); }
      inline bool is_prop() const { return src_.is_prop(); }
      inline bool is_expr() const { return src_.is_expr(); }
      inline bool is_dref() const { return src_.is_dref(); }

      // public, for the VSplitter use-case:
      TDataSrc src_;
      double scale_;
      double translate_;

    protected:
      mutable double data_;
    };


    // reference setters:
    inline void set(const TDataRef & other,
                    double s,
                    double t = 0.0,
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == 1.0 && t == 0.0)
      {
        TBase::set(other);
      }
      else
      {
        typedef Affine<DataRefSrc> TDataSrc;
        TDataSrc data_src(DataRefSrc(other), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(const TDataProperties & properties,
                    Property property,
                    double s,
                    double t = 0.0,
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == 1.0 && t == 0.0)
      {
        TBase::set(properties, property, caching);
      }
      else
      {
        typedef Affine<PropDataSrc> TDataSrc;
        TDataSrc data_src(PropDataSrc(properties, property), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(TExpression * expression,
                    double s,
                    double t = 0.0,
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == 1.0 && t == 0.0)
      {
        TBase::set(expression, caching);
      }
      else
      {
        typedef Affine<ExprDataSrc> TDataSrc;
        TDataSrc data_src(ExprDataSrc(expression), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }


    // constructor helpers:
    inline static ItemRef
    constant(const double & data)
    {
      ItemRef ref;
      ref.TBase::set(data);
      return ref;
    }

    inline static ItemRef
    reference(const TDataProperties & properties,
              Property property,
              double s = 1.0,
              double t = 0.0,
              DataRefCaching caching = kEnableCaching)
    {
      ItemRef ref;
      ref.set(properties, property, s, t, caching);
      return ref;
    }

    inline static ItemRef
    uncacheable(const TDataProperties & properties,
                Property property,
                double s = 1.0,
                double t = 0.0)
    {
      ItemRef ref;
      ref.set(properties, property, s, t, kDisableCaching);
      return ref;
    }

    inline static ItemRef
    scale(const TDataProperties & props,
          Property prop,
          double s = 1.0,
          DataRefCaching caching = kEnableCaching)
    {
      return reference(props, prop, s, 0.0, caching);
    }

    inline static ItemRef
    offset(const TDataProperties & props,
           Property prop,
           double t = 0.0,
           DataRefCaching caching = kEnableCaching)
    {
      return reference(props, prop, 1.0, t, caching);
    }

    inline static ItemRef
    reference(const TDataRef & other,
              double s = 1.0,
              double t = 0.0,
              DataRefCaching caching = kEnableCaching)
    {
      ItemRef ref;
      ref.set(other, s, t, caching);
      return ref;
    }

    inline static ItemRef
    uncacheable(const TDataRef & other,
                double s = 1.0,
                double t = 0.0)
    {
      ItemRef ref;
      ref.set(other, s, t, kDisableCaching);
      return ref;
    }

    inline static ItemRef
    scale(const TDataRef & other,
          double s = 1.0,
          DataRefCaching caching = kEnableCaching)
    {
      return reference(other, s, 0.0, caching);
    }

    inline static ItemRef
    offset(const TDataRef & other,
           double t = 0.0,
           DataRefCaching caching = kEnableCaching)
    {
      return reference(other, 1.0, t, caching);
    }

    inline static ItemRef
    expression(TExpression * expr,
               double s = 1.0,
               double t = 0.0,
               DataRefCaching caching = kEnableCaching)
    {
      ItemRef ref;
      ref.set(expr, s, t, caching);
      return ref;
    }
  };


  //----------------------------------------------------------------
  // SegmentRef
  //
  typedef DataRef<Segment> SegmentRef;

  //----------------------------------------------------------------
  // BBoxRef
  //
  typedef DataRef<BBox> BBoxRef;


  //----------------------------------------------------------------
  // BoolRef
  //
  struct BoolRef : DataRef<bool>
  {
    typedef DataRef<bool> TBase;
    using TBase::set;
    using TBase::operator=;

    // constructors:
    BoolRef() {}
    explicit BoolRef(bool data) { TBase::set(data); }

    //----------------------------------------------------------------
    // Inverse
    //
    template <typename TDataSrc>
    struct Inverse : IDataSrc
    {
      Inverse(const TDataSrc & src):
        src_(src)
      {}

      // virtual:
      inline const TData & get() const
      {
        data_ = !src_.get();
        return data_;
      }

      // virtual:
      inline void get(TData & data) const
      {
        src_.get(data);
        data = !data;
      }

      // virtual:
      inline bool is_memo() const { return src_.is_memo(); }
      inline bool is_data() const { return src_.is_data(); }
      inline bool is_prop() const { return src_.is_prop(); }
      inline bool is_expr() const { return src_.is_expr(); }
      inline bool is_dref() const { return src_.is_dref(); }

      const TDataSrc src_;

    protected:
      mutable bool data_;
    };


    // reference setters:
    inline void set(const TDataRef & other,
                    bool inverse,
                    DataRefCaching caching = kEnableCaching)
    {
      if (!inverse)
      {
        TBase::set(other);
      }
      else
      {
        typedef Inverse<DataRefSrc> TDataSrc;
        TDataSrc data_src((DataRefSrc(other)));
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(const TDataProperties & properties,
                    Property property,
                    bool inverse,
                    DataRefCaching caching = kEnableCaching)
    {
      if (!inverse)
      {
        TBase::set(properties, property, caching);
      }
      else
      {
        typedef Inverse<PropDataSrc> TDataSrc;
        TDataSrc data_src(PropDataSrc(properties, property));
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(TExpression * expr,
                    bool inverse,
                    DataRefCaching caching = kEnableCaching)
    {
      if (!inverse)
      {
        TBase::set(expr, caching);
      }
      else
      {
        typedef Inverse<ExprDataSrc> TDataSrc;
        TDataSrc data_src((ExprDataSrc(expr)));
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    // constructor helpers:
    inline static BoolRef
    constant(const bool & data)
    {
      BoolRef ref;
      ref.TBase::set(data);
      return ref;
    }

    inline static BoolRef
    reference(const TDataProperties & properties,
              Property property,
              bool inverse = false,
              DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(properties, property, inverse, caching);
      return ref;
    }

    inline static BoolRef
    inverse(const TDataProperties & properties,
            Property property,
            DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(properties, property, true, caching);
      return ref;
    }

    inline static BoolRef
    reference(const TDataRef & other,
              bool inverse = false,
              DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(other, inverse, caching);
      return ref;
    }

    inline static BoolRef
    inverse(const TDataRef & other,
            DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(other, true, caching);
      return ref;
    }

    inline static BoolRef
    expression(TExpression * expr,
               bool inverse = false,
               DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(expr, inverse, caching);
      return ref;
    }

    inline static BoolRef
    inverse(TExpression * expr,
            DataRefCaching caching = kEnableCaching)
    {
      BoolRef ref;
      ref.set(expr, true, caching);
      return ref;
    }
  };


  //----------------------------------------------------------------
  // TVarRef
  //
  typedef DataRef<TVar> TVarRef;

  //----------------------------------------------------------------
  // TVec2DRef
  //
  typedef DataRef<TVec2D> TVec2DRef;

  //----------------------------------------------------------------
  // ColorRef
  //
  struct ColorRef : DataRef<Color>
  {
    typedef DataRef<Color> TBase;
    using TBase::set;
    using TBase::operator=;

    // constructors:
    ColorRef() {}
    explicit ColorRef(const Color & data) { TBase::set(data); }


    //----------------------------------------------------------------
    // Affine
    //
    template <typename TDataSrc>
    struct Affine : IDataSrc
    {
      Affine(const TDataSrc & src,
             const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
             const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0)):
        src_(src),
        scale_(scale),
        translate_(translate)
      {}

      // virtual:
      inline const TData & get() const
      {
        TVec4D data(src_.get());
        data *= scale_;
        data += translate_;
        data.clamp(0.0, 1.0);
        data_ = Color(data);
        return data_;
      }

      // virtual:
      inline void get(TData & data) const
      {
        src_.get(data);
        TVec4D vec4(data);
        vec4 *= scale_;
        vec4 += translate_;
        vec4.clamp(0.0, 1.0);
        data = Color(vec4);
      }

      // virtual:
      inline bool is_memo() const { return src_.is_memo(); }
      inline bool is_data() const { return src_.is_data(); }
      inline bool is_prop() const { return src_.is_prop(); }
      inline bool is_expr() const { return src_.is_expr(); }
      inline bool is_dref() const { return src_.is_dref(); }

      const TDataSrc src_;
      const TVec4D scale_;
      const TVec4D translate_;

    protected:
      mutable Color data_;
    };


    // reference setters:
    inline void set(const TDataRef & other,
                    const TVec4D & s,
                    const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0),
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == TVec4D(1.0, 1.0, 1.0, 1.0) &&
          t == TVec4D(0.0, 0.0, 0.0, 0.0))
      {
        TBase::set(other);
      }
      else
      {
        typedef Affine<DataRefSrc> TDataSrc;
        TDataSrc data_src(DataRefSrc(other), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(const TDataProperties & properties,
                    Property property,
                    const TVec4D & s,
                    const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0),
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == TVec4D(1.0, 1.0, 1.0, 1.0) &&
          t == TVec4D(0.0, 0.0, 0.0, 0.0))
      {
        TBase::set(properties, property, caching);
      }
      else
      {
        typedef Affine<PropDataSrc> TDataSrc;
        TDataSrc data_src(PropDataSrc(properties, property), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }

    inline void set(TExpression * expr,
                    const TVec4D & s,
                    const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0),
                    DataRefCaching caching = kEnableCaching)
    {
      if (s == TVec4D(1.0, 1.0, 1.0, 1.0) &&
          t == TVec4D(0.0, 0.0, 0.0, 0.0))
      {
        TBase::set(expr, caching);
      }
      else
      {
        typedef Affine<ExprDataSrc> TDataSrc;
        TDataSrc data_src(ExprDataSrc(expr), s, t);
        TBase::set<TDataSrc>(data_src, caching);
      }
    }


    // constructor helpers:
    inline static ColorRef
    constant(const Color & color)
    {
      ColorRef ref;
      ref.TBase::set(color);
      return ref;
    }

    inline static ColorRef
    reference(const TDataProperties & properties,
              Property property,
              const TVec4D & s = TVec4D(1.0, 1.0, 1.0, 1.0),
              const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0),
              DataRefCaching caching = kEnableCaching)
    {
      ColorRef ref;
      ref.set(properties, property, s, t, caching);
      return ref;
    }

    inline static ColorRef
    transparent(const TDataProperties & properties,
                Property property,
                double sa = 0.0,
                DataRefCaching caching = kEnableCaching)
    {
      return reference(properties,
                       property,
                       TVec4D(sa, 1.0, 1.0, 1.0),
                       TVec4D(0.0, 0.0, 0.0, 0.0),
                       caching);
    }

    inline static ColorRef
    scale(const TDataProperties & properties,
          Property property,
          const TVec4D & scale,
          DataRefCaching caching = kEnableCaching)
    {
      return reference(properties,
                       property,
                       scale,
                       TVec4D(0.0, 0.0, 0.0, 0.0),
                       caching);
    }

    inline static ColorRef
    offset(const TDataProperties & properties,
           Property property,
           const TVec4D & translate,
           DataRefCaching caching = kEnableCaching)
    {
      return reference(properties,
                       property,
                       TVec4D(1.0, 1.0, 1.0, 1.0),
                       translate,
                       caching);
    }

    inline static ColorRef
    expression(TExpression * expr,
               const TVec4D & s = TVec4D(1.0, 1.0, 1.0, 1.0),
               const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0),
               DataRefCaching caching = kEnableCaching)
    {
      ColorRef ref;
      ref.set(expr, s, t, caching);
      return ref;
    }
  };

  //----------------------------------------------------------------
  // TGradientRef
  //
  typedef DataRef<TGradientPtr> TGradientRef;

}


#endif // YAE_ITEM_REF_H_

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


namespace yae
{

  //----------------------------------------------------------------
  // DataRef
  //
  template <typename TData>
  struct DataRef
  {
    //----------------------------------------------------------------
    // value_type
    //
    typedef TData value_type;

    //----------------------------------------------------------------
    // TDataRef
    //
    typedef DataRef<TData> TDataRef;

    //----------------------------------------------------------------
    // TDataProperties
    //
    typedef IProperties<TData> TDataProperties;

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

      virtual const TDataProperties * get_reference() const = 0;
      virtual Property get_property() const = 0;

      virtual bool is_relative() const = 0;
      virtual bool is_cached() const = 0;
      virtual bool is_cacheable() const = 0;
      virtual void set_cacheable(bool cacheable) = 0;
      virtual void uncache() const = 0;
      virtual void cache(const TData & value) const = 0;
      virtual const TData & get_value() const = 0;
    };

    //----------------------------------------------------------------
    // Const
    //
    struct Const : IRef
    {
      Const(const TData & value):
        value_(value)
      {}

      virtual Const * copy() const
      { return new Const(*this); }

      virtual const TDataProperties * get_reference() const { return NULL; }
      virtual Property get_property() const { return kPropertyConstant; }

      virtual bool is_relative() const { return false; }
      virtual bool is_cached() const { return true; }
      virtual bool is_cacheable() const { return false; }
      virtual void set_cacheable(bool cacheable) { (void)cacheable; }
      virtual void uncache() const {}
      virtual void cache(const TData & value) const { (void)value; }
      virtual const TData & get_value() const { return value_; }

      TData value_;
   };

    //----------------------------------------------------------------
    // Ref
    //
    struct Ref : IRef
    {
      Ref(const TDataProperties & ref, Property prop, bool cacheable = true):
        ref_(ref),
        prop_(prop),
        cacheable_(cacheable),
        visited_(false),
        cached_(false)
      {
        YAE_ASSERT(prop != kPropertyUnspecified);
      }

      virtual Ref * copy() const
      { return new Ref(*this); }

      virtual const TDataProperties * get_reference() const { return &ref_; }
      virtual Property get_property() const { return prop_; }

      virtual bool is_relative() const { return true; }
      virtual bool is_cached() const { return cached_; }
      virtual bool is_cacheable() const { return cacheable_; }

      virtual void set_cacheable(bool cacheable)
      {
        cacheable_ = cacheable;
      }

      // caching is used to avoid re-calculating the same property:
      virtual void uncache() const
      {
        visited_ = false;
        cached_ = false;
      }

      // cache an externally computed value:
      virtual void cache(const TData & value) const
      {
        cached_ = cacheable_;
        value_ = value;
      }

      virtual const TData & get_value() const
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
        visited_ = cacheable_;

        TData v;
        ref_.get(prop_, v);
        value_ = v;

        cached_ = cacheable_;
        return value_;
      }

      // reference properties:
      const TDataProperties & ref_;
      const Property prop_;
      bool cacheable_;

      // reference state:
      mutable bool visited_;
      mutable bool cached_;
      mutable TData value_;
    };

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TData & value):
      private_(new Const(value))
    {}

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            bool cacheable = true)
    {
      if (reference)
      {
        private_.reset(new Ref(*reference, property, cacheable));
      }
    }

    inline void reset()
    { private_.reset(); }

    // constructor helpers:
    inline static DataRef<TData>
    reference(const TDataProperties & ref, Property prop)
    { return DataRef<TData>(&ref, prop); }

    inline static DataRef<TData>
    constant(const TData & t)
    { return DataRef<TData>(t); }

    inline static DataRef<TData>
    expression(const TDataProperties & ref)
    { return DataRef<TData>(&ref, kPropertyExpression); }

    // check whether this property reference is valid:
    inline bool isValid() const
    { return private_; }

    // accessor to reference source, if any:
    inline const TDataProperties * ref() const
    { return private_ ? private_->get_reference() : NULL; }

    // check whether this reference is relative:
    inline bool isRelative() const
    { return private_ && private_->is_relative(); }

    inline bool isConstant() const
    { return private_ && private_->get_property() == kPropertyConstant; }

    inline bool isCached() const
    { return private_ && private_->is_cached(); }

    // caching is used to avoid re-calculating the same property:
    inline void uncache() const
    { if (private_) private_->uncache(); }

    // cache an externally computed value:
    inline void cache(const TData & value) const
    { if (private_) private_->cache(value); }

    inline bool isCacheable() const
    { return private_ && private_->is_cacheable(); }

    inline void enableCaching()
    {
      YAE_ASSERT(private_);
      private_->set_cacheable(true);
    }

    inline void disableCaching()
    {
      YAE_ASSERT(private_);
      private_->set_cacheable(false);
    }

    inline const TData & get() const
    {
      YAE_ASSERT(private_);
      return private_->get_value();
    }

    template <typename TProp>
    inline TProp *
    unwrap() const
    {
      const Ref * ref = dynamic_cast<const Ref *>(private_.get());
      if (ref)
      {
        const TProp * prop = dynamic_cast<const TProp *>(&ref->ref_);
        return const_cast<TProp *>(prop);
      }

      return NULL;
    }

    // implementation details:
    typedef yae::optional<typename TDataRef::IRef,
                          typename TDataRef::IRef,
                          typename TDataRef::IRef::Copy> TOptionalRef;

    TOptionalRef private_;
  };

  //----------------------------------------------------------------
  // ItemRef
  //
  struct ItemRef : public DataRef<double>
  {
    typedef DataRef<double> TDataRef;
    typedef IProperties<double> TDataProperties;

    //----------------------------------------------------------------
    // Affine
    //
    struct Affine : public TDataRef::Ref
    {
      Affine(const TDataProperties & ref,
             Property prop,
             double scale,
             double translate,
             bool cacheable = true):
        TDataRef::Ref(ref, prop, cacheable),
        scale_(scale),
        translate_(translate)
      {}

      virtual Affine * copy() const
      { return new Affine(*this); }

      virtual const double & get_value() const
      {
        if (!TDataRef::Ref::cached_)
        {
          double v = TDataRef::Ref::get_value();
          v *= scale_;
          v += translate_;
          TDataRef::Ref::cache(v);
        }

        return TDataRef::Ref::value_;
      }

      double scale_;
      double translate_;
    };

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            double scale = 1.0,
            double translate = 0.0,
            bool cacheable = true)
    {
      if (reference)
      {
        private_.reset(new Affine(*reference,
                                  property,
                                  scale,
                                  translate,
                                  cacheable));
      }
    }

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const double & value):
      TDataRef(value)
    {}

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const ItemRef & other):
      TDataRef(other)
    {}

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const TDataRef & dataRef,
            double scale,
            double translate = 0.0,
            bool cacheable = true)
    {
      if (!dataRef.isValid())
      {
        YAE_ASSERT(false);
        throw std::runtime_error("reference to an invalid data reference");
      }

      // shortcut:
      const IRef & other = *(dataRef.private_);

      if (other.get_property() == kPropertyConstant)
      {
        // pre-evaluate constant values:
        double value = dataRef.get() * scale + translate;
        private_.reset(new TDataRef::Const(value));
      }
      else
      {
        double s = scale;
        double t = translate;

        Affine * affine = dataRef.private_.cast<Affine>();
        if (affine)
        {
          double s_other = affine->scale_;
          double t_other = affine->translate_;

          s = scale * s_other;
          t = scale * t_other + translate;
        }

        if (s == 1.0 && t == 0.0)
        {
          private_.reset(new TDataRef::Ref(*other.get_reference(),
                                           other.get_property(),
                                           cacheable));
        }
        else
        {
          private_.reset(new Affine(*other.get_reference(),
                                    other.get_property(),
                                    s,
                                    t,
                                    cacheable));
        }
      }
    }

    // constructor helpers:
    inline static ItemRef
    reference(const TDataProperties & ref,
              Property prop,
              double s = 1.0,
              double t = 0.0)
    { return ItemRef(&ref, prop, s, t); }

    inline static ItemRef
    uncacheable(const TDataProperties & ref,
                Property prop,
                double s = 1.0,
                double t = 0.0)
    { return ItemRef(&ref, prop, s, t, false); }

    inline static ItemRef
    reference(const TDataRef & dataRef,
              double s = 1.0,
              double t = 0.0,
              bool cacheable = true)
    { return ItemRef(dataRef, s, t, cacheable); }

    inline static ItemRef
    uncacheable(const TDataRef & dataRef,
                double s = 1.0,
                double t = 0.0)
    { return ItemRef(dataRef, s, t, false); }

    inline static ItemRef
    constant(const double & t)
    { return ItemRef(t); }

    inline static ItemRef
    expression(const TDataProperties & ref, double s = 1.0, double t = 0.0)
    { return ItemRef(&ref, kPropertyExpression, s, t); }

    inline static ItemRef
    scale(const TDataProperties & ref, Property prop, double s = 1.0)
    { return ItemRef(&ref, prop, s, 0.0); }

    inline static ItemRef
    offset(const TDataProperties & ref, Property prop, double t = 0.0)
    { return ItemRef(&ref, prop, 1.0, t); }
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
  struct BoolRef : public DataRef<bool>
  {
    typedef DataRef<bool> TDataRef;
    typedef IProperties<bool> TDataProperties;

    //----------------------------------------------------------------
    // Inverse
    //
    struct Inverse : TDataRef::Ref
    {
      Inverse(const TDataProperties & ref,
              Property prop,
              bool cacheable = true):
        TDataRef::Ref(ref, prop, cacheable)
      {}

      virtual Inverse * copy() const
      { return new Inverse(*this); }

      virtual const bool & get_value() const
      {
        if (!TDataRef::Ref::cached_)
        {
          // invert the value:
          bool v = !TDataRef::Ref::get_value();
          TDataRef::Ref::cache(v);
        }

        return TDataRef::Ref::value_;
      }
    };

    //----------------------------------------------------------------
    // BoolRef
    //
    BoolRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            bool inverse = false,
            bool cacheable = true)
    {
      if (reference)
      {
        if (inverse)
        {
          private_.reset(new Inverse(*reference,
                                     property,
                                     cacheable));
        }
        else
        {
          private_.reset(new TDataRef::Ref(*reference,
                                           property,
                                           cacheable));
        }
      }
    }

    //----------------------------------------------------------------
    // BoolRef
    //
    BoolRef(const bool & value):
      TDataRef(value)
    {}

    //----------------------------------------------------------------
    // BoolRef
    //
    BoolRef(const BoolRef & other,
            bool inverse = false,
            bool cacheable = true)
    {
      if (!other.isValid())
      {
        YAE_ASSERT(false);
        throw std::runtime_error("reference to an invalid data reference");
      }

      if (other.isConstant())
      {
        // pre-evaluate constant values:
        bool value = inverse ^ other.get();
        private_.reset(new TDataRef::Const(value));
      }
      else
      {
        const TDataRef::Ref * ref = other.private_.cast<TDataRef::Ref>();
        const Inverse * inv = other.private_.cast<Inverse>();

        if (inv && inverse || !inv && !inverse)
        {
          private_.reset(new TDataRef::Ref(ref->ref_, ref->prop_, cacheable));
        }
        else
        {
          private_.reset(new Inverse(ref->ref_, ref->prop_, cacheable));
        }
      }
    }

    // constructor helpers:
    inline static BoolRef
    constant(const bool & t)
    { return BoolRef(t); }

    inline static BoolRef
    reference(const TDataProperties & ref, Property prop, bool inverse = false)
    { return BoolRef(&ref, prop, inverse); }

    inline static BoolRef
    reference(const BoolRef & ref, bool inverse = false)
    { return BoolRef(ref, inverse); }

    inline static BoolRef
    expression(const TDataProperties & ref, bool inverse = false)
    { return BoolRef(&ref, kPropertyExpression, inverse); }

    inline static BoolRef
    inverse(const TDataProperties & ref, Property prop)
    { return BoolRef(&ref, prop, true); }

    inline static BoolRef
    inverse(const BoolRef & ref)
    { return BoolRef(ref, true); }
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
  struct ColorRef : public DataRef<Color>
  {
    typedef DataRef<Color> TDataRef;
    typedef IProperties<Color> TDataProperties;

    //----------------------------------------------------------------
    // Affine
    //
    struct Affine : TDataRef::Ref
    {
      Affine(const TDataProperties & ref,
             Property prop,
             const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
             const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0),
             bool cacheable = true):
        TDataRef::Ref(ref, prop, cacheable),
        scale_(scale),
        translate_(translate)
      {}

      virtual Affine * copy() const
      { return new Affine(*this); }

      virtual const Color & get_value() const
      {
        if (!TDataRef::Ref::cached_)
        {
          TVec4D v(TDataRef::Ref::get_value());
          v *= scale_;
          v += translate_;
          v.clamp(0.0, 1.0);
          TDataRef::Ref::cache(Color(v));
        }

        return TDataRef::Ref::value_;
      }

      TVec4D scale_;
      TVec4D translate_;
    };

    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const TDataProperties * reference = NULL,
             Property property = kPropertyUnspecified,
             const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
             const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0),
             bool cacheable = true)
    {
      if (reference)
      {
        private_.reset(new Affine(*reference,
                                  property,
                                  scale,
                                  translate,
                                  cacheable));
      }
    }

    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const Color & value):
      TDataRef(value)
    {}

    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const ColorRef & other):
      TDataRef(other)
    {}

    // constructor helpers:
    inline static ColorRef
    constant(const Color & t)
    { return ColorRef(t); }

    inline static ColorRef
    reference(const TDataProperties & ref,
              Property prop,
              const TVec4D & s = TVec4D(1.0, 1.0, 1.0, 1.0),
              const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0))
    { return ColorRef(&ref, prop, s, t); }

    inline static ColorRef
    expression(const TDataProperties & ref,
               const TVec4D & s = TVec4D(1.0, 1.0, 1.0, 1.0),
               const TVec4D & t = TVec4D(0.0, 0.0, 0.0, 0.0))
    { return ColorRef(&ref, kPropertyExpression, s, t); }

    inline static ColorRef
    scale(const TDataProperties & ref, Property prop, const TVec4D & s)
    { return ColorRef(&ref, prop, s); }

    inline static ColorRef
    offset(const TDataProperties & ref, Property prop, const TVec4D & t)
    { return ColorRef(&ref, prop, TVec4D(1.0, 1.0, 1.0, 1.0), t); }

    inline static ColorRef
    transparent(const TDataProperties & ref, Property prop, double sa = 0.0)
    { return ColorRef(&ref, prop, TVec4D(sa, 1.0, 1.0, 1.0)); }
  };

  //----------------------------------------------------------------
  // TGradientRef
  //
  typedef DataRef<TGradientPtr> TGradientRef;

}


#endif // YAE_ITEM_REF_H_

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
    // TDataProperties
    //
    typedef IProperties<TData> TDataProperties;

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            const TData & defaultValue = TData(),
            bool cachingEnabled = true):
      ref_(reference),
      property_(property),
      cachingEnabled_(cachingEnabled),
      visited_(false),
      cached_(false),
      value_(defaultValue)
    {}

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TData & constantValue):
      ref_(NULL),
      property_(kPropertyConstant),
      cachingEnabled_(true),
      visited_(false),
      cached_(false),
      value_(constantValue)
    {}

    inline void reset()
    {
      ref_ = NULL;
      property_ = kPropertyUnspecified;
      cachingEnabled_ = true;
    }

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
    { return property_ != kPropertyUnspecified; }

    // check whether this reference is relative:
    inline bool isRelative() const
    { return ref_ != NULL; }

    inline bool isCached() const
    { return cached_; }

    // caching is used to avoid re-calculating the same property:
    void uncache() const
    {
      visited_ = false;
      cached_ = false;
    }

    // cache an externally computed value:
    void cache(const TData & value) const
    {
      cached_ = cachingEnabled_;
      value_ = value;
    }

    const TData & get() const
    {
      if (cached_)
      {
        return value_;
      }

      if (!ref_)
      {
        YAE_ASSERT(property_ == kPropertyConstant);
      }
      else if (visited_)
      {
        // cycle detected:
        YAE_ASSERT(false);
        throw std::runtime_error("property reference cycle detected");
      }
      else
      {
        // NOTE: reference cycles can not be detected
        //       for items with disabled caching:
        visited_ = cachingEnabled_;

        TData v;
        ref_->get(property_, v);
        value_ = v;
      }

      cached_ = cachingEnabled_;
      return value_;
    }

    // reference properties:
    const TDataProperties * ref_;
    Property property_;
    bool cachingEnabled_;

  protected:
    mutable bool visited_;
    mutable bool cached_;
    mutable TData value_;
  };

  //----------------------------------------------------------------
  // ItemRef
  //
  struct ItemRef : public DataRef<double>
  {
    typedef DataRef<double> TDataRef;
    typedef IProperties<double> TDataProperties;

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            double scale = 1.0,
            double translate = 0.0,
            const double & defaultValue = 0.0,
            bool cachingEnabled = true):
      TDataRef(reference, property, defaultValue, cachingEnabled),
      scale_(scale),
      translate_(translate)
    {}

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const double & constantValue):
      TDataRef(constantValue),
      scale_(1.0),
      translate_(0.0)
    {}

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const TDataRef & dataRef,
            double scale = 1.0,
            double translate = 0.0,
            bool cachingEnabled = true):
      TDataRef(dataRef),
      scale_(scale),
      translate_(translate)
    {
      cachingEnabled_ = cachingEnabled;

      if (property_ == kPropertyConstant)
      {
        // pre-evaluate constant values:
        value_ = value_ * scale + translate;
        scale_ = 1.0;
        translate_ = 0.0;
      }
    }

    inline void reset()
    {
      TDataRef::reset();
      scale_ = 1.0;
      translate_ = 0.0;
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
    { return ItemRef(&ref, prop, s, t, 0.0, false); }

    inline static ItemRef
    reference(const TDataRef & dataRef,
              double s = 1.0,
              double t = 0.0,
              bool cachingEnabled = true)
    { return ItemRef(dataRef, s, t, cachingEnabled); }

    inline static ItemRef
    reference(const ItemRef & dataRef,
              double s = 1.0,
              double t = 0.0,
              bool cachingEnabled = true)
    {
      return ItemRef(dataRef,
                     s * dataRef.scale_,
                     s * dataRef.translate_ + t,
                     cachingEnabled);
    }

    inline static ItemRef
    uncacheable(const TDataRef & dataRef,
                double s = 1.0,
                double t = 0.0)
    {
      ItemRef r(dataRef, s, t);
      r.cached_ = false;
      r.cachingEnabled_ = false;
      return r;
    }

    inline static ItemRef
    uncacheable(const ItemRef & dataRef,
                double s = 1.0,
                double t = 0.0)
    {
      ItemRef r(dataRef, s * dataRef.scale_, s * dataRef.translate_ + t);
      r.cached_ = false;
      r.cachingEnabled_ = false;
      return r;
    }

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

    const double & get() const
    {
      if (TDataRef::cached_)
      {
        return TDataRef::value_;
      }

      if (!TDataRef::ref_)
      {
        YAE_ASSERT(TDataRef::property_ == kPropertyConstant);
      }
      else if (TDataRef::visited_)
      {
        // cycle detected:
        YAE_ASSERT(false);
        throw std::runtime_error("property reference cycle detected");
      }
      else
      {
        TDataRef::visited_ = cachingEnabled_;

        double v;
        ref_->get(TDataRef::property_, v);
        v *= scale_;
        v += translate_;
        TDataRef::value_ = v;
      }

      TDataRef::cached_ = cachingEnabled_;
      return TDataRef::value_;
    }

    // reference properties:
    double scale_;
    double translate_;
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
  typedef DataRef<bool> BoolRef;

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
    // ColorRef
    //
    ColorRef(const TDataProperties * reference = NULL,
             Property property = kPropertyUnspecified,
             const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
             const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0),
             const Color & defaultValue = Color()):
      TDataRef(reference, property, defaultValue),
      scale_(scale),
      translate_(translate)
    {}

    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const Color & constantValue):
      TDataRef(constantValue),
      scale_(1.0, 1.0, 1.0, 1.0),
      translate_(0.0, 0.0, 0.0, 0.0)
    {}
    /*
    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const TDataRef & dataRef,
             const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
             const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0)):
      TDataRef(dataRef),
      scale_(scale),
      translate_(translate)
    {}
    */
    //----------------------------------------------------------------
    // ColorRef
    //
    ColorRef(const ColorRef & dataRef):
      TDataRef(dataRef),
      scale_(dataRef.scale_),
      translate_(dataRef.translate_)
    {}

    inline void reset()
    {
      ref_ = NULL;
      property_ = kPropertyUnspecified;
    }

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

    const Color & get() const
    {
      if (TDataRef::cached_)
      {
        return TDataRef::value_;
      }

      if (!TDataRef::ref_)
      {
        YAE_ASSERT(TDataRef::property_ == kPropertyConstant);
      }
      else if (TDataRef::visited_)
      {
        // cycle detected:
        YAE_ASSERT(false);
        throw std::runtime_error("property reference cycle detected");
      }
      else
      {
        TDataRef::visited_ = cachingEnabled_;

        Color color;
        ref_->get(TDataRef::property_, color);

        TVec4D v(color);
        v *= scale_;
        v += translate_;
        v.clamp(0.0, 1.0);

        TDataRef::value_ = Color(v);
      }

      TDataRef::cached_ = cachingEnabled_;
      return TDataRef::value_;
    }

    TVec4D scale_;
    TVec4D translate_;
  };

  //----------------------------------------------------------------
  // TGradientRef
  //
  typedef DataRef<TGradientPtr> TGradientRef;

}


#endif // UAE_ITEM_REF_H_

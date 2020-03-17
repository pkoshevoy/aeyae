// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PROPERTY_H_
#define YAE_PROPERTY_H_

// standard libraries:
#include <cmath>

// Qt interfaces:
#include <QVariant>

// aeyae:
#include "yae/api/yae_shared_ptr.h"

// local interfaces:
#include "yaeBBox.h"
#include "yaeColor.h"
#include "yaeSegment.h"
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // TVar
  //
  struct TVar : public QVariant
  {
    TVar():
      QVariant()
    {}

    template <typename TData>
    TVar(const TData & value):
      QVariant(value)
    {}

    TVar(const std::string & value):
      QVariant(QString::fromUtf8(value.c_str()))
    {}

    inline TVar & operator *= (double scale)
    {
      (void)scale;
      return *this;
    }

    inline TVar & operator += (double translate)
    {
      (void)translate;
      return *this;
    }
  };


  //----------------------------------------------------------------
  // Property
  //
  enum Property
  {
    kPropertyUnspecified,
    kPropertyConstant,
    kPropertyExpression,
    kPropertyTransition,
    kPropertyAnchorLeft,
    kPropertyAnchorRight,
    kPropertyAnchorTop,
    kPropertyAnchorBottom,
    kPropertyAnchorHCenter,
    kPropertyAnchorVCenter,
    kPropertyMarginLeft,
    kPropertyMarginRight,
    kPropertyMarginTop,
    kPropertyMarginBottom,
    kPropertyWidth,
    kPropertyHeight,
    kPropertyLeft,
    kPropertyRight,
    kPropertyTop,
    kPropertyBottom,
    kPropertyHCenter,
    kPropertyVCenter,
    kPropertyXContent,
    kPropertyYContent,
    kPropertyXExtent,
    kPropertyYExtent,
    kPropertyContentWidth,
    kPropertyContentHeight,
    kPropertyBBoxContent,
    kPropertyBBox,
    kPropertyDonutHoleLeft,
    kPropertyDonutHoleRight,
    kPropertyDonutHoleTop,
    kPropertyDonutHoleBottom,
    kPropertyDonutHoleWidth,
    kPropertyDonutHoleHeight,
    kPropertyVisible,
    kPropertyOpacity,
    kPropertyBorderWidth,
    kPropertyWeight,
    kPropertyColor,
    kPropertyColorBorder,
    kPropertyColorBg,
    kPropertyColorCursor,
    kPropertyColorSelFg,
    kPropertyColorSelBg,
    kPropertyColorNoFocusBg,
    kPropertyColorOnFocusBg,
    kPropertyCursorWidth,
    kPropertyHasText,
    kPropertyText,
    kPropertyEnabled,
    kPropertyChecked,
    kPropertyR1,
    kPropertyR2,
    kPropertyFontSize,
    kPropertyFontAscent,
    kPropertyFontDescent,
    kPropertyFontHeight,
    kPropertyFontXHeight,
    kPropertyTextLeftBearing,
    kPropertyTextRightBearing,
    kPropertyScrollviewXPos,
    kPropertyScrollviewYPos,
    kUnitSize = kPropertyWidth,
  };

  //----------------------------------------------------------------
  // IPropertiesBase
  //
  struct IPropertiesBase
  {
    virtual ~IPropertiesBase() {}
  };

  //----------------------------------------------------------------
  // TPropertiesBasePtr
  //
  typedef yae::shared_ptr<IPropertiesBase> TPropertiesBasePtr;

  //----------------------------------------------------------------
  // IProperties
  //
  template <typename TData>
  struct IProperties : public IPropertiesBase
  {
    // property accessors:
    //
    // 1. accessing a property specified via a cyclical reference
    //    will throw a runtime exception.
    // 2. accessing an unsupported property will throw a runtime exception.
    //
    virtual void get(Property property, TData & data) const = 0;
  };

  //----------------------------------------------------------------
  // TDoubleProp
  //
  typedef IProperties<double> TDoubleProp;

  //----------------------------------------------------------------
  // TSegmentProp
  //
  typedef IProperties<Segment> TSegmentProp;

  //----------------------------------------------------------------
  // TBBoxProp
  //
  typedef IProperties<BBox> TBBoxProp;

  //----------------------------------------------------------------
  // TBoolProp
  //
  typedef IProperties<bool> TBoolProp;

  //----------------------------------------------------------------
  // TColorProp
  //
  typedef IProperties<Color> TColorProp;

  //----------------------------------------------------------------
  // TVarProp
  //
  typedef IProperties<TVar> TVarProp;

  //----------------------------------------------------------------
  // TVec2DProp
  //
  typedef IProperties<TVec2D> TVec2DProp;

}


#endif // YAE_PROPERTY_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_EXPRESSION_H_
#define YAE_EXPRESSION_H_

// standard libraries:
#include <stdexcept>

// yae includes:
#include "yae/api/yae_api.h"

// local interfaces:
#include "yaeProperty.h"


namespace yae
{

  //----------------------------------------------------------------
  // Expression
  //
  template <typename TData>
  struct Expression : public IProperties<TData>
  {
    virtual void evaluate(TData & result) const = 0;

    // virtual:
    void get(Property property, TData & result) const
    {
      if (property != kPropertyExpression)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("requested a non-expression property");
      }

      evaluate(result);
    }
  };

  //----------------------------------------------------------------
  // TDoubleExpr
  //
  typedef Expression<double> TDoubleExpr;

  //----------------------------------------------------------------
  // TSegmentExpr
  //
  typedef Expression<Segment> TSegmentExpr;

  //----------------------------------------------------------------
  // TBBoxExpr
  //
  typedef Expression<BBox> TBBoxExpr;

  //----------------------------------------------------------------
  // TBoolExpr
  //
  typedef Expression<bool> TBoolExpr;

  //----------------------------------------------------------------
  // TVarExpr
  //
  typedef Expression<TVar> TVarExpr;

  //----------------------------------------------------------------
  // TColorExpr
  //
  typedef Expression<Color> TColorExpr;

  //----------------------------------------------------------------
  // TVec2DExpr
  //
  typedef Expression<TVec2D> TVec2DExpr;

  //----------------------------------------------------------------
  // TGradientExpr
  //
  typedef Expression<TGradientPtr> TGradientExpr;

}


#endif // YAE_EXPRESSION_H_

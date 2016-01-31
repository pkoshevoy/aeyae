// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXT_H_
#define YAE_TEXT_H_

// Qt library:
#include <QFont>

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  // forward declarations:
  class Text;


  //----------------------------------------------------------------
  // GetFontAscent
  //
  struct GetFontAscent : public TDoubleExpr
  {
    GetFontAscent(const Text & item);

    // virtual:
    void evaluate(double & result) const;

    const Text & item_;
  };


  //----------------------------------------------------------------
  // GetFontDescent
  //
  struct GetFontDescent : public TDoubleExpr
  {
    GetFontDescent(const Text & item);

    // virtual:
    void evaluate(double & result) const;

    const Text & item_;
  };


  //----------------------------------------------------------------
  // GetFontHeight
  //
  struct GetFontHeight : public TDoubleExpr
  {
    GetFontHeight(const Text & item);

    // virtual:
    void evaluate(double & result) const;

    const Text & item_;
  };


  //----------------------------------------------------------------
  // Supersample
  //
  template <typename TFontSizeItem>
  struct Supersample : public TDoubleExpr
  {
    Supersample(const TFontSizeItem & item, double minFontSize = 72.0):
      item_(item),
      minFontSize_(minFontSize)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double fontSize = item_.fontSize_.get();
      result = std::max<double>(1.0, minFontSize_ / fontSize);
      result = std::min<double>(8.0, result);
    }

    const TFontSizeItem & item_;
    double minFontSize_;
  };


  //----------------------------------------------------------------
  // Text
  //
  class Text : public Item
  {
    Text(const Text &);
    Text & operator = (const Text &);

    BBoxRef bboxText_;

  public:
    Text(const char * id);
    ~Text();

    // helper: flag bitmask used for QFontMetricsF and QPainter::drawText
    int textFlags() const;

    // helpers:
    double fontAscent() const;
    double fontDescent() const;
    double fontHeight() const;

    // virtual:
    double calcContentWidth() const;
    double calcContentHeight() const;

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // accessors to current text payload:
    QString text() const;

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    void get(Property property, TVar & value) const;

    // helper:
    void copySettings(const Text & src);

    struct TPrivate;
    TPrivate * p_;

    QFont font_;
    Qt::AlignmentFlag alignment_;
    Qt::TextElideMode elide_;

    TVarRef text_;
    ItemRef fontSize_; // in points
    ItemRef supersample_;
    ItemRef maxWidth_;
    ItemRef maxHeight_;
    ItemRef opacity_;
    ColorRef color_;
    ColorRef background_;
  };

}


#endif // YAE_TEXT_H_

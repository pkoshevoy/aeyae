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
    void get(Property property, TVar & value) const;

    struct TPrivate;
    TPrivate * p_;

    QFont font_;
    Qt::AlignmentFlag alignment_;
    Qt::TextElideMode elide_;
    double supersample_;

    TVarRef text_;
    ItemRef fontSize_; // in points
    ItemRef maxWidth_;
    ItemRef maxHeight_;
    ColorRef color_;
  };

}


#endif // YAE_TEXT_H_

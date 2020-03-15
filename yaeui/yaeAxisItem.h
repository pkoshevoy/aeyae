// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 10 21:07:34 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AXIS_ITEM_H_
#define YAE_AXIS_ITEM_H_

// yaeui:
#include "yaeItem.h"
#include "yaePlotItem.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // IDataFormatter
  //
  struct YAEUI_API IDataFormatter
  {
    virtual ~IDataFormatter() {}
    virtual std::string get(double i) const = 0;
  };

  //----------------------------------------------------------------
  // TDataFormatterPtr
  //
  typedef yae::shared_ptr<IDataFormatter> TDataFormatterPtr;


  //----------------------------------------------------------------
  // AxisItem
  //
  class YAEUI_API AxisItem : public Item
  {
  public:
    AxisItem(const char * name);
    virtual ~AxisItem();

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // virtual:
    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const;

    // virtual:
    void get(Property property, Color & value) const;

  protected:
    // intentionally disabled:
    AxisItem(const AxisItem &);
    AxisItem & operator = (const AxisItem &);

    // keep implementation details private:
    struct Private;
    Private * private_;

  public:
    // axist color:
    ColorRef color_;

    // axis start and end positions:
    ItemRef t0_;
    ItemRef t1_;

    // distance between tickmark[i] and tickmark[i + 1]
    ItemRef tick_dt_;

    // label every tickmark[i] where (i mod n == 0):
    ItemRef mark_n_;

    ItemRef font_size_; // in points
    QFont font_;

    // tickmark label formatter:
    TDataFormatterPtr formatter_;
  };

}


#endif // YAE_AXIS_ITEM_H_

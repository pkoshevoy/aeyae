// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Oct  6 10:48:47 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLOT_ITEM_H_
#define YAE_PLOT_ITEM_H_

// local:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // IDataSource
  //
  template <typename TData>
  struct IDataSource
  {
    virtual ~IDataSource() {}
    virtual std::size_t size() const = 0;
    virtual bool get(std::size_t i, TData & v) const = 0;
    virtual bool get_range(TData & min, TData & max) const = 0;
  };


  //----------------------------------------------------------------
  // TDataSource
  //
  typedef IDataSource<double> TDataSource;

  //----------------------------------------------------------------
  // TDataSourcePtr
  //
  typedef yae::shared_ptr<TDataSource> TDataSourcePtr;


  //----------------------------------------------------------------
  // ScaleLinear
  //
  // see https://github.com/d3/d3-scale
  //
  struct YAE_API ScaleLinear
  {
    ScaleLinear(double d0 = 0, double d1 = 1,
                double r0 = 0, double r1 = 1):
      domain_(d0, d1 - d0),
      range_(r0, r1 - r0)
    {}

    inline ScaleLinear & input_domain(double d0, double d1)
    {
      domain_.origin_ = d0;
      domain_.length_ = d1 - d0;
      return *this;
    }

    inline ScaleLinear & output_range(double r0, double r1)
    {
      range_.origin_ = r0;
      range_.length_ = r1 - r0;
      return *this;
    }

    // transform input domain point to output range point:
    inline double operator()(double domain_pt) const
    { return range_.to_wcs(domain_.to_lcs(domain_pt)); }

    // transform output range point to input domain point:
    inline double invert(double range_pt) const
    { return domain_.to_wcs(range_.to_lcs(range_pt)); }

    Segment domain_;
    Segment range_;
  };


  //----------------------------------------------------------------
  // PlotItem
  //
  class YAE_API PlotItem : public Item
  {
  public:
    PlotItem(const char * name, const TDataSourcePtr & d = TDataSourcePtr());
    virtual ~PlotItem();

    void setData(const TDataSourcePtr & data);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const;

    // virtual:
    void get(Property property, Color & value) const;

  protected:
    // intentionally disabled:
    PlotItem(const PlotItem &);
    PlotItem & operator = (const PlotItem &);

    // keep implementation details private:
    struct Private;
    Private * private_;

  public:
    // line color:
    ColorRef color_;
  };

}


#endif // YAE_PLOT_ITEM_H_

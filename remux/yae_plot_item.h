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
    virtual TData get(std::size_t i) const = 0;
    virtual void get_range(TData & min, TData & max) const = 0;

    inline TData operator[](std::size_t i) const
    { return this->get(i); }
  };


  //----------------------------------------------------------------
  // TDataSource
  //
  struct TDataSource : public IDataSource<double>
  {
    inline Segment range() const
    {
      double v[2];
      get_range(v[0], v[1]);
      return Segment(v[0], v[1] - v[0]);
    }
  };

  //----------------------------------------------------------------
  // TDataSourcePtr
  //
  typedef yae::shared_ptr<TDataSource> TDataSourcePtr;


  //----------------------------------------------------------------
  // MockDataSource
  //
  struct YAE_API MockDataSource : public TDataSource
  {
    MockDataSource(std::size_t n = 4096, double v_min = 0, double v_max = 1);

    // virtual:
    std::size_t size() const
    { return data_.size(); }

    // virtual:
    double get(std::size_t i) const
    { return data_[i]; }

    // virtual:
    void get_range(double & min, double & max) const
    {
      min = min_;
      max = max_;
    }

    std::vector<double> data_;
    double min_;
    double max_;
  };


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
    { return this->get(domain_pt); }

    inline double get(double domain_pt) const
    { return range_.to_wcs(domain_.to_lcs(domain_pt)); }

    // transform output range point to input domain point:
    inline double invert(double range_pt) const
    { return domain_.to_wcs(range_.to_lcs(range_pt)); }

    Segment domain_;
    Segment range_;
  };

  //----------------------------------------------------------------
  // TSegmentPtr
  //
  typedef yae::shared_ptr<Segment> TSegmentPtr;


  //----------------------------------------------------------------
  // PlotItem
  //
  class YAE_API PlotItem : public Item
  {
  public:
    PlotItem(const char * name);
    virtual ~PlotItem();

    // here y[i] = f(x[i]), so cardinality of x and y should be equal:
    void set_data(const TDataSourcePtr & data_x,
                  const TDataSourcePtr & data_y);

    void set_domain(const TSegmentPtr & domain);
    void set_range(const TSegmentPtr & range);

    // accessors:
    inline const TDataSourcePtr & data_x() const
    { return data_x_; }

    inline const TDataSourcePtr & data_y() const
    { return data_y_; }

    inline const TSegmentPtr & domain() const
    { return domain_; }

    inline const TSegmentPtr & range() const
    { return range_; }

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

    // y[i] = f(x[i]):
    TDataSourcePtr data_x_;
    TDataSourcePtr data_y_;

    // to compare apples-to-apples (overlapping plots of similar data)
    // make sure to put them on the same scales by using the same domain/range:
    TSegmentPtr domain_;
    TSegmentPtr range_;

  public:
    // line color:
    ColorRef color_;

    // default is 1:
    ItemRef line_width_;
  };

}


#endif // YAE_PLOT_ITEM_H_

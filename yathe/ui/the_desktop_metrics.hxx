// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_desktop_metrics.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Feb 5 15:51:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : The base class for desktop metrics (DPI)

#ifndef THE_DESKTOP_METRICS_HXX_
#define THE_DESKTOP_METRICS_HXX_


//----------------------------------------------------------------
// the_desktop_metrics_t
// 
class the_desktop_metrics_t
{
public:
  virtual ~the_desktop_metrics_t() {}
  
  // dots per inch:
  virtual float dpi_x() const
  {
#ifdef __APPLE__
    return 72;
#else
    return 96;
#endif
  }
  
  virtual float dpi_y() const
  {
#ifdef __APPLE__
    return 72;
#else
    return 96;
#endif
  }
  
  inline float pixels_per_inch() const
  { return dpi_x(); }
  
  inline float pixels_per_millimeter() const
  {
    // 1 inch == 25.4 millimeters:
    return pixels_per_inch() / 25.4f;
  }
};

//----------------------------------------------------------------
// the_desktop_metrics
// 
extern const the_desktop_metrics_t *
the_desktop_metrics(unsigned int desktop = 0);

//----------------------------------------------------------------
// the_desktop_metrics
// 
extern void
the_desktop_metrics(the_desktop_metrics_t * m, unsigned int desktop = 0);


#endif // THE_DESKTOP_METRICS_HXX_

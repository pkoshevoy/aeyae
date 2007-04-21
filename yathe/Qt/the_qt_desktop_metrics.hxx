// File         : the_qt_desktop_metrics.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Feb 5 16:11:00 MST 2007
// Copyright    : (C) 2007
// License      : GPL.
// Description  : The Qt wrapper for desktop metrics (DPI)

#ifndef THE_QT_DESKTOP_METRICS_HXX_
#define THE_QT_DESKTOP_METRICS_HXX_

// local includs:
#include "ui/the_desktop_metrics.hxx"

// Qt includes:
#include <QApplication>
#include <QPaintDevice>
#include <QDesktopWidget>


//----------------------------------------------------------------
// the_qt_desktop_metrics_t
// 
class the_qt_desktop_metrics_t : public the_desktop_metrics_t
{
public:
  // virtual:
  float dpi_x() const
  {
    const QPaintDevice & qpdm = *(QApplication::desktop());
    float dpi = (float)(qpdm.logicalDpiX());
    return dpi;
  }
  
  float dpi_y() const
  {
    const QPaintDevice & qpdm = *(QApplication::desktop());
    float dpi = (float)(qpdm.logicalDpiY());
    return dpi;
  }
};


#endif // THE_QT_DESKTOP_METRICS_HXX_

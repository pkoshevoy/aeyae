// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_desktop_metrics.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Feb 5 16:11:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : The Qt wrapper for desktop metrics (DPI)

#ifndef THE_QT_DESKTOP_METRICS_HXX_
#define THE_QT_DESKTOP_METRICS_HXX_

// local includs:
#include "ui/the_desktop_metrics.hxx"

// Qt includes:
#include <qapplication.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qdesktopwidget.h>

//----------------------------------------------------------------
// the_qt_desktop_metrics_t
//
class the_qt_desktop_metrics_t : public the_desktop_metrics_t
{
public:
  // virtual:
  float dpi_x() const
  {
    QPaintDeviceMetrics qpdm = QPaintDeviceMetrics(QApplication::desktop());
    float dpi = (float)(qpdm.logicalDpiX());
    return dpi;
  }

  float dpi_y() const
  {
    QPaintDeviceMetrics qpdm = QPaintDeviceMetrics(QApplication::desktop());
    float dpi = (float)(qpdm.logicalDpiY());
    return dpi;
  }
};


#endif // THE_QT_DESKTOP_METRICS_HXX_

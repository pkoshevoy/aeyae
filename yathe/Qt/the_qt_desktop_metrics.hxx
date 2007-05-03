/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


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

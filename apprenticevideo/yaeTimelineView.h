// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 23:01:16 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_VIEW_H_
#define YAE_TIMELINE_VIEW_H_

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeItemView.h"


namespace yae
{
  // forward declarations:
  class TimelineControls;

  //----------------------------------------------------------------
  // TimelineView
  //
  class YAE_API TimelineView : public ItemView
  {
    Q_OBJECT;

  public:
    TimelineView();

    // data source:
    void setModel(TimelineControls * model);

  protected:
    TimelineControls * model_;
  };

}


#endif // YAE_TIMELINE_VIEW_H_

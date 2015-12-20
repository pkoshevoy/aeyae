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
  class TimelineModel;

  //----------------------------------------------------------------
  // TimelineView
  //
  class YAE_API TimelineView : public ItemView
  {
    Q_OBJECT;

  public:
    TimelineView();

    // virtual:
    void mouseTracking(const TVec2D & mousePt);

    // data source:
    void setModel(TimelineModel * model);

    inline TimelineModel * model() const
    { return model_; }

  public slots:
    void modelChanged();

  protected:
    TimelineModel * model_;
  };

}


#endif // YAE_TIMELINE_VIEW_H_

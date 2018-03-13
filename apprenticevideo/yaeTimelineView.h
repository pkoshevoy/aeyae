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
  class MainWindow;
  class TimelineModel;
  class PlaylistView;

  //----------------------------------------------------------------
  // TimelineView
  //
  class YAE_API TimelineView : public ItemView
  {
    Q_OBJECT;

  public:
    TimelineView();

    // need to reference playlist view for common style info:
    void setup(MainWindow * mainWindow, PlaylistView * playlist);

    // virtual:
    const ItemViewStyle * style() const;

    // virtual:
    void setEnabled(bool enable);

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // timeline data source:
    void setModel(TimelineModel * model);

    // accessors:
    inline TimelineModel * model() const
    { return model_; }

    inline MainWindow * mainWindow() const
    { return mainWindow_; }

    inline PlaylistView * playlistView() const
    { return playlist_; }

  public slots:
    void modelChanged();

  public:
    // helpers:
    void maybeAnimateOpacity();
    void maybeAnimateControls();
    void forceAnimateControls();

    MainWindow * mainWindow_;
    PlaylistView * playlist_;

  protected:
    TimelineModel * model_;
    TAnimatorPtr animator_;
    TAnimatorPtr animatorForControls_;
  };

}


#endif // YAE_TIMELINE_VIEW_H_

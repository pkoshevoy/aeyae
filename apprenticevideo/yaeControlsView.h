// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 22 19:20:22 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CONTROLS_VIEW_H_
#define YAE_CONTROLS_VIEW_H_

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeItemView.h"


namespace yae
{
  // forward declarations:
  class MainWindow;
  class PlaylistView;

  //----------------------------------------------------------------
  // ControlsView
  //
  class YAE_API ControlsView : public ItemView
  {
    Q_OBJECT;

  public:
    ControlsView();

    // need to reference playlist view for common style info:
    void setup(MainWindow * mainWindow, PlaylistView * playlist);

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // accessors:
    inline MainWindow * mainWindow() const
    { return mainWindow_; }

    inline PlaylistView * playlistView() const
    { return playlist_; }

  public slots:
    void controlsChanged();

  public:
    MainWindow * mainWindow_;
    PlaylistView * playlist_;
  };

}


#endif // YAE_CONTROLS_VIEW_H_

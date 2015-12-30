// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Jul 24 22:10:26 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SCREENSAVER_INHIBITOR_H_
#define YAE_SCREENSAVER_INHIBITOR_H_

// Qt includes:
#include <QObject>
#include <QTimer>

// yae includes:
#include "yae/api/yae_api.h"


namespace yae
{

  //----------------------------------------------------------------
  // ScreenSaverInhibitor
  //
  class YAE_API ScreenSaverInhibitor : public QObject
  {
    Q_OBJECT;

  public:
    ScreenSaverInhibitor();

  public slots:
    void screenSaverInhibit();
    void screenSaverUnInhibit();

  protected:
    // single shot timers for (un)inhibiting screen saver:
    QTimer timerScreenSaver_;
    QTimer timerScreenSaverUnInhibit_;

    unsigned int cookie_;
  };

}


#endif // YAE_SCREENSAVER_INHIBITOR_H_

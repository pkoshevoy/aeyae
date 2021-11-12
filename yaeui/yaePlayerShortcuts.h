// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Nov 11 18:41:01 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_SHORTCUTS_H_
#define YAE_PLAYER_SHORTCUTS_H_

// Qt:
#include <QShortcut>
#include <QWidget>


namespace yae
{
  //----------------------------------------------------------------
  // PlayerShortcuts
  //
  struct PlayerShortcuts
  {
    PlayerShortcuts(QWidget * parent);

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut fullScreen_;
    QShortcut fillScreen_;
    QShortcut showTimeline_;
    QShortcut play_;
    QShortcut loop_;
    QShortcut cropNone_;
    QShortcut autoCrop_;
    QShortcut crop1_33_;
    QShortcut crop1_78_;
    QShortcut crop1_85_;
    QShortcut crop2_40_;
    QShortcut cropOther_;
    QShortcut nextChapter_;
    QShortcut aspectRatioNone_;
    QShortcut aspectRatio1_33_;
    QShortcut aspectRatio1_78_;
  };

}


#endif // YAE_PLAYER_SHORTCUTS_H_

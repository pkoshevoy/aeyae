// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Nov 11 18:43:42 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local:
#include "yaePlayerShortcuts.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerShortcuts::PlayerShortcuts
  //
  PlayerShortcuts::PlayerShortcuts(QWidget * parent):
    fullScreen_(parent),
    fillScreen_(parent),
    showTimeline_(parent),
    play_(parent),
    loop_(parent),
    cropNone_(parent),
    autoCrop_(parent),
    crop1_33_(parent),
    crop1_78_(parent),
    crop1_85_(parent),
    crop2_40_(parent),
    cropOther_(parent),
    nextChapter_(parent),
    aspectRatioNone_(parent),
    aspectRatio1_33_(parent),
    aspectRatio1_78_(parent)
  {
    fullScreen_.setContext(Qt::ApplicationShortcut);
    fillScreen_.setContext(Qt::ApplicationShortcut);
    showTimeline_.setContext(Qt::ApplicationShortcut);
    play_.setContext(Qt::ApplicationShortcut);
    nextChapter_.setContext(Qt::ApplicationShortcut);
    loop_.setContext(Qt::ApplicationShortcut);
    cropNone_.setContext(Qt::ApplicationShortcut);
    crop1_33_.setContext(Qt::ApplicationShortcut);
    crop1_78_.setContext(Qt::ApplicationShortcut);
    crop1_85_.setContext(Qt::ApplicationShortcut);
    crop2_40_.setContext(Qt::ApplicationShortcut);
    cropOther_.setContext(Qt::ApplicationShortcut);
    autoCrop_.setContext(Qt::ApplicationShortcut);
    aspectRatioNone_.setContext(Qt::ApplicationShortcut);
    aspectRatio1_33_.setContext(Qt::ApplicationShortcut);
    aspectRatio1_78_.setContext(Qt::ApplicationShortcut);
  }

}

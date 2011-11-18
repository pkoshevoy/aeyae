// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed Nov 16 22:02:00 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APPLE_REMOTE_CONTROL_H_
#define YAE_APPLE_REMOTE_CONTROL_H_

namespace yae
{
  
  //----------------------------------------------------------------
  // TRemoteControlButtonId
  // 
  enum TRemoteControlButtonId {
    kRemoteControlButtonUndefined = 0,
    kRemoteControlVolumeUp        = 1,
    kRemoteControlVolumeDown      = 2,
    kRemoteControlMenuButton      = 3,
    kRemoteControlPlayButton      = 4,
    kRemoteControlLeftButton      = 5,
    kRemoteControlRightButton     = 6
  };
  
  //----------------------------------------------------------------
  // TRemoteControlObserver
  // 
  typedef void(*TRemoteControlObserver)(void * /* observerContext */,
                                        TRemoteControlButtonId /* button */,
                                        bool /* pressed down */,
                                        unsigned int /* click count */,
                                        bool /* held down */);

  //----------------------------------------------------------------
  // appleRemoteControlOpen
  // 
  void * appleRemoteControlOpen(bool exclusive,
                                bool countClicks,
                                bool simulateHold,
                                TRemoteControlObserver observer,
                                void * observerContext);

  //----------------------------------------------------------------
  // appleRemoteControlClose
  // 
  void appleRemoteControlClose(void * appleRemoteControl);
  
}

#endif // YAE_APPLE_REMOTE_CONTROL_H_

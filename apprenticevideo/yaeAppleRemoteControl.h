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
    kRemoteControlVolumeUp,
    kRemoteControlVolumeDown,
    kRemoteControlMenuButton,
    kRemoteControlTogglePlayback,
    kRemoteControlLeftButton,
    kRemoteControlRightButton
  };
  
  //----------------------------------------------------------------
  // TRemoteControlObserver
  // 
  typedef void(*TRemoteControlObserver)(void * /* observerContext */,
                                        TRemoteControlButtonId /* button */,
                                        bool /* pressed */,
                                        bool /* held down autorepeat */,
                                        unsigned int /* click count */);

  //----------------------------------------------------------------
  // appleRemoteControlOpen
  // 
  void * appleRemoteControlOpen(bool exclusive,
                                bool countClicks);

  //----------------------------------------------------------------
  // appleRemoteControlClose
  // 
  void appleRemoteControlClose(void * appleRemoteControl);

  //----------------------------------------------------------------
  // appleRemoteControlSetObserver
  // 
  void appleRemoteControlSetObserver(void * appleRemoteControl,
                                     TRemoteControlObserver observer,
                                     void * observerContext);
  
}

#endif // YAE_APPLE_REMOTE_CONTROL_H_

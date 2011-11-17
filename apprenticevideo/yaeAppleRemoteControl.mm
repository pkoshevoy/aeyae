// -*- Mode: objc; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed Nov 16 22:35:52 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// Apple imports:
#import <Cocoa/Cocoa.h>

// local imports:
#include "yaeAppleRemoteControl.h"
#import "AppleRemote.h"
#import "MultiClickRemoteBehavior.h"


//----------------------------------------------------------------
// MainController
// 
@interface MainController : NSObject
{
  RemoteControl * remoteControl;
  MultiClickRemoteBehavior * remoteBehavior;
}
@end

@implementation MainController 

//----------------------------------------------------------------
// dealloc
// 
- (void) dealloc {
	[remoteControl autorelease];
	[remoteBehavior autorelease];
	[super dealloc];
}

//----------------------------------------------------------------
// remoteButton:
// 
// delegate method for the MultiClickRemoteBehavior
// 
- (void) remoteButton: (RemoteControlEventIdentifier)buttonIdentifier
          pressedDown: (BOOL) pressedDown
           clickCount: (unsigned int)clickCount
{
  NSString * buttonName = nil;

  switch (buttonIdentifier)
  {
    case kRemoteButtonPlus:
      buttonName = @"Volume up";
      break;

    case kRemoteButtonMinus:
      buttonName = @"Volume down";
      break;

    case kRemoteButtonMenu:
      buttonName = @"Menu";
      break;

    case kRemoteButtonPlay:
      buttonName = @"Play";
      break;

    case kRemoteButtonRight:
      buttonName = @"Right";
      break;

    case kRemoteButtonLeft:
      buttonName = @"Left";
      break;

    case kRemoteButtonRight_Hold:
      buttonName = @"Right holding";
      break;

    case kRemoteButtonLeft_Hold:
      buttonName = @"Left holding";
      break;

    case kRemoteButtonPlus_Hold:
      buttonName = @"Volume up holding";
      break;

    case kRemoteButtonMinus_Hold:
      buttonName = @"Volume down holding";
      break;

    case kRemoteButtonPlay_Hold:
      buttonName = @"Play (sleep mode)";
      break;

    case kRemoteButtonMenu_Hold:
      buttonName = @"Menu (long)";
      break;

    case kRemoteControl_Switched:
      buttonName = @"Remote Control Switched";
      break;

    default:
      NSLog(@"Unmapped event for button %d", buttonIdentifier);
      break;
  }
}

@end

namespace yae
{

  //----------------------------------------------------------------
  // appleRemoteControlOpen
  // 
  void * appleRemoteControlOpen(bool exclusive, bool countClicks)
  {
    return NULL;
  }

  //----------------------------------------------------------------
  // appleRemoteControlClose
  // 
  void appleRemoteControlClose(void * appleRemoteControl)
  {}

  //----------------------------------------------------------------
  // appleRemoteControlSetObserver
  // 
  void appleRemoteControlSetObserver(void * appleRemoteControl,
                                     TRemoteControlObserver observer,
                                     void * observerContext)
  {}
  
}

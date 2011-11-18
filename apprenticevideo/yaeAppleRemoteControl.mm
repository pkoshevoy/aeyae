// -*- Mode: objc; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed Nov 16 22:35:52 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// Apple imports:
#import <Cocoa/Cocoa.h>

// local imports:
#import "yaeAppleRemoteControl.h"
#import "AppleRemote.h"
#import "MultiClickRemoteBehavior.h"


//----------------------------------------------------------------
// TAppleRemoteControlBridge
// 
@interface TAppleRemoteControlBridge : NSObject
{
  RemoteControl * rc_;
  MultiClickRemoteBehavior * rcBehavior_;
  yae::TRemoteControlObserver observer_;
  void * observerContext_;
}

// for bindings access:
- (RemoteControl *) remoteControl;
- (void) setRemoteControl: (RemoteControl*) newControl;

// constructor:
- (id) initWithExclusiveAccess: (BOOL) exclusiveAccess
                   countClicks: (BOOL) countClicks
                      observer: (yae::TRemoteControlObserver) observer
               observerContext: (void *) observerContext;

// event notification:
- (void) remoteButton: (RemoteControlEventIdentifier) buttonIdentifier
          pressedDown: (BOOL) pressedDown
           clickCount: (unsigned int) clickCount;

@end

@implementation TAppleRemoteControlBridge

//----------------------------------------------------------------
// dealloc
// 
- (void) dealloc
{
  [rc_ stopListening: self];
  [rc_ autorelease];
  [rcBehavior_ autorelease];
  [super dealloc];
}

//----------------------------------------------------------------
// remoteControl
// 
- (RemoteControl *) remoteControl
{
  return rc_;
}

//----------------------------------------------------------------
// setRemoteControl
// 
- (void) setRemoteControl: (RemoteControl *) newControl
{
  [rc_ autorelease];
  rc_ = [newControl retain];
}

//----------------------------------------------------------------
// initWithExclusiveAccess
// 
- (id) initWithExclusiveAccess: (BOOL) exclusiveAccess
                   countClicks: (BOOL) countClicks
                      observer: (yae::TRemoteControlObserver) observer
               observerContext: (void *) observerContext
{
  AppleRemote * newRemoteControl = [[[AppleRemote alloc] initWithDelegate: self] autorelease];
  [newRemoteControl setDelegate: self];

  // The MultiClickRemoteBehavior adds extra functionality.
  // It works like a middle man between the delegate and the remote control
  MultiClickRemoteBehavior * newRemoteBehavior = [MultiClickRemoteBehavior new];
  [newRemoteBehavior setDelegate: self];
  [newRemoteControl setDelegate: newRemoteBehavior];

  // set new remote control which will update bindings
  [self setRemoteControl: newRemoteControl];

  [rcBehavior_ autorelease];
  rcBehavior_ = newRemoteBehavior;

  observer_ = observer;
  observerContext_ = observerContext;

  [rc_ startListening: self];

  return self;
}

//----------------------------------------------------------------
// remoteButton:
// 
// delegate method for the MultiClickRemoteBehavior
// 
- (void) remoteButton: (RemoteControlEventIdentifier) buttonIdentifier
          pressedDown: (BOOL) pressedDown
           clickCount: (unsigned int) clickCount
{
  if (!observer_)
  {
    return;
  }
  
  yae::TRemoteControlButtonId buttonId = yae::kRemoteControlButtonUndefined;
  bool heldDown = false;
  
  switch (buttonIdentifier)
  {
    case kRemoteButtonPlus:
    case kRemoteButtonPlus_Hold:
      buttonId = yae::kRemoteControlVolumeUp;
      heldDown = (buttonIdentifier == kRemoteButtonPlus_Hold);
      break;

    case kRemoteButtonMinus:
    case kRemoteButtonMinus_Hold:
      buttonId = yae::kRemoteControlVolumeDown;
      heldDown = (buttonIdentifier == kRemoteButtonMinus_Hold);
      break;

    case kRemoteButtonMenu:
    case kRemoteButtonMenu_Hold:
      buttonId = yae::kRemoteControlMenuButton;
      heldDown = (buttonIdentifier == kRemoteButtonMenu_Hold);
      break;

    case kRemoteButtonPlay:
    case kRemoteButtonPlay_Hold:
      buttonId = yae::kRemoteControlPlayButton;
      heldDown = (buttonIdentifier == kRemoteButtonPlay_Hold);
      break;

    case kRemoteButtonLeft:
    case kRemoteButtonLeft_Hold:
      buttonId = yae::kRemoteControlLeftButton;
      heldDown = (buttonIdentifier == kRemoteButtonLeft_Hold);
      break;

    case kRemoteButtonRight:
    case kRemoteButtonRight_Hold:
      buttonId = yae::kRemoteControlRightButton;
      heldDown = (buttonIdentifier == kRemoteButtonRight_Hold);
      break;

    case kRemoteControl_Switched:
      NSLog(@"Remote control switched");
      return;

    default:
      NSLog(@"Unmapped event for button %d", buttonIdentifier);
      return;
  }
  
  // forward to the observer:
  observer_(observerContext_, buttonId, pressedDown, clickCount, heldDown);
}

@end

namespace yae
{

  //----------------------------------------------------------------
  // appleRemoteControlOpen
  // 
  void * appleRemoteControlOpen(bool exclusiveAccess,
                                bool countClicks,
                                TRemoteControlObserver observer,
                                void * observerContext)
  {
    TAppleRemoteControlBridge * rc =
      [[TAppleRemoteControlBridge alloc] initWithExclusiveAccess:exclusiveAccess
                                         countClicks:countClicks
                                         observer:observer
                                         observerContext:observerContext];
    return rc;
  }

  //----------------------------------------------------------------
  // appleRemoteControlClose
  // 
  void appleRemoteControlClose(void * appleRemoteControl)
  {
    if (appleRemoteControl)
    {
      TAppleRemoteControlBridge * rc =
        (TAppleRemoteControlBridge *)appleRemoteControl;
      [rc release];
    }
  }
  
}

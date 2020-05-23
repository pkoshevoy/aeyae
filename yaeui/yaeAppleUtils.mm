// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <vector>
#include <string>

// boost:
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

// Apple imports:
#import <Cocoa/Cocoa.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
#include <objc/runtime.h>
#include <objc/message.h>
#else
#include <objc/objc-runtime.h>
#endif

// aeyae:
#include "yae/api/yae_api.h"

// local:
#include "yaeAppleUtils.h"


namespace yae
{

  //----------------------------------------------------------------
  // stringFrom
  //
  static std::string
  stringFrom(CFStringRef cfStr)
  {
    std::string result;

    CFIndex strLen = CFStringGetLength(cfStr);
    CFIndex bufSize = CFStringGetMaximumSizeForEncoding(strLen + 1,
                                                        kCFStringEncodingUTF8);

    std::vector<char> buffer(bufSize + 1);
    char * buf = &buffer[0];

    if (CFStringGetCString(cfStr,
                           buf,
                           bufSize,
                           kCFStringEncodingUTF8))
    {
      result = buf;
    }

    return result;
  }

  //----------------------------------------------------------------
  // absoluteUrlFrom
  //
  std::string
  absoluteUrlFrom(const char * utf8_url)
  {
    std::string result(utf8_url);

#if !(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060)
    CFStringRef cfStr = CFStringCreateWithCString(kCFAllocatorDefault,
                                                  utf8_url,
                                                  kCFStringEncodingUTF8);
    if (cfStr)
    {
      CFURLRef cfUrl = CFURLCreateWithString(kCFAllocatorDefault,
                                             cfStr,
                                             NULL);
      if (cfUrl)
      {
        CFErrorRef error = 0;
        CFURLRef cfUrlAbs = CFURLCreateFilePathURL(kCFAllocatorDefault,
                                                   cfUrl,
                                                   &error);
        if (cfUrlAbs)
        {
          CFStringRef cfStrAbsUrl = CFURLGetString(cfUrlAbs);
          result = stringFrom(cfStrAbsUrl);

          CFRelease(cfUrlAbs);
        }

        CFRelease(cfUrl);
      }

      CFRelease(cfStr);
    }
#endif

    return result;
  }

  //----------------------------------------------------------------
  // PreventAppNap::Private
  //
  // tell App Nap that this is latency critical
  //
  struct PreventAppNap::Private
  {
    struct Activity
    {
      id activity_;
      uint64_t count_;

      Activity():
        activity_(NULL),
        count_(0)
      {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
        NSProcessInfo * processInfo = [NSProcessInfo processInfo];
        if (!processInfo)
        {
          return;
        }

        // create a reason string
        NSString * because =
          [[NSString alloc] initWithUTF8String:"yae::PreventAppNap"];

        // start activity that tells App Nap to mind its own business:
        // (NSActivityUserInitiatedAllowingIdleSystemSleep |
        //  NSActivityLatencyCritical)
        NSActivityOptions opts = 0x00FFFFFFULL | 0xFF00000000ULL;
        activity_ = [processInfo
                     beginActivityWithOptions:opts
                     reason:because];
        [activity_ retain];
        [because release];
#endif
      }

      ~Activity()
      {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
        if (activity_)
        {
          NSProcessInfo * processInfo = [NSProcessInfo processInfo];
          if (!processInfo)
          {
            return;
          }

          [processInfo endActivity: activity_];
          [activity_ release];
        }
#endif
      }
    };

    static boost::mutex mutex_;
    static Activity * singleton_;

    Private()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (!singleton_)
      {
        singleton_ = new Activity();
      }

      singleton_->count_++;
    }

    ~Private()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (singleton_)
      {
        singleton_->count_--;
        if (!singleton_->count_)
        {
          delete singleton_;
          singleton_ = NULL;
        }
      }
    }
  };

  //----------------------------------------------------------------
  // PreventAppNap::Private::mutex_
  //
  boost::mutex
  PreventAppNap::Private::mutex_;

  //----------------------------------------------------------------
  // PreventAppNap::Private::singleton_
  //
  PreventAppNap::Private::Activity *
  PreventAppNap::Private::singleton_;

  //----------------------------------------------------------------
  // PreventAppNap::PreventAppNap
  //
  PreventAppNap::PreventAppNap():
    private_(new PreventAppNap::Private())
  {}

  //----------------------------------------------------------------
  // PreventAppNap::~PreventAppNap
  //
  PreventAppNap::~PreventAppNap()
  {
    delete private_;
  }


  //----------------------------------------------------------------
  // AppleApp::Private
  //
  struct AppleApp::Private
  {
    Private();
    ~Private();

  private:
    NSObject * obj_;
  };

}


//----------------------------------------------------------------
// query_dark_mode
//
static bool
query_dark_mode()
{
  bool dark = false;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
  NSAppearance * ea = [NSApp effectiveAppearance];
  NSString * ns_appearance = [ea name];
  dark =
    (ns_appearance == NSAppearanceNameDarkAqua ||
     ns_appearance == NSAppearanceNameAccessibilityHighContrastDarkAqua ||
     ns_appearance == NSAppearanceNameAccessibilityHighContrastVibrantDark);
#endif

  return dark;
}


//----------------------------------------------------------------
// TApplicationBridge
//
@interface TApplicationBridge : NSObject {}
- (id) init;
- (void) themeChanged: (NSNotification *) notification;
@end

@implementation TApplicationBridge

//----------------------------------------------------------------
// init
//
- (id) init
{
  self = [super init];

  NSDistributedNotificationCenter * notifications =
    [NSDistributedNotificationCenter defaultCenter];

  [notifications addObserver:self
                    selector:@selector(themeChanged:)
                        name:@"AppleInterfaceThemeChangedNotification"
                      object:nil];
  return self;
}

//----------------------------------------------------------------
// themeChanged:
//
- (void) themeChanged: (NSNotification *) notification
{
  qApp->postEvent(qApp, new yae::ThemeChangedEvent());
}

@end


namespace yae
{

  //----------------------------------------------------------------
  // AppleApp::AppleApp
  //
  AppleApp::Private::Private():
    obj_(NULL)
  {
    obj_ = [[TApplicationBridge alloc] init];
  }

  //----------------------------------------------------------------
  // AppleApp::AppleApp
  //
  AppleApp::Private::~Private()
  {
    [obj_ release];
  }

  //----------------------------------------------------------------
  // AppleApp::AppleApp
  //
  AppleApp::AppleApp():
    private_(new AppleApp::Private())
  {}

  //----------------------------------------------------------------
  // AppleApp::~AppleApp
  //
  AppleApp::~AppleApp()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // AppleApp::query_dark_mode
  //
  bool
  AppleApp::query_dark_mode() const
  {
    return ::query_dark_mode();
  }

}

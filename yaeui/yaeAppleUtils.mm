// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <vector>
#include <string>

// posix:
#include <pthread.h>

// system:
#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
#include <objc/runtime.h>
#include <objc/message.h>
#else
#include <objc/objc-runtime.h>
#endif

// yaeui:
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
  // showInFinder
  //
  void
  showInFinder(const char * utf8_path)
  {
    NSString * path = [NSString stringWithUTF8String:utf8_path];
    NSString * root = [path stringByDeletingLastPathComponent];
    [[NSWorkspace sharedWorkspace]
      selectFile:path
      inFileViewerRootedAtPath:root];
  }

  //----------------------------------------------------------------
  // Mutex
  //
  struct Mutex
  {
    Mutex()
    {
      pthread_mutex_init(&mutex_, NULL);
    }

    ~Mutex()
    {
      pthread_mutex_destroy(&mutex_);
    }

    inline int lock()
    {
      return pthread_mutex_lock(&mutex_);
    }

    inline int trylock()
    {
      return pthread_mutex_trylock(&mutex_);
    }

    inline int unlock()
    {
      return pthread_mutex_unlock(&mutex_);
    }

  private:
    Mutex(const Mutex &);
    Mutex & operator = (const Mutex &);

    pthread_mutex_t mutex_;
  };


  //----------------------------------------------------------------
  // Lock
  //
  template <typename TMutex>
  struct Lock
  {
    Lock(TMutex & mutex, bool lock_now = true):
      mutex_(mutex),
      locked_(false)
    {
      if (lock_now)
      {
        lock();
      }
    }

    ~Lock()
    {
      unlock();
    }

    bool lock()
    {
      if (!locked_)
      {
        int err = mutex_.lock();
        locked_ = !err;
      }

      return locked_;
    }

    bool trylock()
    {
      if (!locked_)
      {
        int err = mutex_.trylock();
        locked_ = !err;
      }

      return locked_;
    }

    bool unlock()
    {
      if (locked_)
      {
        int err = mutex_.unlock();
        locked_ = !(!err);
      }

      return !locked_;
    }

  private:
    Lock(const Lock &);
    Lock & operator = (const Lock &);

    TMutex & mutex_;
    bool locked_;
  };

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

    static Mutex mutex_;
    static Activity * singleton_;

    Private()
    {
      Lock<Mutex> lock(mutex_);
      if (!singleton_)
      {
        singleton_ = new Activity();
      }

      singleton_->count_++;
    }

    ~Private()
    {
      Lock<Mutex> lock(mutex_);
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
  Mutex
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
  SEL sel = NSSelectorFromString(@"effectiveAppearance");
  if ([NSApp respondsToSelector:sel])
  {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
    NSAppearance * ea = [NSApp performSelector:sel];
    NSString * ns_appearance = [ea name];
    dark =
      (ns_appearance == NSAppearanceNameDarkAqua ||
       ns_appearance == NSAppearanceNameAccessibilityHighContrastDarkAqua ||
       ns_appearance == NSAppearanceNameAccessibilityHighContrastVibrantDark);
#endif
  }

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

//----------------------------------------------------------------
// TApplicationNotificationObserver
//
@interface TApplicationNotificationObserver : NSObject {}
- (id) init;
- (void) applicationWillFinishLaunching: (NSNotification *) notification;
- (void) applicationDidFinishLaunching: (NSNotification *) notification;
@end

@implementation TApplicationNotificationObserver

//----------------------------------------------------------------
// init
//
- (id) init
{
  self = [super init];
  NSNotificationCenter * notifications =
    [NSNotificationCenter defaultCenter];

  NSLog(@"addObserver NSApplicationWillFinishLaunchingNotification");
  [notifications addObserver:self
                 selector:@selector(applicationWillFinishLaunching:)
                 name:@"NSApplicationWillFinishLaunchingNotification"
                 object:nil];

  NSLog(@"addObserver NSApplicationDidFinishLaunchingNotification");
  [notifications addObserver:self
                 selector:@selector(applicationDidFinishLaunching:)
                 name:@"NSApplicationDidFinishLaunchingNotification"
                 object:nil];
  return self;
}

//----------------------------------------------------------------
// applicationWillFinishLaunching:
//
- (void) applicationWillFinishLaunching: (NSNotification *) notification
{
  yae::transform_process_type_to_foreground_app();

  NSNotificationCenter * notifications =
    [NSNotificationCenter defaultCenter];

  NSLog(@"removeObserver NSApplicationWillFinishLaunchingNotification");

  [notifications removeObserver:self
                 name:@"NSApplicationWillFinishLaunchingNotification"
                 object:nil];
}

//----------------------------------------------------------------
// applicationDidFinishLaunching:
//
- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
  // yae::transform_process_type_to_foreground_app();

  NSNotificationCenter * notifications =
    [NSNotificationCenter defaultCenter];

  NSLog(@"removeObserver NSApplicationDidFinishLaunchingNotification");

  [notifications removeObserver:self
                 name:@"NSApplicationDidFinishLaunchingNotification"
                 object:nil];
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

  //----------------------------------------------------------------
  // transform_process_type_to_foreground_app
  //
  void
  transform_process_type_to_foreground_app()
  {
    NSLog(@"transform_process_type_to_foreground_app");

    // show the Dock icon:
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  }

  //----------------------------------------------------------------
  // setup_transform_process_type_to_foreground_app
  //
  void
  setup_transform_process_type_to_foreground_app()
  {
    static TApplicationNotificationObserver * ao =
      [[TApplicationNotificationObserver alloc] init];
  }

}

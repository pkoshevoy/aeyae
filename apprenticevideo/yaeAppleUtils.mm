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
      uint64_t count_;
      Class class_NSProcessInfo_;
      Class class_NSString_;
      SEL sel_processInfo_;
      SEL sel_beginActivityWithOptions_;
      SEL sel_endActivity_;
      SEL sel_alloc_;
      SEL sel_initWithUTF8String_;
      id processInfo_;
      id reason_;
      id activity_;

      Activity():
        count_(0),
        class_NSProcessInfo_((Class)objc_getClass("NSProcessInfo")),
        class_NSString_((Class)objc_getClass("NSString")),
        sel_processInfo_(sel_getUid("processInfo")),
        sel_beginActivityWithOptions_(sel_getUid("beginActivityWithOptions:"
                                                 "reason:")),
        sel_endActivity_(sel_getUid("endActivity:")),
        sel_alloc_(sel_getUid("alloc")),
        sel_initWithUTF8String_(sel_getUid("initWithUTF8String:")),
        processInfo_(NULL),
        reason_(NULL),
        activity_(NULL)
      {
        if (!(class_NSProcessInfo_ &&
              class_NSString_ &&
              class_getClassMethod(class_NSProcessInfo_,
                                   sel_processInfo_) &&
              class_getInstanceMethod(class_NSProcessInfo_,
                                      sel_beginActivityWithOptions_)))
        {
          return;
        }

        processInfo_ = objc_msgSend((id)class_NSProcessInfo_,
                                    sel_processInfo_);
        if (!processInfo_)
        {
          return;
        }

        // create a reason string
        reason_ = objc_msgSend(class_NSString_, sel_alloc_);
        reason_ = objc_msgSend(reason_,
                               sel_initWithUTF8String_,
                               "Timing Crititcal");

        // start activity that tells App Nap to mind its own business:
        // (NSActivityUserInitiatedAllowingIdleSystemSleep |
        //  NSActivityLatencyCritical)
        activity_ = objc_msgSend(processInfo_,
                                 sel_beginActivityWithOptions_,
                                 0x00FFFFFFULL | 0xFF00000000ULL,
                                 reason_);
      }

      ~Activity()
      {
        if (activity_)
        {
          objc_msgSend(processInfo_, sel_endActivity_, activity_);
        }
      }
    };

    static boost::mutex mutex_;
    static Activity * activity_;

    Private()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (!activity_)
      {
        activity_ = new Activity();
      }

      activity_->count_++;
    }

    ~Private()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (activity_)
      {
        activity_->count_--;
        if (!activity_->count_)
        {
          delete activity_;
          activity_ = NULL;
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
  // PreventAppNap::Private::activity_
  //
  PreventAppNap::Private::Activity *
  PreventAppNap::Private::activity_;

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

}

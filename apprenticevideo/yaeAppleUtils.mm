// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <vector>
#include <string>

// Apple imports:
#import <Cocoa/Cocoa.h>


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
}

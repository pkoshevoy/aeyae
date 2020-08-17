// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APPLE_UTILS_H_
#define YAE_APPLE_UTILS_H_

// standard:
#include <string>

// yaeui:
#include "yaeApplication.h"


namespace yae
{
  //----------------------------------------------------------------
  // absoluteUrlFrom
  //
  std::string absoluteUrlFrom(const char * utf8_url);

  //----------------------------------------------------------------
  // PreventAppNap
  //
  struct PreventAppNap
  {
    PreventAppNap();
    ~PreventAppNap();

  private:
    // intentionally disabled:
    PreventAppNap(const PreventAppNap &);
    PreventAppNap & operator = (const PreventAppNap &);

    struct Private;
    Private * private_;
  };


  //----------------------------------------------------------------
  // AppleApp
  //
  struct AppleApp : public yae::Application::Private
  {
    AppleApp();
    ~AppleApp();

    // virtual:
    bool query_dark_mode() const;

    struct Private;
    Private * private_;

  private:
    // intentionally disabled:
    AppleApp(const AppleApp &);
    AppleApp & operator = (const AppleApp &);
  };
}


#endif // YAE_APPLE_UTILS_H_

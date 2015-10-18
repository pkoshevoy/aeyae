// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Oct 17 15:47:01 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APPLE_UTILS_H_
#define YAE_APPLE_UTILS_H_

// standard C++:
#include <string>


namespace yae
{
  //----------------------------------------------------------------
  // absoluteUrlFrom
  //
  std::string absoluteUrlFrom(const char * utf8_url);
}


#endif // YAE_APPLE_UTILS_H_

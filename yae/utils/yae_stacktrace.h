// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Nov 23 19:28:16 MST 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_STACKTRACE_H_
#define YAE_STACKTRACE_H_

// standard:
#include <string>


namespace yae
{

  //----------------------------------------------------------------
  // StackTrace
  //
  struct YAE_API StackTrace
  {
    StackTrace();
    StackTrace(const StackTrace & bt);
    ~StackTrace();

    StackTrace & operator = (const StackTrace & bt);

    std::string to_str(std::size_t offset = 2, const char * sep = "\n") const;

  protected:
    struct Private;
    Private * private_;
  };

  //----------------------------------------------------------------
  // get_stacktrace_str
  //
  YAE_API std::string
  get_stacktrace_str(std::size_t offset = 2, const char * sep = "\n");

}


#endif // YAE_STACKTRACE_H_

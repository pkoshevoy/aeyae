// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Sep  1 11:43:52 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASSERT_H_
#define YAE_ASSERT_H_

// standard:
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>

// aeyae:
#include "../api/yae_log.h"

namespace yae
{
  //----------------------------------------------------------------
  // StackFrame
  //
  struct YAE_API StackFrame
  {
    std::string module_;
    std::string func_;
    std::string offset_;
    std::string address_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & os, const StackFrame & f);

  //----------------------------------------------------------------
  // demangle
  //
  YAE_API void
  demangle(StackFrame & frame, const char * line);

  //----------------------------------------------------------------
  // capture_backtrace
  //
  YAE_API void
  capture_backtrace(std::list<StackFrame> & backtrace, std::size_t offset = 2);

  //----------------------------------------------------------------
  // dump
  //
  YAE_API std::ostream &
  dump(std::ostream & os, const std::list<StackFrame> & traceback);

  //----------------------------------------------------------------
  // dump_stacktrace
  //
  YAE_API std::ostream &
  dump_stacktrace(std::ostream & os);

  //----------------------------------------------------------------
  // get_stacktrace_str
  //
  YAE_API std::string get_stacktrace_str();

}

//----------------------------------------------------------------
// YAE_BREAKPOINT
//
#if defined(__APPLE__)
#  if defined(__ppc__)
#    if __GNUC__ <= 4
#      define YAE_BREAKPOINT() __asm { trap }
#    else
#      define YAE_BREAKPOINT() asm("trap")
#    endif
#  else
#    define YAE_BREAKPOINT() asm("int $3")
#  endif
#elif __GNUC__
#  define YAE_BREAKPOINT() asm("int $3")
#else
#  define YAE_BREAKPOINT()
#endif

//----------------------------------------------------------------
// YAE_BREAKPOINT_IF
//
#define YAE_BREAKPOINT_IF(expr) if (!(expr)) {} else YAE_BREAKPOINT()

//----------------------------------------------------------------
// YAE_BREAKPOINT_IF_DEBUG_BUILD
//
#ifndef NDEBUG
#define YAE_BREAKPOINT_IF_DEBUG_BUILD() YAE_BREAKPOINT()
#else
#define YAE_BREAKPOINT_IF_DEBUG_BUILD()
#endif

//----------------------------------------------------------------
// YAE_ASSERT
//
#define YAE_ASSERT(expr) if (!(expr)) do {              \
      yae::log(yae::TLog::kError,                       \
               __FILE__ ":" YAE_STR(__LINE__),          \
               "assertion failed: %s, stacktrace:\n%s", \
               YAE_STR(expr),                           \
               yae::get_stacktrace_str().c_str());      \
      YAE_BREAKPOINT_IF_DEBUG_BUILD();                  \
    } while (false)

//----------------------------------------------------------------
// YAE_EXPECT
//
#define YAE_EXPECT(expr) if (!(expr)) do {       \
      yae::log(yae::TLog::kError,                \
               __FILE__ ":" YAE_STR(__LINE__),   \
               "unexpected condition: %s",       \
               YAE_STR(expr));                   \
    } while (false)

//----------------------------------------------------------------
// YAE_THROW_IF
//
#define YAE_THROW_IF(expr) if ((expr)) do {      \
      yae::log(yae::TLog::kError,                \
               __FILE__ ":" YAE_STR(__LINE__),   \
               "%s",                             \
               YAE_STR(expr));                   \
      throw std::runtime_error(YAE_STR(expr));   \
    } while (false)

//----------------------------------------------------------------
// YAE_THROW
//
#define YAE_THROW(fmt, ...) do {                                        \
    std::string msg = yae::strfmt((fmt), ##__VA_ARGS__);                \
    msg = yae::strfmt("%s:%i, %s", __FILE__, __LINE__, msg.c_str());    \
    throw std::runtime_error(msg);                                      \
  } while (false)

//----------------------------------------------------------------
// YAE_RETURN_IF
//
#define YAE_RETURN_IF(expr) if ((expr)) do { return; } while (false)



#endif // YAE_ASSERT_H_

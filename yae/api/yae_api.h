// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:09:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_API_H_
#define YAE_API_H_

// standard C:
#include <stdint.h>
#include <assert.h>

// standard C++:
#if __cplusplus < 201103L
// C++03
#define YAE_FINAL
#define YAE_NOEXCEPT
#define YAE_OVERRIDE
#else
// C++11
#define YAE_FINAL final
#define YAE_NOEXCEPT noexcept
#define YAE_OVERRIDE override
#endif


//----------------------------------------------------------------
// YAE_API
//
// http://gcc.gnu.org/wiki/Visibility
//
#if defined _WIN32
#  define YAE_API_EXPORT __declspec(dllexport)
#  define YAE_API_IMPORT __declspec(dllexport)
#  ifdef YAE_DLL_EXPORTS
#    define YAE_API YAE_API_EXPORT
#  elif !defined(YAE_STATIC)
#    define YAE_API YAE_API_IMPORT
#  else
#    define YAE_API
#  endif
#else
#  define YAE_API_EXPORT __attribute__ ((visibility("default")))
#  define YAE_API_IMPORT YAE_API_EXPORT
#  if __GNUC__ >= 4
#    define YAE_API YAE_API_EXPORT
#  else
#    define YAE_API
#  endif
#endif


//----------------------------------------------------------------
// YAE_ALIGN
//
#if defined(_MSC_VER)
# define YAE_ALIGN(N, T) __declspec(align(N)) T
#elif __GNUC__ >= 4
# define YAE_ALIGN(N, T) T __attribute__ ((aligned(N)))
#else
# define YAE_ALIGN(N, T) T
#endif


//----------------------------------------------------------------
// YAE_DISABLE_DEPRECATION_WARNINGS
//
#if defined(__ICL) || defined (__INTEL_COMPILER)
#  define YAE_DISABLE_DEPRECATION_WARNINGS                               \
  __pragma(warning(push)) __pragma(warning(disable:1478))
#  define YAE_ENABLE_DEPRECATION_WARNINGS                                \
  __pragma(warning(pop))
#elif defined(_MSC_VER)
#  define YAE_DISABLE_DEPRECATION_WARNINGS                               \
  __pragma(warning(push)) __pragma(warning(disable:4996))
#  define YAE_ENABLE_DEPRECATION_WARNINGS                                \
  __pragma(warning(pop))
#elif __GNUC__
#  define YAE_DISABLE_DEPRECATION_WARNINGS                               \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#  define YAE_ENABLE_DEPRECATION_WARNINGS                                \
  _Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#else
#  define YAE_DISABLE_DEPRECATION_WARNINGS
#  define YAE_ENABLE_DEPRECATION_WARNINGS
#endif


//----------------------------------------------------------------
// YAE_STR
//
#define YAE_STR_HIDDEN(a) #a
#define YAE_STR(a) YAE_STR_HIDDEN(a)


namespace yae
{
  //----------------------------------------------------------------
  // uint64
  //
  typedef ::uint64_t uint64;

  //----------------------------------------------------------------
  // int64
  //
  typedef ::int64_t int64;
}


#include "yae_assert.h"


#endif // YAE_API_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:09:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef AEYAE_API_HXX_
#define AEYAE_API_HXX_

// standard C++ library:
#include <assert.h>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif

//----------------------------------------------------------------
// YAE_API
//
// http://gcc.gnu.org/wiki/Visibility
//
#if defined _WIN32
#  ifdef YAE_DLL_EXPORTS
#    define YAE_API __declspec(dllexport)
#  elif !defined(YAE_STATIC)
#    define YAE_API __declspec(dllimport)
#  else
#    define YAE_API
#  endif
#else
#  if __GNUC__ >= 4
#    define YAE_API __attribute__ ((visibility("default")))
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
// YAE_ASSERT
//
#if defined(NDEBUG)
# define YAE_ASSERT(expr)
#else
# if defined(__APPLE__)
#  if defined(__ppc__)
#   define YAE_ASSERT(expr) if (!(expr)) __asm { trap }
#  else
#   define YAE_ASSERT(expr) if (!(expr)) asm("int $3")
#  endif
# elif __GNUC__
#   define YAE_ASSERT(expr) if (!(expr)) asm("int $3")
# else
#  define YAE_ASSERT(expr) assert(expr)
# endif
#endif


namespace yae
{
  //----------------------------------------------------------------
  // uint64
  //
  typedef boost::uint64_t uint64;

  //----------------------------------------------------------------
  // int64
  //
  typedef boost::int64_t int64;
}


#endif // AEYAE_API_HXX_

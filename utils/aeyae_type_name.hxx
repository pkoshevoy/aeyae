// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef AEYAE_TYPE_NAME_HXX_
#define AEYAE_TYPE_NAME_HXX_

#include <iostream>
#include <string>
#include <typeinfo>

#ifdef __GXX_ABI_VERSION
#include <cxxabi.h>
#include <stdlib.h>
#endif

namespace yae
{
  //----------------------------------------------------------------
  // type_name
  //
  template <typename T>
  inline std::string type_name(T t)
  {
#ifdef __GXX_ABI_VERSION
    int status = 0;
    const char * mangled_name = typeid(t).name();
    char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);
    std::string r(real_name);
    ::free(real_name);
#else
    std::string r(typeid(t).name());
#endif
    return r;
  }
}


#endif // AEYAE_TYPE_NAME_HXX_

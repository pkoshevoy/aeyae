// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  7 22:28:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_plugin_registry.h"
#include "yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // TPluginRegistry::load
  //
  bool
  TPluginRegistry::load(const char * pluginsFolder)
  {
    try
    {
      TOpenFolder folder(pluginsFolder);

      return true;
    }
    catch (const std::exception & e)
    {}

    return false;
  }

}

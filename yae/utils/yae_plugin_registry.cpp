// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  7 22:28:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost:
#include <boost/algorithm/string/predicate.hpp>

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

      bool foundPlugins = false;
      do
      {
        std::string srcName = folder.itemName();
        std::string srcPath = folder.itemPath();

        if (folder.itemIsFolder())
        {
          if (strcmp(srcName.c_str(), ".") == 0 ||
              strcmp(srcName.c_str(), "..") == 0)
          {
            continue;
          }

          if (this->load(srcPath.c_str()))
          {
            foundPlugins = true;
          }

          continue;
        }

        if (!boost::algorithm::ends_with(srcName, ".yae"))
        {
          continue;
        }

        void * module = loadLibrary(srcPath.c_str());
        if (!module)
        {
          continue;
        }

        TPluginFactory factory =
          (TPluginFactory)getSymbol(module, "yae_create_plugin");
        if (!factory)
        {
          continue;
        }

        for (std::size_t i = 0; ; i++)
        {
          IPluginPtr plugin(factory(i));

          if (!plugin)
          {
            break;
          }

          std::string plugin_id(plugin->guid());
          (*this)[plugin_id] = plugin;
          foundPlugins = true;
        }

      } while (folder.parseNextItem());

      return foundPlugins;
    }
    catch (const std::exception & e)
    {}

    return false;
  }

}

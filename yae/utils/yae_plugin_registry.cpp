// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  7 22:28:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_plugin_registry.h"
#include "yae_utils.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string/predicate.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS


namespace yae
{

  //----------------------------------------------------------------
  // TPluginRegistry::load
  //
  bool
  TPluginRegistry::load(const char * plugins_folder)
  {
    try
    {
      TOpenFolder folder(plugins_folder);

      bool foundPlugins = false;
      do
      {
        std::string src_name = folder.item_name();
        std::string src_path = folder.item_path();

        if (folder.item_is_folder())
        {
          if (strcmp(src_name.c_str(), ".") == 0 ||
              strcmp(src_name.c_str(), "..") == 0)
          {
            continue;
          }

          if (this->load(src_path.c_str()))
          {
            foundPlugins = true;
          }

          continue;
        }

        if (!boost::algorithm::ends_with(src_name, ".yae"))
        {
          continue;
        }

        void * module = load_library(src_path.c_str());
        if (!module)
        {
          continue;
        }

        TPluginFactory factory =
          (TPluginFactory)get_symbol(module, "yae_create_plugin");
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

      } while (folder.parse_next_item());

      return foundPlugins;
    }
    catch (const std::exception & e)
    {}

    return false;
  }

}

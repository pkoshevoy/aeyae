// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  7 22:28:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLUGIN_REGISTRY_H_
#define YAE_PLUGIN_REGISTRY_H_

// standard C++ library:
#include <list>
#include <map>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// aeyae:
#include "../api/yae_plugin_interface.h"


namespace yae
{

  //----------------------------------------------------------------
  // TPluginRegistry
  //
  struct YAE_API TPluginRegistry : protected std::map<std::string, IPluginPtr>
  {
    // return an instance of each plugin of a given type
    template <typename TPlugin>
    bool
    find(std::list<boost::shared_ptr<TPlugin> > & plugins) const
    {
      typedef boost::shared_ptr<TPlugin> TPluginPtr;
      bool found = false;

      for (const_iterator i = this->begin(), end = this->end(); i != end; ++i)
      {
        TPluginPtr plugin =
          boost::dynamic_pointer_cast<TPlugin, IPlugin>(i->second);

        if (plugin)
        {
          plugins.push_back(plugin);
          found = true;
        }
      }

      return found;
    }

    // populate the registry with plugins from a given folder,
    // return true if plugins were found and loaded/registered:
    bool load(const char * plugins_folder);
  };

}


#endif // YAE_PLUGIN_REGISTRY_H_

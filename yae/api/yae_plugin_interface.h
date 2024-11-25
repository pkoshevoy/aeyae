// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 18:15:32 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLUGIN_INTERFACE_H_
#define YAE_PLUGIN_INTERFACE_H_

// aeyae:
#include "yae_api.h"
#include "yae_settings_interface.h"
#include "yae_shared_ptr.h"

// standard:
#include <cstddef>
#include <limits>


namespace yae
{

  //----------------------------------------------------------------
  // IPlugin
  //
  struct YAE_API IPlugin
  {
  protected:
    IPlugin() {}
    virtual ~IPlugin() {}

  public:

    //! The de/structor is intentionally hidden, use destroy() method instead.
    //! This is necessary in order to avoid conflicting memory manager
    //! problems that arise on windows when various libs are linked to
    //! different versions of runtime library.  Each library uses its own
    //! memory manager, so allocating in one library call and deallocating
    //! in another library will not work.  This can be avoided by hiding
    //! the standard constructor/destructor and providing an explicit
    //! interface for de/allocating an object instance, thus ensuring that
    //! the same memory manager will perform de/allocation.
    virtual void destroy() = 0;

    //! a prototype factory method for constructing objects of the same kind,
    //! but not necessarily deep copies of the original prototype object:
    virtual IPlugin * clone() const = 0;

    //! a human-readable name for this plugin:
    virtual const char * name() const = 0;

    //! a unique identifier for this plugin (use uuidgen to make one):
    virtual const char * guid() const = 0;

    //! accessor to a nested (and ordered) collection of configuration
    //! parameters required for the operation of this plugin, if any exist.
    //! NOTE: the settings, if there are any, belong to the plugin!
    virtual ISettingGroup * settings() = 0;
  };

  //----------------------------------------------------------------
  // IPluginPtr
  //
  typedef yae::shared_ptr<IPlugin, IPlugin, yae::call_destroy> IPluginPtr;

  //----------------------------------------------------------------
  // TPluginFactory
  //
  // a plugin file will provide an extern "C" function
  // called yae_create_plugin, taking an int as a parameter
  // to indicate an index of the plugin to create, incase
  // the module provides multiple plugins.
  //
  typedef yae::IPlugin *(*TPluginFactory)(std::size_t /* index */);

}


#endif // YAE_PLUGIN_INTERFACE_H_

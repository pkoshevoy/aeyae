// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 18:37:06 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_H_
#define YAE_AUDIO_RENDERER_H_

// system includes:
#include <string>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // IAudioRenderer
  // 
  struct YAE_API IAudioRenderer
  {
  protected:
    IAudioRenderer();
    virtual ~IAudioRenderer();
    
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
    
    //! return a human readable name for this renderer (preferably unique):
    virtual const char * getName() const = 0;

    //! there may be multiple audio rendering devices available:
    virtual unsigned int countAvailableDevices() const = 0;
    
    //! return index of the system default audio rendering device:
    virtual unsigned int getDefaultDeviceIndex() const = 0;
    
    //! get device name and max audio resolution capabilities:
    virtual bool getDeviceName(unsigned int deviceIndex,
                               std::string & deviceName) const = 0;
    
    //! initialize a given audio rendering device:
    virtual bool open(unsigned int deviceIndex,
                      IReader * reader) = 0;
    
    //! terminate audio rendering:
    virtual void close() = 0;
  };
}


#endif // YAE_AUDIO_RENDERER_H_

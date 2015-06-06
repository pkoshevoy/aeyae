// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 17:16:04 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef AEYAE_FRAME_OBSERVER_INTERFACE_HXX_
#define AEYAE_FRAME_OBSERVER_INTERFACE_HXX_

// aeyae:
#include "aeyae_api.hxx"
#include "aeyae_plugin_interface.hxx"


namespace yae
{

  struct YAE_API IFrameObserver : public IPlugin
  {
    //! NOTE: this may throw an exception:
    //! perform internal initialization of the observer here in preparation
    //! to accept audio/video frame pairs via the 'push' method:
    virtual void start() = 0;

    //! NOTE: this may throw an exception:
    virtual void push(const TVideoFramePtr & video,
                      const TAudioFramePtr & audio) = 0;

    //! notify the observer that there will be no additional audio/video
    //! frame pairs 'push'ed to it, so it can shut down gracefully:
    virtual void stop() = 0;
  };

}


#endif // AEYAE_FRAME_OBSERVER_INTERFACE_HXX_

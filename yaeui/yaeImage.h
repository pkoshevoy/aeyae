// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_IMAGE_H_
#define YAE_IMAGE_H_

// local interfaces:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeThumbnailProvider.h"


namespace yae
{

  //----------------------------------------------------------------
  // Image
  //
  class Image : public Item
  {
    Image(const Image &);
    Image & operator = (const Image &);

  public:
    Image(const char * id);
    ~Image();

    // for thumbnail providers, repaint requests, etc...
    void setContext(const Canvas::ILayer & view);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // virtual:
    bool visible() const;

    // this gets complicated due to asynchronous loading of images:
    struct TPrivate;
    TPrivate * p_;

    // what to load, requires a matching image provider:
    TVarRef url_;

    ItemRef opacity_;
  };

}


#endif // YAE_IMAGE_H_

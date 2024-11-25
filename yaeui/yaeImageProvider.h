// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 13 20:56:51 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_IMAGE_PROVIDER_H_
#define YAE_IMAGE_PROVIDER_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_reader.h"

// standard:
#include <map>

// Qt:
#include <QImage>
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
#include <QQuickImageProvider>
#endif
#include <QSize>
#include <QString>


namespace yae
{

  //----------------------------------------------------------------
  // ImageProvider
  //
  struct ImageProvider
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    : public QQuickImageProvider
#endif
  {
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    ImageProvider();
#endif

    virtual ~ImageProvider() {}

    // limit how many images to keep in memory:
    virtual void setCacheCapacity(std::size_t cacheCapacity) = 0;

#ifndef YAE_USE_PLAYER_QUICK_WIDGET
    virtual QImage requestImage(const QString & id,
                                QSize * size,
                                const QSize & requestedSize) = 0;
#endif

    //----------------------------------------------------------------
    // ICallback
    //
    struct ICallback
    {
      virtual ~ICallback() {}
      virtual void imageReady(const QImage & image) = 0;
    };

    virtual void requestImageAsync(const QString & id,
                                   const QSize & requestedSize,
                                   const boost::weak_ptr<ICallback> & cb) = 0;

    virtual void cancelRequest(const QString & id) = 0;
  };

  //----------------------------------------------------------------
  // TImageProviderPtr
  //
  typedef yae::shared_ptr<ImageProvider> TImageProviderPtr;

  //----------------------------------------------------------------
  // TImageProviders
  //
  typedef std::map<QString, TImageProviderPtr> TImageProviders;

  //----------------------------------------------------------------
  // lookupImageProvider
  //
  TImageProviderPtr
  lookupImageProvider(const TImageProviders & providers,
                      const QString & resourceUrl,
                      QString & imageId);

}


#endif // YAE_IMAGE_PROVIDER_H_

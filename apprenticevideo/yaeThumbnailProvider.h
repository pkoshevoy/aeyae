// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 21:56:09 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_THUMBNAIL_PROVIDER_H_
#define YAE_THUMBNAIL_PROVIDER_H_

// standard libraries:
#include <map>

// boost includes:
#include <boost/shared_ptr.hpp>

// Qt includes:
#include <QImage>
#ifdef YAE_USE_QT5
#include <QQuickImageProvider>
#endif
#include <QSize>
#include <QString>

// yae includes:
#include "yae/video/yae_reader.h"

// local includes:
#include "yaePlaylistModelProxy.h"


namespace yae
{

  //----------------------------------------------------------------
  // ThumbnailProvider
  //
  struct ThumbnailProvider
#ifdef YAE_USE_QT5
    : public QQuickImageProvider
#endif
  {
    ThumbnailProvider(const IReaderPtr & readerPrototype,
                      const TPlaylistModel & playlist,

                      // default thumbnail size:
                      const QSize & envelopeSize = QSize(384, 216),

                      // maximum number of images that may be cached in memory:
                      std::size_t cacheCapacity = 1024);

    virtual ~ThumbnailProvider();

    // limit how many images to keep in memory:
    void setCacheCapacity(std::size_t cacheCapacity);

    // virtual:
    QImage requestImage(const QString & id,
                        QSize * size,
                        const QSize & requestedSize);

    //----------------------------------------------------------------
    // ICallback
    //
    struct ICallback
    {
      virtual ~ICallback() {}
      virtual void imageReady(const QImage & image) = 0;
    };

    void requestImageAsync(const QString & id,
                           const QSize & requestedSize,
                           const boost::weak_ptr<ICallback> & callback);

    void cancelRequest(const QString & id);

    struct TPrivate;
    TPrivate * private_;
  };

  //----------------------------------------------------------------
  // TImageProviderPtr
  //
  typedef boost::shared_ptr<ThumbnailProvider> TImageProviderPtr;

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


#endif // YAE_THUMBNAIL_PROVIDER_H_

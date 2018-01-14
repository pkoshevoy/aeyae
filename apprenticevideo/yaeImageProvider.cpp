// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 22:00:05 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <map>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// Qt headers:
#include <QtGlobal>
#include <QFileInfo>
#include <QUrl>

// yae includes:
#include "yae/api/yae_settings.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_pixel_format_traits.h"

// local includes:
#include "yaePlaylist.h"
#include "yaeImageProvider.h"
#include "yaeUtilsQt.h"


namespace yae
{

#ifdef YAE_USE_PLAYER_QUICK_WIDGET
  //----------------------------------------------------------------
  // ImageProvider::ImageProvider
  //
  ImageProvider::ImageProvider():
    QQuickImageProvider(QQmlImageProviderBase::Image,
                        QQmlImageProviderBase::ForceAsynchronousImageLoading)
  {}
#endif

  //----------------------------------------------------------------
  // lookupImageProvider
  //
  TImageProviderPtr
  lookupImageProvider(const TImageProviders & providers,
                      const QString & resource,
                      QString & imageId)
  {
    static const QString kImage = QString::fromUtf8("image");

    QUrl url(resource);
    if (url.scheme() != kImage)
    {
      return TImageProviderPtr();
    }

    QString host = url.host();
    TImageProviders::const_iterator found = providers.find(host);
    if (found == providers.end())
    {
      return TImageProviderPtr();
    }

    imageId = url.path();

    // trim the leading '/' character:
    imageId = imageId.right(imageId.size() - 1);

    return found->second;
  }

}

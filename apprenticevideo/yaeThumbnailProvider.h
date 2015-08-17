// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 21:56:09 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_THUMBNAIL_PROVIDER_H_
#define YAE_THUMBNAIL_PROVIDER_H_

// standard C++ library:
#include <map>

// Qt includes:
#include <QImage>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>

// yae includes:
#include "yae/video/yae_reader.h"

// local includes:
#include "yaePlaylist.h"


namespace yae
{

  //----------------------------------------------------------------
  // ThumbnailProvider
  //
  struct ThumbnailProvider : public QQuickImageProvider
  {
    ThumbnailProvider(const IReaderPtr & readerPrototype,
                      yae::mvc::Playlist & playlist,

                      // default thumbnail size:
                      const QSize & envelopeSize = QSize(384, 216)
                      // const QSize & envelopeSize = QSize(160, 90)
                      );

    // virtual:
    QImage requestImage(const QString & id,
                        QSize * size,
                        const QSize & requestedSize);

  protected:
    IReaderPtr readerPrototype_;
    yae::mvc::Playlist & playlist_;
    QSize envelopeSize_;
    std::map<QString, QImage> cache_;
  };

}


#endif // YAE_THUMBNAIL_PROVIDER_H_

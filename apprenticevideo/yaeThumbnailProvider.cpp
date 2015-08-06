// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 22:00:05 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/video/yae_pixel_format_traits.h"

// local includes:
#include "yaePlaylist.h"
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // TCleanup
  //
  struct TCleanup
  {
    TCleanup(const TVideoFramePtr & frame):
      frame_(frame)
    {}

    static void cleanup(void * context)
    {
      TCleanup * c = (TCleanup *)context;
      delete c;
    }

    TVideoFramePtr frame_;
  };

  //----------------------------------------------------------------
  // ThumbnailProvider::ThumbnailProvider
  //
  ThumbnailProvider::ThumbnailProvider(const IReaderPtr & readerPrototype,
                                       yae::mvc::Playlist & playlist):
    QQuickImageProvider(QQmlImageProviderBase::Image,
                        QQmlImageProviderBase::ForceAsynchronousImageLoading),
    readerPrototype_(readerPrototype),
    playlist_(playlist)
  {}

  //----------------------------------------------------------------
  // ThumbnailProvider::requestImage
  //
  QImage
  ThumbnailProvider::requestImage(const QString & id,
                                  QSize * size,
                                  const QSize & requestedSize)
  {
    // parse the id (group-hash/item-hash)
    std::string groupHashItemHash(id.toUtf8().constData());
    std::size_t t = groupHashItemHash.find_first_of('/');
    std::string groupHash = groupHashItemHash.substr(0, t);
    std::string itemHash = groupHashItemHash.substr(t + 1);

    std::size_t itemIndex = ~0;
    yae::mvc::PlaylistGroup * group = NULL;
    yae::mvc::PlaylistItem * item = playlist_.lookup(groupHash,
                                                     itemHash,
                                                     &itemIndex,
                                                     &group);
    if (!item)
    {
      return QImage();
    }

    IReaderPtr reader = yae::openFile(readerPrototype_, item->path_);
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (!numVideoTracks)
    {
      return QImage();
    }

    reader->selectVideoTrack(0);

    VideoTraits vtts;
    if (!reader->getVideoTraits(vtts))
    {
      return QImage();
    }

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    vtts.pixelFormat_ = kPixelFormatGRAY8;
    QImage::Format qimageFormat = QImage::Format_Grayscale8;

    if (ptts)
    {
      if ((ptts->flags_ & pixelFormat::kAlpha) &&
          (ptts->flags_ & pixelFormat::kColor))
      {
        vtts.pixelFormat_ = kPixelFormatBGRA;
        qimageFormat = QImage::Format_ARGB32;
      }
      else if ((ptts->flags_ & pixelFormat::kColor) ||
               (ptts->flags_ & pixelFormat::kPaletted))
      {
        vtts.pixelFormat_ = kPixelFormatRGB24;
        qimageFormat = QImage::Format_RGB888;
      }
    }

    reader->setVideoTraitsOverride(vtts);

    if (!reader->getVideoTraitsOverride(vtts) ||
        !(ptts = pixelFormat::getTraits(vtts.pixelFormat_)))
    {
      return QImage();
    }

#if 1
    std::cerr
      << "yae: thumbnail format: " << ptts->name_
      << ", par: " << vtts.pixelAspectRatio_
      << ", " << vtts.visibleWidth_
      << " x " << vtts.visibleHeight_;

    if (vtts.pixelAspectRatio_ != 0.0)
    {
      std::cerr
        << ", dar: "
        << (double(vtts.visibleWidth_) *
            vtts.pixelAspectRatio_ /
            double(vtts.visibleHeight_))
        << ", " << int(vtts.visibleWidth_ *
                       vtts.pixelAspectRatio_ +
                       0.5)
        << " x " << vtts.visibleHeight_;
    }

    std::cerr
      << ", fps: " << vtts.frameRate_
      << std::endl;
#endif

    QImage image;

    TTime start;
    TTime duration;
    if (reader->isSeekable() && reader->getVideoDuration(start, duration))
    {
      double t0 = start.toSeconds();
      double offset = std::min<double>(duration.toSeconds() * 1e-2, 90.0);
      reader->seek(t0 + offset);
    }

    reader->threadStart();

    TVideoFramePtr frame;
    QueueWaitMgr waitMgr_;
    while (reader->readVideo(frame, &waitMgr_) &&
           (!frame || yae::resetTimeCountersIndicated(frame.get())))
    {}

    if (frame)
    {
      // FIXME:
      //
      // * the image should be scaled-down to requested size,
      //   or another appropriate thumbnail size
      //
      // * pixel aspect ratio should be square
      //
      const unsigned char * data = frame->data_->data(0);
      std::size_t rowSize = frame->data_->rowBytes(0);

      image = QImage(data,
                     frame->traits_.visibleWidth_,
                     frame->traits_.visibleHeight_,
                     rowSize,
                     qimageFormat,
                     &TCleanup::cleanup,
                     new TCleanup(frame));

      image = image.scaledToHeight(90, Qt::SmoothTransformation);

      *size = image.size();
    }

    return image;
  }

}

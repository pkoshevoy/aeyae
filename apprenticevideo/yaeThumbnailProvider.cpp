// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 22:00:05 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/api/yae_settings.h"
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
    // FIXME: this could be a parameter of the thumbnail provider:
    static const QSize envelope(160, 90);

    QImage image;

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
      return image;
    }

    IReaderPtr reader = yae::openFile(readerPrototype_, item->path_);
    if (!reader)
    {
      return image;
    }

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (!numVideoTracks)
    {
      return image;
    }

    reader->selectVideoTrack(0);
    reader->skipLoopFilter(true);
    reader->skipNonReferenceFrames(true);
    reader->setDeinterlacing(false);

    VideoTraits vtts;
    if (!reader->getVideoTraits(vtts))
    {
      return image;
    }

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    VideoTraits override = vtts;
    override.pixelFormat_ = kPixelFormatGRAY8;
    QImage::Format qimageFormat = QImage::Format_Grayscale8;

    if (ptts)
    {
      if ((ptts->flags_ & pixelFormat::kAlpha) &&
          (ptts->flags_ & pixelFormat::kColor))
      {
        override.pixelFormat_ = kPixelFormatBGRA;
        qimageFormat = QImage::Format_ARGB32;
      }
      else if ((ptts->flags_ & pixelFormat::kColor) ||
               (ptts->flags_ & pixelFormat::kPaletted))
      {
        override.pixelFormat_ = kPixelFormatRGB24;
        qimageFormat = QImage::Format_RGB888;
      }
    }

    double src_w = double(vtts.visibleWidth_) * vtts.pixelAspectRatio_;
    double src_h = double(vtts.visibleHeight_);

    bool src_rotated = (override.cameraRotation_ % 180 != 0);
    if (src_rotated)
    {
      std::swap(src_w, src_h);
    }

    *size = requestedSize.isValid() ? requestedSize : envelope;

    // crop, deinterlace, flip, rotate, scale, color-convert:
    override.encodedHeight_ = size->height();
    override.encodedWidth_ = size->width();
    override.offsetTop_ = 0;
    override.offsetLeft_ = 0;
    override.visibleWidth_ = override.encodedWidth_;
    override.visibleHeight_ = override.encodedHeight_;
    override.pixelAspectRatio_ = 1.0;
    override.cameraRotation_ = 0;
    override.isUpsideDown_ = false;

    reader->setVideoTraitsOverride(override);

    if (!reader->getVideoTraitsOverride(override) ||
        !(ptts = pixelFormat::getTraits(override.pixelFormat_)))
    {
      return image;
    }

    TTime start;
    TTime duration;
    if (reader->isSeekable() && reader->getVideoDuration(start, duration))
    {
      double t0 = start.toSeconds();
      double offset = std::min<double>(duration.toSeconds() * 1e-2, 90.0);
      reader->seek(t0 + offset);
    }

    ISettingGroup * readerSettings = reader->settings();
    if (readerSettings)
    {
      ISettingUInt32 * frameQueueSize =
        settingById<ISettingUInt32>(*readerSettings, "video_queue_size");

      if (frameQueueSize)
      {
        frameQueueSize->traits().setValue(1);
      }
    }

    reader->setPlaybackEnabled(true);
    reader->threadStart();

    TVideoFramePtr frame;
    QueueWaitMgr waitMgr_;
    while (reader->readVideo(frame, &waitMgr_) &&
           (!frame || yae::resetTimeCountersIndicated(frame.get())))
    {}

    if (frame && frame->data_)
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

      bool sizeAcceptable =
        (image.height() <= size->height() &&
         image.width() <= size->width());
      YAE_ASSERT(sizeAcceptable);

      if (!sizeAcceptable)
      {
        image = image.scaledToHeight(90, Qt::SmoothTransformation);
      }

      *size = image.size();
#if 0
      image.save(QString::fromUtf8
                 ("/Users/pavel/Pictures/Thumbnails/%1-%2.jpg").
                 arg(groupHash.c_str()).
                 arg(itemHash.c_str()),
                 "JPEG");
#endif
    }

    return image;
  }

}

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

// yae includes:
#include "yae/api/yae_settings.h"
#include "yae/thread/yae_threading.h"
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

#if 0
  //----------------------------------------------------------------
  // getIcon
  //
  static TVideoFramePtr
  getIcon(const char * resourcePath)
  {
    TVideoFramePtr frame;

    QImage image(QString::fromUtf8(resourcePath));
    if (image.isNull())
    {
      YAE_ASSERT(false);
      return frame;
    }

    frame.reset(new TVideoFrame());
    frame->data_.reset(new TQImageBuffer(image));

    VideoTraits & vtts = frame->traits_;
    QImage::Format qimageFormat = image.format();

    vtts.pixelFormat_ =
#ifdef _BIG_ENDIAN
      (qimageFormat == QImage::Format_ARGB32) ? kPixelFormatARGB :
#else
      (qimageFormat == QImage::Format_ARGB32) ? kPixelFormatBGRA :
#endif
      (qimageFormat == QImage::Format_RGB888) ? kPixelFormatRGB24 :
      (qimageFormat == QImage::Format_Grayscale8) ? kPixelFormatGRAY8 :
      kInvalidPixelFormat;

    YAE_ASSERT(vtts.pixelFormat_ != kInvalidPixelFormat);

    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = vtts.encodedWidth_;
    vtts.visibleHeight_ = vtts.encodedHeight_;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    return frame;
  }
#endif

  //----------------------------------------------------------------
  // getThumbnail
  //
  static TVideoFramePtr
  getThumbnail(const yae::IReaderPtr & reader, const QSize & envelope)
  {
    TVideoFramePtr frame;

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (!numVideoTracks)
    {
      return frame;
    }

    reader->selectVideoTrack(0);
    reader->skipLoopFilter(true);
    reader->skipNonReferenceFrames(true);
    reader->setDeinterlacing(false);

    VideoTraits vtts;
    if (!reader->getVideoTraits(vtts))
    {
      return frame;
    }

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    VideoTraits override = vtts;
    override.pixelFormat_ = kPixelFormatGRAY8;

    if (ptts)
    {
      if ((ptts->flags_ & pixelFormat::kAlpha) &&
          (ptts->flags_ & pixelFormat::kColor))
      {
        override.pixelFormat_ = kPixelFormatBGRA;
      }
      else if ((ptts->flags_ & pixelFormat::kColor) ||
               (ptts->flags_ & pixelFormat::kPaletted))
      {
        override.pixelFormat_ = kPixelFormatRGB24;
      }
    }

    // crop, deinterlace, flip, rotate, scale, color-convert:
    override.offsetTop_ = 0;
    override.offsetLeft_ = 0;
    override.visibleWidth_ = envelope.width();
    override.visibleHeight_ = envelope.height();
    override.pixelAspectRatio_ = 1.0;
    override.cameraRotation_ = 0;
    override.isUpsideDown_ = false;

    reader->setVideoTraitsOverride(override);

    if (!reader->getVideoTraitsOverride(override) ||
        !(ptts = pixelFormat::getTraits(override.pixelFormat_)))
    {
      return frame;
    }

    TTime start;
    TTime duration;
    if (reader->isSeekable() && reader->getVideoDuration(start, duration))
    {
      double t0 = start.toSeconds();
      // double offset = std::min<double>(duration.toSeconds() * 2e-2, 90.0);
      double offset = std::min<double>(duration.toSeconds() * 8e-2, 288.0);

      // FIXME: check for a bookmark, seek to the bookmarked position:
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

    QueueWaitMgr waitMgr_;
    while (reader->readVideo(frame, &waitMgr_) &&
           (!frame || !frame->data_ ||
            yae::resetTimeCountersIndicated(frame.get())))
    {}

    return frame;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::requestImage
  //
  static QImage
  getThumbnail(const yae::IReaderPtr & readerPrototype,
               const QSize & thumbnailMaxSize,
               const TPlaylistModel & playlist,
               const QString & id)
  {
    static QVector<QRgb> palette(256);
    static bool palette_ready = false;
    if (!palette_ready)
    {
      for (std::size_t i = 0; i < 256; i++)
      {
        palette[i] = qRgb(i, i, i);
      }
      palette_ready = true;
    }

    static QImage iconAudio
      (QString::fromUtf8(":/images/music-note.png"));

    static QImage iconVideo
      (QString::fromUtf8(":/images/video-frame.png"));

    static QImage iconBroken
      (QString::fromUtf8(":/images/broken-glass.png"));

    QImage image;

    QString itemFilePath = playlist.lookupItemFilePath(id);
#if 0
    std::cerr
      << "\nFIXME: getThumbnail"
      << "\n item id: " << id.toUtf8().constData()
      << "\nfilepath: " << itemFilePath.toUtf8().constData()
      << std::endl;
#endif

    IReaderPtr reader = yae::openFile(readerPrototype, itemFilePath);
    if (!reader)
    {
      return iconBroken;
    }

    TVideoFramePtr frame = getThumbnail(reader, thumbnailMaxSize);
    if (!frame || !frame->data_)
    {
      std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
      if (numAudioTracks)
      {
        return iconAudio;
      }

      std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
      if (numVideoTracks)
      {
        return iconVideo;
      }

      return iconBroken;
    }

    // shortcut:
    const VideoTraits & vtts = frame->traits_;

    QImage::Format qimageFormat =
#ifdef _BIG_ENDIAN
      (vtts.pixelFormat_ == kPixelFormatARGB) ? QImage::Format_ARGB32 :
#else
      (vtts.pixelFormat_ == kPixelFormatBGRA) ? QImage::Format_ARGB32 :
#endif
      (vtts.pixelFormat_ == kPixelFormatRGB24) ? QImage::Format_RGB888 :
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
      QImage::Format_Indexed8
#else
      QImage::Format_Grayscale8
#endif
               ;

    const unsigned char * data = frame->data_->data(0);
    std::size_t rowSize = frame->data_->rowBytes(0);

#ifdef YAE_USE_QT5
    image = QImage(data,
                   frame->traits_.visibleWidth_,
                   frame->traits_.visibleHeight_,
                   rowSize,
                   qimageFormat,
                   &TCleanup::cleanup,
                   new TCleanup(frame));
#else
    image = QImage(data,
                   frame->traits_.visibleWidth_,
                   frame->traits_.visibleHeight_,
                   rowSize,
                   qimageFormat);
#endif

    if (qimageFormat == QImage::Format_Indexed8)
    {
      image.setColorTable(palette);
    }

#ifndef YAE_USE_QT5
    // make a deep copy to avoid leaving QImage with dangling pointers
    // to frame data:
    image = image.copy();
#endif

#if 0
    image.save(QString::fromUtf8
               ("/Users/pavel/Pictures/Thumbnails/%1.bmp").
               arg(QFileInfo(itemFilePath).completeBaseName()),
               "BMP");
#endif

    return image;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate
  //
  struct ThumbnailProvider::TPrivate
  {
    typedef ThumbnailProvider::ICallback ICallback;

    TPrivate(const IReaderPtr & readerPrototype,
             const TPlaylistModel & playlist,
             const QSize & envelopeSize);
    ~TPrivate();

    QImage requestImage(const QString & id,
                        QSize * size,
                        const QSize & requestedSize);

    void requestImageAsync(const QString & id,
                           const QSize & requestedSize,
                           const boost::weak_ptr<ICallback> & callback);

    void discardImage(const QString & id);

    void threadLoop();

    //----------------------------------------------------------------
    // Request
    //
    struct Request
    {
      Request(const QString & id = QString(),
              const QSize & size = QSize(),
              const boost::weak_ptr<ICallback> & callback =
              boost::weak_ptr<ICallback>(),
              std::size_t priority = 0):
        id_(id),
        size_(size),
        callback_(callback),
        priority_(priority)
      {}

      QString id_;
      QSize size_;
      boost::weak_ptr<ICallback> callback_;
      boost::uint64_t priority_;
    };

    IReaderPtr readerPrototype_;
    const TPlaylistModel & playlist_;
    QSize envelopeSize_;

    // thumbnail request queue and image cache:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable ready_;
    boost::uint64_t requestCounter_;
    std::map<QString, Request> request_;
    std::map<boost::uint64_t, Request *> priority_;
    std::map<QString, QImage> cache_;

    // request processing worker thread:
    Thread<ThumbnailProvider::TPrivate> thread_;
  };

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::TPrivate
  //
  ThumbnailProvider::TPrivate::TPrivate(const IReaderPtr & readerPrototype,
                                        const TPlaylistModel & playlist,
                                        const QSize & envelopeSize):
    readerPrototype_(readerPrototype),
    playlist_(playlist),
    envelopeSize_(envelopeSize),
    requestCounter_(0),
    thread_(this)
  {}

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::~TPrivate
  //
  ThumbnailProvider::TPrivate::~TPrivate()
  {
    thread_.stop();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::requestImage
  //
  QImage
  ThumbnailProvider::TPrivate::requestImage(const QString & id,
                                            QSize * size,
                                            const QSize & requestedSize)
  {
    QImage image;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      image = cache_[id];
    }

    const QSize & thumbnailMaxSize =
      requestedSize.isValid() ? requestedSize : envelopeSize_;

    if (image.isNull())
    {
      image = getThumbnail(readerPrototype_,
                           thumbnailMaxSize,
                           playlist_,
                           id);

      bool sizeAcceptable =
        (image.height() <= thumbnailMaxSize.height() &&
         image.width() <= thumbnailMaxSize.width());

      if (!sizeAcceptable)
      {
        image = image.scaledToHeight(90, Qt::SmoothTransformation);
      }

      boost::unique_lock<boost::mutex> lock(mutex_);
      cache_[id] = image;
    }

    if (size)
    {
      *size = image.size();
    }

    return image;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::requestImageAsync
  //
  void
  ThumbnailProvider::TPrivate::
  requestImageAsync(const QString & id,
                    const QSize & requestedSize,
                    const boost::weak_ptr<ICallback> & callback)
  {
    if (!thread_.isRunning())
    {
      thread_.run();
    }

    boost::lock_guard<boost::mutex> lock(mutex_);
    std::map<QString, Request>::iterator found = request_.lower_bound(id);
    if (found == request_.end() || request_.key_comp()(id, found->first))
    {
      // create new request:
      Request request(id, requestedSize, callback, requestCounter_);
      found = request_.insert(found, std::make_pair(id, request));
      priority_[request.priority_] = &(found->second);
    }
    else
    {
      // re-prioritize previous request:
      Request & request = found->second;
      priority_.erase(request.priority_);
      request.priority_ = requestCounter_;
      priority_[request.priority_] = &request;
    }

    requestCounter_++;
    ready_.notify_all();
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::discardImage
  //
  void
  ThumbnailProvider::TPrivate::discardImage(const QString & id)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    cache_.erase(id);

    std::map<QString, Request>::iterator found = request_.find(id);
    if (found != request_.end())
    {
      Request & request = found->second;
      priority_.erase(request.priority_);
      request_.erase(found);
      ready_.notify_all();
    }
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::threadLoop
  //
  void
  ThumbnailProvider::TPrivate::threadLoop()
  {
    while (true)
    {
      try
      {
        // process the highest priority request:
        Request request;
        {
          boost::this_thread::interruption_point();
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (request_.empty())
          {
            ready_.wait(lock);
            boost::this_thread::interruption_point();
          }

          std::map<boost::uint64_t, Request *>::reverse_iterator
            next = priority_.rbegin();

          request = *(next->second);
          priority_.erase(request.priority_);
          request_.erase(request.id_);
        }

        boost::shared_ptr<ICallback> callback = request.callback_.lock();
        if (callback)
        {
          QImage image = requestImage(request.id_, NULL, request.size_);
          callback->imageReady(image);
        }
      }
      catch (...)
      {
        break;
      }
    }

    boost::unique_lock<boost::mutex> lock(mutex_);
    priority_.clear();
    request_.clear();
  }


  //----------------------------------------------------------------
  // ThumbnailProvider::ThumbnailProvider
  //
  ThumbnailProvider::ThumbnailProvider(const IReaderPtr & readerPrototype,
                                       const TPlaylistModel & playlist,
                                       const QSize & envelopeSize):
#ifdef YAE_USE_QT5
    QQuickImageProvider(QQmlImageProviderBase::Image,
                        QQmlImageProviderBase::ForceAsynchronousImageLoading),
#endif
    private_(new TPrivate(readerPrototype, playlist, envelopeSize))
  {}

  //----------------------------------------------------------------
  // ThumbnailProvider::~ThumbnailProvider
  //
  ThumbnailProvider::~ThumbnailProvider()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::requestImage
  //
  QImage
  ThumbnailProvider::requestImage(const QString & id,
                                  QSize * size,
                                  const QSize & requestedSize)
  {
    return private_->requestImage(id, size, requestedSize);
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::requestImageAsync
  //
  void
  ThumbnailProvider::requestImageAsync(const QString & id,
                                       const QSize & requestedSize,
                                       const boost::weak_ptr<ICallback> & cb)
  {
    private_->requestImageAsync(id, requestedSize, cb);
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::discardImage
  //
  void
  ThumbnailProvider::discardImage(const QString & id)
  {
    private_->discardImage(id);
  }
}

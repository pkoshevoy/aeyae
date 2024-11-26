// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Aug  4 22:00:05 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_settings.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_pixel_format_traits.h"

// standard:
#include <map>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// Qt headers:
#include <QtGlobal>
#include <QFileInfo>
#include <QUrl>

// yaeui:
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // TGetFilePathPtr
  //
  typedef boost::shared_ptr<ThumbnailProvider::GetFilePath> TGetFilePathPtr;

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

    TPixelFormatId pixelFormat =
#ifdef _BIG_ENDIAN
      (qimageFormat == QImage::Format_ARGB32) ? kPixelFormatARGB :
#else
      (qimageFormat == QImage::Format_ARGB32) ? kPixelFormatBGRA :
#endif
      (qimageFormat == QImage::Format_RGB888) ? kPixelFormatRGB24 :
      (qimageFormat == QImage::Format_Grayscale8) ? kPixelFormatGRAY8 :
      kInvalidPixelFormat;

    YAE_ASSERT(pixelFormat != kInvalidPixelFormat);
    vtts.setPixelFormat(pixelFormat);

    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = vtts.encodedWidth_;
    vtts.visibleHeight_ = vtts.encodedHeight_;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.vflip_ = false;
    vtts.hflip_ = false;

    return frame;
  }
#endif

  //----------------------------------------------------------------
  // get_duration
  //
  extern bool
  get_duration(const IReader * reader, TTime & start, TTime & duration);

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

    VideoTraits traits = vtts;
    traits.setPixelFormat(kPixelFormatGRAY8);

    if (ptts)
    {
      if ((ptts->flags_ & pixelFormat::kAlpha) &&
          (ptts->flags_ & pixelFormat::kColor))
      {
        traits.setPixelFormat(kPixelFormatBGRA);
      }
      else if ((ptts->flags_ & pixelFormat::kColor) ||
               (ptts->flags_ & pixelFormat::kPaletted))
      {
        traits.setPixelFormat(kPixelFormatRGB24);
      }
    }

    // crop, deinterlace, flip, rotate, scale, color-convert:
    traits.offsetTop_ = 0;
    traits.offsetLeft_ = 0;
    traits.visibleWidth_ = envelope.width();
    traits.visibleHeight_ = envelope.height();
    traits.pixelAspectRatio_ = 1.0;
    traits.cameraRotation_ = 0;
    traits.vflip_ = false;
    traits.hflip_ = false;

    reader->setVideoTraitsOverride(traits);

    if (!reader->getVideoTraitsOverride(traits) ||
        !(ptts = pixelFormat::getTraits(traits.pixelFormat_)))
    {
      return frame;
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

    QueueWaitMgr waitMgr;
    TTime start;
    TTime duration;
    if (reader->isSeekable() && get_duration(reader.get(), start, duration))
    {
      double offset = std::min<double>(duration.sec() * 8e-2, 288.0);

      // avoid seeking very short files (.jpg):
      if (offset >= 0.016 && reader->readVideo(frame, &waitMgr))
      {
        if (frame)
        {
          start = frame->time_;
        }

        // FIXME: check for a bookmark, seek to the bookmarked position:
        double t0 = start.sec();
        reader->seek(t0 + offset);
      }
    }

    while (reader->readVideo(frame, &waitMgr) &&
           (!frame || !frame->data_ ||
            yae::resetTimeCountersIndicated(frame.get())))
    {}

    return frame;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::requestImage
  //
  static QImage
  getThumbnail(const TReaderFactoryPtr & readerFactory,
               const QSize & thumbnailMaxSize,
               const QString & itemFilePath)
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

    IReaderPtr reader = yae::openFile(readerFactory, itemFilePath);
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
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

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
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

    TPrivate(const TReaderFactoryPtr & readerFactory,
             const TGetFilePathPtr & getFilePath,
             const QSize & envelopeSize,
             std::size_t cacheCapacity);
    ~TPrivate();

    void setCacheCapacity(std::size_t cacheCapacity);

  private:
    void clearCacheFor(std::size_t n);

  public:
    QImage requestImage(const QString & id,
                        QSize * size,
                        const QSize & requestedSize);

    void requestImageAsync(const QString & id,
                           const QSize & requestedSize,
                           const boost::weak_ptr<ICallback> & callback);

    void cancelRequest(const QString & id);

    void thread_loop();

    //----------------------------------------------------------------
    // Request
    //
    struct Request
    {
      Request(const QString & id = QString(),
              const QSize & size = QSize(),
              const boost::weak_ptr<ICallback> & callback =
              boost::weak_ptr<ICallback>(),
              boost::uint64_t priority = 0):
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

    //----------------------------------------------------------------
    // Payload
    //
    struct Payload
    {
      Payload(const QString & id = QString(),
              const QImage & image = QImage(),
              boost::uint64_t mru = 0):
        id_(id),
        image_(image),
        mru_(mru)
      {}

      QString id_;
      QImage image_;
      boost::uint64_t mru_;
    };

    TReaderFactoryPtr readerFactory_;
    TGetFilePathPtr getFilePath_;
    QSize envelopeSize_;

    // thumbnail request queue and image cache:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable ready_;
    boost::uint64_t submitted_;
    boost::uint64_t completed_;
    std::map<QString, Request> request_;
    std::map<boost::uint64_t, Request *> priority_;

    // how many items can be kept in the cache:
    std::size_t cacheCapacity_;

    // image cache, along with MRU timestamp:
    std::map<QString, Payload> cache_;

    // most-recently-used map of image ids, used
    // to decide which image should be removed from the cache:
    std::map<boost::uint64_t, Payload *> mru_;

    // request processing worker thread:
    Thread<ThumbnailProvider::TPrivate> thread_;
  };

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::TPrivate
  //
  ThumbnailProvider::TPrivate::TPrivate(const TReaderFactoryPtr & readerFactory,
                                        const TGetFilePathPtr & getFilePath,
                                        const QSize & envelopeSize,
                                        std::size_t cacheCapacity):
    readerFactory_(readerFactory),
    getFilePath_(getFilePath),
    envelopeSize_(envelopeSize),
    submitted_(0),
    completed_(0),
    cacheCapacity_(cacheCapacity),
    thread_(this)
  {}

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::~TPrivate
  //
  ThumbnailProvider::TPrivate::~TPrivate()
  {
    thread_.interrupt();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::setCacheCapacity
  //
  void
  ThumbnailProvider::TPrivate::setCacheCapacity(std::size_t cacheCapacity)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    cacheCapacity_ = cacheCapacity;
    clearCacheFor(0);
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::clearCacheFor
  //
  void
  ThumbnailProvider::TPrivate::clearCacheFor(std::size_t n)
  {
    // discard cache entries until there is room for the requested entries:
    while (!cache_.empty() && cache_.size() + n >= cacheCapacity_)
    {
      Payload oldest = *(mru_.begin()->second);
      cache_.erase(oldest.id_);
      mru_.erase(oldest.mru_);
    }
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::requestImage
  //
  QImage
  ThumbnailProvider::TPrivate::requestImage(const QString & id,
                                            QSize * size,
                                            const QSize & requestedSize)
  {
    Payload * payload = NULL;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      std::map<QString, Payload>::iterator found = cache_.find(id);

      if (found != cache_.end())
      {
        payload = &(found->second);
        mru_.erase(payload->mru_);
        payload->mru_ = completed_;
        completed_++;
        mru_[payload->mru_] = payload;
      }
      else
      {
        clearCacheFor(1);
      }
    }

    const QSize & thumbnailMaxSize =
      requestedSize.isValid() ? requestedSize : envelopeSize_;

    if (!payload)
    {
      const GetFilePath & get_file_path = *getFilePath_;
      QString itemFilePath = get_file_path(id);

#if 0
      yae_debug
        << "\nFIXME: getThumbnail"
        << "\n item id: " << id.toUtf8().constData()
        << "\nfilepath: " << itemFilePath.toUtf8().constData()
        << "\n";
#endif

      QImage image = getThumbnail(readerFactory_,
                                  thumbnailMaxSize,
                                  itemFilePath);

      bool sizeAcceptable =
        (image.height() <= thumbnailMaxSize.height() &&
         image.width() <= thumbnailMaxSize.width());

      if (!sizeAcceptable)
      {
        image = image.scaledToHeight(90, Qt::SmoothTransformation);
      }

      boost::unique_lock<boost::mutex> lock(mutex_);
      std::map<QString, Payload>::iterator where = cache_.
        insert(std::make_pair(id, Payload(id, image, completed_))).first;
      completed_++;

      payload = &(where->second);
      mru_[payload->mru_] = payload;
    }

    if (size)
    {
      *size = payload->image_.size();
    }

    return payload->image_;
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
      Request request(id, requestedSize, callback, submitted_);
      found = request_.insert(found, std::make_pair(id, request));
      priority_[request.priority_] = &(found->second);
    }
    else
    {
      // re-prioritize previous request:
      Request & request = found->second;
      priority_.erase(request.priority_);
      request.priority_ = submitted_;
      priority_[request.priority_] = &request;
    }

    submitted_++;
    ready_.notify_all();
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::TPrivate::cancelRequest
  //
  void
  ThumbnailProvider::TPrivate::cancelRequest(const QString & id)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
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
  // ThumbnailProvider::TPrivate::thread_loop
  //
  void
  ThumbnailProvider::TPrivate::thread_loop()
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
  ThumbnailProvider::ThumbnailProvider(const TReaderFactoryPtr & readerFactory,
                                       const TGetFilePathPtr & getFilePath,
                                       const QSize & envelopeSize,
                                       std::size_t cacheCapacity):
    ImageProvider(),
    private_(new TPrivate(readerFactory,
                          getFilePath,
                          envelopeSize,
                          cacheCapacity))
  {}

  //----------------------------------------------------------------
  // ThumbnailProvider::~ThumbnailProvider
  //
  ThumbnailProvider::~ThumbnailProvider()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ThumbnailProvider::setCacheCapacity
  //
  void
  ThumbnailProvider::setCacheCapacity(std::size_t cacheCapacity)
  {
    private_->setCacheCapacity(cacheCapacity);
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
  // ThumbnailProvider::cancelRequest
  //
  void
  ThumbnailProvider::cancelRequest(const QString & id)
  {
    private_->cancelRequest(id);
  }

}

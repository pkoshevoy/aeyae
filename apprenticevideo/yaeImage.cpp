// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>
#include <QString>
#include <QUrl>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeImage.h"
#include "yaePlaylistView.h"
#include "yaeTexture.h"
#include "yaeThumbnailProvider.h"


namespace yae
{

  //----------------------------------------------------------------
  // ImagePrivate
  //
  struct ImagePrivate : public ThumbnailProvider::ICallback
  {
    typedef PlaylistView::TImageProviderPtr TImageProviderPtr;

    enum Status
    {
      kImageNotReady,
      kImageRequested,
      kImageReady
    };

    ImagePrivate():
      view_(NULL),
      status_(kImageNotReady)
    {}

    inline void setContext(PlaylistView & view)
    { view_ = &view; }

    // virtual:
    void imageReady(const QImage & image)
    {
      // update the image:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        img_ = image;
        status_ = kImageReady;
      }

      if (view_)
      {
        view_->delegate()->requestRepaint();
      }
    }

    void clearImage()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      img_ = QImage();
      status_ = kImageNotReady;
    }

    void clearImageAndCancelRequest()
    {
      if (provider_)
      {
        provider_->cancelRequest(id_);
      }

      boost::lock_guard<boost::mutex> lock(mutex_);
      img_ = QImage();
      status_ = kImageNotReady;
    }

    inline void setImageStatusImageRequested()
    { status_ = kImageRequested; }

    inline bool isImageRequested() const
    { return status_ == kImageRequested; }

    inline bool isImageReady() const
    { return status_ == kImageReady; }

    QImage getImage() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return img_;
    }

    const PlaylistView * view_;
    TImageProviderPtr provider_;
    QString resource_;
    QString id_;

  protected:
    mutable boost::mutex mutex_;
    Status status_;
    QImage img_;
  };

  //----------------------------------------------------------------
  // Image::TPrivate
  //
  struct Image::TPrivate
  {
    typedef PlaylistView::TImageProviders TImageProviders;

    TPrivate();
    ~TPrivate();

    inline void setContext(PlaylistView & view)
    {
      image_->setContext(view);
    }

    void unpaint();
    bool load(const QString & thumbnail);
    bool uploadTexture(const Image & item);
    void paint(const Image & item);

    boost::shared_ptr<ImagePrivate> image_;
    BoolRef ready_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
  };

  //----------------------------------------------------------------
  // Image::TPrivate::TPrivate
  //
  Image::TPrivate::TPrivate():
    image_(new ImagePrivate()),
    texId_(0),
    iw_(0),
    ih_(0)
  {}

  //----------------------------------------------------------------
  // Image::TPrivate::~TPrivate
  //
  Image::TPrivate::~TPrivate()
  {
    unpaint();
  }

  //----------------------------------------------------------------
  // Image::TPrivate::unpaint
  //
  void
  Image::TPrivate::unpaint()
  {
    // shortcut:
    ImagePrivate & image = *image_;
    image.clearImageAndCancelRequest();

    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::load
  //
  bool
  Image::TPrivate::load(const QString & resource)
  {
    // shortcut:
    ImagePrivate & image = *image_;

    if (image.resource_ == resource)
    {
      if (image.isImageReady())
      {
        // already loaded:
        return true;
      }
      else if (image.isImageRequested())
      {
        // wait for the image request to be processed:
        return false;
      }
    }

    static const QString kImage = QString::fromUtf8("image");
    QUrl url(resource);
    if (url.scheme() != kImage || !image.view_)
    {
      YAE_ASSERT(false);
      return false;
    }

    QString host = url.host();
    const TImageProviders & providers = image.view_->imageProviders();
    TImageProviders::const_iterator found = providers.find(host);
    if (found == providers.end())
    {
      YAE_ASSERT(false);
      return false;
    }

    QString id = url.path();

    // trim the leading '/' character:
    id = id.right(id.size() - 1);

    image.provider_ = found->second;
    image.resource_ = resource;
    image.id_ = id;
    image.clearImage();
    image.setImageStatusImageRequested();

    static const QSize kDefaultSize(256, 256);
    ThumbnailProvider & provider = *(image.provider_);
    boost::weak_ptr<ThumbnailProvider::ICallback> callback(image_);
    provider.requestImageAsync(image.id_, kDefaultSize, callback);
    return false;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::uploadTexture
  //
  bool
  Image::TPrivate::uploadTexture(const Image & item)
  {
    bool ok = yae::uploadTexture2D(image_->getImage(), texId_, iw_, ih_,
                                   GL_LINEAR);

    // no need to keep a duplicate image around once the texture is ready:
    image_->clearImage();

    return ok;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::paint
  //
  void
  Image::TPrivate::paint(const Image & item)
  {
    if (!texId_ && !load(item.url_.get().toString()))
    {
      // image is not yet loaded:
      return;
    }

    if (!ready_.get())
    {
      YAE_ASSERT(false);
      return;
    }

    BBox bbox;
    item.get(kPropertyBBox, bbox);

    double arBBox = bbox.aspectRatio();
    double arImage = double(iw_) / double(ih_);

    if (arImage < arBBox)
    {
      // letterbox pillars:
      double w = bbox.h_ * arImage;
      double dx = (bbox.w_ - w) * 0.5;
      bbox.x_ += dx;
      bbox.w_ = w;
    }
    else if (arBBox < arImage)
    {
      double h = bbox.w_ / arImage;
      double dy = (bbox.h_ - h) * 0.5;
      bbox.y_ += dy;
      bbox.h_ = h;
    }

    paintTexture2D(bbox, texId_, iw_, ih_);
  }

  //----------------------------------------------------------------
  // Image::Image
  //
  Image::Image(const char * id):
    Item(id),
    p_(new Image::TPrivate())
  {
    p_->ready_ = addExpr(new UploadTexture<Image>(*this));
  }

  //----------------------------------------------------------------
  // Image::~Image
  //
  Image::~Image()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Image::setContext
  //
  void
  Image::setContext(PlaylistView & view)
  {
    p_->setContext(view);
  }

  //----------------------------------------------------------------
  // Image::uncache
  //
  void
  Image::uncache()
  {
    url_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Image::paintContent
  //
  void
  Image::paintContent() const
  {
    p_->paint(*this);
  }

  //----------------------------------------------------------------
  // Image::unpaintContent
  //
  void
  Image::unpaintContent() const
  {
    p_->unpaint();
  }

}

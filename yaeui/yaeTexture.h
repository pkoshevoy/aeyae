// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXTURE_H_
#define YAE_TEXTURE_H_

// aeyae:
#include "yae/api/yae_api.h"

// Qt:
#include <QImage>

// yaeui:
#include "yaeCanvasRenderer.h"
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // downsampleImage
  //
  // Supersampled texture should not be uploaded at full resolution,
  // it should be scaled down first:
  //
  // returns power-of-two factor that was used to scale down
  // given supersampled image
  //
  unsigned int
  downsampleImage(QImage & img, double supersampled);

  //----------------------------------------------------------------
  // uploadTexture2D
  //
  bool
  uploadTexture2D(const QImage & img,
                  GLuint & texId,
                  GLenum textureFilterMin,
                  GLenum textureFilterMag = GL_LINEAR);

  //----------------------------------------------------------------
  // paintTexture2D
  //
  void
  paintTexture2D(const BBox & bbox,
                 GLuint texId,
                 GLuint iw,
                 GLuint ih,
                 double opacity);


  //----------------------------------------------------------------
  // UploadTexture
  //
  template <typename TItem>
  struct UploadTexture : public TBoolExpr
  {
    UploadTexture(const TItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = item_.p_->uploadTexture(item_);
    }

    const TItem & item_;
  };


  //----------------------------------------------------------------
  // Texture
  //
  class Texture : public Item
  {
    Texture(const Texture &);
    Texture & operator = (const Texture &);

  public:
    Texture(const char * id, const QImage & image);
    ~Texture();

    // helpers:
    void setImage(const QImage & image);
    unsigned int getImageWidth() const;
    unsigned int getImageHeight() const;

    // virtual:
    void get(Property property, double & value) const;

    bool bind(double & uMax, double & vMax) const;
    void unbind() const;

    // keep implementation details private:
    struct TPrivate;
    TPrivate * p_;
  };

  //----------------------------------------------------------------
  // TTexturePtr
  //
  typedef yae::shared_ptr<Texture, Item> TTexturePtr;

  //----------------------------------------------------------------
  // TTextureRef
  //
  typedef DataRef<TTexturePtr> TTextureRef;

  //----------------------------------------------------------------
  // TTextureExpr
  //
  typedef Expression<TTexturePtr> TTextureExpr;

}


#endif // YAE_TEXTURE_H_

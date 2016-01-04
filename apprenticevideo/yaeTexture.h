// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXTURE_H_
#define YAE_TEXTURE_H_

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // uploadTexture2D
  //
  bool
  uploadTexture2D(const QImage & img,
                  GLuint & texId,
                  GLuint & iw,
                  GLuint & ih,
                  GLenum textureFilterMin,
                  GLenum textureFilterMag = GL_LINEAR);

  //----------------------------------------------------------------
  // paintTexture2D
  //
  void
  paintTexture2D(const BBox & bbox, GLuint texId, GLuint iw, GLuint ih);


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

    // helper:
    void setImage(const QImage & image);

    // virtual:
    void get(Property property, double & value) const;

    bool bind(double & uMax, double & vMax) const;
    void unbind() const;

    // keep implementation details private:
    struct TPrivate;
    TPrivate * p_;
  };

}


#endif // YAE_TEXTURE_H_

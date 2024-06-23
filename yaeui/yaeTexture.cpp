// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeCanvasQPainterUtils.h"
#include "yaeCanvasRenderer.h"
#include "yaeTexture.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // downsampleImage
  //
  unsigned int
  downsampleImage(QImage & img, double supersampled)
  {
    unsigned int n = 1;

    while (supersampled >= 2.0)
    {
      n *= 2;
      supersampled /= 2.0;
    }

    if (n > 1)
    {
      int iw = img.width();
      int ih = img.height();
      img = img.scaled(iw / n,
                       ih / n,
                       Qt::IgnoreAspectRatio,
                       Qt::SmoothTransformation);
    }

    return n;
  }

  //----------------------------------------------------------------
  // uploadTexture2D
  //
  bool
  uploadTexture2D(const QImage & img,
                  GLuint & texId,
                  GLenum textureFilterMin,
                  GLenum textureFilterMag)
  {
    QImage::Format imgFormat = img.format();

    TPixelFormatId formatId = pixelFormatIdFor(imgFormat);
    const pixelFormat::Traits * ptts = pixelFormat::getTraits(formatId);
    if (!ptts)
    {
      YAE_ASSERT(false);
      return false;
    }

    unsigned char stride[4] = { 0 };
    unsigned char planes = ptts->getPlanes(stride);
    if (planes > 1 || stride[0] % 8)
    {
      YAE_ASSERT(false);
      return false;
    }

    GLuint iw = img.width();
    GLuint ih = img.height();
    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));
    YAE_OGL_11(glDeleteTextures(1, &texId));
    YAE_OGL_11(glGenTextures(1, &texId));
    if (!texId)
    {
      return false;
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId));
    if (!YAE_OGL_11(glIsTexture(texId)))
    {
      YAE_ASSERT(false);
      return false;
    }

    GLint mipmap =
      (textureFilterMin == GL_LINEAR_MIPMAP_LINEAR) ? GL_TRUE : GL_FALSE;
    GLint mipmapLevels =
      mipmap ? floor_log2(std::min<GLuint>(iw, ih)) : 0;

    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_GENERATE_MIPMAP,
                               GL_TRUE));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_WRAP_S,
                               GL_CLAMP_TO_EDGE));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_WRAP_T,
                               GL_CLAMP_TO_EDGE));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_BASE_LEVEL,
                               0));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MAX_LEVEL,
                               mipmapLevels));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MAG_FILTER,
                               textureFilterMag));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MIN_FILTER,
                               textureFilterMin));
    yae_assert_gl_no_error();

    if (mipmap)
    {
      YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, mipmap));
      yae_assert_gl_no_error();
    }

    GLint internalFormat = 0;
    GLenum pixelFormatGL = 0;
    GLenum dataType = 0;
    GLint shouldSwapBytes = 0;

    yae_to_opengl(formatId,
                  internalFormat,
                  pixelFormatGL,
                  dataType,
                  shouldSwapBytes);

    YAE_OGL_11(glTexImage2D(GL_TEXTURE_2D,
                            0, // mipmap level
                            internalFormat,
                            widthPowerOfTwo,
                            heightPowerOfTwo,
                            0, // border width
                            pixelFormatGL,
                            dataType,
                            NULL));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SWAP_BYTES,
                             shouldSwapBytes));

    const QImage & constImg = img;
    const unsigned char * data = constImg.bits();
    const unsigned char bytesPerPixel = stride[0] >> 3;
    const int bytesPerRow = constImg.bytesPerLine();
    const int rowSize = bytesPerRow / bytesPerPixel;
    const int padding = alignmentFor(data, bytesPerRow);

    YAE_OGL_11(glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint)(padding)));
    YAE_OGL_11(glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(rowSize)));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    yae_assert_gl_no_error();

    YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                               0, // mipmap level
                               0, // x-offset
                               0, // y-offset
                               iw,
                               ih,
                               pixelFormatGL,
                               dataType,
                               data));
    yae_assert_gl_no_error();

    if (ih < (unsigned int)heightPowerOfTwo)
    {
      // copy the padding row:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, ih - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 0, // x-offset
                                 ih, // y-offset
                                 iw,
                                 1,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    if (iw < (unsigned int)widthPowerOfTwo)
    {
      // copy the padding column:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, iw - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 iw, // x-offset
                                 0, // y-offset
                                 1,
                                 ih,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    if (ih < (unsigned int)heightPowerOfTwo &&
        iw < (unsigned int)widthPowerOfTwo)
    {
      // copy the bottom-right padding corner:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, ih - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, iw - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 iw, // x-offset
                                 ih, // y-offset
                                 1,
                                 1,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
    return true;
  }

  //----------------------------------------------------------------
  // paintTexture2D
  //
  void
  paintTexture2D(const BBox & bbox,
                 GLuint texId,
                 GLuint iw,
                 GLuint ih,
                 double opacity)
  {
    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    double u0 = 0.0;
    double u1 = double(iw) / double(widthPowerOfTwo);

    double v0 = 0.0;
    double v1 = double(ih) / double(heightPowerOfTwo);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = x0 + bbox.w_;
    double y1 = y0 + bbox.h_;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));

    YAE_OPENGL_HERE();
    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    YAE_OGL_11(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    YAE_OGL_11(glColor4d(1.0, 1.0, 1.0, opacity));

    {
      yaegl::BeginEnd mode(GL_TRIANGLE_STRIP);
      YAE_OGL_11(glTexCoord2d(u0, v0));
      YAE_OGL_11(glVertex2d(x0, y0));

      YAE_OGL_11(glTexCoord2d(u0, v1));
      YAE_OGL_11(glVertex2d(x0, y1));

      YAE_OGL_11(glTexCoord2d(u1, v0));
      YAE_OGL_11(glVertex2d(x1, y0));

      YAE_OGL_11(glTexCoord2d(u1, v1));
      YAE_OGL_11(glVertex2d(x1, y1));
    }

    // un-bind:
    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, 0));
    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
  }

  //----------------------------------------------------------------
  // Texture::TPrivate
  //
  struct Texture::TPrivate
  {
    TPrivate(const QImage & image);
    ~TPrivate();

    bool uploadTexture(const Texture & item);
    bool bind(double & uMax, double & vMax) const;
    void unbind() const;

    BoolRef ready_;
    QImage image_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
    double u1_;
    double v1_;
  };

  //----------------------------------------------------------------
  // Texture::TPrivate::TPrivate
  //
  Texture::TPrivate::TPrivate(const QImage & image):
    image_(image),
    texId_(0),
    iw_(image.width()),
    ih_(image.height()),
    u1_(0.0),
    v1_(0.0)
  {}

  //----------------------------------------------------------------
  // Texture::TPrivate::~TPrivate
  //
  Texture::TPrivate::~TPrivate()
  {
    if (texId_)
    {
      YAE_OGL_11_HERE();
      YAE_OGL_11(glDeleteTextures(1, &texId_));
      texId_ = 0;
    }
  }

  //----------------------------------------------------------------
  // Texture::TPrivate::uploadTexture
  //
  bool
  Texture::TPrivate::uploadTexture(const Texture & item)
  {
    bool ok = yae::uploadTexture2D(image_, texId_,
                                   // should this be a user option?
                                   GL_LINEAR_MIPMAP_LINEAR);

    if (ok)
    {
      // no need to keep the image around once the texture is ready:
      image_ = QImage();

      GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw_);
      GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih_);

      u1_ = double(iw_) / double(widthPowerOfTwo);
      v1_ = double(ih_) / double(heightPowerOfTwo);
    }

    return ok;
  }

  //----------------------------------------------------------------
  // Texture::TPrivate::bind
  //
  bool
  Texture::TPrivate::bind(double & uMax, double & vMax) const
  {
    if (!ready_.get())
    {
      YAE_ASSERT(false);
      return false;
    }

    uMax = u1_;
    vMax = v1_;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));

    YAE_OPENGL_HERE();
    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId_));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    YAE_OGL_11(glColor3f(1.f, 1.f, 1.f));
    YAE_OGL_11(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));

    return true;
  }

  //----------------------------------------------------------------
  // Texture::TPrivate::unbind
  //
  void
  Texture::TPrivate::unbind() const
  {
    if (!texId_)
    {
      return;
    }

    YAE_OPENGL_HERE();
    YAE_OGL_11_HERE();

    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, 0));
    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
  }

  //----------------------------------------------------------------
  // Texture::Texture
  //
  Texture::Texture(const char * id, const QImage & image):
    Item(id),
    p_(NULL)
  {
    setImage(image);
  }

  //----------------------------------------------------------------
  // Texture::~Texture
  //
  Texture::~Texture()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Texture::setImage
  //
  void
  Texture::setImage(const QImage & image)
  {
    delete p_;
    p_ = new TPrivate(image);
    p_->ready_ = addExpr(new UploadTexture<Texture>(*this));
  }

  //----------------------------------------------------------------
  // Texture::get
  //
  void
  Texture::get(Property property, double & value) const
  {
    if (property == kPropertyWidth)
    {
      value = double(p_->iw_);
    }
    else if (property == kPropertyHeight)
    {
      value = double(p_->ih_);
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // Texture::getImageWidth
  //
  unsigned int
  Texture::getImageWidth() const
  { return p_->iw_; }

  //----------------------------------------------------------------
  // Texture::getImageHeight
  //
  unsigned int
  Texture::getImageHeight() const
  { return p_->ih_; }

  //----------------------------------------------------------------
  // Texture::bind
  //
  bool
  Texture::bind(double & uMax, double & vMax) const
  {
    return p_->bind(uMax, vMax);
  }

  //----------------------------------------------------------------
  // Texture::unbind
  //
  void
  Texture::unbind() const
  {
    p_->unbind();
  }

}

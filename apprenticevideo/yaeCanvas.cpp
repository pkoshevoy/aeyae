// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <math.h>

// GLEW includes:
#include <GL/glew.h>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>
#include <yaePixelFormatTraits.h>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>


//----------------------------------------------------------------
// yae_to_opengl
// 
unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes)
{
  shouldSwapBytes = GL_FALSE;
  
  switch (yaePixelFormat)
  {
    case yae::kPixelFormatYUYV422:
      //! packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    case yae::kPixelFormatUYVY422:
      //! packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
      
      if (glewIsExtensionSupported("GL_APPLE_ycbcr_422"))
      {
        internalFormat = 3;
        format = GL_YCBCR_422_APPLE;
#ifdef __BIG_ENDIAN__
        dataType =
          yaePixelFormat == yae::kPixelFormatYUYV422 ?
          GL_UNSIGNED_SHORT_8_8_APPLE :
          GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#else
        dataType =
          yaePixelFormat == yae::kPixelFormatYUYV422 ?
          GL_UNSIGNED_SHORT_8_8_REV_APPLE :
          GL_UNSIGNED_SHORT_8_8_APPLE;
#endif
        return 3;
      }
      break;
      
    case yae::kPixelFormatYUV420P:
      //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case yae::kPixelFormatYUV422P:
      //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case yae::kPixelFormatYUV444P:
      //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case yae::kPixelFormatYUV410P:
      //! planar YUV 4:1:0, 9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    case yae::kPixelFormatYUV411P:
      //! planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    case yae::kPixelFormatGRAY8:
      //! Y, 8bpp
    case yae::kPixelFormatPAL8:
      //! 8 bit with kPixelFormatRGB32 palette
    case yae::kPixelFormatNV12:
      //! planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV
      //! components, which are interleaved (first byte U and the
      //! following byte V)
    case yae::kPixelFormatNV21:
      //! as above, but U and V bytes are swapped
    case yae::kPixelFormatYUV440P:
      //! planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    case yae::kPixelFormatYUVA420P:
      //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
      //! samples)
    case yae::kPixelFormatYUVJ420P:
      //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples), JPEG
    case yae::kPixelFormatYUVJ422P:
      //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ444P:
      //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ440P:
      //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples), JPEG
      
      internalFormat = GL_LUMINANCE;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_BYTE;
      return 1;
      
    case yae::kPixelFormatRGB24:
      //! packed RGB 8:8:8, 24bpp, RGBRGB...
      internalFormat = GL_RGB;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE;
      return 3;
      
    case yae::kPixelFormatBGR24:
      //! packed RGB 8:8:8, 24bpp, BGRBGR...
      internalFormat = 3;
      format = GL_BGR;
      dataType = GL_UNSIGNED_BYTE;
      return 3;
      
    case yae::kPixelFormatRGB8:
      //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_3_3_2;
      return 3;
      
    case yae::kPixelFormatBGR8:
      //! packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_2_3_3_REV;
      return 3;
      
    case yae::kPixelFormatARGB:
      //! packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
      internalFormat = 4;
      format = GL_BGRA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#endif
      return 4;
      
    case yae::kPixelFormatRGBA:
      //! packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
      internalFormat = 4;
      format = GL_RGBA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      return 4;
      
    case yae::kPixelFormatABGR:
      //! packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
      internalFormat = 4;
      format = GL_RGBA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#endif
      return 4;
      
    case yae::kPixelFormatBGRA:
      //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
      internalFormat = 4;
      format = GL_BGRA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      return 4;
      
    case yae::kPixelFormatGRAY16BE:
      //! Y, 16bpp, big-endian
    case yae::kPixelFormatYUV420P16BE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV422P16BE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV444P16BE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;
      
    case yae::kPixelFormatGRAY16LE:
      //! Y, 16bpp, little-endian
    case yae::kPixelFormatYUV420P16LE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV422P16LE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV444P16LE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;
      
    case yae::kPixelFormatRGB48BE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB16;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT;
      return 3;
      
    case yae::kPixelFormatRGB48LE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB16;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT;
      return 3;
      
    case yae::kPixelFormatRGB565BE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5;
      return 3;
      
    case yae::kPixelFormatRGB565LE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5;
      return 3;
      
    case yae::kPixelFormatBGR565BE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5_REV;
      return 3;
      
    case yae::kPixelFormatBGR565LE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB555BE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
      //! most significant bit to 0
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB555LE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
      //! most significant bit to 0
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatBGR555BE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
      //! most significant bit to 1
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatBGR555LE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
      //! most significant bit to 1
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB444BE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
      //! most significant bits to 0
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;
      
    case yae::kPixelFormatRGB444LE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
      //! most significant bits to 0
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;
      
    case yae::kPixelFormatBGR444BE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
      //! most significant bits to 1
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;
      
    case yae::kPixelFormatBGR444LE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
      //! most significant bits to 1
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;
      
    case yae::kPixelFormatY400A:
      //! 8bit gray, 8bit alpha
      internalFormat = GL_LUMINANCE8_ALPHA8;
      format = GL_LUMINANCE_ALPHA;
      dataType = GL_UNSIGNED_BYTE;
      return 2;
      
    default:
      break;
  }
  
  return 0;
}

namespace yae
{

  //----------------------------------------------------------------
  // TGLSaveState
  // 
  struct TGLSaveState
  {
    TGLSaveState(GLbitfield mask):
      applied_(false)
    {
      glPushAttrib(mask);
      
      GLenum err = glGetError();
      if (err == GL_NO_ERROR)
      {
        applied_ = true;
      }
      else
      {
        const GLubyte * str = gluErrorString(err);
        std::cerr << "GL_ERROR: " << str << std::endl;
      }
    }
    
    ~TGLSaveState()
    {
      if (applied_)
      {
        glPopAttrib();
      }
    }
    
  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveClientState
  // 
  struct TGLSaveClientState
  {
    TGLSaveClientState(GLbitfield mask):
      applied_(false)
    {
      glPushClientAttrib(mask);
      
      GLenum err = glGetError();
      if (err == GL_NO_ERROR)
      {
        applied_ = true;
      }
      else
      {
        const GLubyte * str = gluErrorString(err);
        std::cerr << "GL_ERROR: " << str << std::endl;
      }
    }
    
    ~TGLSaveClientState()
    {
      if (applied_)
      {
        glPopClientAttrib();
      }
    }
    
  protected:
    bool applied_;
  };
  
  //----------------------------------------------------------------
  // powerOfTwoLEQ
  // 
  // calculate largest power-of-two less then or equal to the given
  // 
  template <typename TScalar>
  inline static TScalar
  powerOfTwoLEQ(const TScalar & given)
  {
    const std::size_t n = sizeof(given) * 8;
    TScalar smaller = TScalar(0);
    TScalar closest = TScalar(1);
    for (std::size_t i = 0; (i < n) && (closest <= given); i++)
    {
      smaller = closest;
      closest *= TScalar(2);
    }
    
    return smaller;
  }
  
  //----------------------------------------------------------------
  // Tuple
  // 
  template <std::size_t size, typename TScalar>
  struct Tuple
  {
    enum { kSize = size };
    TScalar data_[size];
    
    inline TScalar & operator [] (std::size_t i)
    { return data_[i]; }
    
    inline const TScalar & operator [] (std::size_t i) const
    { return data_[i]; }
  };
  
  //----------------------------------------------------------------
  // TMakeCurrentContext
  // 
  struct TMakeCurrentContext
  {
    TMakeCurrentContext(QGLWidget * canvas):
      canvas_(canvas)
    {
      canvas_->makeCurrent();
    }
    
    ~TMakeCurrentContext()
    {
      canvas_->doneCurrent();
    }
    
    QGLWidget * canvas_;
  };
  
  
  //----------------------------------------------------------------
  // Canvas::TPrivate
  // 
  class Canvas::TPrivate
  {
  public:
    virtual ~TPrivate() {}
    
    virtual bool loadFrame(QGLWidget * canvas,
                           const TVideoFramePtr & frame) = 0;
    virtual void draw() = 0;
    
    // helper:
    inline const pixelFormat::Traits * pixelTraits() const
    {
      return (frame_ ?
              pixelFormat::getTraits(frame_->traits_.pixelFormat_) :
              NULL);
    }
    
    inline double imageWidth() const
    {
      if (frame_)
      {
        if (frame_->traits_.pixelAspectRatio_ != 0.0)
        {
          return floor(double(frame_->traits_.visibleWidth_) *
                       frame_->traits_.pixelAspectRatio_ +
                       0.5);
        }

        return double(frame_->traits_.visibleWidth_);
      }
      
      return 0.0;
    }

    inline double imageHeight() const
    {
      return (frame_ ? double(frame_->traits_.visibleHeight_) : 0.0);
    }
    
  protected:
    mutable boost::mutex mutex_;
    TVideoFramePtr frame_;
  };
  
  //----------------------------------------------------------------
  // TModernCanvas
  // 
  struct TModernCanvas : public Canvas::TPrivate
  {
    TModernCanvas();

    // virtual:
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);
    
    // virtual:
    void draw();

  protected:
    GLuint texId_;
  };
  
  //----------------------------------------------------------------
  // TModernCanvas::TModernCanvas
  // 
  TModernCanvas::TModernCanvas():
    texId_(0)
  {}
  
  //----------------------------------------------------------------
  // TModernCanvas::loadFrame
  // 
  bool
  TModernCanvas::loadFrame(QGLWidget * canvas,
                              const TVideoFramePtr & frame)
  {
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;
    
    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);
    
    if (!ptts)
    {
      // don't know how to handle this pixel format:
      return false;
    }
    
    GLint internalFormatGL;
    GLenum pixelFormatGL;
    GLenum dataTypeGL;
    GLint shouldSwapBytes;
    unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                   internalFormatGL,
                                                   pixelFormatGL,
                                                   dataTypeGL,
                                                   shouldSwapBytes);
    if (!supportedChannels)
    {
      return false;
    }
    
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);
    
    glDeleteTextures(1, &texId_);
    texId_ = 0;
    
    frame_ = frame;
    
    glGenTextures(1, &texId_);
    
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texId_);

    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    {
      glPixelStorei(GL_UNPACK_SWAP_BYTES, shouldSwapBytes);
      
      glPixelStorei(GL_UNPACK_ROW_LENGTH,
                    frame->sampleBuffer_->rowBytes(0) /
                    (ptts->stride_[0] / 8));

      // order of bits in a byte only matters for bitmaps:
      // glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);

      // use this to crop the image perhaps?
      // glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels_ + offset_x);
      // glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows_ + offset_y);
      
      if (glewIsExtensionSupported("GL_APPLE_client_storage"))
      {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
      }
      
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_RECTANGLE_EXT,
                   0, // always level-0 for GL_TEXTURE_RECTANGLE_EXT
                   internalFormatGL,
                   vtts.encodedWidth_,
                   vtts.encodedHeight_,
                   0, // border width
                   pixelFormatGL,
                   dataTypeGL,
                   frame->sampleBuffer_->samples(0));
    }
    return true;
  }

  //----------------------------------------------------------------
  // TModernCanvas::draw
  // 
  void
  TModernCanvas::draw()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (!texId_)
    {
      return;
    }
    
    // video traits shortcut:
    const VideoTraits & vtts = frame_->traits_;
    
    glEnable(GL_TEXTURE_RECTANGLE_EXT);
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texId_);
    
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
#if 0
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_MAX_LEVEL, 0);
#endif
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.f, 1.f, 1.f);
    
    glBegin(GL_QUADS);
    {
      glTexCoord2i(vtts.offsetLeft_,
                   vtts.offsetTop_);
      glVertex2i(0, 0);
      
      glTexCoord2i(vtts.offsetLeft_ + vtts.visibleWidth_ - 1,
                   vtts.offsetTop_);
      glVertex2i(int(imageWidth()), 0);
      
      glTexCoord2i(vtts.offsetLeft_ + vtts.visibleWidth_ - 1,
                   vtts.offsetTop_ + vtts.visibleHeight_ - 1);
      glVertex2i(int(imageWidth()), vtts.visibleHeight_);
      
      glTexCoord2i(vtts.offsetLeft_,
                   vtts.offsetTop_ + vtts.visibleHeight_ - 1);
      glVertex2i(0, vtts.visibleHeight_);
    }
    glEnd();
  }

  //----------------------------------------------------------------
  // TFrameTile
  // 
  struct TFrameTile
  {
    // quad coordinates:
    GLsizei x_;
    GLsizei y_;

    // quad width, height:
    GLsizei w_;
    GLsizei h_;
    
    // texture coordinates:
    GLdouble s0_;
    GLdouble s1_;
    GLdouble t0_;
    GLdouble t1_;
  };
  
  //----------------------------------------------------------------
  // TLegacyCanvas
  // 
  // This is a subclass implementing frame rendering on OpenGL
  // hardware that doesn't support GL_EXT_texture_rectangle
  // 
  struct TLegacyCanvas : public Canvas::TPrivate
  {
    // virtual:
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);
    
    // virtual:
    void draw();
    
  protected:
    // unpadded image dimensions:
    GLsizei w_;
    GLsizei h_;
    
    // additional padding due to odd image dimensions:
    GLsizei wOdd_;
    GLsizei hOdd_;
    
    // padded image dimensions:
    GLsizei wPadded_;
    GLsizei hPadded_;
    
    // padded image texture data:
    std::vector<unsigned char> textureData_;

    std::vector<TFrameTile> tiles_;
    std::vector<GLuint> texId_;
  };

  //----------------------------------------------------------------
  // TEdge
  // 
  typedef Tuple<2, GLsizei> TEdge;

  //----------------------------------------------------------------
  // calculateEdges
  // 
  static void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgePadded,
                 GLsizei edgeMax)
  {
    if (!edgePadded)
    {
      return;
    }

    GLsizei offset = 0;
    GLsizei extent = edgePadded;
    while (true)
    {
      edges.push_back(TEdge());
      TEdge & edge = edges.back();
      
      edge[0] = offset;
      edge[1] = std::min(edgeMax, powerOfTwoLEQ(extent));
      
      if (edge[1] == extent)
      {
        break;
      }
        
      offset += (edge[1] - 2);
      extent -= (edge[1] - 2);
    }
  }
  
  //----------------------------------------------------------------
  // TLegacyCanvas::loadFrame
  // 
  bool
  TLegacyCanvas::loadFrame(QGLWidget * canvas,
                           const TVideoFramePtr & frame)
  {
    // FIXME: I should try to calculate this at run-time:
    static const GLsizei textureEdgeMax = 2048;
    
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;
    
    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);
    
    if (!ptts)
    {
      // don't know how to handle this pixel format:
      return false;
    }
    
    GLint internalFormatGL;
    GLenum pixelFormatGL;
    GLenum dataTypeGL;
    GLint shouldSwapBytes;
    unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                   internalFormatGL,
                                                   pixelFormatGL,
                                                   dataTypeGL,
                                                   shouldSwapBytes);
    if (!supportedChannels)
    {
      return false;
    }
    
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    bool mayReuseTextures =
      frame_ && frame &&
      frame_->traits_.pixelFormat_ == frame->traits_.pixelFormat_ &&
      frame_->traits_.encodedWidth_ == frame->traits_.encodedWidth_ &&
      frame_->traits_.encodedHeight_ == frame->traits_.encodedHeight_;

    // take the new frame:
    frame_ = frame;
    
    if (!mayReuseTextures)
    {
      if (!texId_.empty())
      {
        glDeleteTextures(texId_.size(), &(texId_.front()));
        texId_.clear();
        textureData_.clear();
      }
      
      w_ = frame_->traits_.visibleWidth_;
      h_ = frame_->traits_.visibleHeight_;
      
      wOdd_ = (GLsizei)(w_ & 0x01);
      hOdd_ = (GLsizei)(h_ & 0x01);
      
      wPadded_ = w_ + 2 + wOdd_;
      hPadded_ = h_ + 2 + hOdd_;
      
      // calculate x-min, x-max coordinates for each tile:
      std::deque<TEdge> x;
      calculateEdges(x, wPadded_, textureEdgeMax);
      
      // calculate y-min, y-max coordinates for each tile:
      std::deque<TEdge> y;
      calculateEdges(y, hPadded_, textureEdgeMax);
      
      // setup the tiles:
      const std::size_t rows = y.size();
      const std::size_t cols = x.size();
      tiles_.resize(rows * cols);

      texId_.resize(rows * cols);
      glGenTextures(texId_.size(), &(texId_.front()));
      
      for (std::size_t j = 0; j < rows; j++)
      {
        for (std::size_t i = 0; i < cols; i++)
        {
          std::size_t tileIndex = j * cols + i;
          TFrameTile & tile = tiles_[tileIndex];

          tile.x_ = x[i][0];
          tile.y_ = y[j][0];
          tile.w_ = x[i][1] - 2;
          tile.h_ = y[j][1] - 2;
          
          tile.s0_ = GLdouble(1.0 / double(x[i][1]));
          tile.s1_ = GLdouble(1.0 - 1.0 / double(x[i][1]));
          tile.t0_ = GLdouble(1.0 / double(y[j][1]));
          tile.t1_ = GLdouble(1.0 - 1.0 / double(y[j][1]));
          
          if (i + 1 == cols)
          {
            tile.w_ -= wOdd_;
            tile.s1_ = GLdouble(1.0 - double(1 + wOdd_) / double(x[i][1]));
          }
          
          if (j + 1 == rows)
          {
            tile.h_ -= hOdd_;
            tile.t1_ = GLdouble(1.0 - double(1 + hOdd_) / double(y[j][1]));
          }

          GLuint texId = texId_[tileIndex];
          glBindTexture(GL_TEXTURE_2D, texId);
          
          if (!glIsTexture(texId))
          {
            assert(false);
            return false;
          }
          
          glTexImage2D(GL_TEXTURE_2D,
                       0, // mipmap level
                       internalFormatGL,
                       tile.w_ + 2,
                       tile.h_ + 2,
                       0, // border width
                       pixelFormatGL,
                       dataTypeGL,
                       NULL);
          
          GLenum err = glGetError();
          if (err != GL_NO_ERROR)
          {
            assert(false);
            return false;
          }
        }
      }
    }
    
    // creating a padded frame buffer:
    std::size_t bytesPerPixel = ptts->stride_[0] / 8;
    if (!mayReuseTextures)
    {
      textureData_.resize(wPadded_ * hPadded_ * bytesPerPixel);
    }
    
    const std::size_t dstStride = bytesPerPixel * wPadded_;
    const std::size_t srcStride = frame_->sampleBuffer_->rowBytes(0);
    const std::size_t rowBytes = bytesPerPixel * w_;
    
    const unsigned char * src =
      frame_->sampleBuffer_->samples(0) +
      frame_->traits_.offsetTop_ * srcStride +
      frame_->traits_.offsetLeft_ * bytesPerPixel;
    
    unsigned char * dst = &(textureData_.front());
    
    // pad on the top:
    memcpy(dst + bytesPerPixel, src, rowBytes);
    memcpy(dst, src, bytesPerPixel);
    memcpy(dst + rowBytes + bytesPerPixel,
           src + rowBytes - bytesPerPixel,
           bytesPerPixel);
    
    // copy the frame, pad on left and right:
    for (GLsizei i = 0; i < h_; i++)
    {
      const unsigned char * srcRow = src + srcStride * i;
      unsigned char * dstRow = dst + dstStride * (i + 1);
      memcpy(dstRow + bytesPerPixel, srcRow, rowBytes);
      
      memcpy(dstRow, srcRow, bytesPerPixel);
      memcpy(dstRow + rowBytes + bytesPerPixel,
             srcRow + rowBytes - bytesPerPixel,
             bytesPerPixel);
      
    }
    
    // pad on the bottom:
    memcpy(dst + (h_ + 1) * dstStride + bytesPerPixel,
           src + (h_ - 1) * srcStride,
           rowBytes);
    memcpy(dst + (h_ + 1) * dstStride,
           src + (h_ - 1) * srcStride,
           bytesPerPixel);
    memcpy(dst + (h_ + 1) * dstStride + rowBytes + bytesPerPixel,
           src + (h_ - 1) * srcStride + rowBytes - bytesPerPixel,
           bytesPerPixel);
    
    // upload the texture data:
    glPushClientAttrib(GL_UNPACK_ALIGNMENT);
    {
      glPixelStorei(GL_UNPACK_SWAP_BYTES, shouldSwapBytes);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, wPadded_);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      
      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        const TFrameTile & tile = tiles_[i];
        GLuint texId = texId_[i];
        glBindTexture(GL_TEXTURE_2D, texId);
        
        if (!glIsTexture(texId))
        {
          assert(false);
          continue;
        }
        
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, tile.x_);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, tile.y_);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0, // mipmap level
                        0, // x-offset
                        0, // y-offset
                        tile.w_ + 2,
                        tile.h_ + 2,
                        pixelFormatGL,
                        dataTypeGL,
                        &(textureData_.front()));
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
          assert(false);
          continue;
        }
      }
    }
    glPopClientAttrib();
    return true;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::draw
  // 
  void
  TLegacyCanvas::draw()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (texId_.empty() || !frame_)
    {
      return;
    }
    
    // video traits shortcut:
    const VideoTraits & vtts = frame_->traits_;
    
    glEnable(GL_TEXTURE_2D);
    for (std::size_t i = 0; i < tiles_.size(); ++i)
    {
      const TFrameTile & tile = tiles_[i];
      GLuint texId = texId_[i];
      glBindTexture(GL_TEXTURE_2D, texId);
      
      if (!glIsTexture(texId))
      {
        assert(false);
        continue;
      }
        
      glTexParameteri(GL_TEXTURE_2D,
                      GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,
                      GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      
      glTexParameteri(GL_TEXTURE_2D,
                      GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
      glTexParameteri(GL_TEXTURE_2D,
                      GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D,
                      GL_TEXTURE_MAX_LEVEL, 0);
      
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      
      glDisable(GL_LIGHTING);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glColor3f(1.f, 1.f, 1.f);
      
      glBegin(GL_QUADS);
      {
        glTexCoord2d(tile.s0_, tile.t0_);
        glVertex2i(tile.x_, tile.y_);
        
        glTexCoord2d(tile.s1_, tile.t0_);
        glVertex2i(tile.x_ + tile.w_, tile.y_);
        
        glTexCoord2d(tile.s1_, tile.t1_);
        glVertex2i(tile.x_ + tile.w_, tile.y_ + tile.h_);
        
        glTexCoord2d(tile.s0_, tile.t1_);
        glVertex2i(tile.x_, tile.y_ + tile.h_);
      }
      glEnd();
    }
  }
  
  //----------------------------------------------------------------
  // Canvas::Canvas
  // 
  Canvas::Canvas(const QGLFormat & format,
                 QWidget * parent,
                 const QGLWidget * shareWidget,
                 Qt::WindowFlags f):
    QGLWidget(format, parent, shareWidget, f),
    private_(NULL)
  {
    setObjectName("yae::Canvas");
    setAttribute(Qt::WA_NoSystemBackground);
    
    // setFocusPolicy(Qt::StrongFocus);
    // setMouseTracking(true);
  }

  //----------------------------------------------------------------
  // Canvas::~Canvas
  // 
  Canvas::~Canvas()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // Canvas::initializePrivateBackend
  // 
  void
  Canvas::initializePrivateBackend()
  {
    TMakeCurrentContext currentContext(this);
    
    delete private_;
    private_ = NULL;
    
    if (glewIsExtensionSupported("GL_EXT_texture_rectangle") ||
        glewIsExtensionSupported("GL_ARB_texture_rectangle"))
    {
      std::cerr << "TModernCanvas" << std::endl;
      private_ = new TModernCanvas();
    }
    else
    {
      std::cerr << "TLegacyCanvas" << std::endl;
      private_ = new TLegacyCanvas();
    }
  }
  
  //----------------------------------------------------------------
  // Canvas::gl_context_is_valid
  // 
  bool
  Canvas::gl_context_is_valid() const
  {
    return QGLWidget::isValid();
  }
  
  //----------------------------------------------------------------
  // Canvas::gl_make_current
  // 
  void
  Canvas::gl_make_current()
  {
    QGLWidget::makeCurrent();
  }
  
  //----------------------------------------------------------------
  // Canvas::gl_done_current
  // 
  void
  Canvas::gl_done_current()
  {
    QGLWidget::doneCurrent();
  }
  
  //----------------------------------------------------------------
  // Canvas::refresh
  // 
  void
  Canvas::refresh()
  {
    QGLWidget::updateGL();
    QGLWidget::doneCurrent();
  }
  
  //----------------------------------------------------------------
  // Canvas::render
  // 
  bool
  Canvas::render(const TVideoFramePtr & frame)
  {
    bool postThePayload = payload_.set(frame);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new RenderFrameEvent(payload_));
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // Canvas::event
  // 
  bool
  Canvas::event(QEvent * event)
  {
    if (event->type() == QEvent::User)
    {
      RenderFrameEvent * renderEvent = dynamic_cast<RenderFrameEvent *>(event);
      if (renderEvent)
      {
        event->accept();
        
        TVideoFramePtr frame;
        renderEvent->payload_.get(frame);
        loadFrame(frame);
        
        return true;
      }
    }
    
    return QGLWidget::event(event);
  }

  //----------------------------------------------------------------
  // Canvas::initializeGL
  // 
  void
  Canvas::initializeGL()
  {
    QGLWidget::initializeGL();
    
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    
    glShadeModel(GL_SMOOTH);
    glClearDepth(0);
    glClearStencil(0);
    glClearAccum(0, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
  }

  //----------------------------------------------------------------
  // Canvas::resizeGL
  // 
  void
  Canvas::resizeGL(int width, int height)
  {
    QGLWidget::resizeGL(width, height);
  }

  //----------------------------------------------------------------
  // Canvas::paintGL
  // 
  void
  Canvas::paintGL()
  {
    if (width() == 0 || height() == 0)
    {
      return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;
    
    if (ptts)
    {
      TGLSaveState pushAttr(GL_ALL_ATTRIB_BITS);
      
      int canvasWidth = width();
      int canvasHeight = height();
      
      // draw a checkerboard to help visualize the alpha channel:
      if (ptts->flags_ & (pixelFormat::kAlpha | pixelFormat::kPaletted))
      {
        glViewport(0, 0, canvasWidth, canvasHeight);
        glMatrixMode(GL_PROJECTION);
        gluOrtho2D(0, canvasWidth, canvasHeight, 0);
        
        float zebra[2][3] =
        {
          { 1.0f, 1.0f, 1.0f },
          { 0.7f, 0.7f, 0.7f }
        };
      
        int edgeSize = 24;
        bool evenRow = false;
        for (int y = 0; y < canvasHeight; y += edgeSize, evenRow = !evenRow)
        {
          int y1 = std::min(y + edgeSize, canvasHeight);
          
          bool evenCol = false;
          for (int x = 0; x < canvasWidth; x += edgeSize, evenCol = !evenCol)
          {
            int x1 = std::min(x + edgeSize, canvasWidth);

            float * color = (evenRow ^ evenCol) ? zebra[0] : zebra[1];
            glColor3fv(color);
            
            glRecti(x, y, x1, y1);
          }
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      else
      {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
      }
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      double imageWidth = private_->imageWidth();
      double imageHeight = private_->imageHeight();
      
      double dar = imageWidth / imageHeight;
      double car = double(canvasWidth) / double(canvasHeight);
      
      double x = 0.0;
      double y = 0.0;
      double w = double(canvasWidth);
      double h = double(canvasHeight);
      
      if (dar < car)
      {
        w = double(canvasHeight) * dar;
        x = 0.5 * (double(canvasWidth) - w);
      }
      else
      {
        h = double(canvasWidth) / dar;
        y = 0.5 * (double(canvasHeight) - h);
      }
#if 0
      std::cerr << "dar: " << dar
                << ", car: " << car
                << ", xy: " << x << " " << y
                << ", wh: " << w << " " << h
                << std::endl;
#endif
      
      glViewport(GLint(x + 0.5), GLint(y + 0.5),
                 GLsizei(w + 0.5), GLsizei(h + 0.5));
      glMatrixMode(GL_PROJECTION);
      gluOrtho2D(0.0, imageWidth, imageHeight, 0.0);
      
      glEnable(GL_TEXTURE_2D);
      private_->draw();
    }
    else
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }
  }
  
  //----------------------------------------------------------------
  // Canvas::loadFrame
  // 
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
    bool ok = private_->loadFrame(this, frame);
    refresh();
    return ok;
  }
}

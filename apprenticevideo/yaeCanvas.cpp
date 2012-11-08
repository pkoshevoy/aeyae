// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <math.h>
#include <deque>

// GLEW includes:
#include <GL/glew.h>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>
#include <yaePixelFormatTraits.h>
#include <yaeThreading.h>
#include <yaeUtils.h>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QTime>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#elif !defined(_WIN32)
#include <QtDBus/QtDBus>
#endif

// libass includes:
// #undef YAE_USE_LIBASS
#ifdef YAE_USE_LIBASS
extern "C"
{
#include <ass/ass.h>
}
#endif


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

//----------------------------------------------------------------
// yae_assert_gl_no_error
//
static bool
yae_assert_gl_no_error()
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    return true;
  }

  const GLubyte * str = gluErrorString(err);
  std::cerr << "GL_ERROR: " << str << std::endl;
  YAE_ASSERT(false);
  return false;
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
      applied_ = yae_assert_gl_no_error();
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
      applied_ = yae_assert_gl_no_error();
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
  // TGLSaveMatrixState
  //
  struct TGLSaveMatrixState
  {
    TGLSaveMatrixState(GLenum mode):
      matrixMode_(mode)
    {
      glMatrixMode(matrixMode_);
      glPushMatrix();
    }

    ~TGLSaveMatrixState()
    {
      glMatrixMode(matrixMode_);
      glPopMatrix();
    }

  protected:
    GLenum matrixMode_;
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
    TPrivate():
      dar_(0.0),
      darCropped_(0.0),
      verticalScalingEnabled_(false)
    {}

    virtual ~TPrivate() {}

    virtual void clear(QGLWidget * canvas) = 0;

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

    void enableVerticalScaling(bool enable)
    {
      verticalScalingEnabled_ = enable;
    }

    bool getCroppedFrame(TCropFrame & crop) const
    {
      if (!frame_)
      {
        return false;
      }

      if (crop_.isEmpty())
      {
        const VideoTraits & vtts = frame_->traits_;

        crop.x_ = vtts.offsetLeft_;
        crop.y_ = vtts.offsetTop_;
        crop.w_ = vtts.visibleWidth_;
        crop.h_ = vtts.visibleHeight_;

        if (darCropped_)
        {
          double par = (vtts.pixelAspectRatio_ != 0.0 &&
                        vtts.pixelAspectRatio_ != 1.0 ?
                        vtts.pixelAspectRatio_ : 1.0);

          double dar = double(par * crop.w_) / double(crop.h_);

          if (dar < darCropped_)
          {
            // adjust height:
            int h = int(0.5 + double(par * crop.w_) / darCropped_);
            crop.y_ += (crop.h_ - h) / 2;
            crop.h_ = h;
          }
          else
          {
            // adjust width:
            int w = int(0.5 + double(crop.h_ / par) * darCropped_);
            crop.x_ += (crop.w_ - w) / 2;
            crop.w_ = w;
          }
        }
      }
      else
      {
        crop = crop_;
      }

      return true;
    }

    bool imageWidthHeight(double & w, double & h) const
    {
      TCropFrame crop;
      if (getCroppedFrame(crop))
      {
        // video traits shortcut:
        const VideoTraits & vtts = frame_->traits_;

        w = crop.w_;
        h = crop.h_;

        if (!verticalScalingEnabled_)
        {
          if (dar_ != 0.0)
          {
            w = floor(0.5 + dar_ * h);
          }
          else if (vtts.pixelAspectRatio_ != 0.0)
          {
            w = floor(0.5 + w * vtts.pixelAspectRatio_);
          }
        }
        else
        {
          if (dar_ != 0.0)
          {
            double wh = w / h;

            if (dar_ > wh)
            {
              w = floor(0.5 + dar_ * h);
            }
            else if (dar_ < wh)
            {
              h = floor(0.5 + w / dar_);
            }
          }
          else if (vtts.pixelAspectRatio_ > 1.0)
          {
            w = floor(0.5 + w * vtts.pixelAspectRatio_);
          }
          else if (vtts.pixelAspectRatio_ < 1.0)
          {
            h = floor(0.5 + h / vtts.pixelAspectRatio_);
          }
        }

        return true;
      }

      return false;
    }

    inline void overrideDisplayAspectRatio(double dar)
    {
      dar_ = dar;
    }

    inline void cropFrame(double darCropped)
    {
      crop_.clear();
      darCropped_ = darCropped;
    }

    inline void cropFrame(const TCropFrame & crop)
    {
      darCropped_ = 0.0;
      crop_ = crop;
    }

    inline void getFrame(TVideoFramePtr & frame) const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      frame = frame_;
    }

  protected:

    inline bool setFrame(const TVideoFramePtr & frame)
    {
      // NOTE: this assumes that the mutex is already locked:
      bool frameSizeChanged = false;

      if (!frame_ || !frame || !frame_->traits_.sameFrameSize(frame->traits_))
      {
        crop_.clear();
        frameSizeChanged = true;
      }

      frame_ = frame;
      return frameSizeChanged;
    }

    mutable boost::mutex mutex_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    double dar_;
    double darCropped_;
    bool verticalScalingEnabled_;
  };

  //----------------------------------------------------------------
  // TModernCanvas
  //
  struct TModernCanvas : public Canvas::TPrivate
  {
    TModernCanvas();

    // virtual:
    void clear(QGLWidget * canvas);

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
  // TModernCanvas::clear
  //
  void
  TModernCanvas::clear(QGLWidget * canvas)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    glDeleteTextures(1, &texId_);
    texId_ = 0;
    dar_ = 0.0;
    darCropped_ = 0.0;
    crop_.clear();
    frame_ = TVideoFramePtr();
  }

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

    setFrame(frame);

    glGenTextures(1, &texId_);
    glEnable(GL_TEXTURE_RECTANGLE_EXT);
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texId_);

#ifdef __APPLE__
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_STORAGE_HINT_APPLE,
                    GL_STORAGE_CACHED_APPLE);
#endif

    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    yae_assert_gl_no_error();

    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    {
      glPixelStorei(GL_UNPACK_SWAP_BYTES, shouldSwapBytes);

      glPixelStorei(GL_UNPACK_ROW_LENGTH,
                    (GLint)(frame->data_->rowBytes(0) /
                            (ptts->stride_[0] / 8)));

      // order of bits in a byte only matters for bitmaps:
      // glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);

#ifdef __APPLE__
      if (glewIsExtensionSupported("GL_APPLE_client_storage"))
      {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
      }
#endif

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_RECTANGLE_EXT,
                   0, // always level-0 for GL_TEXTURE_RECTANGLE_EXT
                   internalFormatGL,
                   vtts.encodedWidth_,
                   vtts.encodedHeight_,
                   0, // border width
                   pixelFormatGL,
                   dataTypeGL,
                   frame->data_->data(0));
    }
    glDisable(GL_TEXTURE_RECTANGLE_EXT);

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

    glEnable(GL_TEXTURE_RECTANGLE_EXT);
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texId_);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.f, 1.f, 1.f);

    double w = 0.0;
    double h = 0.0;
    imageWidthHeight(w, h);

    TCropFrame crop;
    getCroppedFrame(crop);

    glBegin(GL_QUADS);
    {
      glTexCoord2i(crop.x_, crop.y_);
      glVertex2i(0, 0);

      glTexCoord2i(crop.x_ + crop.w_, crop.y_);
      glVertex2i(int(w), 0);

      glTexCoord2i(crop.x_ + crop.w_, crop.y_ + crop.h_);
      glVertex2i(int(w), int(h));

      glTexCoord2i(crop.x_, crop.y_ + crop.h_);
      glVertex2i(0, int(h));
    }
    glEnd();
    glDisable(GL_TEXTURE_RECTANGLE_EXT);
  }

  //----------------------------------------------------------------
  // TEdge
  //
  struct TEdge
  {
    // texture:
    GLsizei offset_;
    GLsizei extent_;
    GLsizei length_;

    // padding:
    GLsizei v0_;
    GLsizei v1_;

    // texture coordinates:
    GLdouble t0_;
    GLdouble t1_;
  };

  //----------------------------------------------------------------
  // TFrameTile
  //
  struct TFrameTile
  {
    TEdge x_;
    TEdge y_;
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
    void clear(QGLWidget * canvas);

    // virtual:
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);

    // virtual:
    void draw();

  protected:
    // unpadded image dimensions:
    GLsizei w_;
    GLsizei h_;

    // padded image texture data:
    std::vector<unsigned char> textureData_;

    std::vector<TFrameTile> tiles_;
    std::vector<GLuint> texId_;
  };

  //----------------------------------------------------------------
  // TLegacyCanvas::clear
  //
  void
  TLegacyCanvas::clear(QGLWidget * canvas)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    frame_ = TVideoFramePtr();
    w_ = 0;
    h_ = 0;
    dar_ = 0.0;
    darCropped_ = 0.0;
    crop_.clear();

    if (!texId_.empty())
    {
      glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
    }

    texId_.clear();
    textureData_.clear();
    tiles_.clear();
  }

  //----------------------------------------------------------------
  // calculateEdges
  //
  static void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgeSize,
                 GLsizei textureEdgeMax)
  {
    if (!edgeSize)
    {
      return;
    }

    GLsizei offset = 0;
    GLsizei extent = edgeSize;
    GLsizei segmentStart = 0;

    while (true)
    {
      edges.push_back(TEdge());
      TEdge & edge = edges.back();

      edge.offset_ = offset;
      edge.extent_ = std::min<GLsizei>(textureEdgeMax, powerOfTwoLEQ(extent));

      if (edge.extent_ < extent &&
          edge.extent_ < textureEdgeMax)
      {
        edge.extent_ *= 2;
      }

      // padding:
      GLsizei p0 = (edge.offset_ > 0) ? 1 : 0;
      GLsizei p1 = (edge.extent_ < extent) ? 1 : 0;

      edge.length_ = std::min<GLsizei>(edge.extent_, extent);
      edge.v0_ = segmentStart;
      edge.v1_ = edge.v0_ + edge.length_ - (p0 + p1);
      segmentStart = edge.v1_;

      edge.t0_ = double(p0) / double(edge.extent_);
      edge.t1_ = double(edge.length_ - p1) / double(edge.extent_);

      if (edge.extent_ < extent)
      {
        offset += edge.extent_ - 2;
        extent -= edge.extent_ - 2;
        continue;
      }

      break;
    }
  }

  //----------------------------------------------------------------
  // calcTextureEdgeMax
  //
  static GLsizei
  calcTextureEdgeMax()
  {
    GLsizei edgeMax = 64;

    for (unsigned int i = 0; i < 8; i++, edgeMax *= 2)
    {
      glTexImage2D(GL_PROXY_TEXTURE_2D,
                   0, // level
                   GL_RGBA,
                   edgeMax * 2, // width
                   edgeMax * 2, // height
                   0,
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   NULL);// texels
      GLenum err = glGetError();
      if (err != GL_NO_ERROR)
      {
        break;
      }

      GLint width = 0;
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
                               0, // level
                               GL_TEXTURE_WIDTH,
                               &width);
      if (width != GLint(edgeMax * 2))
      {
        break;
      }
    }

    return edgeMax;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::loadFrame
  //
  bool
  TLegacyCanvas::loadFrame(QGLWidget * canvas,
                           const TVideoFramePtr & frame)
  {
    static const GLsizei textureEdgeMax = calcTextureEdgeMax();

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

    // take the new frame:
    bool frameSizeChanged = setFrame(frame);

    TCropFrame crop;
    getCroppedFrame(crop);

    if (frameSizeChanged || w_ != crop.w_ || h_ != crop.h_)
    {
      if (!texId_.empty())
      {
        glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
        texId_.clear();
        textureData_.clear();
      }

      w_ = crop.w_;
      h_ = crop.h_;

      // calculate x-min, x-max coordinates for each tile:
      std::deque<TEdge> x;
      calculateEdges(x, w_, textureEdgeMax);

      // calculate y-min, y-max coordinates for each tile:
      std::deque<TEdge> y;
      calculateEdges(y, h_, textureEdgeMax);

      // setup the tiles:
      const std::size_t rows = y.size();
      const std::size_t cols = x.size();
      tiles_.resize(rows * cols);

      texId_.resize(rows * cols);
      glGenTextures((GLsizei)(texId_.size()), &(texId_.front()));

      for (std::size_t j = 0; j < rows; j++)
      {
        for (std::size_t i = 0; i < cols; i++)
        {
          std::size_t tileIndex = j * cols + i;

          TFrameTile & tile = tiles_[tileIndex];
          tile.x_ = x[i];
          tile.y_ = y[j];

          GLuint texId = texId_[tileIndex];
          glBindTexture(GL_TEXTURE_2D, texId);

          if (!glIsTexture(texId))
          {
            YAE_ASSERT(false);
            return false;
          }

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_BASE_LEVEL, 0);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MAX_LEVEL, 0);

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          yae_assert_gl_no_error();

          glTexImage2D(GL_TEXTURE_2D,
                       0, // mipmap level
                       internalFormatGL,
                       tile.x_.extent_,
                       tile.y_.extent_,
                       0, // border width
                       pixelFormatGL,
                       dataTypeGL,
                       NULL);

          if (!yae_assert_gl_no_error())
          {
            return false;
          }
        }
      }
    }

    // get the source data pointer:
    const std::size_t bytesPerRow = frame_->data_->rowBytes(0);
    const std::size_t bytesPerPixel = ptts->stride_[0] / 8;
    const unsigned char * src =
      frame_->data_->data(0) +
      crop.y_ * bytesPerRow +
      crop.x_ * bytesPerPixel;

    // upload the texture data:
    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    {
      glPixelStorei(GL_UNPACK_SWAP_BYTES, shouldSwapBytes);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, vtts.encodedWidth_);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      yae_assert_gl_no_error();

      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        const TFrameTile & tile = tiles_[i];
        GLuint texId = texId_[i];
        glBindTexture(GL_TEXTURE_2D, texId);

        if (!glIsTexture(texId))
        {
          YAE_ASSERT(false);
          continue;
        }

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, tile.x_.offset_);
        yae_assert_gl_no_error();

        glPixelStorei(GL_UNPACK_SKIP_ROWS, tile.y_.offset_);
        yae_assert_gl_no_error();

        glTexSubImage2D(GL_TEXTURE_2D,
                        0, // mipmap level
                        0, // x-offset
                        0, // y-offset
                        tile.x_.length_,
                        tile.y_.length_,
                        pixelFormatGL,
                        dataTypeGL,
                        src);
        yae_assert_gl_no_error();

        if (tile.x_.length_ < tile.x_.extent_)
        {
          // extend on the right to avoid texture filtering artifacts:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, (tile.x_.offset_ +
                                                tile.x_.length_ - 1));
          yae_assert_gl_no_error();

          glPixelStorei(GL_UNPACK_SKIP_ROWS, tile.y_.offset_);
          yae_assert_gl_no_error();

          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          tile.x_.length_,
                          0,

                          // width, height
                          1,
                          tile.y_.length_,

                          pixelFormatGL,
                          dataTypeGL,
                          src);
          yae_assert_gl_no_error();
        }

        if (tile.y_.length_ < tile.y_.extent_)
        {
          // extend on the bottom to avoid texture filtering artifacts:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, tile.x_.offset_);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, (tile.y_.offset_ +
                                              tile.y_.length_ - 1));
          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          0,
                          tile.y_.length_,

                          // width, height
                          tile.x_.length_,
                          1,

                          pixelFormatGL,
                          dataTypeGL,
                          src);
          yae_assert_gl_no_error();
        }

        if (tile.x_.length_ < tile.x_.extent_ &&
            tile.y_.length_ < tile.y_.extent_)
        {
          // extend the bottom-right corner:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, (tile.x_.offset_ +
                                                tile.x_.length_ - 1));
          glPixelStorei(GL_UNPACK_SKIP_ROWS, (tile.y_.offset_ +
                                              tile.y_.length_ - 1));
          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          tile.x_.length_,
                          tile.y_.length_,

                          // width, height
                          1,
                          1,

                          pixelFormatGL,
                          dataTypeGL,
                          src);
          yae_assert_gl_no_error();
        }
      }
    }

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

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);

    double iw = 0.0;
    double ih = 0.0;
    imageWidthHeight(iw, ih);

    TCropFrame crop;
    getCroppedFrame(crop);

    double sx = iw / double(crop.w_);
    double sy = ih / double(crop.h_);
    glScaled(sx, sy, 1.0);

    glEnable(GL_TEXTURE_2D);
    for (std::size_t i = 0; i < tiles_.size(); ++i)
    {
      const TFrameTile & tile = tiles_[i];
      GLuint texId = texId_[i];
      glBindTexture(GL_TEXTURE_2D, texId);

      if (!glIsTexture(texId))
      {
        YAE_ASSERT(false);
        continue;
      }

      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      glDisable(GL_LIGHTING);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glColor3f(1.f, 1.f, 1.f);

      glBegin(GL_QUADS);
      {
        glTexCoord2d(tile.x_.t0_, tile.y_.t0_);
        glVertex2i(tile.x_.v0_, tile.y_.v0_);

        glTexCoord2d(tile.x_.t1_, tile.y_.t0_);
        glVertex2i(tile.x_.v1_, tile.y_.v0_);

        glTexCoord2d(tile.x_.t1_, tile.y_.t1_);
        glVertex2i(tile.x_.v1_, tile.y_.v1_);

        glTexCoord2d(tile.x_.t0_, tile.y_.t1_);
        glVertex2i(tile.x_.v0_, tile.y_.v1_);
      }
      glEnd();
    }
    glDisable(GL_TEXTURE_2D);
  }

#ifdef YAE_USE_LIBASS
  //----------------------------------------------------------------
  // getFontsConf
  //
  static bool
  getFontsConf(std::string & fontsConf, bool & removeAfterUse)
  {
#if !defined(_WIN32)
    fontsConf = "/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the system fontconfig file:
      removeAfterUse = false;
      return true;
    }
#endif

#if defined(__APPLE__)
    fontsConf = "/opt/local/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the macports fontconfig file:
      removeAfterUse = false;
      return true;
    }
#endif

    removeAfterUse = true;
    int64 appPid = QCoreApplication::applicationPid();

    QString tempDir =
      QDesktopServices::storageLocation(QDesktopServices::TempLocation);

    QString fontsDir =
      QDesktopServices::storageLocation(QDesktopServices::FontsLocation);

    QString cacheDir =
      QDesktopServices::storageLocation(QDesktopServices::CacheLocation);

    QString fontconfigCache =
      cacheDir + QString::fromUtf8("/apprenticevideo-fontconfig-cache");

    std::ostringstream os;
    os << "<?xml version=\"1.0\"?>" << std::endl
       << "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">" << std::endl
       << "<fontconfig>" << std::endl
       << "\t<dir>"
       << QDir::toNativeSeparators(fontsDir).toUtf8().constData()
       << "</dir>" << std::endl;

#ifdef __APPLE__
    os << "\t<dir>/Library/Fonts</dir>" << std::endl
       << "\t<dir>~/Library/Fonts</dir>" << std::endl;
#endif

#ifndef _WIN32
    const char * fontdir[] = {
      "/usr/share/fonts",
      "/usr/X11R6/lib/X11/fonts",
      "/opt/kde3/share/fonts",
      "/usr/local/share/fonts"
    };

    std::size_t nfontdir = sizeof(fontdir) / sizeof(fontdir[0]);
    for (std::size_t i = 0; i < nfontdir; i++)
    {
      QString path = QString::fromUtf8(fontdir[i]);
      if (QFileInfo(path).exists())
      {
        os << "\t<dir>" << fontdir[i] << "</dir>" << std::endl;
      }
    }
#endif

    os << "\t<cachedir>"
       << QDir::toNativeSeparators(fontconfigCache).toUtf8().constData()
       << "</cachedir>" << std::endl
       << "</fontconfig>" << std::endl;

    QString fn =
      tempDir +
      QString::fromUtf8("/apprenticevideo.fonts.conf.") +
      QString::number(appPid);

    fontsConf = QDir::toNativeSeparators(fn).toUtf8().constData();
    std::cerr << "fonts.conf: " << fontsConf << std::endl;

    std::FILE * fout = fopenUtf8(fontsConf.c_str(), "w");
    if (!fout)
    {
      return false;
    }

    std::string xml = os.str().c_str();
    std::cerr << "fonts.conf content:\n" << xml << std::endl;

    std::size_t nout = fwrite(xml.c_str(), 1, xml.size(), fout);
    fclose(fout);

    return nout == xml.size();
  }

  //----------------------------------------------------------------
  // TLibassInitDoneCallback
  //
  typedef void(*TLibassInitDoneCallback)(void *);

  //----------------------------------------------------------------
  // TLibassInit
  //
  struct TLibassInit
  {
    TLibassInit():
      callbackContext_(NULL),
      callback_(NULL),
      finished_(false),
      success_(0)
    {}

    void setCallback(void * context, TLibassInitDoneCallback callback)
    {
      YAE_ASSERT(!callback_ || !callback);
      callbackContext_ = context;
      callback_ = callback;
    }

    static int init(ASS_Library *& library,
                    ASS_Renderer *& renderer,
                    ASS_Track *& track)
    {
      library = ass_library_init();
      renderer = ass_renderer_init(library);
      track = ass_new_track(library);

      // lookup Fontconfig configuration file path:
      std::string fontsConf;
      bool removeAfterUse = false;
      getFontsConf(fontsConf, removeAfterUse);

      const char * defaultFont = NULL;
      const char * defaultFamily = NULL;
      int useFontconfig = 1;
      int updateFontCache = 0;

      ass_set_fonts(renderer,
                    defaultFont,
                    defaultFamily,
                    useFontconfig,
                    fontsConf.size() ? fontsConf.c_str() : NULL,
                    updateFontCache);

      int err = ass_fonts_update(renderer);

      if (removeAfterUse)
      {
        // remove the temporary fontconfig file:
        QFile::remove(QString::fromUtf8(fontsConf.c_str()));
      }

      return err;
    }

    static void done(ASS_Library *& library,
                     ASS_Renderer *& renderer,
                     ASS_Track *& track)
    {
      ass_free_track(track);
      track = NULL;

      ass_renderer_done(renderer);
      renderer = NULL;

      ass_library_done(library);
      library = NULL;
    }

    void threadLoop()
    {
      // begin:
      finished_ = false;

      // this can take a while to rebuild the font cache:
      ASS_Library * library = NULL;
      ASS_Renderer * renderer = NULL;
      ASS_Track * track = NULL;
      success_ = init(library, renderer, track);

      // cleanup:
      done(library, renderer, track);

      // done:
      finished_ = true;

      if (callback_)
      {
        callback_(callbackContext_);
      }
    }

    void * callbackContext_;
    TLibassInitDoneCallback callback_;
    bool finished_;
    int success_;
  };

  //----------------------------------------------------------------
  // libassInit
  //
  static TLibassInit libassInit;

  //----------------------------------------------------------------
  // libassInitThread
  //
  static Thread<TLibassInit> libassInitThread;
#endif

  //----------------------------------------------------------------
  // initLibass
  //
  bool
  initLibass(Canvas * canvas)
  {
#ifdef YAE_USE_LIBASS
    if (libassInit.finished_)
    {
      return libassInit.success_ != 0;
    }

    if (!libassInitThread.isRunning())
    {
      libassInit.setCallback(canvas, &Canvas::libassInitDoneCallback);
      libassInitThread.setContext(&libassInit);
      libassInitThread.run();
    }
#endif

    return false;
  }

  //----------------------------------------------------------------
  // TLibass
  //
  class TLibass
  {
  public:

#ifdef YAE_USE_LIBASS
    struct TLine
    {
      TLine(int64 pts = 0,
            const unsigned char * data = NULL,
            std::size_t size = 0):
        pts_(pts),
        data_((const char *)data, (const char *)data + size)
      {}

      bool operator == (const TLine & sub) const
      {
        return pts_ == sub.pts_ && data_ == sub.data_;
      }

      // presentation timestamp expressed in milliseconds:
      int64 pts_;

      // subtitle dialog line:
      std::string data_;
    };

    TLibass(const unsigned char * codecPrivate,
            std::size_t codecPrivateSize):
      library_(NULL),
      renderer_(NULL),
      track_(NULL),
      success_(0),
      bufferSize_(0)
    {
      success_ = TLibassInit::init(library_, renderer_, track_);

      if (codecPrivate && codecPrivateSize && track_)
      {
        ass_process_codec_private(track_,
                                  (char *)codecPrivate,
                                  (int)codecPrivateSize);
      }
    }

    ~TLibass()
    {
      TLibassInit::done(library_, renderer_, track_);
    }

    inline bool ready() const
    {
      return success_ != 0;
    }

    void setFrameSize(int w, int h)
    {
      ass_set_frame_size(renderer_, w, h);

      double ar = double(w) / double(h);
      ass_set_aspect_ratio(renderer_, ar, ar);
    }

    void processData(const unsigned char * data, std::size_t size, int64 pts)
    {
      TLine line(pts, data, size);
      if (has(buffer_, line))
      {
        return;
      }

      // std::cerr << "ass_process_data: " << line.data_ << std::endl;

      if (bufferSize_)
      {
        const TLine & first = buffer_.front();
        if (pts < first.pts_)
        {
          // user skipped back in time, purge cached subs:
          ass_flush_events(track_);
          buffer_.clear();
          bufferSize_ = 0;
        }
      }

      if (bufferSize_ < 10)
      {
        bufferSize_++;
      }
      else
      {
        buffer_.pop_front();
      }

      buffer_.push_back(line);
      ass_process_data(track_, (char *)data, (int)size);
    }

    ASS_Image * renderFrame(int64 now, int * detectChange)
    {
      return ass_render_frame(renderer_,
                              track_,
                              (long long)now,
                              detectChange);
    }

    ASS_Library * library_;
    ASS_Renderer * renderer_;
    ASS_Track * track_;
    int success_;
    std::list<TLine> buffer_;
    std::size_t bufferSize_;
#endif
  };

  //----------------------------------------------------------------
  // Canvas::Canvas
  //
  Canvas::Canvas(const QGLFormat & format,
                 QWidget * parent,
                 const QGLWidget * shareWidget,
                 Qt::WindowFlags f):
    QGLWidget(format, parent, shareWidget, f),
    private_(NULL),
    overlay_(NULL),
    libass_(NULL),
    showTheGreeting_(true),
    subsInOverlay_(false),
    timerHideCursor_(this),
    timerScreenSaver_(this)
  {
    setObjectName("yae::Canvas");
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoBufferSwap(true);
    setAutoFillBackground(false);
    setMouseTracking(true);

    timerHideCursor_.setSingleShot(true);
    timerHideCursor_.setInterval(3000);

    timerScreenSaver_.setSingleShot(true);
    timerScreenSaver_.setInterval(29000);

    bool ok = true;
    ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                 this, SLOT(hideCursor()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaver_, SIGNAL(timeout()),
                 this, SLOT(wakeScreenSaver()));
    YAE_ASSERT(ok);

    greeting_ = tr("drop videos/music here\n\n"
                   "press spacebar to pause/resume\n\n"
                   "alt-left/alt-right to navigate playlist\n\n"
#ifdef __APPLE__
                   "use apple remote for volume and seeking\n\n"
#endif
                   "explore the menus for more options");
  }

  //----------------------------------------------------------------
  // Canvas::~Canvas
  //
  Canvas::~Canvas()
  {
    delete private_;
    delete overlay_;
    delete libass_;
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

    delete overlay_;
    overlay_ = NULL;

    if ((glewIsExtensionSupported("GL_EXT_texture_rectangle") ||
         glewIsExtensionSupported("GL_ARB_texture_rectangle")))
    {
      private_ = new TModernCanvas();
      overlay_ = new TModernCanvas();
    }
    else
    {
      private_ = new TLegacyCanvas();
      overlay_ = new TLegacyCanvas();
    }

    initLibass(this);
  }

  //----------------------------------------------------------------
  // Canvas::clear
  //
  void
  Canvas::clear()
  {
    private_->clear(this);
    clearOverlay();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::clearOverlay
  //
  void
  Canvas::clearOverlay()
  {
    overlay_->clear(this);

    delete libass_;
    libass_ = NULL;

    showTheGreeting_ = false;
    subsInOverlay_ = false;
    subs_.clear();
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

    if (autoCropThread_.isRunning())
    {
      autoCrop_.setFrame(frame);
    }

    return true;
  }

  //----------------------------------------------------------------
  // UpdateOverlayEvent
  //
  struct UpdateOverlayEvent : public QEvent
  {
    UpdateOverlayEvent(): QEvent(QEvent::User) {}
  };

  //----------------------------------------------------------------
  // LibassInitDoneEvent
  //
  struct LibassInitDoneEvent : public QEvent
  {
    LibassInitDoneEvent(): QEvent(QEvent::User) {}
  };

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

      UpdateOverlayEvent * overlayEvent =
        dynamic_cast<UpdateOverlayEvent *>(event);
      if (overlayEvent)
      {
        event->accept();

        updateOverlay(true);
        refresh();
        return true;
      }

      LibassInitDoneEvent * libassInitDoneEvent =
        dynamic_cast<LibassInitDoneEvent *>(event);
      if (libassInitDoneEvent)
      {
        event->accept();

#ifdef YAE_USE_LIBASS
        std::cerr << "LIBASS INIT DONE: "
                  << libassInit.finished_
                  << ", success: "
                  << libassInit.success_
                  << std::endl;

        libassInitThread.stop();
        libassInitThread.wait();
        libassInitThread.setContext(NULL);
        libassInit.setCallback(NULL, NULL);
#endif
        return true;
      }
    }

    return QGLWidget::event(event);
  }

  //----------------------------------------------------------------
  // Canvas::mouseMoveEvent
  //
  void
  Canvas::mouseMoveEvent(QMouseEvent * event)
  {
    setCursor(QCursor(Qt::ArrowCursor));
    timerHideCursor_.start();
  }

  //----------------------------------------------------------------
  // Canvas::mouseDoubleClickEvent
  //
  void
  Canvas::mouseDoubleClickEvent(QMouseEvent * event)
  {
    emit toggleFullScreen();
  }

  //----------------------------------------------------------------
  // Canvas::resizeEvent
  //
  void
  Canvas::resizeEvent(QResizeEvent * event)
  {
    QGLWidget::resizeEvent(event);

    if (overlay_ && (subsInOverlay_ || showTheGreeting_))
    {
      updateOverlay(true);
    }
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

    // glShadeModel(GL_SMOOTH);
    glShadeModel(GL_FLAT);
    glClearDepth(0);
    glClearStencil(0);
    glClearAccum(0, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
  }

  //----------------------------------------------------------------
  // calcImageWidth
  //
  static double
  calcImageWidth(const Canvas::TPrivate * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    canvas->imageWidthHeight(w, h);
    return w;
  }

  //----------------------------------------------------------------
  // calcImageHeight
  //
  static double
  calcImageHeight(const Canvas::TPrivate * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    canvas->imageWidthHeight(w, h);
    return h;
  }

  //----------------------------------------------------------------
  // paintImage
  //
  static void
  paintImage(Canvas::TPrivate * canvas,
             int canvasWidth,
             int canvasHeight)
  {
    double croppedWidth = calcImageWidth(canvas);
    double croppedHeight = calcImageHeight(canvas);

    double dar = croppedWidth / croppedHeight;
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

    glViewport(GLint(x + 0.5), GLint(y + 0.5),
               GLsizei(w + 0.5), GLsizei(h + 0.5));

    double uncroppedWidth = 0.0;
    double uncroppedHeight = 0.0;
    canvas->imageWidthHeight(uncroppedWidth, uncroppedHeight);

    double left = (uncroppedWidth - croppedWidth) * 0.5;
    double right = left + croppedWidth;
    double top = (uncroppedHeight - croppedHeight) * 0.5;
    double bottom = top + croppedHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(left, right, bottom, top);

    canvas->draw();
    yae_assert_gl_no_error();
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

    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    if (!ptts)
    {
      // unsupported pixel format:
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      if (!showTheGreeting_)
      {
        return;
      }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int canvasWidth = width();
    int canvasHeight = height();

    // draw a checkerboard to help visualize the alpha channel:
    if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                 pixelFormat::kPaletted)))
    {
      glViewport(0, 0, canvasWidth, canvasHeight);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluOrtho2D(0, canvasWidth, canvasHeight, 0);

      float zebra[2][3] = {
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

    if (ptts)
    {
      // draw the frame:
      paintImage(private_, canvasWidth, canvasHeight);
    }

    // draw the overlay:
    if (subsInOverlay_ || showTheGreeting_)
    {
      if (overlay_ && overlay_->pixelTraits())
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        paintImage(overlay_, canvasWidth, canvasHeight);
        glDisable(GL_BLEND);
      }
      else
      {
        qApp->postEvent(this, new UpdateOverlayEvent());
      }
    }
  }

  //----------------------------------------------------------------
  // Canvas::loadFrame
  //
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
    bool ok = private_->loadFrame(this, frame);
    showTheGreeting_ = false;
    setSubs(frame->subs_);

    refresh();

    if (ok && !timerScreenSaver_.isActive())
    {
      timerScreenSaver_.start();
    }

    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::currentFrame
  //
  TVideoFramePtr
  Canvas::currentFrame() const
  {
    TVideoFramePtr frame;

    if (private_)
    {
      private_->getFrame(frame);
    }

    return frame;
  }

  //----------------------------------------------------------------
  // Canvas::setSubs
  //
  void
  Canvas::setSubs(const std::list<TSubsFrame> & subs)
  {
    std::list<TSubsFrame> renderSubs;

    for (std::list<TSubsFrame>::const_iterator i = subs.begin();
         i != subs.end(); ++i)
    {
      const TSubsFrame & subs = *i;
      if (subs.render_)
      {
        renderSubs.push_back(subs);
      }
    }

    bool reparse = (renderSubs != subs_);
    if (reparse)
    {
      subs_ = renderSubs;
    }

    updateOverlay(reparse);
  }

  //----------------------------------------------------------------
  // TQImageBuffer
  //
  struct TQImageBuffer : public IPlanarBuffer
  {
    QImage qimg_;

    TQImageBuffer(int w, int h, QImage::Format fmt):
      qimg_(w, h, fmt)
    {
      unsigned char * dst = qimg_.bits();
      int rowBytes = qimg_.bytesPerLine();
      memset(dst, 0, rowBytes * h);
    }

    // virtual:
    void destroy()
    { delete this; }

    // virtual:
    std::size_t planes() const
    { return 1; }

    // virtual:
    unsigned char * data(std::size_t plane) const
    {
      const uchar * bits = qimg_.bits();
      return const_cast<unsigned char *>(bits);
    }

    // virtual:
    std::size_t rowBytes(std::size_t planeIndex) const
    {
      int n = qimg_.bytesPerLine();
      return (std::size_t)n;
    }
  };

  //----------------------------------------------------------------
  // drawPlainText
  //
  static bool
  drawPlainText(const std::string & text,
                QPainter & painter,
                QRect & bbox,
                int textAlignment)
  {
    QString qstr = QString::fromUtf8(text.c_str()).trimmed();
    if (!qstr.isEmpty())
    {
      QRect used;
      drawTextWithShadowToFit(painter,
                              bbox,
                              textAlignment,
                              qstr,
                              QPen(Qt::black),
                              true, // outline shadow
                              1, // shadow offset
                              &used);
      if (!used.isNull())
      {
        // avoid overwriting subs on top of each other:
        bbox.setBottom(used.top());
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // TPainterWrapper
  //
  struct TPainterWrapper
  {
    TPainterWrapper(int w, int h):
      painter_(NULL),
      w_(w),
      h_(h)
    {}

    ~TPainterWrapper()
    {
      delete painter_;
    }

    inline TVideoFramePtr & getFrame()
    {
      if (!frame_)
      {
        frame_.reset(new TVideoFrame());

        TQImageBuffer * imageBuffer =
          new TQImageBuffer(w_, h_, QImage::Format_ARGB32);
        // imageBuffer->qimg_.fill(0);

        frame_->data_.reset(imageBuffer);
      }

      return frame_;
    }

    inline QImage & getImage()
    {
      TVideoFramePtr & vf = getFrame();
      TQImageBuffer * imageBuffer = (TQImageBuffer *)(vf->data_.get());
      return imageBuffer->qimg_;
    }

    inline QPainter & getPainter()
    {
      if (!painter_)
      {
        QImage & image = getImage();
        painter_ = new QPainter(&image);

        painter_->setPen(Qt::white);
        painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);

        QFont ft;
        int px = std::max<int>(20, 56.0 * (h_ / 1024.0));
        ft.setPixelSize(px);
        painter_->setFont(ft);
      }

      return *painter_;
    }

    inline void painterEnd()
    {
      if (painter_)
      {
        painter_->end();
      }
    }

  private:
    TVideoFramePtr frame_;
    QPainter * painter_;
    int w_;
    int h_;
  };

  //----------------------------------------------------------------
  // Canvas::loadSubs
  //
  bool
  Canvas::updateOverlay(bool reparse)
  {
    if (showTheGreeting_)
    {
      return updateGreeting();
    }

    double imageWidth = 0.0;
    double imageHeight = 0.0;
    private_->imageWidthHeight(imageWidth, imageHeight);
    double dar = imageWidth / imageHeight;

    double w = this->width();
    double h = this->height();

    if (h > 1024.0)
    {
      w *= 1024.0 / h;
      h = 1024.0;
    }

    if (w > 1024.0)
    {
      h *= 1024.0 / w;
      w = 1024.0;
    }

    double car = w / h;
    double fw = w;
    double fh = h;
    double fx = 0;
    double fy = 0;

    if (dar < car)
    {
      fw = h * dar;
      fx = 0.5 * (w - fw);
    }
    else
    {
      fh = w / dar;
      fy = 0.5 * (h - fh);
    }

    int ix = int(fx);
    int iy = int(fy);
    int iw = int(fw);
    int ih = int(fh);

    TPainterWrapper wrapper((int)w, (int)h);

    int textAlignment = Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignBottom;
    bool paintedSomeSubs = false;
    bool libassSameSubs = false;

    QRect canvasBBox(16, 16, (int)w - 32, (int)h - 32);
    TVideoFramePtr frame = currentFrame();

    for (std::list<TSubsFrame>::const_iterator i = subs_.begin();
         i != subs_.end() && reparse; ++i)
    {
      const TSubsFrame & subs = *i;
      const TSubsFrame::IPrivate * subExt = subs.private_.get();
      const unsigned int nrects = subExt ? subExt->numRects() : 0;
      unsigned int nrectsPainted = 0;

      for (unsigned int j = 0; j < nrects; j++)
      {
        TSubsFrame::TRect r;
        subExt->getRect(j, r);

        if (r.type_ == kSubtitleBitmap)
        {
          const unsigned char * pal = r.data_[1];

          QImage img(r.w_, r.h_, QImage::Format_ARGB32);
          unsigned char * dst = img.bits();
          int dstRowBytes = img.bytesPerLine();

          for (int y = 0; y < r.h_; y++)
          {
            const unsigned char * srcLine = r.data_[0] + y * r.rowBytes_[0];
            unsigned char * dstLine = dst + y * dstRowBytes;

            for (int x = 0; x < r.w_; x++, dstLine += 4, srcLine++)
            {
              int colorIndex = *srcLine;
              memcpy(dstLine, pal + colorIndex * 4, 4);
            }
          }

          double rw = double(subs.rw_ ? subs.rw_ : imageWidth);
          double rh = double(subs.rh_ ? subs.rh_ : imageHeight);

          double sx = fw / rw;
          double sy = fh / rh;

          QPoint dstPos((int)(fx + sx * double(r.x_)),
                        (int)(fy + sy * double(r.y_)));
          QSize dstSize((int)(sx * double(r.w_)),
                        (int)(sy * double(r.h_)));

          wrapper.getPainter().drawImage(QRect(dstPos, dstSize),
                                         img, img.rect());
          paintedSomeSubs = true;
          nrectsPainted++;
        }
        else if (r.type_ == kSubtitleASS)
        {
          std::string assa(r.assa_);
          bool done = false;

#ifdef YAE_USE_LIBASS
          if (!libass_ && initLibass(this))
          {
            if (subs.traits_ == kSubsSSA && subs.extraData_)
            {
              libass_ = new TLibass(subs.extraData_->data(0),
                                    subs.extraData_->rowBytes(0));
            }
            else if (subExt->headerSize())
            {
              libass_ = new TLibass(subExt->header(), subExt->headerSize());
            }
          }

          if (libass_ && libass_->ready())
          {
            int64 pts = (int64)(subs.time_.toSeconds() * 1000.0 + 0.5);
            libass_->processData((unsigned char *)&assa[0], assa.size(), pts);
            nrectsPainted++;
            done = true;
          }
#endif
          if (!done)
          {
            std::string text = assaToPlainText(assa);
            text = convertEscapeCodes(text);

            if (drawPlainText(text,
                              wrapper.getPainter(),
                              canvasBBox,
                              textAlignment))
            {
              paintedSomeSubs = true;
              nrectsPainted++;
            }
          }
        }
      }

      if (!nrectsPainted && subs.data_ &&
          (subs.traits_ == kSubsSSA ||
           subs.traits_ == kSubsText ||
           subs.traits_ == kSubsSUBRIP))
      {
        const unsigned char * str = subs.data_->data(0);
        const unsigned char * end = str + subs.data_->rowBytes(0);

        std::string text(str, end);
        if (subs.traits_ == kSubsSSA)
        {
          text = assaToPlainText(text);
        }
        else
        {
          text = stripHtmlTags(text);
        }

        text = convertEscapeCodes(text);
        if (drawPlainText(text,
                          wrapper.getPainter(),
                          canvasBBox,
                          textAlignment))
        {
          paintedSomeSubs = true;
        }
      }
    }

#ifdef YAE_USE_LIBASS
    if (libass_ && frame)
    {
      libass_->setFrameSize(iw, ih);

      // the list of images is owned by libass,
      // libass is responsible for their deallocation:
      int64 now = (int64)(frame->time_.toSeconds() * 1000.0 + 0.5);

      int changeDetected = 0;
      ASS_Image * pic = libass_->renderFrame(now, &changeDetected);
      libassSameSubs = !changeDetected;
      paintedSomeSubs = changeDetected;

      unsigned char bgra[4];
      while (pic && changeDetected)
      {
#ifdef __BIG_ENDIAN__
        bgra[3] = 0xFF & (pic->color >> 8);
        bgra[2] = 0xFF & (pic->color >> 16);
        bgra[1] = 0xFF & (pic->color >> 24);
        bgra[0] = 0xFF & (pic->color);
#else
        bgra[0] = 0xFF & (pic->color >> 8);
        bgra[1] = 0xFF & (pic->color >> 16);
        bgra[2] = 0xFF & (pic->color >> 24);
        bgra[3] = 0xFF & (pic->color);
#endif
        QImage tmp(pic->w, pic->h, QImage::Format_ARGB32);
        int dstRowBytes = tmp.bytesPerLine();
        unsigned char * dst = tmp.bits();

        for (int y = 0; y < pic->h; y++)
        {
          const unsigned char * srcLine = pic->bitmap + pic->stride * y;
          unsigned char * dstLine = dst + dstRowBytes * y;

          for (int x = 0; x < pic->w; x++, dstLine += 4, srcLine++)
          {
            unsigned char alpha = *srcLine;
#ifdef __BIG_ENDIAN__
            dstLine[0] = alpha;
            memcpy(dstLine + 1, bgra + 1, 3);
#else
            memcpy(dstLine, bgra, 3);
            dstLine[3] = alpha;
#endif
          }
        }

        wrapper.getPainter().drawImage(QRect(pic->dst_x + ix,
                                             pic->dst_y + iy,
                                             pic->w,
                                             pic->h),
                                       tmp, tmp.rect());

        pic = pic->next;
      }
    }
#endif

    wrapper.painterEnd();

    if (reparse && !libassSameSubs)
    {
      subsInOverlay_ = paintedSomeSubs;
    }

    if (!paintedSomeSubs)
    {
      return true;
    }

    TVideoFramePtr & vf = wrapper.getFrame();
    VideoTraits & vtts = vf->traits_;
    QImage & image = wrapper.getImage();

#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    subsInOverlay_ = overlay_->loadFrame(this, vf);
    YAE_ASSERT(subsInOverlay_);
    return subsInOverlay_;
  }

  //----------------------------------------------------------------
  // Canvas::setGreeting
  //
  void
  Canvas::setGreeting(const QString & greeting)
  {
    showTheGreeting_ = true;
    greeting_ = greeting;
    updateGreeting();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::updateGreeting
  //
  bool
  Canvas::updateGreeting()
  {
    if (!overlay_)
    {
      return false;
    }

    double w = this->width();
    double h = this->height();

    if (h > 1024.0)
    {
      w *= 1024.0 / h;
      h = 1024.0;
    }

    if (w > 1024.0)
    {
      h *= 1024.0 / w;
      w = 1024.0;
    }

    TVideoFramePtr vf(new TVideoFrame());
    TQImageBuffer * imageBuffer =
      new TQImageBuffer((int)w, (int)h, QImage::Format_ARGB32);
    vf->data_.reset(imageBuffer);

    // shortcut:
    QImage & subsFrm = imageBuffer->qimg_;
    subsFrm.fill(0);

    QPainter painter(&subsFrm);
    painter.setPen(QColor(0x7f, 0x7f, 0x7f, 0x7f));

    QFont ft;
    int px = std::max<int>(12, 56.0 * (std::min<double>(w, h) / 1024.0));
    ft.setPixelSize(px);
    painter.setFont(ft);

    int textAlignment = Qt::TextWordWrap | Qt::AlignCenter;
    QRect canvasBBox = subsFrm.rect();

    std::string text(greeting_.toUtf8().constData());
    if (!drawPlainText(text, painter, canvasBBox, textAlignment))
    {
      return false;
    }

    painter.end();

    VideoTraits & vtts = vf->traits_;
#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = subsFrm.bytesPerLine() / 4;
    vtts.encodedHeight_ = subsFrm.byteCount() / subsFrm.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    bool ok = overlay_->loadFrame(this, vf);
    YAE_ASSERT(ok);
    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::enableVerticalScaling
  //
  void
  Canvas::enableVerticalScaling(bool enable)
  {
    private_->enableVerticalScaling(enable);
  }

  //----------------------------------------------------------------
  // Canvas::overrideDisplayAspectRatio
  //
  void
  Canvas::overrideDisplayAspectRatio(double dar)
  {
    private_->overrideDisplayAspectRatio(dar);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(double darCropped)
  {
    private_->cropFrame(darCropped);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(const TCropFrame & crop)
  {
    cropAutoDetectStop();

    std::cerr << "\nCROP FRAME AUTO DETECTED: "
              << "x = " << crop.x_ << ", "
              << "y = " << crop.y_ << ", "
              << "w = " << crop.w_ << ", "
              << "h = " << crop.h_
              << std::endl;

    private_->cropFrame(crop);
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetect
  //
  void
  Canvas::cropAutoDetect(void * callbackContext, TAutoCropCallback callback)
  {
    if (!autoCropThread_.isRunning())
    {
      autoCrop_.reset(callbackContext, callback);
      autoCropThread_.setContext(&autoCrop_);
      autoCropThread_.run();
    }
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetectStop
  //
  void
  Canvas::cropAutoDetectStop()
  {
    autoCrop_.stop();
    autoCropThread_.stop();
    autoCropThread_.wait();
    autoCropThread_.setContext(NULL);
  }

  //----------------------------------------------------------------
  // Canvas::imageWidth
  //
  double
  Canvas::imageWidth() const
  {
    return private_ ? calcImageWidth(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageHeight
  //
  double
  Canvas::imageHeight() const
  {
    return private_ ? calcImageHeight(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageAspectRatio
  //
  double
  Canvas::imageAspectRatio(double & w, double & h) const
  {
    double dar = 0.0;
    w = 0.0;
    h = 0.0;

    if (private_)
    {
      dar = private_->imageWidthHeight(w, h) ? w / h : 0.0;
    }

    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::libassInitDoneCallback
  //
  void
  Canvas::libassInitDoneCallback(void * context)
  {
    Canvas * canvas = (Canvas *)context;
    qApp->postEvent(canvas, new LibassInitDoneEvent());
  }

  //----------------------------------------------------------------
  // Canvas::hideCursor
  //
  void
  Canvas::hideCursor()
  {
    setCursor(QCursor(Qt::BlankCursor));
  }

  //----------------------------------------------------------------
  // Canvas::wakeScreenSaver
  //
  void
  Canvas::wakeScreenSaver()
  {
#ifdef __APPLE__
    UpdateSystemActivity(UsrActivity);
#elif defined(_WIN32)
    // http://www.codeproject.com/KB/system/disablescreensave.aspx
    //
    // Call the SystemParametersInfo function to query and reset the
    // screensaver time-out value.  Use the user's default settings
    // in case your application terminates abnormally.
    //

    static UINT spiGetter[] = { SPI_GETLOWPOWERTIMEOUT,
                                SPI_GETPOWEROFFTIMEOUT,
                                SPI_GETSCREENSAVETIMEOUT };

    static UINT spiSetter[] = { SPI_SETLOWPOWERTIMEOUT,
                                SPI_SETPOWEROFFTIMEOUT,
                                SPI_SETSCREENSAVETIMEOUT };

    std::size_t numParams = sizeof(spiGetter) / sizeof(spiGetter[0]);
    for (std::size_t i = 0; i < numParams; i++)
    {
      UINT val = 0;
      BOOL ok = SystemParametersInfo(spiGetter[i], 0, &val, 0);
      YAE_ASSERT(ok);
      if (ok)
      {
        ok = SystemParametersInfo(spiSetter[i], val, NULL, 0);
        YAE_ASSERT(ok);
      }
    }

#else
    // try using DBUS to talk to the screensaver...
    bool done = false;

    if (QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        screensaver.call(QDBus::NoBlock, "SimulateUserActivity");
        done = true;
      }
    }

    if (!done)
    {
      // FIXME: not sure how to do this yet
      std::cerr << "wakeScreenSaver" << std::endl;
    }
#endif
  }

}

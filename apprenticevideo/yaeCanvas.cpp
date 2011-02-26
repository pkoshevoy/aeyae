// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// GLEW includes:
#include <GL/glew.h>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>
#include <yaePixelFormatTraits.h>

// the includes:
#include <image/image_tile_generator.hxx>
#include <opengl/image_tile_dl_elem.hxx>
#include <opengl/glsl.hxx>
#include <utils/the_utils.hxx>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>


//----------------------------------------------------------------
// yae_to_opengl
// 
static unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType)
{
  // FIXME: add YCbCr422 OpenGL APPLE extension here:
  
  switch (yaePixelFormat)
  {
#ifdef __APPLE__
    case yae::kPixelFormatYUYV422:
      //! packed RGB 8:8:8, 24bpp, RGBRGB...
      internalFormat = 3;
      format = GL_YCBCR_422_APPLE;
      dataType = GL_UNSIGNED_SHORT_8_8_APPLE;
      return 3;
      
    case yae::kPixelFormatUYVY422:
      //! packed RGB 8:8:8, 24bpp, RGBRGB...
      internalFormat = 3;
      format = GL_YCBCR_422_APPLE;
      dataType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
      return 3;
#endif
      
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
      
#if 0
    case yae::kPixelFormatMONOWHITE:
      //! Y, 1bpp, 0 is white, 1 is black, in each byte pixels are
      //! ordered from the msb to the lsb
    case yae::kPixelFormatMONOBLACK:
      //! Y, 1bpp, 0 is black, 1 is white, in each byte pixels are
      //! ordered from the msb to the lsb
#if 0
      glEnable(GL_TEXTURE_2D);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, texID);
      glColorTableEXT(GL_TEXTURE_2D,
                      GL_RGBA8,
                      256,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      palette);
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_COLOR_INDEX8_EXT, // internal format
                   width,
                   height,
                   0,
                   GL_COLOR_INDEX, // format
                   GL_UNSIGNED_BYTE, // datatype
                   texture);
#endif
      internalFormat = GL_LUMINANCE;
      format = GL_COLOR_INDEX;
      dataType = GL_BITMAP;
      return 1;
#endif
      
    case yae::kPixelFormatBGR8:
      //! packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_2_3_3_REV;
      return 3;
      
    case yae::kPixelFormatRGB8:
      //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_3_3_2;
      return 3;
      
    case yae::kPixelFormatARGB:
      //! packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
      internalFormat = 4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_INT_8_8_8_8;
      return 4;
      
    case yae::kPixelFormatRGBA:
      //! packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
      internalFormat = 4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
      return 4;
      
    case yae::kPixelFormatABGR:
      //! packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
      internalFormat = 4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_INT_8_8_8_8;
      return 4;
      
    case yae::kPixelFormatBGRA:
      //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
      internalFormat = 4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
      return 4;
      
    case yae::kPixelFormatGRAY16BE:
      //! Y, 16bpp, big-endian
    case yae::kPixelFormatGRAY16LE:
      //! Y, 16bpp, little-endian
    case yae::kPixelFormatYUV420P16LE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV420P16BE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV422P16LE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV422P16BE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV444P16LE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV444P16BE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! big-endian
      
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;
      
    case yae::kPixelFormatRGB48BE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as big-endian
    case yae::kPixelFormatRGB48LE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as little-endian
      
      internalFormat = GL_RGB16;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT;
      return 3;
      
    case yae::kPixelFormatRGB565BE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
    case yae::kPixelFormatBGR565LE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
      
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB565LE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
    case yae::kPixelFormatBGR565BE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
      
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5;
      return 3;
      
    case yae::kPixelFormatRGB555BE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
      //! most significant bit to 0
    case yae::kPixelFormatBGR555LE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
      //! most significant bit to 1
      
      internalFormat = 3;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB555LE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
      //! most significant bit to 0
    case yae::kPixelFormatBGR555BE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
      //! most significant bit to 1
      
      internalFormat = 3;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;
      
    case yae::kPixelFormatRGB444BE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
      //! most significant bits to 0
    case yae::kPixelFormatBGR444LE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
      //! most significant bits to 1
      
      internalFormat = GL_RGB4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;
      
    case yae::kPixelFormatRGB444LE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
      //! most significant bits to 0
    case yae::kPixelFormatBGR444BE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
      //! most significant bits to 1
      
      internalFormat = GL_RGB4;
      format = GL_BGRA;
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
  // mayReuseTextureObject
  // 
  static bool
  mayReuseTextureObject(const TVideoFramePtr & a,
                        const TVideoFramePtr & b)
  {
    if (!a || !b)
    {
      return false;
    }
    
    const VideoTraits & at = a->traits_;
    const VideoTraits & bt = b->traits_;
    return (at.pixelFormat_ == bt.pixelFormat_ &&
            at.encodedWidth_ == bt.encodedWidth_ &&
            at.encodedHeight_ == bt.encodedHeight_);
  }
  
  
  //----------------------------------------------------------------
  // Canvas::TPrivate
  // 
  class Canvas::TPrivate
  {
  public:
    TPrivate();
    
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);
    void draw();

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
          return
            double(frame_->traits_.visibleWidth_) *
            frame_->traits_.pixelAspectRatio_;
        }

        return double(frame_->traits_.visibleWidth_);
      }
      
      return 0.0;
    }

    inline double imageHeight() const
    {
      return (frame_ ? double(frame_->traits_.visibleHeight_) : 0.0);
    }
    
    TVideoFramePtr frame_;
    
  private:
    GLuint texId_;
    mutable boost::mutex mutex_;
  };
  
  //----------------------------------------------------------------
  // Canvas::TPrivate::TPrivate
  // 
  Canvas::TPrivate::TPrivate():
    texId_(0)
  {}

  //----------------------------------------------------------------
  // getImageSpecsGL
  // 
  static unsigned int
  getImageSpecsGL(const TVideoFramePtr & frame,
                  GLint & internalFormatGL,
                  GLenum & pixelFormatGL,
                  GLenum & dataTypeGL)
  {
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);
    
    if (!ptts)
    {
      // don't know how to handle this pixel format:
      assert(false);
      return false;
    }
    
    return yae_to_opengl(vtts.pixelFormat_,
                         internalFormatGL,
                         pixelFormatGL,
                         dataTypeGL);
  }

  //----------------------------------------------------------------
  // Canvas::TPrivate::loadFrame
  // 
  bool
  Canvas::TPrivate::loadFrame(QGLWidget * canvas,
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
    unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                   internalFormatGL,
                                                   pixelFormatGL,
                                                   dataTypeGL);
    if (// supportedChannels != ptts->channels_ ||
        !supportedChannels)
    {
      return false;
    }
    
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);
    
    bool reuseTextureObject = mayReuseTextureObject(frame_, frame) && texId_;
    if (!reuseTextureObject)
    {
      glDeleteTextures(1, &texId_);
      texId_ = 0;
    }
    
    frame_ = frame;
    
    if (!texId_)
    {
      glGenTextures(1, &texId_);
    }
    
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texId_);

    if (!reuseTextureObject)
    {
      glTexImage2D(GL_TEXTURE_RECTANGLE_EXT,
                   0, // always level-0 for GL_TEXTURE_RECTANGLE_EXT
                   internalFormatGL,
                   vtts.encodedWidth_,
                   vtts.encodedHeight_,
                   0, // border width
                   pixelFormatGL,
                   dataTypeGL,
                   NULL); // probably can't do this with APPLE_client_storage
    }

    glPushClientAttrib(GL_UNPACK_ALIGNMENT);
    {
      // glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_);
      // glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels_ + offset_x);
      // glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows_ + offset_y);

      // FIXME: will these solve the BE/LE mismatch problem?
      // glPixelStorei(GL_UNPACK_SWAP_BYTES, swap_bytes_);
      // glPixelStorei(GL_UNPACK_LSB_FIRST, lsb_first_);
      
#ifdef __APPLE__
      glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
#endif
      
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT,
                      0, // always level-0 for GL_TEXTURE_RECTANGLE_EXT
                      0, // x-offset
                      0, // y-offset
                      vtts.encodedWidth_,
                      vtts.encodedHeight_,
                      pixelFormatGL,
                      dataTypeGL,
                      frame->getBuffer<unsigned char>());
    }
    glPopClientAttrib();
    return true;
  }

  //----------------------------------------------------------------
  // Canvas::TPrivate::draw
  // 
  void
  Canvas::TPrivate::draw()
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
  // Canvas::Canvas
  // 
  Canvas::Canvas(const QGLFormat & format,
                 QWidget * parent,
                 const QGLWidget * shareWidget,
                 Qt::WindowFlags f):
    QGLWidget(format, parent, shareWidget, f),
    private_(new TPrivate())
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
  {}
  
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

#if 0
    // image_tile_dl_elem_t needs access to current OpenGL context:
    the_scoped_variable_t<the_gl_context_interface_t *>
      active_context(the_gl_context_interface_t::current_, this, NULL);
    
    // make a local copy of the auto pointer to avoid a race condition:
    TImagePtr image = image_;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if (image.get())
    {
      the_scoped_gl_attrib_t push_attr(GL_ALL_ATTRIB_BITS);
      
      int canvasWidth = width();
      int canvasHeight = height();
      
      GLenum data_type = GL_UNSIGNED_BYTE;
      GLenum format_internal = GL_RGB8;
      GLenum format = GL_RGB;
      image->get_texture_info(data_type, format_internal, format);
      
      if (format == GL_BGRA ||
          format == GL_RGBA ||
          format == GL_ALPHA ||
          format == GL_LUMINANCE_ALPHA ||
          format == GL_COLOR_INDEX)
      {
        glViewport(0, 0, canvasWidth, canvasHeight);
        glMatrixMode(GL_PROJECTION);
        gluOrtho2D(0, canvasWidth, canvasHeight, 0);
        
        float zebra[2][3] =
        {
          1.0f, 1.0f, 1.0f,
          0.7f, 0.7f, 0.7f
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
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      the_bbox_t bbox;
      image->update_bbox(bbox);

      float imageWidth = bbox.aa().max_.x() - bbox.aa().min_.x();
      float imageHeight = bbox.aa().max_.y() - bbox.aa().min_.y();
      
      glViewport(0, 0, canvasWidth, canvasHeight);
      glMatrixMode(GL_PROJECTION);
      gluOrtho2D(0, imageWidth, imageHeight, 0);
      
      glEnable(GL_TEXTURE_2D);
      image->draw();
    }
    else
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }
#else
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const pixelFormat::Traits * ptts = private_->pixelTraits();
    if (ptts)
    {
      the_scoped_gl_attrib_t push_attr(GL_ALL_ATTRIB_BITS);
      
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
          1.0f, 1.0f, 1.0f,
          0.7f, 0.7f, 0.7f
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
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      float imageWidth = float(private_->imageWidth());
      float imageHeight = float(private_->imageHeight());
      
      glViewport(0, 0, canvasWidth, canvasHeight);
      glMatrixMode(GL_PROJECTION);
      gluOrtho2D(0, imageWidth, imageHeight, 0);
      
      glEnable(GL_TEXTURE_2D);
      private_->draw();
    }
    else
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }
#endif
  }
  
  //----------------------------------------------------------------
  // Canvas::loadFrame
  // 
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
#if 0
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);
    
    if (!ptts)
    {
      // don't know how to handle this pixel format:
      assert(false);
      return false;
    }
    
    GLint format_internal = 0;
    GLenum format = 0;
    GLenum data_type = 0;
    
    unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                   format_internal,
                                                   format,
                                                   data_type);
    if (!supportedChannels)
    {
      // can't render any part of the given frame using OpenGL:
      return false;
    }
    
    // gather some stats about the image:
    std::size_t bytes_per_pixel = ptts->stride_[supportedChannels - 1] / 8;
    std::size_t imgWidth = frame->traits_.visibleWidth_;
    std::size_t imgHeight = frame->traits_.visibleHeight_;
    
    image_tile_generator_t tile_generator;
    tile_generator.layout(imgWidth, imgHeight);
    
    const unsigned char * dataBuffer = frame->getBuffer<unsigned char>();
    unsigned int dataBufferAlignment = 1;
    
    tile_generator.convert_and_pad(dataBuffer,
                                   dataBufferAlignment,
                                   bytes_per_pixel,
                                   bytes_per_pixel,
                                   copy_pixels_t());
    
    const size_t max_texture = 1024;
    tile_generator.make_tiles(data_type,
                              format_internal,
                              format,
                              max_texture);
    
    GLenum filter = GL_LINEAR; // GL_NEAREST;
    TImagePtr image(new image_tile_dl_elem_t(tile_generator,
                                             filter,
                                             filter));
    image_ = image;
    refresh();
    
    return true;
#else
    bool ok = private_->loadFrame(this, frame);
    refresh();
    return ok;
#endif
  }
}

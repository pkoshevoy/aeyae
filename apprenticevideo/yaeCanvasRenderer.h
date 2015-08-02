// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_RENDERER_H_
#define YAE_CANVAS_RENDERER_H_

// system includes:
#include <string>

// Qt includes:
#define GL_GLEXT_PROTOTYPES
#include <QtOpenGL>

// yae includes:
#include "yae/video/yae_auto_crop.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video.h"


//----------------------------------------------------------------
// yae_is_opengl_extension_supported
//
YAE_API bool
yae_is_opengl_extension_supported(const char * extension);

//----------------------------------------------------------------
// yae_to_opengl
//
// returns number of sample planes supported by OpenGL,
// passes back parameters to use with glTexImage2D
//
YAE_API unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes);

//----------------------------------------------------------------
// yae_reset_opengl_to_initial_state
//
YAE_API void
yae_reset_opengl_to_initial_state();

//----------------------------------------------------------------
// yae_assert_gl_no_error
//
YAE_API bool
yae_assert_gl_no_error();

namespace yae
{
  // forward declarations:
  struct TBaseCanvas;
  struct TLegacyCanvas;
  struct TModernCanvas;
  struct TFragmentShader;

  //----------------------------------------------------------------
  // IOpenGLContext
  //
  struct YAE_API IOpenGLContext
  {
    virtual bool makeCurrent() = 0;
    virtual void doneCurrent() = 0;
  };

  //----------------------------------------------------------------
  // TMakeCurrentContext
  //
  struct TMakeCurrentContext
  {
    TMakeCurrentContext(IOpenGLContext & context):
      context_(context),
      current_(false)
    {
      current_ = context_.makeCurrent();
      YAE_ASSERT(current_);
    }

    ~TMakeCurrentContext()
    {
      if (current_)
      {
        context_.doneCurrent();
      }
    }

    IOpenGLContext & context_;
    bool current_;
  };

  //----------------------------------------------------------------
  // TGLSaveState
  //
  struct TGLSaveState
  {
    TGLSaveState(GLbitfield mask);
    ~TGLSaveState();

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveClientState
  //
  struct TGLSaveClientState
  {
    TGLSaveClientState(GLbitfield mask);
    ~TGLSaveClientState();

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveMatrixState
  //
  struct TGLSaveMatrixState
  {
    TGLSaveMatrixState(GLenum mode);
    ~TGLSaveMatrixState();

  protected:
    GLenum matrixMode_;
  };

  //----------------------------------------------------------------
  // CanvasRenderer
  //
  class CanvasRenderer
  {
    std::string openglVendorInfo_;
    std::string openglRendererInfo_;
    std::string openglVersionInfo_;

    TLegacyCanvas * legacy_;
    TModernCanvas * modern_;
    TBaseCanvas * renderer_;

    // maximum texture size supported by the GL_EXT_texture_rectangle;
    // frames with width/height in excess of this value will be processed
    // using the legacy canvas renderer, which cuts frames into tiles
    // of supported size and renders them seamlessly:
    unsigned int maxTexSize_;

  public:
    CanvasRenderer();
    ~CanvasRenderer();

    void clear(IOpenGLContext & context);

    TBaseCanvas * rendererFor(const VideoTraits & vtts) const;

    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

    void draw();

    const pixelFormat::Traits * pixelTraits() const;

    void skipColorConverter(IOpenGLContext & context, bool enable);

    void enableVerticalScaling(bool enable);

    bool getCroppedFrame(TCropFrame & crop) const;

    bool imageWidthHeight(double & w, double & h) const;
    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const;

    void overrideDisplayAspectRatio(double dar);

    void cropFrame(double darCropped);
    void cropFrame(const TCropFrame & crop);

    void getFrame(TVideoFramePtr & frame) const;

    const TFragmentShader *
    fragmentShaderFor(const VideoTraits & vtts) const;
  };

}

#endif // YAE_CANVAS_RENDERER_H_

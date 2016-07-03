// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_RENDERER_H_
#define YAE_CANVAS_RENDERER_H_

// system includes:
#include <deque>
#include <string>
#include <stdexcept>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// Qt includes:
#ifdef YAE_USE_QOPENGL_WIDGET
#define GL_GLEXT_PROTOTYPES
#include <QtOpenGL>
#include <QOpenGLFunctions_1_0>
#include <QOpenGLFunctions_1_1>
#include <QOpenGLFunctions_1_4>
#include <QOpenGLFunctions_2_0>
#else
// GLEW includes:
#include <GL/glew.h>
#endif

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

#ifdef YAE_USE_QOPENGL_WIDGET
  //----------------------------------------------------------------
  // TProgramStringARB
  //
  typedef void (APIENTRYP TProgramStringARB)(GLenum target,
                                             GLenum format,
                                             GLsizei len,
                                             const void * string);

  //----------------------------------------------------------------
  // TGetProgramivARB
  //
  typedef void (APIENTRYP TGetProgramivARB)(GLenum target,
                                            GLenum pname,
                                            GLint * params);

  //----------------------------------------------------------------
  // TDeleteProgramsARB
  //
  typedef void (APIENTRYP TDeleteProgramsARB)(GLsizei n,
                                              const GLuint * programs);

  //----------------------------------------------------------------
  // TBindProgramARB
  //
  typedef void (APIENTRYP TBindProgramARB)(GLenum target,
                                           GLuint program);

  //----------------------------------------------------------------
  // TGenProgramsARB
  //
  typedef void (APIENTRYP TGenProgramsARB)(GLsizei n,
                                           GLuint * programs);

  //----------------------------------------------------------------
  // TProgramLocalParameter4dvARB
  //
  typedef void (APIENTRYP TProgramLocalParameter4dvARB)(GLenum target,
                                                        GLuint index,
                                                        const GLdouble *);

  //----------------------------------------------------------------
  // YAE_GL_FRAGMENT_PROGRAM_ARB
  //
  struct YAE_API OpenGLFunctionPointers : public QOpenGLFunctions
  {
    TProgramStringARB glProgramStringARB;
    TGetProgramivARB glGetProgramivARB;
    TDeleteProgramsARB glDeleteProgramsARB;
    TBindProgramARB glBindProgramARB;
    TGenProgramsARB glGenProgramsARB;
    TProgramLocalParameter4dvARB glProgramLocalParameter4dvARB;

    OpenGLFunctionPointers();

    static OpenGLFunctionPointers & get();
  };

  //----------------------------------------------------------------
  // ogl_context
  //
  inline static QOpenGLContext & ogl_context()
  {
    QOpenGLContext * context = QOpenGLContext::currentContext();
    YAE_ASSERT(context);
    if (!context)
    {
      throw std::runtime_error("QOpenGLContext::currentContext() is NULL");
    }

    return *context;
  }

  //----------------------------------------------------------------
  // ogl_11
  //
  inline static QOpenGLFunctions_1_1 & ogl_11()
  {
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_1>());
  }

  //----------------------------------------------------------------
  // ogl_14
  //
  inline static QOpenGLFunctions_1_4 & ogl_14()
  {
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_4>());
  }

  //----------------------------------------------------------------
  // ogl_20
  //
  inline static QOpenGLFunctions_2_0 & ogl_20()
  {
    return *(ogl_context().versionFunctions<QOpenGLFunctions_2_0>());
  }

#define YAE_OGL_11_HERE() QOpenGLFunctions_1_1 & ogl_11 = yae::ogl_11()
#define YAE_OGL_14_HERE() QOpenGLFunctions_1_4 & ogl_14 = yae::ogl_14()
#define YAE_OGL_20_HERE() QOpenGLFunctions_2_0 & ogl_20 = yae::ogl_20()
#define YAE_OPENGL_HERE() \
  yae::OpenGLFunctionPointers & opengl = yae::OpenGLFunctionPointers::get()


#define YAE_OGL_11(x) ogl_11.x
#define YAE_OGL_14(x) ogl_14.x
#define YAE_OGL_20(x) ogl_20.x
#define YAE_OPENGL(x) opengl.x

#else
#define YAE_OGL_11_HERE()
#define YAE_OGL_14_HERE()
#define YAE_OGL_20_HERE()
#define YAE_OPENGL_HERE()

#define YAE_OGL_11(x) x
#define YAE_OGL_14(x) x
#define YAE_OGL_20(x) x
#define YAE_OPENGL(x) x

#endif

  //----------------------------------------------------------------
  // IOpenGLContext
  //
  struct YAE_API IOpenGLContext
  {
    IOpenGLContext(): n_(0) {}
    virtual ~IOpenGLContext() {}

    inline bool lock()
    {
      mutex_.lock();
      n_++;

      if (n_ == 1)
      {
        this->makeCurrent();
      }

      return true;
    }

    inline void unlock()
    {
      YAE_ASSERT(n_ > 0);
      n_--;

      if (n_ == 0)
      {
        this->doneCurrent();
      }

      mutex_.unlock();
    }

  protected:
    virtual bool makeCurrent() = 0;
    virtual void doneCurrent() = 0;

  private:
    boost::recursive_mutex mutex_;
    std::size_t n_;
  };

  //----------------------------------------------------------------
  // TMakeCurrentContext
  //
  struct TMakeCurrentContext
  {
    TMakeCurrentContext(IOpenGLContext & context):
      context_(context)
    {
      context_.lock();
    }

    ~TMakeCurrentContext()
    {
      context_.unlock();
    }

    IOpenGLContext & context_;
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
  // powerOfTwoGEQ
  //
  // calculate least power-of-two greater then or equal to the given
  //
  template <typename TScalar>
  inline static TScalar
  powerOfTwoGEQ(const TScalar & given)
  {
    TScalar leq = powerOfTwoLEQ<TScalar>(given);
    return (leq == given) ? leq : leq * TScalar(2);
  }

  //----------------------------------------------------------------
  // alignmentFor
  //
  extern int
  alignmentFor(const unsigned char * data, std::size_t rowBytes);


  //----------------------------------------------------------------
  // TFragmentShaderProgram
  //
  struct YAE_API TFragmentShaderProgram
  {
    TFragmentShaderProgram(const char * code = NULL);

    // delete the program:
    void destroy();

    // helper:
    inline bool loaded() const
    { return code_ && handle_; }

    // GL_ARB_fragment_program source code:
    const char * code_;

    // GL_ARB_fragment_program handle:
    GLuint handle_;
  };


  //----------------------------------------------------------------
  // TFragmentShader
  //
  struct YAE_API TFragmentShader
  {
    TFragmentShader(const TFragmentShaderProgram * program = NULL,
                    TPixelFormatId format = kInvalidPixelFormat);

    // pointer to the shader program:
    const TFragmentShaderProgram * program_;

    // number of texture objects required for this pixel format:
    unsigned char numPlanes_;

    // sample stride per texture object:
    unsigned char stride_[4];

    // sample plane (sub)sampling per texture object:
    unsigned char subsample_x_[4];
    unsigned char subsample_y_[4];

    GLint internalFormatGL_[4];
    GLenum pixelFormatGL_[4];
    GLenum dataTypeGL_[4];
    GLenum magFilterGL_[4];
    GLenum minFilterGL_[4];
    GLint shouldSwapBytes_[4];
  };


  //----------------------------------------------------------------
  // TBaseCanvas
  //
  struct TBaseCanvas
  {
    TBaseCanvas();
    virtual ~TBaseCanvas();

    virtual void createFragmentShaders() = 0;

    virtual void clear(IOpenGLContext & context) = 0;

    virtual bool loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame) = 0;

    virtual void draw(double opacity) = 0;

    // helper:
    const pixelFormat::Traits * pixelTraits() const;

    void skipColorConverter(IOpenGLContext & context, bool enable);

    inline bool skipColorConverter() const
    { return skipColorConverter_; }

    void enableVerticalScaling(bool enable);

    bool getCroppedFrame(TCropFrame & crop) const;

    bool imageWidthHeight(double & w, double & h) const;
    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const;

    void overrideDisplayAspectRatio(double dar);

    inline double overrideDisplayAspectRatio() const
    { return dar_; }

    void cropFrame(double darCropped);
    void cropFrame(const TCropFrame & crop);

    void getFrame(TVideoFramePtr & frame) const;

    // helper:
    const TFragmentShader *
    fragmentShaderFor(TPixelFormatId format) const;

  protected:
    // helper:
    const TFragmentShader *
    findSomeShaderFor(TPixelFormatId format) const;

    // helper:
    void destroyFragmentShaders();

    // helper:
    bool createBuiltinFragmentShader(const char * code);

    // helper:
    bool createFragmentShadersFor(const TPixelFormatId * formats,
                                  const std::size_t numFormats,
                                  const char * code);

    // helper:
    bool setFrame(const TVideoFramePtr & frame,
                  bool & colorSpaceOrRangeChanged);

    mutable boost::mutex mutex_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    double dar_;
    double darCropped_;
    bool skipColorConverter_;
    bool verticalScalingEnabled_;

    TFragmentShaderProgram builtinShaderProgram_;
    TFragmentShader builtinShader_;

    std::list<TFragmentShaderProgram> shaderPrograms_;
    std::map<TPixelFormatId, TFragmentShader> shaders_;

    // shader selected for current frame:
    const TFragmentShader * shader_;

    // 3x4 matrix for color conversion to full-range RGB,
    // including luma scale and shift:
    double m34_to_rgb_[12];
  };

  //----------------------------------------------------------------
  // TModernCanvas
  //
  struct TModernCanvas : public TBaseCanvas
  {
    // virtual:
    void createFragmentShaders();

    // virtual:
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

    // virtual:
    void draw(double opacity);

  protected:
    std::vector<GLuint> texId_;
  };


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
  // calculateEdges
  //
  extern void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgeSize,
                 GLsizei textureEdgeMax);

  //----------------------------------------------------------------
  // getTextureEdgeMax
  //
  extern GLsizei
  getTextureEdgeMax();

  //----------------------------------------------------------------
  // TLegacyCanvas
  //
  // This is a subclass implementing frame rendering on OpenGL
  // hardware that doesn't support GL_EXT_texture_rectangle
  //
  struct TLegacyCanvas : public TBaseCanvas
  {
    TLegacyCanvas();

    // virtual:
    void createFragmentShaders();

    // virtual:
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

    // virtual:
    void draw(double opacity);

  protected:
    // unpadded image dimensions:
    GLsizei w_;
    GLsizei h_;

    std::vector<TFrameTile> tiles_;
    std::vector<GLuint> texId_;
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

    void draw(double opacity = 1.0) const;

    const pixelFormat::Traits * pixelTraits() const;

    void skipColorConverter(IOpenGLContext & context, bool enable);

    inline bool skipColorConverter() const
    { return legacy_->skipColorConverter(); }

    void enableVerticalScaling(bool enable);

    bool getCroppedFrame(TCropFrame & crop) const;

    bool imageWidthHeight(double & w, double & h) const;
    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const;

    void overrideDisplayAspectRatio(double dar);

    inline double overrideDisplayAspectRatio() const
    { return legacy_->overrideDisplayAspectRatio(); }

    void cropFrame(double darCropped);
    void cropFrame(const TCropFrame & crop);

    void getFrame(TVideoFramePtr & frame) const;

    const TFragmentShader *
    fragmentShaderFor(const VideoTraits & vtts) const;

    // helper:
    void paintImage(double x,
                    double y,
                    double w_max,
                    double h_max,
                    double opacity = 1.0) const;
  };
}


#endif // YAE_CANVAS_RENDERER_H_

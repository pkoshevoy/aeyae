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
#if defined(YAE_USE_QT4) || \
  (defined(YAE_USE_QT5) && !defined(YAE_USE_QOPENGL_WIDGET))
#ifndef YAE_USE_QGL_WIDGET
#define YAE_USE_QGL_WIDGET
#endif
#else
#include <QOpenGLWidget>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QOpenGLVersionFunctionsFactory>
#endif
#endif

#ifdef YAE_USE_QGL_WIDGET
// GLEW:
#include <GL/glew.h>
#include <QGLWidget>
#else
#define GL_GLEXT_PROTOTYPES
#include <QtOpenGL>
#include <QOpenGLFunctions_1_0>
#include <QOpenGLFunctions_1_1>
#include <QOpenGLFunctions_1_2>
#include <QOpenGLFunctions_1_3>
#include <QOpenGLFunctions_1_4>
#include <QOpenGLFunctions_2_0>
#endif

// yae includes:
#include "yae/video/yae_auto_crop.h"
#include "yae/video/yae_color_transform.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video.h"


//----------------------------------------------------------------
// yae_is_opengl_extension_supported
//
YAEUI_API bool
yae_is_opengl_extension_supported(const char * extension);

//----------------------------------------------------------------
// yae_to_opengl
//
// returns number of sample planes supported by OpenGL,
// passes back parameters to use with glTexImage2D
//
YAEUI_API unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes);

//----------------------------------------------------------------
// yae_reset_opengl_to_initial_state
//
YAEUI_API void
yae_reset_opengl_to_initial_state();

//----------------------------------------------------------------
// yae_assert_gl_no_error
//
YAEUI_API bool
yae_assert_gl_no_error();

//----------------------------------------------------------------
// yae_opengl_debug_message_cb
//
extern "C" void
yae_opengl_debug_message_cb(GLenum src,
                            GLenum t,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar * message,
                            const void * userParam);

namespace yae
{
  // forward declarations:
  struct TBaseCanvas;
  struct TLegacyCanvas;
  struct TModernCanvas;
  struct TFragmentShader;
}

namespace yaegl
{
  YAEUI_API bool assert_no_error();

#ifndef YAE_USE_QGL_WIDGET

  //----------------------------------------------------------------
  // TDEBUGPROC
  //
  typedef void (APIENTRYP TDEBUGPROC)(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar * message,
                                      const void * userParam);

  //----------------------------------------------------------------
  // TDebugMessageCallback
  //
  typedef void (APIENTRYP TDebugMessageCallback)(TDEBUGPROC callback,
                                                 void * userParam);

  //----------------------------------------------------------------
  // TBegin
  //
  typedef void (APIENTRYP TBegin)(GLenum mode);

  //----------------------------------------------------------------
  // TEnd
  //
  typedef void (APIENTRYP TEnd)();

  //----------------------------------------------------------------
  // TClear
  //
  typedef void (APIENTRYP TClear)(GLbitfield mask);

  //----------------------------------------------------------------
  // TClearAccum
  //
  typedef void (APIENTRYP TClearAccum)(GLfloat red,
                                       GLfloat green,
                                       GLfloat blue,
                                       GLfloat alpha);

  //----------------------------------------------------------------
  // TClearColor
  //
  typedef void (APIENTRYP TClearColor)(GLclampf red,
                                       GLclampf green,
                                       GLclampf blue,
                                       GLclampf alpha);

  //----------------------------------------------------------------
  // TClearDepth
  //
  typedef void (APIENTRYP TClearDepth)(GLclampd depth);

  //----------------------------------------------------------------
  // TClearStencil
  //
  typedef void (APIENTRYP TClearStencil)(GLint s);

  //----------------------------------------------------------------
  // TDepthFunc
  //
  typedef void (APIENTRYP TDepthFunc)(GLenum func);

  //----------------------------------------------------------------
  // TDepthMask
  //
  typedef void (APIENTRYP TDepthMask)(GLboolean flag);

  //----------------------------------------------------------------
  // TStencilFunc
  //
  typedef void (APIENTRYP TStencilFunc)(GLenum func,
                                        GLint  ref,
                                        GLuint mask);

  //----------------------------------------------------------------
  // TStencilMask
  //
  typedef void (APIENTRYP TStencilMask)(GLuint mask);

  //----------------------------------------------------------------
  // TStencilOp
  //
  typedef void (APIENTRYP TStencilOp)(GLenum fail,
                                      GLenum zfail,
                                      GLenum zpass);

  //----------------------------------------------------------------
  // TColorMask
  //
  typedef void (APIENTRYP TColorMask)(GLboolean red,
                                      GLboolean green,
                                      GLboolean blue,
                                      GLboolean alpha);

  //----------------------------------------------------------------
  // TColor3d
  //
  typedef void (APIENTRYP TColor3d)(GLdouble r,
                                    GLdouble g,
                                    GLdouble b);

  //----------------------------------------------------------------
  // TColor3f
  //
  typedef void (APIENTRYP TColor3f)(GLfloat red,
                                    GLfloat green,
                                    GLfloat blue);

  //----------------------------------------------------------------
  // TColor3fv
  //
  typedef void (APIENTRYP TColor3fv)(const GLfloat * v);

  //----------------------------------------------------------------
  // TColor4d
  //
  typedef void (APIENTRYP TColor4d)(GLdouble red,
                                    GLdouble green,
                                    GLdouble blue,
                                    GLdouble alpha);

  //----------------------------------------------------------------
  // TColor4ub
  //
  typedef void (APIENTRYP TColor4ub)(GLubyte red,
                                     GLubyte green,
                                     GLubyte blue,
                                     GLubyte alpha);

  //----------------------------------------------------------------
  // TVertex2d
  //
  typedef void (APIENTRYP TVertex2d)(GLdouble x,
                                     GLdouble y);

  //----------------------------------------------------------------
  // TVertex2dv
  //
  typedef void (APIENTRYP TVertex2dv)(const GLdouble * v);

  //----------------------------------------------------------------
  // TVertex2i
  //
  typedef void (APIENTRYP TVertex2i)(GLint x,
                                     GLint y);

  //----------------------------------------------------------------
  // TRecti
  //
  typedef void (APIENTRYP TRecti)(GLint x1,
                                  GLint y1,
                                  GLint x2,
                                  GLint y2);

  //----------------------------------------------------------------
  // TRectd
  //
  typedef void (APIENTRYP TRectd)(GLdouble x1,
                                  GLdouble y1,
                                  GLdouble x2,
                                  GLdouble y2);

  //----------------------------------------------------------------
  // TMatrixMode
  //
  typedef void (APIENTRYP TMatrixMode)(GLenum mode);

  //----------------------------------------------------------------
  // TPushMatrix
  //
  typedef void (APIENTRYP TPushMatrix)(void);

  //----------------------------------------------------------------
  // TPopMatrix
  //
  typedef void (APIENTRYP TPopMatrix)(void);

  //----------------------------------------------------------------
  // TViewport
  //
  typedef void (APIENTRYP TViewport)(GLint   x,
                                     GLint   y,
                                     GLsizei width,
                                     GLsizei height);

  //----------------------------------------------------------------
  // TOrtho
  //
  typedef void (APIENTRYP TOrtho)(GLdouble left,
                                  GLdouble right,
                                  GLdouble bottom,
                                  GLdouble top,
                                  GLdouble zNear,
                                  GLdouble zFar);

  //----------------------------------------------------------------
  // TLoadIdentity
  //
  typedef void (APIENTRYP TLoadIdentity)(void);

  //----------------------------------------------------------------
  // TRotated
  //
  typedef void (APIENTRYP TRotated)(GLdouble angle,
                                    GLdouble x,
                                    GLdouble y,
                                    GLdouble z);

  //----------------------------------------------------------------
  // TScaled
  //
  typedef void (APIENTRYP TScaled)(GLdouble x,
                                   GLdouble y,
                                   GLdouble z);

  //----------------------------------------------------------------
  // TTranslated
  //
  typedef void (APIENTRYP TTranslated)(GLdouble x,
                                       GLdouble y,
                                       GLdouble z);

  //----------------------------------------------------------------
  // TPolygonMode
  //
  typedef void (APIENTRYP TPolygonMode)(GLenum face,
                                        GLenum mode);

  //----------------------------------------------------------------
  // TShadeModel
  //
  typedef void (APIENTRYP TShadeModel)(GLenum mode);

  //----------------------------------------------------------------
  // TAlphaFunc
  //
  typedef void (APIENTRYP TAlphaFunc)(GLenum func,
                                      GLclampf ref);

  //----------------------------------------------------------------
  // TPushAttrib
  //
  typedef void (APIENTRYP TPushAttrib)(GLbitfield mask);

  //----------------------------------------------------------------
  // TPopAttrib
  //
  typedef void (APIENTRYP TPopAttrib)(void);

  //----------------------------------------------------------------
  // TPushClientAttrib
  //
  typedef void (APIENTRYP TPushClientAttrib)(GLbitfield mask);

  //----------------------------------------------------------------
  // TPopClientAttrib
  //
  typedef void (APIENTRYP TPopClientAttrib)(void);

  //----------------------------------------------------------------
  // TEnable
  //
  typedef void (APIENTRYP TEnable)(GLenum cap);

  //----------------------------------------------------------------
  // TDisable
  //
  typedef void (APIENTRYP TDisable)(GLenum cap);

  //----------------------------------------------------------------
  // TBlendFunc
  //
  typedef void (APIENTRYP TBlendFunc)(GLenum sfactor,
                                      GLenum dfactor);

  //----------------------------------------------------------------
  // THint
  //
  typedef void (APIENTRYP THint)(GLenum target,
                                 GLenum mode);

  //----------------------------------------------------------------
  // TLineStipple
  //
  typedef void (APIENTRYP TLineStipple)(GLint factor,
                                        GLushort pattern);

  //----------------------------------------------------------------
  // TLineWidth
  //
  typedef void (APIENTRYP TLineWidth)(GLfloat width);

  //----------------------------------------------------------------
  // TScissor
  //
  typedef void (APIENTRYP TScissor)(GLint   x,
                                    GLint   y,
                                    GLsizei width,
                                    GLsizei height);

  //----------------------------------------------------------------
  // TBindBuffer
  //
  typedef void (APIENTRYP TBindBuffer)(GLenum target,
                                       GLuint buffer);

  //----------------------------------------------------------------
  // TCheckFramebufferStatus
  //
  typedef GLenum (APIENTRYP TCheckFramebufferStatus)(GLenum target);

  //----------------------------------------------------------------
  // TDeleteTextures
  //
  typedef GLenum (APIENTRYP TDeleteTextures)(GLsizei n,
                                             const GLuint * textures);

  //----------------------------------------------------------------
  // TGenTextures
  //
  typedef GLenum (APIENTRYP TGenTextures)(GLsizei n,
                                          GLuint * textures);

  //----------------------------------------------------------------
  // TGetError
  //
  typedef GLenum (APIENTRYP TGetError)(void);

  //----------------------------------------------------------------
  // TGetString
  //
  typedef const GLubyte * (APIENTRYP TGetString)(GLenum name);

  //----------------------------------------------------------------
  // TGetIntegerv
  //
  typedef GLenum (APIENTRYP TGetIntegerv)(GLenum pname,
                                          GLint * params);

  //----------------------------------------------------------------
  // TGetTexLevelParameteriv
  //
  typedef void (APIENTRYP TGetTexLevelParameteriv)(GLenum target,
                                                   GLint  level,
                                                   GLenum pname,
                                                   GLint * params);

  //----------------------------------------------------------------
  // TIsTexture
  //
  typedef GLboolean (APIENTRYP TIsTexture)(GLuint texture);

  //----------------------------------------------------------------
  // TTexEnvi
  //
  typedef void (APIENTRYP TTexEnvi)(GLenum target,
                                    GLenum pname,
                                    GLint  param);

  //----------------------------------------------------------------
  // TTexCoord2d
  //
  typedef void (APIENTRYP TTexCoord2d)(GLdouble s,
                                       GLdouble t);

  //----------------------------------------------------------------
  // TTexCoord2i
  //
  typedef void (APIENTRYP TTexCoord2i)(GLint s,
                                       GLint t);

  //----------------------------------------------------------------
  // TTexImage2D
  //
  typedef void (APIENTRYP TTexImage2D)(GLenum  target,
                                       GLint   level,
                                       GLint   internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint   border,
                                       GLint   format,
                                       GLenum  type,
                                       const GLvoid * data);

  //----------------------------------------------------------------
  // TTexImage3D
  //
  typedef void (APIENTRYP TTexImage3D)(GLenum  target,
                                       GLint   level,
                                       GLint   internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLint   border,
                                       GLenum  format,
                                       GLenum  type,
                                       const void * data);

  //----------------------------------------------------------------
  // TTexSubImage2D
  //
  typedef void (APIENTRYP TTexSubImage2D)(GLenum  target,
                                          GLint   level,
                                          GLint   xoffset,
                                          GLint   yoffset,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum  format,
                                          GLenum  type,
                                          const GLvoid * data);

  //----------------------------------------------------------------
  // TTexParameteri
  //
  typedef void (APIENTRYP TTexParameteri)(GLenum target,
                                          GLenum pname,
                                          GLint  param);

  //----------------------------------------------------------------
  // TPixelStorei
  //
  typedef void (APIENTRYP TPixelStorei)(GLenum pname,
                                        GLint  param);

  //----------------------------------------------------------------
  // TActiveTexture
  //
  typedef void (APIENTRYP TActiveTexture)(GLenum texture);

  //----------------------------------------------------------------
  // TBindTexture
  //
  typedef void (APIENTRYP TBindTexture)(GLenum target,
                                        GLuint texture);

  //----------------------------------------------------------------
  // TDisableVertexAttribArray
  //
  typedef void (APIENTRYP TDisableVertexAttribArray)(GLuint index);

  //----------------------------------------------------------------
  // TVertexAttribPointer
  //
  typedef void (APIENTRYP TVertexAttribPointer)(GLuint index,
                                                GLint size,
                                                GLenum type,
                                                GLboolean normalized,
                                                GLsizei stride,
                                                const void * pointer);

  //----------------------------------------------------------------
  // TUseProgram
  //
  typedef void (APIENTRYP TUseProgram)(GLuint program);

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
  // TProgramLocalParameter4dARB
  //
  typedef void (APIENTRYP TProgramLocalParameter4dARB)(GLenum target,
                                                       GLuint index,
                                                       GLdouble x,
                                                       GLdouble y,
                                                       GLdouble z,
                                                       GLdouble w);

  //----------------------------------------------------------------
  // OpenGLFunctionPointers
  //
  struct YAEUI_API OpenGLFunctionPointers // : public QOpenGLFunctions
  {
    TDebugMessageCallback glDebugMessageCallback;

    TBegin glBegin;
    TEnd glEnd;

    TBegin _glBegin;
    TEnd _glEnd;

    TClear glClear;
    TClearAccum glClearAccum;
    TClearColor glClearColor;
    TClearDepth glClearDepth;
    TClearStencil glClearStencil;

    TDepthFunc glDepthFunc;
    TDepthMask glDepthMask;
    TColorMask glColorMask;

    TStencilFunc glStencilFunc;
    TStencilMask glStencilMask;
    TStencilOp glStencilOp;

    TColor3d glColor3d;
    TColor3f glColor3f;
    TColor3fv glColor3fv;
    TColor4d glColor4d;
    TColor4ub glColor4ub;

    TVertex2d glVertex2d;
    TVertex2dv glVertex2dv;
    TVertex2i glVertex2i;

    TRecti glRecti;
    TRectd glRectd;

    TMatrixMode glMatrixMode;
    TPushMatrix glPushMatrix;
    TPopMatrix glPopMatrix;

    TViewport glViewport;
    TOrtho glOrtho;

    TLoadIdentity glLoadIdentity;
    TRotated glRotated;
    TScaled glScaled;
    TTranslated glTranslated;

    TPolygonMode glPolygonMode;
    TShadeModel glShadeModel;
    TAlphaFunc glAlphaFunc;

    TPushAttrib glPushAttrib;
    TPopAttrib glPopAttrib;

    TPushClientAttrib glPushClientAttrib;
    TPopClientAttrib glPopClientAttrib;

    TEnable glEnable;
    TDisable glDisable;

    THint glHint;
    TBlendFunc glBlendFunc;
    TLineStipple glLineStipple;
    TLineWidth glLineWidth;
    TScissor glScissor;

    TBindBuffer glBindBuffer;
    TCheckFramebufferStatus glCheckFramebufferStatus;
    TDeleteTextures glDeleteTextures;
    TGenTextures glGenTextures;

    TGetError glGetError;
    TGetString glGetString;
    TGetIntegerv glGetIntegerv;
    TGetTexLevelParameteriv glGetTexLevelParameteriv;
    TIsTexture glIsTexture;

    TTexEnvi glTexEnvi;
    TTexCoord2d glTexCoord2d;
    TTexCoord2i glTexCoord2i;
    TTexImage2D glTexImage2D;
    TTexImage3D glTexImage3D;
    TTexSubImage2D glTexSubImage2D;

    TTexParameteri glTexParameteri;

    TPixelStorei glPixelStorei;

    TActiveTexture glActiveTexture;
    TBindTexture glBindTexture;

    TDisableVertexAttribArray glDisableVertexAttribArray;
    TVertexAttribPointer glVertexAttribPointer;

    TUseProgram glUseProgram;

    TProgramStringARB glProgramStringARB;
    TGetProgramivARB glGetProgramivARB;
    TDeleteProgramsARB glDeleteProgramsARB;
    TBindProgramARB glBindProgramARB;
    TGenProgramsARB glGenProgramsARB;
    TProgramLocalParameter4dvARB glProgramLocalParameter4dvARB;
    TProgramLocalParameter4dARB glProgramLocalParameter4dARB;

    OpenGLFunctionPointers();

    static OpenGLFunctionPointers & get();
  };
#endif
}

namespace yae
{
#ifndef YAE_USE_QGL_WIDGET

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

    YAE_ASSERT(context->isValid());
    return *context;
  }

  //----------------------------------------------------------------
  // ogl_11
  //
  inline static QOpenGLFunctions_1_1 & ogl_11()
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_1>());
#else
    QOpenGLContext & ctx = ogl_context();
    return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_1_1>(&ctx);
#endif
  }

  //----------------------------------------------------------------
  // ogl_12
  //
  inline static QOpenGLFunctions_1_2 & ogl_12()
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_2>());
#else
    QOpenGLContext & ctx = ogl_context();
    return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_1_2>(&ctx);
#endif
  }

  //----------------------------------------------------------------
  // ogl_13
  //
  inline static QOpenGLFunctions_1_3 & ogl_13()
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_3>());
#else
    QOpenGLContext & ctx = ogl_context();
    return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_1_3>(&ctx);
#endif
  }

  //----------------------------------------------------------------
  // ogl_14
  //
  inline static QOpenGLFunctions_1_4 & ogl_14()
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return *(ogl_context().versionFunctions<QOpenGLFunctions_1_4>());
#else
    QOpenGLContext & ctx = ogl_context();
    return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_1_4>(&ctx);
#endif
  }

  //----------------------------------------------------------------
  // ogl_20
  //
  inline static QOpenGLFunctions_2_0 & ogl_20()
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return *(ogl_context().versionFunctions<QOpenGLFunctions_2_0>());
#else
    QOpenGLContext & ctx = ogl_context();
    return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_2_0>(&ctx);
#endif
  }

#define YAE_OGL_11_HERE() \
  yaegl::OpenGLFunctionPointers & ogl_11 = yaegl::OpenGLFunctionPointers::get()

#define YAE_OGL_12_HERE() \
  yaegl::OpenGLFunctionPointers & ogl_12 = yaegl::OpenGLFunctionPointers::get()

#define YAE_OGL_13_HERE() \
  yaegl::OpenGLFunctionPointers & ogl_13 = yaegl::OpenGLFunctionPointers::get()

#define YAE_OGL_14_HERE() \
  yaegl::OpenGLFunctionPointers & ogl_14 = yaegl::OpenGLFunctionPointers::get()

#define YAE_OGL_20_HERE() \
  yaegl::OpenGLFunctionPointers & ogl_20 = yaegl::OpenGLFunctionPointers::get()

#define YAE_OPENGL_HERE() \
  yaegl::OpenGLFunctionPointers & opengl = yaegl::OpenGLFunctionPointers::get()


#define YAE_OGL_11(x) ogl_11.x
#define YAE_OGL_12(x) ogl_12.x
#define YAE_OGL_13(x) ogl_13.x
#define YAE_OGL_14(x) ogl_14.x
#define YAE_OGL_20(x) ogl_20.x
#define YAE_OPENGL(x) opengl.x
#define YAE_OGL_FN(x) true

#else
#define YAE_OGL_11_HERE()
#define YAE_OGL_12_HERE()
#define YAE_OGL_13_HERE()
#define YAE_OGL_14_HERE()
#define YAE_OGL_20_HERE()
#define YAE_OPENGL_HERE()

#define YAE_OGL_11(x) x
#define YAE_OGL_12(x) x
#define YAE_OGL_13(x) x
#define YAE_OGL_14(x) x
#define YAE_OGL_20(x) x
#define YAE_OPENGL(x) x
#define YAE_OGL_FN(x) x

#endif

  //----------------------------------------------------------------
  // IOpenGLContext
  //
  struct YAEUI_API IOpenGLContext
  {
    IOpenGLContext() {}
    virtual ~IOpenGLContext() {}

    inline bool lock()
    {
      mutex_.lock();
      restore_.push_back(getCurrent());
      YAE_ASSERT(this->makeCurrent());
#if 0
      YAE_OPENGL_HERE();
      if (opengl.glDebugMessageCallback)
      {
        opengl.glDebugMessageCallback(yae_opengl_debug_message_cb, NULL);
        opengl.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      }
#endif
      return true;
    }

    inline void unlock()
    {
      YAE_ASSERT(!restore_.empty());

      yae::shared_ptr<ICurrentContext> prev = restore_.back();
      restore_.pop_back();

      if (!prev->restore())
      {
        this->doneCurrent();
      }

      mutex_.unlock();
    }

    //----------------------------------------------------------------
    // ICurrentContext
    //
    struct ICurrentContext
    {
      virtual ~ICurrentContext() {}
      virtual bool restore() = 0;
    };

    virtual yae::shared_ptr<ICurrentContext> getCurrent() const = 0;
    virtual bool makeCurrent() = 0;
    virtual void doneCurrent() = 0;

  private:
    boost::recursive_mutex mutex_;
    std::list<yae::shared_ptr<ICurrentContext> > restore_;
  };

  //----------------------------------------------------------------
  // TMakeCurrentContext
  //
  struct YAEUI_API TMakeCurrentContext
  {
    TMakeCurrentContext(IOpenGLContext * context):
      context_(context)
    {
      maybe_lock();
    }

    TMakeCurrentContext(IOpenGLContext & context):
      context_(&context)
    {
      maybe_lock();
    }

    ~TMakeCurrentContext()
    {
      maybe_unlock();
    }

    inline void maybe_lock()
    {
      if (context_)
      {
        context_->lock();
      }
    }

    inline void maybe_unlock()
    {
      if (context_)
      {
        context_->unlock();
      }
    }

    IOpenGLContext * context_;
  };

  //----------------------------------------------------------------
  // TGLSaveState
  //
  struct YAEUI_API TGLSaveState
  {
    TGLSaveState(GLbitfield mask);
    ~TGLSaveState();

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveClientState
  //
  struct YAEUI_API TGLSaveClientState
  {
    TGLSaveClientState(GLbitfield mask);
    ~TGLSaveClientState();

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveMatrixState
  //
  struct YAEUI_API TGLSaveMatrixState
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
  YAEUI_API int
  alignmentFor(const unsigned char * data, std::size_t rowBytes);


  //----------------------------------------------------------------
  // TFragmentShaderProgram
  //
  struct YAEUI_API TFragmentShaderProgram
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
  // ShaderPrograms
  //
  struct YAEUI_API ShaderPrograms
  {
    ~ShaderPrograms();

    bool createBuiltinShaderProgram(const char * code);
    bool createShaderProgramsFor(const TPixelFormatId * formats,
                                 const std::size_t numFormats,
                                 const char * code);

    TFragmentShaderProgram builtin_;
    std::list<TFragmentShaderProgram> programs_;
    std::map<TPixelFormatId, const TFragmentShaderProgram *> lut_;
  };


  //----------------------------------------------------------------
  // TFragmentShader
  //
  struct YAEUI_API TFragmentShader
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
  // get_max_texture_2d
  //
  YAEUI_API GLsizei get_max_texture_2d();

  //----------------------------------------------------------------
  // get_supports_texture_rectangle
  //
  YAEUI_API bool get_supports_texture_rectangle();

  //----------------------------------------------------------------
  // get_supports_luminance16
  //
  YAEUI_API bool get_supports_luminance16();


  //----------------------------------------------------------------
  // TBaseCanvas
  //
  struct YAEUI_API TBaseCanvas
  {
    TBaseCanvas(const ShaderPrograms & shaders);
    virtual ~TBaseCanvas();

    virtual void clear(IOpenGLContext & context) = 0;

    virtual bool loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame) = 0;

    virtual void draw(double opacity) const = 0;

    // helper:
    const pixelFormat::Traits * pixelTraits() const;

    void skipColorConverter(IOpenGLContext & context, bool enable);

    inline bool skipColorConverter() const
    { return skipColorConverter_; }

    void enableVerticalScaling(bool enable);

    bool getCroppedFrame(TCropFrame & crop) const;

    bool imageWidthHeight(double & w, double & h) const;
    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const;

    double nativeAspectRatioUncropped() const;
    double nativeAspectRatioUncroppedRotated(int & rotate) const;

    double nativeAspectRatio() const;
    double nativeAspectRatioRotated(int & rotate) const;

    void overrideDisplayAspectRatio(double dar);

    inline double overrideDisplayAspectRatio() const
    { return dar_; }

    double displayAspectRatioFor(int cameraRotation) const;

    void cropFrame(double darCropped);
    void cropFrame(const TCropFrame & crop);

    void getFrame(TVideoFramePtr & frame) const;

    // helper:
    const TFragmentShader *
    fragmentShaderFor(TPixelFormatId format) const;

    // helper:
    void paintImage(double x,
                    double y,
                    double w_max,
                    double h_max,
                    double opacity = 1.0) const;

  protected:
    // helper:
    const TFragmentShader *
    findSomeShaderFor(TPixelFormatId format) const;

    // helper:
    bool setFrame(const TVideoFramePtr & frame,
                  bool & colorSpaceOrRangeChanged);

    // helper:
    void clearFrame();

    mutable boost::mutex mutex_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    double dar_;
    double darCropped_;
    bool skipColorConverter_;
    bool verticalScalingEnabled_;

    TFragmentShader builtinShader_;
    std::map<TPixelFormatId, TFragmentShader> shaders_;

    // shader selected for current frame:
    const TFragmentShader * shader_;

    // a 3D LUT to transform any input to BT.709 R'G'B':
    VideoTraits clut_input_;
    TColorTransform3u8 clut_;
    GLuint clut_tex_id_;
  };

  //----------------------------------------------------------------
  // TModernCanvas
  //
  struct YAEUI_API TModernCanvas : public TBaseCanvas
  {
    TModernCanvas();

    // singleton:
    static const ShaderPrograms & shaders();

    // virtual:
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

    // virtual:
    void draw(double opacity) const;

  protected:

    // 2D and Rect textures:
    std::vector<GLuint> texId_;
  };


  //----------------------------------------------------------------
  // TEdge
  //
  struct YAEUI_API TEdge
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
  struct YAEUI_API TFrameTile
  {
    TEdge x_;
    TEdge y_;
  };

  //----------------------------------------------------------------
  // calculateEdges
  //
  YAEUI_API void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgeSize,
                 GLsizei textureEdgeMax,
                 bool flip);

  //----------------------------------------------------------------
  // TLegacyCanvas
  //
  // This is a subclass implementing frame rendering on OpenGL
  // hardware that doesn't support GL_EXT_texture_rectangle
  //
  struct YAEUI_API TLegacyCanvas : public TBaseCanvas
  {
    TLegacyCanvas();

    // singleton:
    static const ShaderPrograms & shaders();

    // virtual:
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

    // virtual:
    void draw(double opacity) const;

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
  class YAEUI_API CanvasRenderer
  {
    TLegacyCanvas * legacy_;
    TModernCanvas * modern_;
    TBaseCanvas * renderer_;

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

    double nativeAspectRatioUncropped() const;
    double nativeAspectRatioUncroppedRotated(int & rotate) const;

    double nativeAspectRatio() const;
    double nativeAspectRatioRotated(int & rotate) const;

    void overrideDisplayAspectRatio(double dar);

    inline double overrideDisplayAspectRatio() const
    { return legacy_->overrideDisplayAspectRatio(); }

    void cropFrame(double darCropped);
    void cropFrame(const TCropFrame & crop);

    void getFrame(TVideoFramePtr & frame) const;

    const TFragmentShader *
    fragmentShaderFor(const VideoTraits & vtts) const;

    // helper:
    inline void paintImage(double x,
                           double y,
                           double w_max,
                           double h_max,
                           double opacity = 1.0) const

    { renderer_->paintImage(x, y, w_max, h_max, opacity); }

    // helper, selects a renderer based on image size,
    // then calls adjust_pixel_format_for_opengl:
    bool adjustPixelFormatForOpenGL(bool skipColorConverter,
                                    const VideoTraits & vtts,
                                    TPixelFormatId & output) const;
  };

  //----------------------------------------------------------------
  // adjust_pixel_format_for_opengl
  //
  YAEUI_API bool
  adjust_pixel_format_for_opengl(const TBaseCanvas * canvas,
                                 bool skipColorConverter,
                                 TPixelFormatId nativeFormat,
                                 TPixelFormatId & adjustedFormat);
}

namespace yaegl
{

  //----------------------------------------------------------------
  // BeginEnd
  //
  struct BeginEnd
  {
    BeginEnd(GLenum mode)
    {
      YAE_OGL_11_HERE();
      YAE_OGL_11(glBegin(mode));
    }

    ~BeginEnd()
    {
      YAE_OGL_11_HERE();
      YAE_OGL_11(glEnd());
    }
  };

}


#endif // YAE_CANVAS_RENDERER_H_

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>

// the includes:
#include <image/image_tile_generator.hxx>
#include <opengl/image_tile_dl_elem.hxx>
#include <opengl/glsl.hxx>
#include <utils/the_utils.hxx>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>


namespace yae
{

  //----------------------------------------------------------------
  // Canvas::Canvas
  // 
  Canvas::Canvas(QWidget * parent,
                 const QGLWidget * shareWidget,
                 Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f)
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
    
    glShadeModel(GL_SMOOTH);
    glClearDepth(0);
    glClearStencil(0);
    glClearAccum(0, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
  }

  //----------------------------------------------------------------
  // Canvas::resizeGL
  // 
  void
  Canvas::resizeGL(int width, int height)
  {
    std::cerr << "resizeGL: " << width << ", " << height << std::endl;
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
    
    // image_tile_dl_elem_t needs access to current OpenGL context:
    the_scoped_variable_t<the_gl_context_interface_t *>
      active_context(the_gl_context_interface_t::current_, this, NULL);
    
    glViewport(0, 0, int(width()), int(height()));
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(-width(), width(), -height(), height());
    glClear(GL_COLOR_BUFFER_BIT);
    
    // make a local copy of the auto pointer to avoid a race condition:
    TImagePtr image = image_;
    if (image.get())
    {
      image->draw();
    }
  }
  
  //----------------------------------------------------------------
  // Canvas::loadFrame
  // 
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
    std::size_t imgW = frame->traits_.visibleWidth_;
    std::size_t imgH = frame->traits_.visibleHeight_;
    const unsigned char * dataBuffer = frame->getBuffer<unsigned char>();
    unsigned int dataBufferAlignment = 1;
    
    // gather some stats about the image:
    size_t bytes_per_pixel = getBitsPerPixel(frame->traits_.colorFormat_) / 8;
    bool has_alpha = hasAlphaChannel(frame->traits_.colorFormat_);
    image_tile_generator_t tile_generator;
    tile_generator.layout(imgW, imgH);
    
    tile_generator.convert_and_pad(dataBuffer,
                                   dataBufferAlignment,
                                   bytes_per_pixel,
                                   bytes_per_pixel,
                                   copy_pixels_t());
    
    GLenum data_type = GL_UNSIGNED_BYTE;
    GLenum format_internal = GL_RGB8;
    GLenum format = GL_RGB;
    GLenum filter = GL_LINEAR; // GL_NEAREST;
    const size_t max_texture = 1024;
    
    tile_generator.make_tiles(data_type,
                              format_internal,
                              format,
                              max_texture);
    
    TImagePtr image(new image_tile_dl_elem_t(tile_generator,
                                             filter,
                                             filter));
    image_ = image;
    refresh();
    
    return true;
  }
}

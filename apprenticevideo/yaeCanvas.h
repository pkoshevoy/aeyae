// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_H_
#define YAE_CANVAS_H_

// boost includes:
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

// the includes:
#include <opengl/the_gl_context.hxx>
#include <opengl/image_tile_dl_elem.hxx>

// Qt includes:
#include <QEvent>
#include <QGLWidget>

// yae includes:
#include <yaeAPI.h>
#include <yaeVideoCanvas.h>


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
              GLenum & dataType);

namespace yae
{

  //----------------------------------------------------------------
  // Canvas
  // 
  class Canvas : public QGLWidget,
                 public IVideoCanvas,
                 public the_gl_context_interface_t
  {
    Q_OBJECT;
    
  public:
    Canvas(const QGLFormat & format,
           QWidget * parent = 0,
           const QGLWidget * shareWidget = 0,
           Qt::WindowFlags f = 0);
    ~Canvas();
    
    // virtual:
    bool gl_context_is_valid() const;
    void gl_make_current();
    void gl_done_current();
    
    // helper:
    void refresh();
    
    // virtual:
    bool render(const TVideoFramePtr & frame);
    
    bool loadFrame(const TVideoFramePtr & frame);
    
  protected:
    // virtual:
    bool event(QEvent * event);
    
    // virtual: Qt/OpenGL stuff:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    
    //----------------------------------------------------------------
    // RenderFrameEvent
    // 
    struct RenderFrameEvent : public QEvent
    {
      //----------------------------------------------------------------
      // TPayload
      // 
      struct TPayload
      {
        bool set(const TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          bool postThePayload = !frame_;
          frame_ = frame;
          return postThePayload;
        }
        
        void get(TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          frame = frame_;
          frame_ = TVideoFramePtr();
        }
        
      private:
        mutable boost::mutex mutex_;
        TVideoFramePtr frame_;
      };
      
      RenderFrameEvent(TPayload & payload):
        QEvent(QEvent::User),
        payload_(payload)
      {}
      
      TPayload & payload_;
    };
    
    RenderFrameEvent::TPayload payload_;

    class TPrivate;
    TPrivate * private_;
    
    //----------------------------------------------------------------
    // TImagePtr
    // 
    typedef boost::shared_ptr<image_tile_dl_elem_t> TImagePtr;
    TImagePtr image_;
  };
}


#endif // YAE_CANVAS_H_

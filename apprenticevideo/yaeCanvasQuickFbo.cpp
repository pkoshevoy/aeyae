// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 26 12:51:43 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ includes:
#include <list>

// Qt includes:
#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQuickFramebufferObject>
#include <QQuickWindow>
#include <QSurface>
#include <QThreadStorage>

// apprenticevideo includes:
#include <yaeCanvas.h>
#include <yaeScreenSaverInhibitor.h>

// local includes:
#include "yaeCanvasQuickFbo.h"


namespace yae
{
  //----------------------------------------------------------------
  // OpenGLContext
  //
  struct OpenGLContext : public IOpenGLContext
  {
    OpenGLContext():
      mutex_(QMutex::Recursive),
      initialized_(false),
      prev_(NULL)
    {
      surface_.create();
    }

    ~OpenGLContext()
    {}

    virtual bool makeCurrent()
    {
      mutex_.lock();

      prev_.push_back(QOpenGLContext::currentContext());

      if (initialized_ || initialize())
      {
        boost::shared_ptr<QOpenGLContext> context = tss_.localData();
        if (!context)
        {
          context.reset(new QOpenGLContext);
          context->setShareContext(context_.get());
          if (context->create())
          {
            tss_.setLocalData(context);
          }
          else
          {
            context.reset();
          }
        }

        if (context && context->makeCurrent(&surface_))
        {
          return true;
        }
      }

      // this shouldn't happen:
      prev_.pop_back();
      mutex_.unlock();
      return false;
    }

    virtual void doneCurrent()
    {
      boost::shared_ptr<QOpenGLContext> context = tss_.localData();
      YAE_ASSERT(context);
      context->doneCurrent();

      if (!prev_.empty())
      {
        QOpenGLContext * restore = prev_.back();
        prev_.pop_back();

        if (restore)
        {
          restore->makeCurrent(&surface_);
        }
      }

      mutex_.unlock();
    }

    bool initialize()
    {
      if (!initialized_)
      {
        QOpenGLContext * context = QOpenGLContext::currentContext();
        if (context)
        {
          context_.reset(new QOpenGLContext);
          context_->setShareContext(context);
          if (context_->create())
          {
            tss_.setLocalData(context_);
            // surface_ = context->surface();
            initialized_ = true;
          }
          else
          {
            context_.reset();
          }
        }
      }

      YAE_ASSERT(initialized_);
      return initialized_;
    }

    mutable QMutex mutex_;
    boost::shared_ptr<QOpenGLContext> context_;
    QThreadStorage<boost::shared_ptr<QOpenGLContext> > tss_;
    QOffscreenSurface surface_;
    bool initialized_;
    std::list<QOpenGLContext *> prev_;
  };

  //----------------------------------------------------------------
  // CanvasQuickFboRenderer
  //
  struct CanvasQuickFboRenderer : public QQuickFramebufferObject::Renderer
  {
    friend struct TDelegate;

    //----------------------------------------------------------------
    // CanvasQuickFboRenderer::TDelegate
    //
    struct TDelegate : public Canvas::IDelegate
    {
      TDelegate(CanvasQuickFboRenderer & renderer):
        renderer_(renderer)
      {}

      virtual bool isVisible()
      {
        QOpenGLFramebufferObject * fbo = renderer_.framebufferObject();
        return true != NULL;
      }

      virtual void requestRepaint()
      {
        renderer_.update();
        // renderer_.fbo_.update();
        // QQuickItem::Flags f = renderer_.fbo_.flags();
        // YAE_ASSERT(f & QQuickItem::ItemHasContents);

        // renderer_.fbo_.window()->update();
      }

      virtual void inhibitScreenSaver()
      {
        ssi_.screenSaverInhibit();
      }

    protected:
      CanvasQuickFboRenderer & renderer_;
      ScreenSaverInhibitor ssi_;
    };


    CanvasQuickFboRenderer(CanvasQuickFbo & fbo):
      fbo_(fbo),
      delegate_(new TDelegate(*this))
    {
      fbo_.canvas_.setDelegate(delegate_);
    };

    // virtual:
    void render()
    {
      QOpenGLFramebufferObject * fbo = this->framebufferObject();
      if (!fbo)
      {
        return;
      }

      QOpenGLContext * context = QOpenGLContext::currentContext();
      if (!context)
      {
        return;
      }

      QSize canvasSize = fbo->size();
      int w = canvasSize.width();
      int h = canvasSize.height();

      fbo_.canvas_.resize(w, h);
      fbo_.canvas_.paintCanvas();
    }

    // virtual:
    QOpenGLFramebufferObject *
    createFramebufferObject(const QSize & size)
    {
      QOpenGLFramebufferObjectFormat format;
      format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
      format.setSamples(0);

      return new QOpenGLFramebufferObject(size, format);
    }

    CanvasQuickFbo & fbo_;
    boost::shared_ptr<TDelegate> delegate_;
  };


  //----------------------------------------------------------------
  // CanvasQuickFbo::CanvasQuickFbo
  //
  CanvasQuickFbo::CanvasQuickFbo():
    canvas_(boost::shared_ptr<OpenGLContext>(new OpenGLContext))
  {
#if 1
    QString greeting =
      QObject::tr("drop videos/music here\n\n"
                  "press spacebar to pause/resume\n\n"
                  "alt-left/alt-right to navigate playlist\n\n"
#ifdef __APPLE__
                  "use apple remote for volume and seeking\n\n"
#endif
                  "explore the menus for more options");

    canvas_.setGreeting(greeting);
#endif
  }

  //----------------------------------------------------------------
  // CanvasQuickFbo::createRenderer
  //
  QQuickFramebufferObject::Renderer *
  CanvasQuickFbo::createRenderer() const
  {
    return new CanvasQuickFboRenderer(const_cast<CanvasQuickFbo &>(*this));
  }

}

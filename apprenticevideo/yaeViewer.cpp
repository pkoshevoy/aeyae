// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Jul  5 19:33:44 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeViewer.h>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>
#include <QGridLayout>


namespace yae
{

  //----------------------------------------------------------------
  // Viewer::Viewer
  // 
  Viewer::Viewer()
  {
    setObjectName("yae::Viewer");
    setFocusPolicy(Qt::StrongFocus);
    
    QGridLayout * grid = new QGridLayout(this);
    grid->setSpacing(0);
    grid->setContentsMargins(0, 0, 0, 0);
    
    labelRGB_ = new QLabel(this);
    grid->addWidget(labelRGB_, 0, 0);
    labelRGB_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    labelRGB_->show();
  }

  //----------------------------------------------------------------
  // Viewer::~Viewer
  // 
  Viewer::~Viewer()
  {}
  
  //----------------------------------------------------------------
  // Viewer::render
  // 
  bool
  Viewer::render(const TVideoFramePtr & frame)
  {
    bool replacedPayloadPriorToDelivery = payload_.set(frame);
    if (!replacedPayloadPriorToDelivery)
    {
      // send an event:
      qApp->postEvent(this, new RenderFrameEvent(payload_));
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // Viewer::setReader
  // 
  void
  Viewer::setReader(IReader * reader)
  {
    VideoTraits traits;
    if (reader)
    {
      reader->getVideoTraitsOverride(traits);
    }
    
    std::size_t imgW = traits.visibleWidth_;
    std::size_t imgH = traits.visibleHeight_;
    
    setMinimumSize(imgW, imgH);
    labelRGB_->setMinimumSize(imgW, imgH);
  }
  
  //----------------------------------------------------------------
  // Viewer::event
  // 
  bool
  Viewer::event(QEvent * event)
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
    
    event->ignore();
    return QWidget::event(event);
  }

  //----------------------------------------------------------------
  // Viewer::loadFrame
  // 
  bool
  Viewer::loadFrame(const TVideoFramePtr & frame)
  {
    std::size_t imgW = frame->traits_.visibleWidth_;
    std::size_t imgH = frame->traits_.visibleHeight_;
    const unsigned char * dataBuffer = frame->getBuffer<unsigned char>();
    
    if (frame->traits_.colorFormat_ == kColorFormatRGB)
    {
      QImage rgb(dataBuffer,
                 imgW,
                 imgH,
                 frame->traits_.encodedWidth_ * 3,
                 QImage::Format_RGB888);
      labelRGB_->setPixmap(QPixmap::fromImage(rgb));
    }
    else if (frame->traits_.colorFormat_ == kColorFormatARGB)
    {
      QImage rgb(dataBuffer,
                 imgW,
                 imgH,
                 frame->traits_.encodedWidth_ * 4,
                 QImage::Format_ARGB32);
      labelRGB_->setPixmap(QPixmap::fromImage(rgb));
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // Viewer::keyPressEvent
  // 
  void
  Viewer::keyPressEvent(QKeyEvent * event)
  {
    const int k = event->key();
    switch (k)
    {
      case Qt::Key_PageUp:
      case Qt::Key_Up:
      case Qt::Key_Left:
        event->accept();
        return;
        
      case Qt::Key_PageDown:
      case Qt::Key_Down:
      case Qt::Key_Right:
        event->accept();
        return;
        
      default:
        break;
    }
    
    event->ignore();
    QWidget::keyPressEvent(event);
  }
  
}

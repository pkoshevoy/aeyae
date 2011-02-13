// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Jul  5 19:22:58 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIEWER_H_
#define YAE_VIEWER_H_

// system includes:
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QEvent>
#include <QImage>
#include <QLabel>
#include <QWidget>
#include <QKeyEvent>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeVideoCanvas.h>


namespace yae
{

  //----------------------------------------------------------------
  // Viewer
  // 
  // Simple UI used for debugging Readers
  // 
  class Viewer : public QWidget,
                 public IVideoCanvas
  {
    Q_OBJECT;
    
  public:
    Viewer();
    ~Viewer();
    
    // virtual:
    bool render(const TVideoFramePtr & frame);
    
    void setReader(IReader * reader);
    
    bool loadFrame(const TVideoFramePtr & frame);
    
  protected:
    // virtual:
    bool event(QEvent * event);
    void keyPressEvent(QKeyEvent * event);
    
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
          bool replacedPayloadPriorToDelivery = frame_ != NULL;
          frame_ = frame;
          return replacedPayloadPriorToDelivery;
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
    
    QLabel * labelRGB_;
    RenderFrameEvent::TPayload payload_;
  };
  
}

#endif // YAE_VIEWER_H_

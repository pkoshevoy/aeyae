// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 13:10:10 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include <yaeVideoRenderer.h>
#include <yaeThreading.h>

// system includes:
#include <algorithm>


namespace yae
{

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate
  // 
  class VideoRenderer::TPrivate
  {
  public:
    TPrivate(SharedClock & clock);
    
    bool open(IVideoCanvas * canvas, IReader * reader);
    void close();
    void threadLoop();
    
    mutable boost::mutex mutex_;
    Thread<TPrivate> thread_;
    
    SharedClock & clock_;
    IVideoCanvas * canvas_;
    IReader * reader_;
  };
  
  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::TPrivate
  // 
  VideoRenderer::TPrivate::TPrivate(SharedClock & clock):
    thread_(this),
    clock_(clock),
    canvas_(NULL),
    reader_(NULL)
  {}

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::open
  // 
  bool
  VideoRenderer::TPrivate::open(IVideoCanvas * canvas, IReader * reader)
  {
    close();
    
    boost::lock_guard<boost::mutex> lock(mutex_);
    canvas_ = canvas;
    reader_ = reader;
    if (!reader_)
    {
      return true;
    }
    
    return thread_.run();
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::close
  // 
  void
  VideoRenderer::TPrivate::close()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    thread_.stop();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::threadLoop
  // 
  void
  VideoRenderer::TPrivate::threadLoop()
  {
    TTime t0;
    TTime dt;
    
    TTime framePosition;
    double frameDuration = 0.0;
    
    while (true)
    {
      boost::this_thread::interruption_point();
      
      // get the time segment we are supposed to render for,
      // and the current time relative to the time segment:
      double relativePlayheadPosition = clock_.getCurrentTime(t0, dt);
      double segmentPosition = double(t0.time_) / double(t0.base_);
      double segmentDuration = double(dt.time_) / double(dt.base_);
      double playheadPosition = (segmentDuration * relativePlayheadPosition +
                                 segmentPosition);
      
      // get position of the frame relative to the current time segment:
      double f0 = double(framePosition.time_) / double(framePosition.base_);
      double f1 = f0 + frameDuration;
      double df = f1 - playheadPosition;
      
      if (df > 0.0)
      {
        // wait until the next frame is required:
        boost::this_thread::sleep(boost::posix_time::milliseconds
                                  (long(df * 1000.0)));
        continue;
      }
      
      // read a frame:
      TVideoFramePtr frame;
      if (!reader_->readVideo(frame))
      {
        break;
      }
      
      framePosition = frame->time_;
      frameDuration = 
        frame->traits_.frameRate_ ?
        1.0 / frame->traits_.frameRate_ :
        1.0 / double(framePosition.base_);
      
      // dispatch the frame to the canvas for rendering:
      if (canvas_)
      {
        canvas_->render(frame);
      }
    }
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::VideoRenderer
  // 
  VideoRenderer::VideoRenderer():
    private_(new TPrivate(ISynchronous::clock_))
  {}

  //----------------------------------------------------------------
  // VideoRenderer::~VideoRenderer
  // 
  VideoRenderer::~VideoRenderer()
  {
    delete private_;
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::create
  // 
  VideoRenderer *
  VideoRenderer::create()
  {
    return new VideoRenderer();
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::destroy
  // 
  void
  VideoRenderer::destroy()
  {
    delete this;
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::open
  // 
  bool
  VideoRenderer::open(IVideoCanvas * canvas, IReader * reader)
  {
    return private_->open(canvas, reader);
  }
  
  //----------------------------------------------------------------
  // VideoRenderer::close
  // 
  void
  VideoRenderer::close()
  {
    private_->close();
  }
}

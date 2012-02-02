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
#include <limits>


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
    
    TTime framePosition;
    double frameDuration = 0.0;
    double lateFrames = 0.0;
    double lateFramesErrorSum = 0.0;
    double playheadPositionPrev = - std::numeric_limits<double>::max();
    
    bool ok = true;
    while (ok)
    {
      boost::this_thread::interruption_point();
      
      clock_.waitForOthers();
      
      // get the time segment we are supposed to render for,
      // and the current time relative to the time segment:
      double playheadPosition = 0.0;
      bool clockIsRunning = clock_.getCurrentTime(t0, playheadPosition);
      
      bool playbackLoopedAround = (playheadPosition < playheadPositionPrev);
      playheadPositionPrev = playheadPosition;
      
      // get position of the frame relative to the current time segment:
      double f0 = double(framePosition.time_) / double(framePosition.base_);
      double f1 = f0 + frameDuration;
      double df = f1 - playheadPosition;

      if (df < -0.067 && clockIsRunning && !playbackLoopedAround)
      {
        lateFramesErrorSum -= df;
        lateFrames += 1.0;
      }
      else
      {
        lateFramesErrorSum = 0.0;
        lateFrames = 0.0;
      }
      
      if (df > 0.0 && !playbackLoopedAround)
      {
        // wait until the next frame is required:
        double secondsToSleep = std::min(df, frameDuration);

        if (df > 0.067)
        {
          std::cerr << "FRAME IS VALID FOR " << df << " sec\n"
                    << "sleep: " << secondsToSleep << " sec"
                    << "\tf0: " << f0
                    << "\tf1: " << f1
                    << "\tt: " << playheadPosition
                    << std::endl;
        }
        
        boost::this_thread::sleep(boost::posix_time::milliseconds
                                  (long(secondsToSleep * 1000.0)));
        continue;
      }
      
      if (clockIsRunning &&
          lateFrames > 29.0 &&
          lateFramesErrorSum / lateFrames > 0.067 &&
          !playbackLoopedAround)
      {
#if 1
        std::cerr << "video is late " << -df << " sec, "
                  << "\tf0: " << f0
                  << "\tf1: " << f1
                  << "\tt: " << playheadPosition
                  << "\nlate frames: " << lateFrames
                  << "\nerror total: " << lateFramesErrorSum
                  << "\naverage err: " << lateFramesErrorSum / lateFrames
                  << std::endl;
#endif
        
        // tell others to wait for the video renderer:
        double delayInSeconds =
          std::min(std::max(-df + frameDuration, 1.0), 2.0);
        
        clock_.waitForMe(delayInSeconds);

        lateFrames = 0.0;
        lateFramesErrorSum = 0.0;
      }
      
      // read a frame:
      TVideoFramePtr frame;
      bool resetTimeCounters = false;
      while (ok)
      {
        ok = reader_->readVideo(frame);
        if (ok)
        {
          if (!frame)
          {
#if 1
            std::cerr << "\nRESET VIDEO TIME COUNTERS, playhead: "
                      << playheadPosition
                      << std::endl << std::endl;
#endif
            resetTimeCounters = true;
            break;
          }
          
          framePosition = frame->time_;
          double t =
            double(framePosition.time_) /
            double(framePosition.base_);
          
          if (t > f0 || frameDuration == 0.0)
          {
#if 0
            std::cerr << "RENDER VIDEO @ " << t
                      << ", clock is running: " << clockIsRunning
                      << ", playhead: " << playheadPosition
                      << std::endl;
#endif
            break;
          }
        }
      }
      
      if (!ok)
      {
#ifndef NDEBUG
        std::cerr << "reader_->readVideo: " << ok << std::endl;
#endif
        break;
      }

      if (!resetTimeCounters)
      {
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
      else
      {
        t0 = TTime();
        framePosition = TTime();
        frameDuration = 0.0;
        lateFrames = 0.0;
        lateFramesErrorSum = 0.0;
      }
      
      if (clock_.allowsSettingTime())
      {
        clock_.setCurrentTime(framePosition);
      }
    }
    
    if (clock_.allowsSettingTime())
    {
      clock_.noteTheClockHasStopped();
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

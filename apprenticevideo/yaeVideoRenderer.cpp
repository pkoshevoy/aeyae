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

    bool open(IVideoCanvas * canvas, IReader * reader, bool forOneFrameOnly);
    void close();
    void pause(bool pause);
    void threadLoop();

    mutable boost::mutex mutex_;
    Thread<TPrivate> thread_;

    // this is a tool for aborting a request for a video frame
    // from the decoded frames queue, used to avoid a deadlock:
    QueueWaitMgr terminator_;

    SharedClock & clock_;
    IVideoCanvas * canvas_;
    IReader * reader_;
    bool forOneFrameOnly_;
    bool pause_;
  };

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::TPrivate
  //
  VideoRenderer::TPrivate::TPrivate(SharedClock & clock):
    clock_(clock),
    canvas_(NULL),
    reader_(NULL),
    pause_(true)
  {
    thread_.setContext(this);
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::open
  //
  bool
  VideoRenderer::TPrivate::open(IVideoCanvas * canvas,
                                IReader * reader,
                                bool forOneFrameOnly)
  {
    close();

    boost::lock_guard<boost::mutex> lock(mutex_);
    forOneFrameOnly_ = forOneFrameOnly;
    canvas_ = canvas;
    reader_ = reader;
    pause_ = true;

    if (!reader_)
    {
      return true;
    }

    terminator_.stopWaiting(false);
    return thread_.run();
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::close
  //
  void
  VideoRenderer::TPrivate::close()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    pause_ = false;
    terminator_.stopWaiting(true);
    thread_.stop();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::pause
  //
  void
  VideoRenderer::TPrivate::pause(bool paused)
  {
    pause_ = paused;
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
    double tempo = 1.0;
    double drift = 0.0;
    double lateFrames = 0.0;
    double lateFramesErrorSum = 0.0;
    double clockPositionPrev = - std::numeric_limits<double>::max();
    static const double secondsToPause = 0.1;

    bool ok = true;
    while (ok && !boost::this_thread::interruption_requested())
    {
      while (pause_ && !boost::this_thread::interruption_requested())
      {
        boost::this_thread::disable_interruption here;
        boost::this_thread::sleep(boost::posix_time::milliseconds
                                  (long(secondsToPause * 1000.0)));
      }

      boost::this_thread::interruption_point();

      clock_.waitForOthers();

      // get the time segment we are supposed to render for,
      // and the current time relative to the time segment:
      double elapsedTime = 0.0;
      bool clockIsRunning = clock_.getCurrentTime(t0, elapsedTime);
      double clockPosition = t0.toSeconds();
      double playheadPosition = clockPosition + elapsedTime * tempo - drift;

      bool playbackLoopedAround = (clockPosition < clockPositionPrev);
      clockPositionPrev = clockPosition;

      // get position of the frame relative to the current time segment:
      double frameDurationScaled = frameDuration / tempo;
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
        double secondsToSleep = std::min(df, frameDurationScaled);

        if (df > 0.067)
        {
#if 0 // ndef NDEBUG
          std::cerr << "FRAME IS VALID FOR " << df << " sec\n"
                    << "sleep: " << secondsToSleep << " sec"
                    << "\tf0: " << f0
                    << "\tf1: " << f1
                    << "\tt: " << playheadPosition
                    << std::endl;
#endif
        }

        boost::this_thread::disable_interruption here;
        boost::this_thread::sleep(boost::posix_time::milliseconds
                                  (long(secondsToSleep * 1000.0)));
        continue;
      }

      if (clockIsRunning &&
          lateFrames > 29.0 &&
          lateFramesErrorSum / lateFrames > 0.067 &&
          !playbackLoopedAround)
      {
#ifndef NDEBUG
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
          std::min(std::max(-df + frameDurationScaled, 1.0), 2.0);

        clock_.waitForMe(delayInSeconds);

        lateFrames = 0.0;
        lateFramesErrorSum = 0.0;
      }

      // read a frame:
      TVideoFramePtr frame;
      bool resetTimeCounters = false;
      while (ok)
      {
        ok = reader_->readVideo(frame, &terminator_);
        if (ok)
        {
          if (!frame)
          {
#if 0
            std::cerr << "\nRESET VIDEO TIME COUNTERS, playhead: "
                      << playheadPosition
                      << std::endl << std::endl;
#endif
            resetTimeCounters = true;
            break;
          }

          framePosition = frame->time_;
          double t = framePosition.toSeconds();

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
#if 0
        std::cerr << "reader_->readVideo: " << ok << std::endl;
#endif
        if (clock_.allowsSettingTime())
        {
          clock_.noteTheClockHasStopped();
        }

        break;
      }

      if (!resetTimeCounters)
      {
        frameDuration =
          frame->traits_.frameRate_ ?
          1.0 / frame->traits_.frameRate_ :
          1.0 / double(framePosition.base_);

        tempo = frame->tempo_;

        // dispatch the frame to the canvas for rendering:
        if (canvas_)
        {
          canvas_->render(frame);
          if (forOneFrameOnly_)
          {
            break;
          }
        }
      }
      else
      {
        t0 = TTime();
        framePosition = TTime();
        frameDuration = 0.0;
        tempo = 1.0;
        drift = 0.0;
        lateFrames = 0.0;
        lateFramesErrorSum = 0.0;
        clockPositionPrev = - std::numeric_limits<double>::max();
      }

      if (clock_.allowsSettingTime())
      {
        double latency = 0.0;
        bool notifyObserver = !resetTimeCounters;
        clock_.setCurrentTime(framePosition, latency, notifyObserver);
        drift = df;
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
  VideoRenderer::open(IVideoCanvas * canvas,
                      IReader * reader,
                      bool forOneFrameOnly)
  {
    return private_->open(canvas, reader, forOneFrameOnly);
  }

  //----------------------------------------------------------------
  // VideoRenderer::close
  //
  void
  VideoRenderer::close()
  {
    private_->close();
  }

  //----------------------------------------------------------------
  // VideoRenderer::pause
  //
  void
  VideoRenderer::pause(bool paused)
  {
    private_->pause(paused);
  }
}

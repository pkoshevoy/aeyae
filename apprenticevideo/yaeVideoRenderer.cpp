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
    void pause(bool pause);
    bool isPaused() const;
    bool skipToNextFrame();

    void threadLoop();
    bool readOneFrame(QueueWaitMgr & terminator,
                      double & frameDuration,
                      double & f0,
                      double & df,
                      double & tempo,
                      double & drift);

    mutable boost::mutex mutex_;
    Thread<TPrivate> thread_;

    // this is a tool for aborting a request for a video frame
    // from the decoded frames queue, used to avoid a deadlock:
    QueueWaitMgr terminator_;

    SharedClock & clock_;
    IVideoCanvas * canvas_;
    IReader * reader_;
    bool pause_;
    TTime framePosition_;

    TVideoFramePtr frame_a_;
    TVideoFramePtr frame_b_;
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
                                IReader * reader)
  {
    close();

    boost::lock_guard<boost::mutex> lock(mutex_);
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
  // VideoRenderer::TPrivate::isPaused
  //
  bool
  VideoRenderer::TPrivate::isPaused() const
  {
    return pause_;
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::skipToNextFrame
  //
  bool
  VideoRenderer::TPrivate::skipToNextFrame()
  {
    if (canvas_ && reader_)
    {
      QueueWaitMgr terminator;
      terminator.stopWaiting(true);

      double frameDuration = 0.0;
      double f0 = double(framePosition_.time_) / double(framePosition_.base_);
      double df = frameDuration;
      double tempo = 1.0;
      double drift = 0.0;

      while (true)
      {
        if (!readOneFrame(terminator, frameDuration, f0, df, tempo, drift))
        {
          return false;
        }

        if (frame_a_ && frame_b_)
        {
          return true;
        }
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::threadLoop
  //
  void
  VideoRenderer::TPrivate::threadLoop()
  {
    TTime t0;

    frame_a_ = TVideoFramePtr();
    frame_b_ = TVideoFramePtr();
    framePosition_ = TTime();

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
        boost::this_thread::sleep(boost::posix_time::microseconds
                                  (long(secondsToPause * 1e+6)));
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
      double f0 = double(framePosition_.time_) / double(framePosition_.base_);
      double f1 = f0 + frameDuration;
      double df = f1 - playheadPosition;

#if 0
      std::cerr
        << "f0: " << f0 << std::endl
        << "f1: " << f1 << std::endl
        << "df: " << df << std::endl
        << "t:  "  << playheadPosition << std::endl
        << std::endl;
#endif

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

      if (df > 1e-3 && !playbackLoopedAround && frame_b_ && f1 > f0)
      {
        // wait until the next frame is required:
        double secondsToSleep = std::min(df, frameDurationScaled);
        secondsToSleep = std::min(1.0, secondsToSleep);

        if (df > 0.5)
        {
#ifndef NDEBUG
          std::cerr << "FRAME IS VALID FOR " << df << " sec\n"
                    << "sleep: " << secondsToSleep << " sec"
                    << "\tf0: " << f0
                    << "\tf1: " << f1
                    << "\tclock: " << clockPosition
                    << "\tplayhead: " << playheadPosition
                    << std::endl;
#endif
        }

#if 0
        std::cerr << "sleep for: " << secondsToSleep << std::endl;
#endif

        boost::this_thread::disable_interruption here;
        boost::this_thread::sleep(boost::posix_time::microseconds
                                  (long(secondsToSleep * 1e+6)));
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
                  << "\tclock: " << clockPosition
                  << "\tplayhead: " << playheadPosition
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
      ok = readOneFrame(terminator_,
                        frameDuration,
                        f0,
                        df,
                        tempo,
                        drift);
      if (!ok)
      {
        break;
      }

      if (frame_b_)
      {
        frameDuration = (frame_b_->time_ - frame_a_->time_).toSeconds();
      }
      else
      {
        frame_a_.reset();

        t0 = TTime();
        framePosition_ = TTime();
        frameDuration = 0.0;
        tempo = 1.0;
        drift = 0.0;
        lateFrames = 0.0;
        lateFramesErrorSum = 0.0;
        clockPositionPrev = - std::numeric_limits<double>::max();

        if (clock_.allowsSettingTime())
        {
          clock_.setCurrentTime(framePosition_, 0.0, false);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::readOneFrame
  //
  bool
  VideoRenderer::TPrivate::readOneFrame(QueueWaitMgr & terminator,
                                        double & frameDuration,
                                        double & f0,
                                        double & df,
                                        double & tempo,
                                        double & drift)
  {
    TVideoFramePtr frame;
    bool ok = true;
    while (ok)
    {
      ok = reader_->readVideo(frame, &terminator);
      if (!ok)
      {
        break;
      }

      if (!frame)
      {
#if 0
        std::cerr << "\nRESET VIDEO TIME COUNTERS" << std::endl;
#endif
        frame_a_ = frame_b_;
        frame_b_.reset();
        framePosition_ = frame_a_ ? frame_a_->time_ : TTime();
        break;
      }

      if (!frame_a_)
      {
#if 0
        std::cerr << "First FRAME @ " << frame->time_.toSeconds()
                  << std::endl;
#endif
        frame_a_ = frame;
        frame_b_ = frame;
        framePosition_ = frame_a_->time_;
        break;
      }

      frame_a_ = frame_b_ ? frame_b_ : frame;
      frame_b_ = frame;
      framePosition_ = frame_a_->time_;

      double t = framePosition_.toSeconds();
      if (t > f0 || frameDuration == 0.0)
      {
#if 0
        std::cerr << "Next FRAME @ " << t << std::endl;
#endif
        break;
      }
    }

    if (!ok)
    {
#if 0
      std::cerr
        << "reader_->readVideo FAILED, aborting..."
        << std::endl;
#endif

      if (clock_.allowsSettingTime())
      {
        clock_.noteTheClockHasStopped();
      }

      return false;
    }

    if (frame_a_)
    {
      tempo = frame_a_->tempo_;

      if (clock_.allowsSettingTime())
      {
        clock_.setCurrentTime(frame_a_->time_, 0.0, true);
        drift = df;
      }

      // dispatch the frame to the canvas for rendering:
      if (canvas_)
      {
#if 0
        std::cerr
          << "RENDER VIDEO @ " << frame_a_->time_.toSeconds()
          << std::endl;
#endif
        canvas_->render(frame_a_);
      }
    }

    return true;
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

  //----------------------------------------------------------------
  // VideoRenderer::skipToNextFrame
  //
  bool
  VideoRenderer::skipToNextFrame(TTime & framePosition)
  {
    bool done = private_->skipToNextFrame();
    framePosition = private_->framePosition_;
    return done;
  }

  //----------------------------------------------------------------
  // VideoRenderer::pause
  //
  void
  VideoRenderer::pause(bool paused)
  {
    private_->pause(paused);
  }

  //----------------------------------------------------------------
  // VideoRenderer::isPaused
  //
  bool
  VideoRenderer::isPaused() const
  {
    return private_->isPaused();
  }
}

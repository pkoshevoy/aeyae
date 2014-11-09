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

    bool open(IVideoCanvas * canvas, IReader * reader, bool frameStepping);
    void close();
    void pause(bool pause);
    bool isPaused() const;
    void skipToNextFrame();
    void skipToTime(const TTime & t, IReader * reader);
    void threadLoop();

    mutable boost::mutex mutex_;
    Thread<TPrivate> thread_;

    // this is a tool for aborting a request for a video frame
    // from the decoded frames queue, used to avoid a deadlock:
    QueueWaitMgr terminator_;

    SharedClock & clock_;
    IVideoCanvas * canvas_;
    IReader * reader_;
    bool frameStepping_;
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
    frameStepping_(false),
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
                                bool frameStepping)
  {
    close();

    boost::lock_guard<boost::mutex> lock(mutex_);
    frameStepping_ = frameStepping;
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
  void
  VideoRenderer::TPrivate::skipToNextFrame()
  {
    if (canvas_ && reader_)
    {
      // run the thread:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        frameStepping_ = true;
        pause_ = false;
      }

      terminator_.stopWaiting(false);

      if (!thread_.isRunning())
      {
        thread_.run();
      }

      thread_.wait();
    }
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::skipToTime
  //
  void
  VideoRenderer::TPrivate::skipToTime(const TTime & t, IReader * reader)
  {
    TTime currentTime;

    double clockPosition = currentTime.toSeconds();
    double seekPosition = t.toSeconds();

    TVideoFramePtr frame;
    bool ok = true;
    while (ok && reader && frameStepping_ && clockPosition < seekPosition)
    {
      ok = reader->readVideo(frame, &terminator_);
      if (ok && frame)
      {
        double dt =
          frame->traits_.frameRate_ ?
          1.0 / frame->traits_.frameRate_ :
          1.0 / 24.0;

        clockPosition = frame->time_.toSeconds();
        double frameEnd = clockPosition + dt;

        if (seekPosition < frameEnd)
        {
          break;
        }
      }
    }

    if (frame)
    {
      if (clock_.allowsSettingTime())
      {
        clock_.setCurrentTime(frame->time_);
      }

      // dispatch the frame to the canvas for rendering:
      if (canvas_)
      {
        canvas_->render(frame);
      }
    }
  }

  //----------------------------------------------------------------
  // VideoRenderer::TPrivate::threadLoop
  //
  void
  VideoRenderer::TPrivate::threadLoop()
  {
    TTime t0;

    TVideoFramePtr frame_a;
    TVideoFramePtr frame_b;
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

      if (df > 1e-3 && !playbackLoopedAround && frame_b)
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
      TVideoFramePtr frame;
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
            frame_a = frame_b;
            frame_b.reset();
            framePosition_ = frame_a ? frame_a->time_ : TTime();
            break;
          }

          if (!frame_a)
          {
#if 0
            std::cerr << "First FRAME @ " << frame->time_.toSeconds()
                      << ", clock is running: " << clockIsRunning
                      << ", playhead: " << playheadPosition
                      << std::endl;
#endif
            frame_a = frame;
            frame_b = frame;
            framePosition_ = frame_a->time_;
            break;
          }

          frame_a = frame_b;
          frame_b = frame;
          framePosition_ = frame_a->time_;

          double t = framePosition_.toSeconds();
          if (t > f0 || frameDuration == 0.0)
          {
#if 0
            std::cerr << "Next FRAME @ " << t
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
        std::cerr
          << "reader_->readVideo FAILED, aborting..."
          << std::endl;
#endif

        if (clock_.allowsSettingTime())
        {
          clock_.noteTheClockHasStopped();
        }

        break;
      }

      if (frame_a)
      {
        tempo = frame_a->tempo_;

        if (clock_.allowsSettingTime())
        {
          clock_.setCurrentTime(frame_a->time_, 0.0, true);
          drift = df;
        }

        // dispatch the frame to the canvas for rendering:
        if (canvas_)
        {
#if 0
          std::cerr
            << "RENDER VIDEO @ " << frame_a->time_.toSeconds()
            << ", clock is running: " << clockIsRunning
            << ", playhead: " << playheadPosition
            << std::endl;
#endif
          canvas_->render(frame_a);
        }
      }

      if (frame_b)
      {
        frameDuration = (frame_b->time_ - frame_a->time_).toSeconds();

        if (frameStepping_)
        {
          break;
        }
      }
      else
      {
        frame_a.reset();

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
                      bool frameStepping)
  {
    return private_->open(canvas, reader, frameStepping);
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
  const TTime &
  VideoRenderer::skipToNextFrame()
  {
    private_->skipToNextFrame();
    return private_->framePosition_;
  }

  //----------------------------------------------------------------
  // VideoRenderer::skipToTime
  //
  void
  VideoRenderer::skipToTime(const TTime & t, IReader * reader)
  {
    private_->skipToTime(t, reader);
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

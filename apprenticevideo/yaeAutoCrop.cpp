// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 11 15:57:39 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// yae includes:
#include <yaeAPI.h>
#include <yaeAutoCrop.h>
#include <yaePixelFormatTraits.h>
#include <yaeThreading.h>
#include <yaeUtils.h>

// boost includes:
#include <boost/thread.hpp>


namespace yae
{

  //----------------------------------------------------------------
  // TFiFo
  //
  template <typename TScalar, int maxSize>
  struct TFiFo
  {
    TFiFo():
      size_(0),
      head_(0),
      tail_(0),
      sum_(0)
    {}

    enum { kSize = maxSize };

    inline const int & size() const
    { return size_; }

    inline bool isFull() const
    { return size_ == maxSize; }

    inline bool isEmpty() const
    { return !size_; }

    inline const TScalar & first() const
    { YAE_ASSERT(size_); return data_[head_]; }

    inline const TScalar & last() const
    { YAE_ASSERT(size_); return data_[tail_ - 1]; }

    inline TScalar mean() const
    { YAE_ASSERT(size_); return sum_ / (TScalar)size_; }

    TScalar push(const TScalar & d)
    {
      TScalar prev = 0;

      if (size_ == maxSize)
      {
        prev = first();
        head_ = (head_ + 1) % maxSize;
        sum_ -= prev;
      }
      else
      {
        size_++;
      }

      data_[tail_] = d;
      tail_ = (tail_ + 1) % maxSize;
      sum_ += d;

      return prev;
    }

    void push(const TScalar & d, TFiFo & next)
    {
      if (size_ == maxSize)
      {
        TScalar prev = push(d);
        next.push(prev);
      }
      else
      {
        push(d);
      }
    }

    void push(TFiFo & next)
    {
      if (size_)
      {
        next.push(first());
        head_ = (head_ + 1) % maxSize;
        size_--;
      }
    }

    TScalar data_[maxSize];
    int size_;
    int head_;
    int tail_;
    TScalar sum_;
  };

  //----------------------------------------------------------------
  // analyze
  //
  static bool
  analyze(const TVideoFramePtr & frame, TCropFrame & crop)
  {
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    if (!ptts)
    {
      // don't know how to handle this pixel format:
      return false;
    }

    const unsigned char * data = frame->data_->data(0);
    const unsigned int rowBytes = frame->data_->rowBytes(0);

    const unsigned int w = frame->traits_.visibleWidth_;
    const unsigned int h = frame->traits_.visibleHeight_;
    const unsigned int x0 = frame->traits_.offsetLeft_;
    const unsigned int y0 = frame->traits_.offsetTop_;

#if 0
    std::vector<unsigned char> img(w * h);
    for (unsigned int y = 0; y < h; y++)
    {
      for (unsigned int x = 0; x < w; x++)
      {
        double t = 255.0 * pixelIntensity(x0 + x, y0 + y,
                                          data, rowBytes, *ptts);
        YAE_ASSERT(t <= 255.0 && t >= 0.0);
        img[y * w + x] = (unsigned char)t;
      }
    }

    static int findex = 0;
    std::string fn = std::string("/tmp/") + toText(++findex) + ".pgm";
    std::cerr << "SAVING " << fn << ", original was " << ptts->name_ << std::endl;
    std::FILE * fout = fopenUtf8(fn.c_str(), "wb");
    std::string header;
    header += "P5\n";
    header += toText(w) + ' '  + toText(h) + '\n';
    header += "255\n";
    fwrite(header.c_str(), 1, header.size(), fout);
    fwrite(&img[0], 1, img.size(), fout);
    fclose(fout);
#endif

    unsigned int step = std::min<unsigned int>(w / 32, h / 32);
    step = std::max<unsigned int>(1, step);

    const std::size_t ny = (h + step - 1) / step;
    const std::size_t nx = (w + step - 1) / step;

    std::vector<double> responseLeft(ny);
    std::vector<double> responseRight(ny);
    std::vector<double> responseTop(nx);
    std::vector<double> responseBottom(nx);

    std::vector<unsigned int> offsetLeft(ny);
    std::vector<unsigned int> offsetRight(ny);
    std::vector<unsigned int> offsetTop(nx);
    std::vector<unsigned int> offsetBottom(nx);

    double epsilon = 1.0 / 256.0;
    double backgnd = (ptts->flags_ & pixelFormat::kYUV ? 16.0 : 1.0) * epsilon;

    for (unsigned int y = 0; y < h; y += step)
    {
      unsigned int sampleIndex = y / step;
      YAE_ASSERT(sampleIndex < ny);

      // left-side edge:
      {
        TFiFo<double, 4> negative;
        TFiFo<double, 4> positive;
        positive.push(backgnd);

        double & best = responseLeft[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetLeft[sampleIndex];
        offset = 0;

        for (unsigned int x = 0; x < w / 3; x++)
        {
          double t = pixelIntensity(x0 + x, y0 + y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn < epsilon)
          {
            continue;
          }

          double mp = positive.mean();
          double response = mp / mn;
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          std::cerr << std::setw(4) << y << ", left offset "
                    << std::setw(3) << x + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << std::endl;
#endif
          if (best < response)
          {
            best = response;
            offset = x + 1 - positive.size();
          }
          else if (best > 1.0 && response < 1.1 && improved < 0.7)
          {
            break;
          }
        }
      }

      // right-side edge:
      {
        TFiFo<double, 4> negative;
        TFiFo<double, 4> positive;
        positive.push(backgnd);

        double & best = responseRight[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetRight[sampleIndex];
        offset = 0;

        for (unsigned int x = 0; x < w / 3; x++)
        {
          double t = pixelIntensity(x0 + w - 1 - x, y0 + y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn < epsilon)
          {
            continue;
          }

          double mp = positive.mean();
          double response = mp / mn;
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          std::cerr << std::setw(4) << y << ", right offset "
                    << std::setw(3) << x + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << std::endl;
#endif
          if (best < response)
          {
            best = response;
            offset = x + 1 - positive.size();
          }
          else if (best > 1.0 && response < 1.1 && improved < 0.7)
          {
            break;
          }
        }
      }
    }

    for (unsigned int x = 0; x < w; x += step)
    {
      unsigned int sampleIndex = x / step;
      YAE_ASSERT(sampleIndex < nx);

      // top-side edge:
      {
        TFiFo<double, 4> negative;
        TFiFo<double, 4> positive;
        positive.push(backgnd);

        double & best = responseTop[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetTop[sampleIndex];
        offset = 0;

        for (unsigned int y = 0; y < h / 3; y++)
        {
          double t = pixelIntensity(x0 + x, y0 + y, data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn < epsilon)
          {
            continue;
          }

          double mp = positive.mean();
          double response = mp / mn;
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          std::cerr << std::setw(4) << x << ", top offset "
                    << std::setw(3) << y + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << std::endl;
#endif
          if (best < response)
          {
            best = response;
            offset = y + 1 - positive.size();
          }
          else if (best > 1.0 && response < 1.1 && improved < 0.7)
          {
            break;
          }
        }
      }

      // bottom-side edge:
      {
        TFiFo<double, 4> negative;
        TFiFo<double, 4> positive;
        positive.push(backgnd);

        double & best = responseBottom[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetBottom[sampleIndex];
        offset = 0;

        for (unsigned int y = 0; y < h / 3; y++)
        {
          double t = pixelIntensity(x0 + x, y0 + h - 1 - y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn < epsilon)
          {
            continue;
          }

          double mp = positive.mean();
          double response = mp / mn;
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          std::cerr << std::setw(4) << x << ", bottom offset "
                    << std::setw(3) << y + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << std::endl;
#endif
          if (best < response)
          {
            best = response;
            offset = y + 1 - positive.size();
          }
          else if (best > 1.0 && response < 1.1 && improved < 0.7)
          {
            break;
          }
        }
      }
    }

#if 0
    std::cerr << "\nLEFT EDGE:" << std::endl;
    for (std::size_t i = 0; i < ny; i++)
    {
      unsigned int y = y0 + i * step;
      std::cerr << "response(" << offsetLeft[i] << ", " << y << ") = "
                << responseLeft[i] << std::endl;
    }

    std::cerr << "\nRIGHT EDGE:" << std::endl;
    for (std::size_t i = 0; i < ny; i++)
    {
      unsigned int y = y0 + i * step;
      std::cerr << "response(" << offsetRight[i] << ", " << y << ") = "
                << responseRight[i] << std::endl;
    }

    std::cerr << "\nTOP EDGE:" << std::endl;
    for (std::size_t i = 0; i < nx; i++)
    {
      unsigned int x = x0 + i * step;
      std::cerr << "response(" << x << ", " << offsetTop[i] << ") = "
                << responseTop[i] << std::endl;
    }

    std::cerr << "\nBOTTOM EDGE:" << std::endl;
    for (std::size_t i = 0; i < nx; i++)
    {
      unsigned int x = x0 + i * step;
      std::cerr << "response(" << x << ", " << offsetBottom[i] << ") = "
                << responseBottom[i] << std::endl;
    }
#endif

    std::sort(offsetLeft.begin(), offsetLeft.end());
    std::sort(offsetRight.begin(), offsetRight.end());
    std::sort(offsetTop.begin(), offsetTop.end());
    std::sort(offsetBottom.begin(), offsetBottom.end());

    const std::size_t ny3 = ny / 3;
    const std::size_t nx3 = nx / 3;

    double left = 0.0;
    double right = 0.0;

    for (std::size_t i = 0; i < ny3; i++)
    {
      left += offsetLeft[i + ny3];
      right += offsetRight[i + ny3];
    }

    left /= double(ny3);
    right /= double(ny3);

    double top = 0.0;
    double bottom = 0.0;

    for (std::size_t i = 0; i < nx3; i++)
    {
      top += offsetTop[i + nx3];
      bottom += offsetBottom[i + nx3];
    }

    top /= double(nx3);
    bottom /= double(nx3);

    crop.x_ = (int)(x0 + left + 0.5);
    crop.w_ = (int)(w - x0 - right - left + 0.5);
    crop.y_ = (int)(y0 + top + 0.5);
    crop.h_ = (int)(h - y0 - bottom - top + 0.5);

    std::cerr << "\ncrop margins: left " << crop.x_
              << ", right " << w - (crop.x_ + crop.w_)
              << ", top " << crop.y_
              << ", bottom " << h - (crop.y_ + crop.h_)
              << std::endl;

    return true;
  }

  //----------------------------------------------------------------
  // same
  //
  static bool
  same(const TCropFrame & a, const TCropFrame & b, int tolerance)
  {
    return (abs(a.x_ - b.x_) <= tolerance &&
            abs(a.y_ - b.y_) <= tolerance &&
            abs(a.w_ - b.w_) <= tolerance &&
            abs(a.h_ - b.h_) <= tolerance);
  }


  //----------------------------------------------------------------
  // TAutoCropDetect::TAutoCropDetect
  //
  TAutoCropDetect::TAutoCropDetect():
    callbackContext_(NULL),
    callback_(NULL),
    done_(false)
  {}

  //----------------------------------------------------------------
  // TAutoCropDetect::reset
  //
  void
  TAutoCropDetect::reset(void * callbackContext, TAutoCropCallback callback)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    callbackContext_ = callbackContext;
    callback_ = callback;
    done_ = false;
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::setFrame
  //
  void
  TAutoCropDetect::setFrame(const TVideoFramePtr & frame)
  {
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      frame_ = frame;
    }

    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::threadLoop
  //
  void
  TAutoCropDetect::threadLoop()
  {
    TCropFrame crop[3];
    int found = 0;
    int failed = 0;

    for (; found < 3 && failed < 100 && !done_; )
    {
      TVideoFramePtr frame;

      while (!frame)
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        frame = frame_;
        frame_.reset();

        if (!done_ && !frame)
        {
          cond_.wait(lock);
        }
      }

      if (done_)
      {
        break;
      }

      // detect frame crop margins, synchronously:
      if (!analyze(frame, crop[found]))
      {
        failed++;
        continue;
      }

      if (found && !same(crop[found - 1], crop[found], 2))
      {
        // look for a consistent crop pattern,
        // if a mismatch is detected -- restart the search:
        found = 0;
        failed++;
        continue;
      }

      found++;
    }

    if (!done_ && found)
    {
      callback_(callbackContext_, crop[found - 1]);
    }
    else
    {
      callback_(callbackContext_, TCropFrame());
    }
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::stop
  //
  void
  TAutoCropDetect::stop()
  {
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      done_ = true;
    }

    cond_.notify_all();
  }

}

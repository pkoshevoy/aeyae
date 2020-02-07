// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 11 15:57:39 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <map>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include "yae_video.h"
#include "yae_auto_crop.h"
#include "yae_pixel_format_traits.h"
#include "../thread/yae_threading.h"
#include "../utils/yae_utils.h"


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
  // TBin
  //
  struct TBin
  {
    TBin():
      size_(0),
      sum_(0)
    {}

    unsigned int size_;
    int sum_;
  };

  //----------------------------------------------------------------
  // updateBin
  //
  inline static void
  updateBin(TBin & bin, int offset, unsigned int weight = 1)
  {
    bin.sum_ += offset * weight;
    bin.size_ += weight;
  }

  //----------------------------------------------------------------
  // updateHistogram
  //
  inline static void
  updateHistogram(std::map<int, TBin> & bins, int offset)
  {
    static const int kGranularity = 2;

    int i = offset / kGranularity;
    updateBin(bins[i - 1], offset);
    updateBin(bins[i], offset, 2);
    updateBin(bins[i + 1], offset);
  }

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
    const std::size_t rowBytes = frame->data_->rowBytes(0);

    const unsigned int w = vtts.visibleWidth_;
    const unsigned int h = vtts.visibleHeight_;
    const unsigned int x0 = vtts.offsetLeft_;
    const unsigned int y0 = vtts.offsetTop_;

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
    yae_debug << "SAVING " << fn << ", original was " << ptts->name_;
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
    double backgnd = 24.0 * epsilon;

    for (unsigned int y = 0; y < h; y += step)
    {
      unsigned int sampleIndex = y / step;
      YAE_ASSERT(sampleIndex < ny);

      // left-side edge:
      {
        TFiFo<double, 16> negative;
        TFiFo<double, 16> positive;
        positive.push(backgnd);

        double & best = responseLeft[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetLeft[sampleIndex];
        offset = 0;

        for (unsigned int x = 0; x < w / 2; x++)
        {
          double t = pixelIntensity(x0 + x, y0 + y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn <= 0.0)
          {
            continue;
          }

          double mp = positive.mean();
          double response = (mp + 0.1) / (mn + 0.1);
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          yae_debug << std::setw(4) << y << ", left offset "
                    << std::setw(3) << x + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << ", max = " << best << " @ " << offset;
#endif
          if (best < response)
          {
            best = response;
            offset = x + 1 - positive.size();
          }
          else if (best > 1.124 && negative.isFull() && response < 1.0 &&
                   improved < 1.0)
          {
            break;
          }
        }
      }

      // right-side edge:
      {
        TFiFo<double, 16> negative;
        TFiFo<double, 16> positive;
        positive.push(backgnd);

        double & best = responseRight[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetRight[sampleIndex];
        offset = 0;

        for (unsigned int x = 0; x < w / 2; x++)
        {
          double t = pixelIntensity(x0 + w - 1 - x, y0 + y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn <= 0.0)
          {
            continue;
          }

          double mp = positive.mean();
          double response = (mp + 0.1) / (mn + 0.1);
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          yae_debug << std::setw(4) << y << ", right offset "
                    << std::setw(3) << x + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << ", max = " << best << " @ " << offset;
#endif
          if (best < response)
          {
            best = response;
            offset = x + 1 - positive.size();
          }
          else if (best > 1.124 && negative.isFull() && response < 1.0 &&
                   improved < 1.0)
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
        TFiFo<double, 16> negative;
        TFiFo<double, 16> positive;
        positive.push(backgnd);

        double & best = responseTop[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetTop[sampleIndex];
        offset = 0;

        for (unsigned int y = 0; y < h / 2; y++)
        {
          double t = pixelIntensity(x0 + x, y0 + y, data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn <= 0.0)
          {
            continue;
          }

          double mp = positive.mean();
          double response = (mp + 0.1) / (mn + 0.1);
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          yae_debug << std::setw(4) << x << ", top offset "
                    << std::setw(3) << y + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << ", max = " << best << " @ " << offset;
#endif
          if (best < response)
          {
            best = response;
            offset = y + 1 - positive.size();
          }
          else if (best > 1.124 && negative.isFull() && response < 1.0 &&
                   improved < 1.0)
          {
            break;
          }
        }
      }

      // bottom-side edge:
      {
        TFiFo<double, 16> negative;
        TFiFo<double, 16> positive;
        positive.push(backgnd);

        double & best = responseBottom[sampleIndex];
        best = 0.0;

        unsigned int & offset = offsetBottom[sampleIndex];
        offset = 0;

        for (unsigned int y = 0; y < h / 2; y++)
        {
          double t = pixelIntensity(x0 + x, y0 + h - 1 - y,
                                    data, rowBytes, *ptts);
          positive.push(t, negative);

          double mn = negative.isEmpty() ? 0.0 : negative.mean();
          if (mn <= 0.0)
          {
            continue;
          }

          double mp = positive.mean();
          double response = (mp + 0.1) / (mn + 0.1);
          double improved = (response + epsilon) / (best + epsilon);
#if 0
          yae_debug << std::setw(4) << x << ", bottom offset "
                    << std::setw(3) << y + 1 - positive.size()
                    << ", " << std::setw(11) << mp
                    << " / " << std::setw(11) << mn
                    << ", r = " << std::setw(11) << response
                    << ", q = " << std::setw(11) << improved
                    << ", max = " << best << " @ " << offset;
#endif
          if (best < response)
          {
            best = response;
            offset = y + 1 - positive.size();
          }
          else if (best > 1.124 && negative.isFull() && response < 1.0 &&
                   improved < 1.0)
          {
            break;
          }
        }
      }
    }

#if 0
    std::ostringstream oss;

    oss << "\nLEFT EDGE:\n";
    for (std::size_t i = 0; i < ny; i++)
    {
      unsigned int y = y0 + i * step;
      oss << "response(" << offsetLeft[i] << ", " << y << ") = "
          << responseLeft[i] << "\n";
    }

    oss << "\nRIGHT EDGE:\n";
    for (std::size_t i = 0; i < ny; i++)
    {
      unsigned int y = y0 + i * step;
      oss << "response(" << offsetRight[i] << ", " << y << ") = "
          << responseRight[i] << "\n";
    }

    oss << "\nTOP EDGE:\n";
    for (std::size_t i = 0; i < nx; i++)
    {
      unsigned int x = x0 + i * step;
      oss << "response(" << x << ", " << offsetTop[i] << ") = "
          << responseTop[i] << "\n";
    }

    oss << "\nBOTTOM EDGE:\n";
    for (std::size_t i = 0; i < nx; i++)
    {
      unsigned int x = x0 + i * step;
      oss << "response(" << x << ", " << offsetBottom[i] << ") = "
          << responseBottom[i] << "\n";
    }

    yae_debug << oss.str();
#endif

    std::map<int, TBin> leftHistogram;
    std::map<int, TBin> rightHistogram;
    std::map<int, TBin> topHistogram;
    std::map<int, TBin> bottomHistogram;

    for (std::size_t i = 0; i < ny; i++)
    {
      updateHistogram(leftHistogram, offsetLeft[i]);
      updateHistogram(rightHistogram, offsetRight[i]);
    }

    for (std::size_t i = 0; i < nx; i++)
    {
      updateHistogram(topHistogram, offsetTop[i]);
      updateHistogram(bottomHistogram, offsetBottom[i]);
    }

    TBin lbest;
    for (std::map<int, TBin>::const_iterator i = leftHistogram.begin();
         i != leftHistogram.end(); ++i)
    {
      const TBin & bin = i->second;
#if 0
      yae_debug << "left(" << i->first << "): "
                << double(bin.sum_) / double(bin.size_)
                << " -> " << bin.size_;
#endif
      if (lbest.size_ < bin.size_)
      {
        lbest = bin;
      }
    }

    TBin rbest;
    for (std::map<int, TBin>::const_iterator i = rightHistogram.begin();
         i != rightHistogram.end(); ++i)
    {
      const TBin & bin = i->second;
#if 0
      yae_debug << "right(" << i->first << "): "
                << double(bin.sum_) / double(bin.size_)
                << " -> " << bin.size_;
#endif
      if (rbest.size_ < bin.size_)
      {
        rbest = bin;
      }
    }

    TBin tbest;
    for (std::map<int, TBin>::const_iterator i = topHistogram.begin();
         i != topHistogram.end(); ++i)
    {
      const TBin & bin = i->second;
#if 0
      yae_debug << "top(" << i->first << "): "
                << double(bin.sum_) / double(bin.size_)
                << " -> " << bin.size_;
#endif
      if (tbest.size_ < bin.size_)
      {
        tbest = bin;
      }
    }

    TBin bbest;
    for (std::map<int, TBin>::const_iterator i = bottomHistogram.begin();
         i != bottomHistogram.end(); ++i)
    {
      const TBin & bin = i->second;
#if 0
      yae_debug << "bottom(" << i->first << "): "
                << double(bin.sum_) / double(bin.size_)
                << " -> " << bin.size_;
#endif
      if (bbest.size_ < bin.size_)
      {
        bbest = bin;
      }
    }

    double lOffset =
      leftHistogram.size() < (ny * 2) && lbest.size_ > (ny / 3) ?
      double(lbest.sum_) / double(lbest.size_) :
      0.0;

    double rOffset =
      rightHistogram.size() < (ny * 2) && rbest.size_ > (ny / 3) ?
      double(rbest.sum_) / double(rbest.size_) :
      0.0;

    double tOffset =
      topHistogram.size() < (nx * 2) && tbest.size_ > (nx / 3) ?
      double(tbest.sum_) / double(tbest.size_) :
      0.0;

    double bOffset =
      bottomHistogram.size() < (nx * 2) && bbest.size_ > (nx / 3) ?
      double(bbest.sum_) / double(bbest.size_) :
      0.0;

    crop.x_ = (int)(x0 + lOffset + 0.5);
    crop.y_ = (int)(y0 + tOffset + 0.5);
    crop.w_ = (int)(w - x0 - rOffset - lOffset + 0.5);
    crop.h_ = (int)(h - y0 - bOffset - tOffset + 0.5);

#if 0
    yae_debug
      << "\ncrop margins:\n"
      << "  left " << lOffset
      << " group(" << lbest.size_ << "), " << leftHistogram.size() << "\n"
      << " right " << rOffset
      << " group(" << rbest.size_ << "), " << rightHistogram.size() << "\n"
      << "   top " << tOffset
      << " group(" << tbest.size_ << "), " << topHistogram.size() << "\n"
      << "bottom " << bOffset
      << " group(" << bbest.size_ << "), " << bottomHistogram.size() << "\n"
      << "\n";
#endif

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
  // TAutoCropDetect::TPrivate
  //
  struct TAutoCropDetect::TPrivate
  {
    TPrivate();

    void reset(void * callbackContext, TAutoCropCallback callback);
    void setFrame(const TVideoFramePtr & frame);
    void thread_loop();
    void stop();

    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;
    void * callbackContext_;
    TAutoCropCallback callback_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    bool done_;
  };

  //----------------------------------------------------------------
  // TAutoCropDetect::TPrivate::TPrivate
  //
  TAutoCropDetect::TPrivate::TPrivate():
    callbackContext_(NULL),
    callback_(NULL),
    done_(false)
  {}

  //----------------------------------------------------------------
  // TAutoCropDetect::TPrivate::reset
  //
  void
  TAutoCropDetect::TPrivate::reset(void * callbackContext, TAutoCropCallback callback)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    callbackContext_ = callbackContext;
    callback_ = callback;
    done_ = false;
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::TPrivate::setFrame
  //
  void
  TAutoCropDetect::TPrivate::setFrame(const TVideoFramePtr & frame)
  {
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      frame_ = frame;
    }

    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::TPrivate::thread_loop
  //
  void
  TAutoCropDetect::TPrivate::thread_loop()
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
          if (found)
          {
            frame = callback_(callbackContext_, crop[found - 1], false);
          }
          else
          {
            frame = callback_(callbackContext_, TCropFrame(), false);
          }
        }

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
      callback_(callbackContext_, crop[found - 1], true);
    }
    else
    {
      callback_(callbackContext_, TCropFrame(), true);
    }
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::TPrivate::stop
  //
  void
  TAutoCropDetect::TPrivate::stop()
  {
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      done_ = true;
    }

    cond_.notify_all();
  }


  //----------------------------------------------------------------
  // TAutoCropDetect::TAutoCropDetect
  //
  TAutoCropDetect::TAutoCropDetect():
    private_(new TPrivate())
  {}

  //----------------------------------------------------------------
  // TAutoCropDetect::~TAutoCropDetect
  //
  TAutoCropDetect::~TAutoCropDetect()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::reset
  //
  void
  TAutoCropDetect::reset(void * callbackContext, TAutoCropCallback callback)
  {
    private_->reset(callbackContext, callback);
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::setFrame
  //
  void
  TAutoCropDetect::setFrame(const TVideoFramePtr & frame)
  {
    private_->setFrame(frame);
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::thread_loop
  //
  void
  TAutoCropDetect::thread_loop()
  {
    private_->thread_loop();
  }

  //----------------------------------------------------------------
  // TAutoCropDetect::stop
  //
  void
  TAutoCropDetect::stop()
  {
    private_->stop();
  }

}

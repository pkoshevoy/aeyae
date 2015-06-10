// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat May 12 15:37:17 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_TEMPO_FILTER_H_
#define YAE_AUDIO_TEMPO_FILTER_H_

#ifdef _WIN32
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// std includes:
#include <math.h>
#include <string.h>
#include <vector>

// yae includes:
#include "yae_audio_fragment.h"
#include "../video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // IAudioTempoFilter
  //
  struct IAudioTempoFilter
  {
    virtual ~IAudioTempoFilter() {}

    virtual void reset(unsigned int sampleRate,
                       unsigned int numChannels) = 0;

    virtual void clear() = 0;

    virtual bool setTempo(double tempo) = 0;

    virtual void apply(const unsigned char ** srcBuffer,
                       const unsigned char * srcBufferEnd,
                       unsigned char ** dstBuffer,
                       unsigned char * dstBufferEnd) = 0;

    virtual bool flush(unsigned char ** dstBuffer,
                       unsigned char * dstBufferEnd) = 0;

    // number of bytes required to store a
    // single multi-channel window of samples:
    virtual std::size_t fragmentSize() const = 0;
  };

  //----------------------------------------------------------------
  // AudioTempoFilter
  //
  // Another version of WSOLA filter
  //
  template <typename TSample, int tmin, int tmax>
  struct AudioTempoFilter : public IAudioTempoFilter
  {
    //----------------------------------------------------------------
    // TData
    //
    typedef TSample TData;

    enum
    {
      kMin = tmin,
      kMax = tmax
    };

    //----------------------------------------------------------------
    // TState
    //
    enum TState {
      kLoadFragment = 0,
      kAdjustPosition = 1,
      kReloadFragment = 2,
      kOutputOverlapAdd = 3,
      kFlushOutput = 4
    };

    //----------------------------------------------------------------
    // AudioTempoFilter
    //
    AudioTempoFilter():
      size_(0),
      head_(0),
      tail_(0),
      channels_(0),
      window_(0),
      tempo_(1.0),
      nfrag_(0),
      state_(kLoadFragment)
    {
      position_[0] = 0;
      position_[1] = 0;

      realToComplex_ = NULL;
      complexToReal_ = NULL;
    }

    //----------------------------------------------------------------
    // ~AudioTempoFilter
    //
    ~AudioTempoFilter()
    {
      av_rdft_end(realToComplex_);
      realToComplex_ = NULL;

      av_rdft_end(complexToReal_);
      complexToReal_ = NULL;
    }

    //----------------------------------------------------------------
    // reset
    //
    void reset(unsigned int sampleRate, unsigned int numChannels)
    {
      channels_ = numChannels;

      // pick a segment window size:
      window_ = sampleRate / 24;

      // adjust window size to be a power-of-two integer:
      unsigned int nlevels = floor_log2(window_);
      unsigned int pot = 1 << nlevels;
      YAE_ASSERT(pot <= window_);

      if (pot < window_)
      {
        window_ = pot * 2;
        nlevels++;
      }

      // attempt to persuade coverity scan that window_ is greater than 1:
      YAE_ASSERT(window_ > 1);
      window_ = std::max<unsigned int>(window_, 2);

      // initialize FFT contexts:
      av_rdft_end(realToComplex_);
      realToComplex_ = NULL;

      av_rdft_end(complexToReal_);
      complexToReal_ = NULL;

      realToComplex_ = av_rdft_init(nlevels + 1, DFT_R2C);
      complexToReal_ = av_rdft_init(nlevels + 1, IDFT_C2R);
      correlation_.resize<FFTComplex>(window_);

      unsigned int samplesToBuffer = window_ * 3;
      buffer_.resize(samplesToBuffer * channels_);

      // sample the Hann window function:
      hann_.resize(window_);
      for (std::size_t i = 0; i < window_; i++)
      {
        hann_[i] = float(0.5 * (1.0 - cos(2.0 * M_PI * double(i) /
                                          double(window_ - 1))));
      }

      clear();
    }

    //----------------------------------------------------------------
    // clear
    //
    void clear()
    {
      size_ = 0;
      head_ = 0;
      tail_ = 0;
      nfrag_ = 0;
      state_ = kLoadFragment;

      position_[0] = 0;
      position_[1] = 0;

      origin_[0] = 0;
      origin_[1] = 0;

      frag_[0].clear();
      frag_[1].clear();

      // shift left position of 1st fragment by half a window
      // so that no re-normalization would be required for
      // the left half of the 1st fragment:
      frag_[0].position_[0] = -int64(window_ / 2);
      frag_[0].position_[1] = -int64(window_ / 2);
    }

    //----------------------------------------------------------------
    // setTempo
    //
    bool setTempo(double tempo)
    {
      if (tempo < 0.5 || tempo > 2.0)
      {
        return false;
      }

      const TAudioFragment & prev = prevFrag();
      origin_[0] = prev.position_[0] + window_ / 2;
      origin_[1] = prev.position_[1] + window_ / 2;
      tempo_ = tempo;
      return true;
    }

    //----------------------------------------------------------------
    // apply
    //
    void apply(const unsigned char ** srcBuffer,
               const unsigned char * srcBufferEnd,
               unsigned char ** dstBuffer,
               unsigned char * dstBufferEnd)
    {
      const TSample * src = (const TSample *)*srcBuffer;
      const TSample * srcEnd = (const TSample *)srcBufferEnd;

      TSample * dst = (TSample *)*dstBuffer;
      TSample * dstEnd = (TSample *)dstBufferEnd;

      while (true)
      {
        if (state_ == kLoadFragment)
        {
          // load additional data for the current fragment:
          if (!loadFrag(src, srcEnd))
          {
            break;
          }

          // build a multi-resolution pyramid for fragment alignment:
          currFrag().template downsample<TSample>(window_,
                                                  float(tmin),
                                                  float(tmax));

          // apply FFT:
          currFrag().transform(realToComplex_);

          // must load the second fragment before alignment can start:
          if (!nfrag_)
          {
            advanceToNextFrag();
            continue;
          }

          state_ = kAdjustPosition;
        }

        if (state_ == kAdjustPosition)
        {
          // adjust position for better alignment:
          if (adjustPosition())
          {
            // reload the fragment at the corrected position, so that the
            // Hann window blending would not require normalization:
            state_ = kReloadFragment;
          }
          else
          {
            state_ = kOutputOverlapAdd;
          }
        }

        if (state_ == kReloadFragment)
        {
          // load additional data if necessary due to position adjustment:
          if (!loadFrag(src, srcEnd))
          {
            break;
          }

          // build a multi-resolution pyramid for fragment alignment:
          currFrag().template downsample<TSample>(window_,
                                                  float(tmin),
                                                  float(tmax));

          // apply FFT:
          currFrag().transform(realToComplex_);

          state_ = kOutputOverlapAdd;
        }

        if (state_ == kOutputOverlapAdd)
        {
          // overlap-add and output the result:
          if (!overlapAdd(dst, dstEnd))
          {
            break;
          }

          // advance to the next fragment, repeat:
          advanceToNextFrag();
          state_ = kLoadFragment;
        }
      }

      // pass back adjusted buffer pointers:
      *srcBuffer = (const unsigned char *)src;
      *dstBuffer = (unsigned char *)dst;
    }

    //----------------------------------------------------------------
    // overlapAdd
    //
    bool
    overlapAdd(TSample *& dst, TSample * dstEnd)
    {
      // shortcuts:
      const TAudioFragment & prev = prevFrag();
      const TAudioFragment & frag = currFrag();

      const int64 startHere = std::max<int64>(position_[1],
                                              frag.position_[1]);

      const int64 stopHere = std::min<int64>(prev.position_[1] +
                                             prev.numSamples_,
                                             frag.position_[1] +
                                             frag.numSamples_);

      const int64 overlap = stopHere - startHere;
      YAE_ASSERT(startHere <= stopHere &&
                 frag.position_[1] <= startHere &&
                 overlap <= (int64)frag.numSamples_);

      const int64 ia = startHere - prev.position_[1];
      const int64 ib = startHere - frag.position_[1];

      const float * wa = &hann_[0] + ia;
      const float * wb = &hann_[0] + ib;

      // this is risky, waveform pyramid does not guarantee alignment:
      const std::size_t stride = channels_ * sizeof(TSample);
      const TSample * a = (const TSample *)(&prev.data_[0] + ia * stride);
      const TSample * b = (const TSample *)(&frag.data_[0] + ib * stride);

      for (int64 i = 0; i < overlap && dst < dstEnd;
           i++, position_[1]++, wa++, wb++)
      {
        float w0 = *wa;
        float w1 = *wb;

        for (unsigned int j = 0; j < channels_; j++, a++, b++, dst++)
        {
          float t0 = float(*a);
          float t1 = float(*b);

          *dst = (frag.position_[0] + i < 0) ? *a : TSample(t0 * w0 + t1 * w1);
        }
      }

      bool done = position_[1] == stopHere;
      return done;
    }

    //----------------------------------------------------------------
    // flush
    //
    bool
    flush(unsigned char ** dstBuffer,
          unsigned char * dstBufferEnd)
    {
      state_ = kFlushOutput;

      // shortcuts:
      TAudioFragment & frag = currFrag();

      if (position_[0] == frag.position_[0] + frag.numSamples_ &&
          position_[1] == frag.position_[1] + frag.numSamples_)
      {
        // the current fragment is already flushed:
        return true;
      }

      if (frag.position_[0] + (int64)frag.numSamples_ < position_[0])
      {
        // finish loading the current (possibly partial) fragment:
        const TSample * nullSrc = NULL;
        loadFrag(nullSrc, NULL);

        if (nfrag_)
        {
          // build a multi-resolution pyramid for fragment alignment:
          frag.template downsample<TSample>(window_,
                                            float(tmin),
                                            float(tmax));

          // apply FFT:
          frag.transform(realToComplex_);

          // align current fragment to previous fragment:
          if (adjustPosition())
          {
            // reload the current fragment due to adjusted position:
            loadFrag(nullSrc, NULL);
          }
        }
      }

      // flush the overlap region:
      TSample * dst = (TSample *)*dstBuffer;
      TSample * dstEnd = (TSample *)dstBufferEnd;

      const int64 overlapEnd =
        frag.position_[1] + std::min<int64>(window_ / 2, frag.numSamples_);
      while (position_[1] < overlapEnd)
      {
        if (!overlapAdd(dst, dstEnd))
        {
          return false;
        }
      }

      // update the buffer pointer:
      *dstBuffer = (unsigned char *)dst;

      // flush the remaininder of the current fragment:
      const int64 startHere = std::max<int64>(position_[1], overlapEnd);
      const int64 stopHere = frag.position_[1] + frag.numSamples_;
      const int64 offset = startHere - frag.position_[1];
      YAE_ASSERT(startHere <= stopHere && frag.position_[1] <= startHere);

      const std::size_t stride = channels_ * sizeof(TSample);
      const unsigned char * src = &frag.data_[0] + offset * stride;

      std::size_t srcSize = (std::size_t)(stopHere - startHere) * stride;
      std::size_t dstSize = dstBufferEnd - *dstBuffer;
      std::size_t nbytes = std::min<std::size_t>(srcSize, dstSize);

      memcpy(*dstBuffer, src, nbytes);
      *dstBuffer += nbytes;

      position_[1] += (nbytes / stride);
      bool done = position_[1] == stopHere;

      return done;
    }

    //----------------------------------------------------------------
    // loadFrag
    //
    bool
    loadFrag(const TSample *& src, const TSample * srcEnd)
    {
      TAudioFragment & frag = currFrag();

      int64 stopHere = frag.position_[0] + window_;
      if (src && !loadData(src, srcEnd, stopHere))
      {
        return false;
      }

      // calculate the number of samples we don't have:
      int64 missing = stopHere > position_[0] ? stopHere - position_[0] : 0;
      unsigned int numSamples =
        missing < int64(window_) ? (unsigned int)(window_ - missing) : 0;

      // setup the output buffer:
      const std::size_t stride = channels_ * sizeof(TSample);
      frag.init(frag.position_[0], numSamples, channels_, stride);

      // shortcut:
      unsigned char * dst = frag.data_.empty() ? NULL : &frag.data_[0];

      int64 start = position_[0] - size_ / channels_;
      int64 zeros = 0;

      if (frag.position_[0] < start)
      {
        // what we don't have we substitute with zeros:
        zeros = std::min<std::size_t>(start - frag.position_[0], numSamples);
        YAE_ASSERT(zeros != numSamples);
        YAE_ASSERT(dst);

        memset(dst, 0, zeros * stride);
        dst += zeros * stride;
      }

      if (zeros == numSamples)
      {
        return true;
      }

      // shortcut:
      const std::size_t capacity = buffer_.size();

      // get the remaining data from the ring buffer:
      std::size_t na =
        (head_ < tail_ ? tail_ - head_ : capacity - head_) / channels_;

      std::size_t nb =
        (head_ < tail_ ? 0 : tail_) / channels_;

      // sanity check:
      YAE_ASSERT(numSamples <= zeros + na + nb);

      const TSample * a = &buffer_[head_];
      const TSample * b = &buffer_[0];

      std::size_t i0 = frag.position_[0] + zeros - start;
      std::size_t i1 = i0 < na ? 0 : i0 - na;

      std::size_t n0 =
        i0 < na ? std::min<std::size_t>(na - i0, numSamples - zeros) : 0;

      std::size_t n1 = numSamples - zeros - n0;

      if (n0)
      {
        memcpy(dst, a + i0 * channels_, n0 * stride);
        dst += n0 * stride;
      }

      if (n1)
      {
        memcpy(dst, b + i1 * channels_, n1 * stride);
        dst += n1 * stride;
      }

      return true;
    }

    //----------------------------------------------------------------
    // loadData
    //
    bool
    loadData(const TSample *& src, const TSample * srcEnd, int64 stopHere)
    {
      if (stopHere <= position_[0])
      {
        return true;
      }

      // shortcut:
      const std::size_t capacity = buffer_.size();
      const std::size_t readSize = (stopHere - position_[0]) * channels_;

      // samples are not expected to be skipped:
      YAE_ASSERT(readSize <= capacity);

      while (position_[0] < stopHere && src < srcEnd)
      {
        std::size_t srcSamples = (srcEnd - src);

        // load data piece-wise, in order to avoid complicating the logic:
        std::size_t numSamples = std::min<std::size_t>(readSize, srcSamples);
        numSamples = std::min<std::size_t>(numSamples, capacity);

        std::size_t na = std::min<std::size_t>(numSamples, capacity - tail_);
        std::size_t nb = std::min<std::size_t>(numSamples - na, capacity);

        if (na)
        {
          TSample * a = &buffer_[tail_];
          memcpy(a, src, na * sizeof(TSample));

          src += na;
          position_[0] += na / channels_;

          size_ = std::min<std::size_t>(size_ + na, capacity);
          tail_ = (tail_ + na) % capacity;
          head_ = size_ < capacity ? tail_ - size_ : tail_;
        }

        if (nb)
        {
          TSample * b = &buffer_[0];
          memcpy(b, src, nb * sizeof(TSample));

          src += nb;
          position_[0] += nb / channels_;

          size_ = std::min<std::size_t>(size_ + nb, capacity);
          tail_ = (tail_ + nb) % capacity;
          head_ = size_ < capacity ? tail_ - size_ : tail_;
        }
      }

      YAE_ASSERT(position_[0] <= stopHere);
      return position_[0] == stopHere;
    }

    //----------------------------------------------------------------
    // advanceToNextFrag
    //
    void advanceToNextFrag()
    {
      nfrag_++;
      const TAudioFragment & prev = prevFrag();
      TAudioFragment &       frag = currFrag();

      const double fragmentStep = tempo_ * double(window_ / 2);
      frag.position_[0] = prev.position_[0] + int64(fragmentStep);
      frag.position_[1] = prev.position_[1] + window_ / 2;
      frag.numSamples_ = 0;
    }

    //----------------------------------------------------------------
    // adjustPosition
    //
    int adjustPosition()
    {
      const TAudioFragment & prev = prevFrag();
      TAudioFragment &       frag = currFrag();

      const double prevOutputPosition =
        (double)(prev.position_[1] - origin_[1] + window_ / 2);

      const double idealOutputPosition =
        (double)(prev.position_[0] - origin_[0] + window_ / 2) / tempo_;

      const int drift = (int)(prevOutputPosition - idealOutputPosition);

      const int deltaMax = window_ / 2;

      const int correction = frag.alignTo(prev,
                                          window_,
                                          deltaMax,
                                          drift,
                                          correlation_.data<FFTSample>(),
                                          complexToReal_);

      if (correction)
      {
        // adjust fragment position:
        frag.position_[0] -= correction;

        // clear so that the fragment can be reloaded:
        frag.numSamples_ = 0;
      }

      return correction;
    }

    // accessor to the current fragment:
    inline TAudioFragment & currFrag()
    { return frag_[nfrag_ % 2]; }

    // accessor to the previous fragment:
    inline TAudioFragment & prevFrag()
    { return frag_[(nfrag_ + 1) % 2]; }

    // virtual:
    std::size_t fragmentSize() const
    { return window_ * channels_ * sizeof(TSample); }

    // ring-buffer of input samples:
    std::vector<TSample> buffer_;

    // ring buffer house keeping:
    std::size_t size_;
    std::size_t head_;
    std::size_t tail_;

    // 0: input sample position corresponding to the ring buffer tail
    // 1: output sample position
    int64 position_[2];

    // number of channels:
    std::size_t channels_;

    // Hann window coefficients, for feathering
    // (blending) the overlapping fragment region:
    std::vector<float> hann_;

    // fragment window size, power-of-two integer:
    unsigned int window_;

    // tempo scaling factor:
    double tempo_;

    // a snapshot of previous fragment input and output position values
    // captured when the tempo scale factor was set most recently:
    int64 origin_[2];

    // current/previous fragment ring-buffer:
    TAudioFragment frag_[2];

    // current fragment index:
    std::size_t nfrag_;

    // current state:
    TState state_;

    // for fast correlation calculation in frequency domain:
    RDFTContext * realToComplex_;
    RDFTContext * complexToReal_;
    TDataBuffer correlation_;
  };

  //----------------------------------------------------------------
  // TAudioTempoFilterU8
  //
  typedef AudioTempoFilter<unsigned char, 0, 255> TAudioTempoFilterU8;

  //----------------------------------------------------------------
  // TAudioTempoFilterI16
  //
  typedef AudioTempoFilter<short int, -32768, 32767> TAudioTempoFilterI16;

  //----------------------------------------------------------------
  // TAudioTempoFilterI32
  //
  typedef AudioTempoFilter<int, -2147483648, 2147483647>
  TAudioTempoFilterI32;

  //----------------------------------------------------------------
  // TAudioTempoFilterF32
  //
  typedef AudioTempoFilter<float, -1, 1> TAudioTempoFilterF32;

  //----------------------------------------------------------------
  // TAudioTempoFilterF64
  //
  typedef AudioTempoFilter<double, -1, 1> TAudioTempoFilterF64;

}


#endif // YAE_AUDIO_TEMPO_FILTER_H_

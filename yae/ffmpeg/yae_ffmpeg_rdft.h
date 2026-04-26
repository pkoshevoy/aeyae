// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 23 11:46:50 AM MST 2024
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_RDFT_H_
#define YAE_FFMPEG_RDFT_H_


// ffmpeg:
extern "C"
{
#include <libavutil/error.h>
#include <libavutil/tx.h>
}

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // rDFT
  //
  // a warpper for libavutil 1D RDFT
  //
  struct rDFT
  {
    //----------------------------------------------------------------
    // re_t
    //
    typedef float re_t;

    //----------------------------------------------------------------
    // cx_t
    //
    typedef AVComplexFloat cx_t;

    //----------------------------------------------------------------
    // Frame
    //
    struct Frame
    {
      typedef rDFT::re_t re_t;
      typedef rDFT::cx_t cx_t;

      // initialize re and cx buffers:
      inline void init(const rDFT & rdft)
      {
        re_.resize<re_t>(rdft.po2_size());
        re_.memset(0);

        cx_.resize<cx_t>(rdft.po2_size() / 2 + 1);
        cx_.memset(0);
      }

      inline void init(rDFT & rdft,
                       const re_t * samples,
                       std::size_t num_samples)
      {
        this->init(rdft);

        num_samples = std::min<std::size_t>(num_samples, rdft.po2_size());
        memcpy(re_.get<re_t>(), samples, sizeof(re_t) * num_samples);

        // apply rDFT:
        re_t * re = re_.get<re_t>();
        cx_t * cx = cx_.get<cx_t>();
        rdft.r2c(re, cx);
      }

      inline void init(rDFT & rdft, const yae::Data & samples)
      { this->init(rdft, samples.get<re_t>(), samples.num<re_t>()); }

      // should this be yae::Data instead?
      yae::Data re_; // N
      yae::Data cx_; // N / 2 + 1
    };

    //----------------------------------------------------------------
    // rDFT
    //
    rDFT():
      r2c_(NULL),
      c2r_(NULL),
      r2c_tx_(NULL),
      c2r_tx_(NULL),
      po2_size_(0)
    {}

    //----------------------------------------------------------------
    // ~rDFT
    //
    ~rDFT()
    { this->uninit(); }

    //----------------------------------------------------------------
    // init
    //
    // NOTE: actual transform size in (real) samples will be
    // a power of 2, greater or equal to the given size.
    //
    bool init(uint32_t size)
    {
      po2_size_ = yae::get_po2_size(size);

      static const re_t r2c_scale = 1.0f;
      static const re_t c2r_scale = 0.5f;

      int ret = av_tx_init(&r2c_,
                           &r2c_tx_,
                           AV_TX_FLOAT_RDFT,
                           false, // it's a forward transform
                           po2_size_,
                           &r2c_scale, // for rescaling the output
                           0); // flags
      YAE_ASSERT_NO_AVERROR_OR_RETURN(ret, false);

      ret = av_tx_init(&c2r_,
                       &c2r_tx_,
                       AV_TX_FLOAT_RDFT,
                       true, // it's an inverse transform
                       po2_size_,
                       &c2r_scale, // for rescaling the output
                       0); // flags
      YAE_ASSERT_NO_AVERROR_OR_RETURN(ret, false);

      buffer_.re_.resize<rDFT::re_t>(po2_size_);
      buffer_.cx_.resize<rDFT::cx_t>(po2_size_ / 2 + 1);
      return true;
    }

    //----------------------------------------------------------------
    // uninit
    //
    void uninit()
    {
      av_tx_uninit(&r2c_);
      av_tx_uninit(&c2r_);
      r2c_tx_ = NULL;
      c2r_tx_ = NULL;
      po2_size_ = 0;
    }

    // accessors:
    inline uint32_t po2_size() const
    { return po2_size_; }

    inline yae::Data & re_buffer()
    { return buffer_.re_; }

    inline yae::Data & cx_buffer()
    { return buffer_.cx_; }

    //----------------------------------------------------------------
    // r2c
    //
    // NOTE: both input and output array pointers must be
    // aligned to av_cpu_max_align()
    //
    inline void r2c(// input N real samples:
                    const re_t * src,
                    // output N/2 + 1 complex samples:
                    cx_t * dst)
    { r2c_tx_(r2c_, dst, const_cast<re_t *>(src), sizeof(re_t)); }

    //----------------------------------------------------------------
    // c2r
    //
    // NOTE: both input and output array pointers must be
    // aligned to av_cpu_max_align()
    //
    // NOTE: the inverse transform always overwrites the input.
    //
    inline void c2r(// input N/2 + 1 complex samples:
                    cx_t * src,
                    // output N real samples:
                    re_t * dst)
    { c2r_tx_(c2r_, dst, src, sizeof(cx_t)); }

  protected:
    rDFT::Frame buffer_;

    AVTXContext * r2c_;
    AVTXContext * c2r_;

    av_tx_fn r2c_tx_;
    av_tx_fn c2r_tx_;

    uint32_t po2_size_; // N
  };

}


#endif // YAE_FFMPEG_RDFT_H_

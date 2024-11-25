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
#include <libavutil/tx.h>
}

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // rdft_t
  //
  // a warpper for libavutil 1D RDFT
  //
  struct rdft_t
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
    // rdft_t
    //
    rdft_t():
      r2c_(NULL),
      c2r_(NULL),
      r2c_tx_(NULL),
      c2r_tx_(NULL),
      n_levels_(0),
      po2_size_(0)
    {}

    //----------------------------------------------------------------
    // ~rdft_t
    //
    ~rdft_t()
    { this->uninit(); }

    //----------------------------------------------------------------
    // init
    //
    // NOTE: actual transform size in (real) samples will be
    // a power of 2, greater or equal to the given size.
    //
    bool init(uint32_t size)
    {
      n_levels_ = 0;
      po2_size_ = 1;

      while (po2_size_ < size)
      {
        ++n_levels_;
        po2_size_ <<= 1;
      }

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

      re_buffer_.resize<rdft_t::re_t>(po2_size_);
      cx_buffer_.resize<rdft_t::cx_t>(po2_size_ / 2 + 1);
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
      n_levels_ = 0;
      po2_size_ = 0;
    }

    // accessors:
    inline uint32_t n_levels() const
    { return n_levels_; }

    inline uint32_t po2_size() const
    { return po2_size_; }

    inline TDataBuffer & re_buffer()
    { return re_buffer_; }

    inline TDataBuffer & cx_buffer()
    { return cx_buffer_; }

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
    AVTXContext * r2c_;
    AVTXContext * c2r_;

    av_tx_fn r2c_tx_;
    av_tx_fn c2r_tx_;

    uint32_t n_levels_; // log2(N)
    uint32_t po2_size_; // N

    TDataBuffer re_buffer_; // N
    TDataBuffer cx_buffer_; // N / 2 + 1
  };

}


#endif // YAE_FFMPEG_RDFT_H_

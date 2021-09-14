// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Aug 30 20:34:33 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXTURE_GENERATOR_H_
#define YAE_TEXTURE_GENERATOR_H_

// ffmpeg:
extern "C"
{
#include <libavutil/pixfmt.h>
}

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_linear_algebra.h"
#include "yae/video/yae_colorspace.h"


namespace yae
{
  //----------------------------------------------------------------
  // TextureGenerator
  //
  struct YAE_API TextureGenerator
  {
    TextureGenerator(int luma_w,
                     int luma_h,
                     const Colorspace * csp = NULL);
    virtual ~TextureGenerator() {}

    virtual void set_offset(uint32_t offset);
    virtual v3x1_t get_ypbpr(int row, int col) const;
    virtual v3x1_t get_rgb(int row, int col) const = 0;

    inline const Colorspace * colorspace() const
    { return csp_; }

    const Colorspace * csp_;
    const unsigned int luma_w_;
    const unsigned int luma_h_;

  protected:
    unsigned int offset_;
  };


  //----------------------------------------------------------------
  // ColorbarsGenerator
  //
  // ARIB STD-B28 Version 1.0
  //
  struct YAE_API ColorbarsGenerator : TextureGenerator
  {
    ColorbarsGenerator(int luma_w,
                       int luma_h,
                       const Colorspace * csp = NULL);

    // virtual:
    v3x1_t get_ypbpr(int row, int col) const;
    v3x1_t get_rgb(int row, int col) const;

    const unsigned int ww_;
    const unsigned int hh_;
  };

  //----------------------------------------------------------------
  // ChromaWheelGenerator
  //
  struct YAE_API ChromaWheelGenerator : TextureGenerator
  {
    ChromaWheelGenerator(int luma_w,
                         int luma_h,
                         const Colorspace * csp = NULL);

    // virtual:
    void set_offset(unsigned int offset);
    v3x1_t get_rgb(int row, int col) const;

  protected:
    m3x3_t rotate_chroma_;
  };

  //----------------------------------------------------------------
  // ShadedSquaresGenerator
  //
  struct YAE_API ShadedSquaresGenerator : TextureGenerator
  {
    ShadedSquaresGenerator(int luma_w,
                           int luma_h,
                           const Colorspace * csp = NULL);

    // virtual:
    v3x1_t get_rgb(int row, int col) const;

    const unsigned int ww_;
    const unsigned int hh_;
  };

}


#endif // YAE_TEXTURE_GENERATOR_H_

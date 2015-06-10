// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_color.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The color class.

// local includes:
#include "math/the_color.hxx"
#include "utils/the_indentation.hxx"

// system includes:
#include <assert.h>
#include <algorithm>


//----------------------------------------------------------------
// the_the_color_t::
//
//
const the_color_t the_color_t::RED(0xff0000);
const the_color_t the_color_t::GREEN(0x00ff00);
const the_color_t the_color_t::BLUE(0x0000ff);
const the_color_t the_color_t::CYAN(0x00ffff);
const the_color_t the_color_t::MAGENTA(0xff00ff);
const the_color_t the_color_t::YELLOW(0xffff00);
const the_color_t the_color_t::BLACK(0x000000);
const the_color_t the_color_t::WHITE(0xffffff);
const the_color_t the_color_t::ORANGE(0xff7f00);
const the_color_t the_color_t::GRAPE(0x600496);
const the_color_t the_color_t::BROWN(0x913a08);

#if 0
const the_color_t the_color_t::GOLD(0xe6cf5c);
#else
const the_color_t the_color_t::GOLD(0xfdf0ce);
#endif

#if 1
const the_color_t the_color_t::AMPAD_DARK(0x55af7f);
const the_color_t the_color_t::AMPAD_LIGHT(0xe2efba);
const the_color_t the_color_t::AMPAD_PENCIL(0x525439);
#elif 0
const the_color_t the_color_t::AMPAD_DARK(0x1eb0fe);
const the_color_t the_color_t::AMPAD_LIGHT(0xe6e2e0);
const the_color_t the_color_t::AMPAD_PENCIL(0x494c5a);
#elif 0
const the_color_t the_color_t::AMPAD_DARK(0x7ba4cf);
const the_color_t the_color_t::AMPAD_LIGHT(0xeeee98);
const the_color_t the_color_t::AMPAD_PENCIL(0x504d30);
#elif 0
const the_color_t the_color_t::AMPAD_DARK(0xaf6bde);
const the_color_t the_color_t::AMPAD_LIGHT(0xfeeab8);
const the_color_t the_color_t::AMPAD_PENCIL(0x8a8662);
#elif 0
const the_color_t the_color_t::AMPAD_DARK(0x6ec8ee);
const the_color_t the_color_t::AMPAD_LIGHT(0xc0dedd);
const the_color_t the_color_t::AMPAD_PENCIL(0x4f585e);
#elif 0
const the_color_t the_color_t::AMPAD_DARK(0x494949);
const the_color_t the_color_t::AMPAD_LIGHT(0x828282);
const the_color_t the_color_t::AMPAD_PENCIL(0xffffff);
#endif

const the_color_t the_color_t::LIGHT_GREY(0xc0c0c0);
const the_color_t the_color_t::GREY(0x7f7f7f);
const the_color_t the_color_t::BLANK(0x000000, 0.0);


//----------------------------------------------------------------
// the_color_t::the_color_t
//
the_color_t::the_color_t(unsigned int rgb, float alpha)
{
  rgba_[0] = float((rgb >> 16) & 0xFF) / 255.0f;
  rgba_[1] = float((rgb >> 8) & 0xFF) / 255.0f;
  rgba_[2] = float(rgb & 0xFF) / 255.0f;
  rgba_[3] = alpha;
}

//----------------------------------------------------------------
// the_color_t::normalize
//
void
the_color_t::normalize()
{
  float s = std::max(rgba_[0], std::max(rgba_[1], rgba_[2]));
  if (s == 0.0f) return;

  float inv_s = 1.0f / s;
  rgba_[0] *= inv_s;
  rgba_[1] *= inv_s;
  rgba_[2] *= inv_s;
}

//----------------------------------------------------------------
// the_color_t::dielectric_attenuation_to_color
//
void
the_color_t::dielectric_attenuation_to_color(the_color_t & color) const
{
  static const float offset = 0.001f;
  static const float scale = 1.0f + offset;

  color.rgba_[0] = expf(rgba_[0] * scale - offset);
  color.rgba_[1] = expf(rgba_[1] * scale - offset);
  color.rgba_[2] = expf(rgba_[2] * scale - offset);
}

//----------------------------------------------------------------
// the_color_t::color_to_dielectric_attenuation
//
void
the_color_t::color_to_dielectric_attenuation(the_color_t & attenuation) const
{
  static const float offset = 0.001f;
  static const float scale_inverse = 1.0f / (1.0f + offset);

  attenuation.rgba_[0] = logf((rgba_[0] + offset) * scale_inverse);
  attenuation.rgba_[1] = logf((rgba_[1] + offset) * scale_inverse);
  attenuation.rgba_[2] = logf((rgba_[2] + offset) * scale_inverse);
}

//----------------------------------------------------------------
// the_color_t::dump
//
void
the_color_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_color_t(" << (void *)this << ") rgba_ = [ ";
  for (unsigned int i = 0; i < 4; i++)
  {
    strm << rgba_[i] << ' ';
  }
  strm << "]" << endl << endl;
}

//----------------------------------------------------------------
// hsv_to_rgb
//
the_color_t
hsv_to_rgb(const the_color_t & HSV)
{
  float H = HSV[0];
  float S = HSV[1];
  float V = HSV[2];

  the_color_t RGB;
  float & R = RGB[0];
  float & G = RGB[1];
  float & B = RGB[2];

  if (S == 0.0f)
  {
    // monochromatic:
    R = V;
    G = V;
    B = V;
    return RGB;
  }

  H *= 6.0;
  float i = floor(H);
  float f = H - i;

  float p = V * (1.0f - S);
  float q = V * (1.0f - S * f);
  float t = V * (1.0f - S * (1.0f - f));

  if (i == 0.0f)
  {
    R = V;
    G = t;
    B = p;
  }
  else if (i == 1.0f)
  {
    R = q;
    G = V;
    B = p;
  }
  else if (i == 2.0f)
  {
    R = p;
    G = V;
    B = t;
  }
  else if (i == 3.0f)
  {
    R = p;
    G = q;
    B = V;
  }
  else if (i == 4.0f)
  {
    R = t;
    G = p;
    B = V;
  }
  else
  {
    // i == 5.0f
    R = V;
    G = p;
    B = q;
  }

  return RGB;
}

//----------------------------------------------------------------
// rgb_to_hsv
//
the_color_t
rgb_to_hsv(const the_color_t & RGB)
{
  float R = RGB[0];
  float G = RGB[1];
  float B = RGB[2];

  the_color_t HSV;
  float & H = HSV[0];
  float & S = HSV[1];
  float & V = HSV[2];

  float min = std::min(R, std::min(G, B));
  float max = std::max(R, std::max(G, B));
  V = max;

  float delta = max - min;
  if (max == 0)
  {
    S = 0;
    H = -1;
  }
  else
  {
    S = delta / max;

    if (delta == 0)
    {
      delta = 1;
    }

    if (R == max)
    {
      // between yellow & magenta
      H = (G - B) / delta;
    }
    else if (G == max)
    {
      // between cyan & yellow
      H = (B - R) / delta + 2;
    }
    else
    {
      // between magenta & cyan
      H = (R - G) / delta + 4;
    }

    H /= 6.0f;

    if (H < 0.0f)
    {
      H = H + 1.0f;
    }
  }

  return HSV;
}

//----------------------------------------------------------------
// make_rainbow
//
void
make_rainbow(const unsigned int & num_colors,
	     the_color_t * colors,
	     const bool scrambled,
	     const float scale)
{
  static const the_color_t EAST  = the_color_t(1, 0, 0);
  static const the_color_t NORTH = the_color_t(0, 1, 0);
  static const the_color_t WEST  = the_color_t(0, 0, 1);
  static const the_color_t SOUTH = the_color_t(0, 0, 0);

  if (scrambled)
  {
    for (unsigned int i = 0; i < num_colors; i++)
    {
      float t = fmodf(float(i % 2) / 2.0f +
		      float(i) / float(num_colors - 1), 1.0f);

      float s = 0.5f + 0.5f * fmodf(float((i + 1) % 3) / 3.0f +
				    float(i) / float(num_colors - 1), 1.0f);
      colors[i] = hsv_to_rgb(the_color_t(t, s, 1.0f)) * scale;
    }
  }
  else
  {
    for (unsigned int i = 0; i < num_colors; i++)
    {
      float t = float(i) / float(num_colors);
      colors[i] = hsv_to_rgb(the_color_t(t, 1.0f, 1.0f)) * scale;
    }
  }
}


//----------------------------------------------------------------
// the_rgba_word_t::dump
//
void
the_rgba_word_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_rgba_word_t(" << (void *)this
       << ") data_ ="
       << "  r: " << (void *)(size_t(data_[0]))
       << ", g: " << (void *)(size_t(data_[1]))
       << ", b: " << (void *)(size_t(data_[2]))
       << ", a: " << (void *)(size_t(data_[3]))
       << endl;
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_color.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The color class.

#ifndef THE_COLOR_HXX_
#define THE_COLOR_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"

// system includes:
#include <algorithm>


//----------------------------------------------------------------
// the_color_t
// 
class the_color_t
{
public:
  the_color_t()
  { rgba_[0] = 0.0; rgba_[1] = 0.0; rgba_[2] = 0.0; rgba_[3] = 1.0; }
  
  the_color_t(float r, float g, float b, float a = 1)
  { rgba_[0] = r; rgba_[1] = g; rgba_[2] = b; rgba_[3] = a; }
  
  the_color_t(unsigned int rgb, float alpha = 1.0);
  
  the_color_t(const the_color_t & c)
  { *this = c; }
  
  ~the_color_t() {}
  
  inline the_color_t & operator = (const the_color_t & c)
  {
    rgba_[0] = c.rgba_[0];
    rgba_[1] = c.rgba_[1];
    rgba_[2] = c.rgba_[2];
    rgba_[3] = c.rgba_[3];
    return *this;
  }
  
  inline the_color_t & operator += (const the_color_t & c)
  {
    rgba_[0] += c.rgba_[0];
    rgba_[1] += c.rgba_[1];
    rgba_[2] += c.rgba_[2];
    rgba_[3] += c.rgba_[3];
    return *this;
  }
  
  inline the_color_t & operator -= (const the_color_t & c)
  {
    rgba_[0] -= c.rgba_[0];
    rgba_[1] -= c.rgba_[1];
    rgba_[2] -= c.rgba_[2];
    rgba_[3] -= c.rgba_[3];
    return *this;
  }
  
  inline the_color_t operator + (const the_color_t & c) const
  {
    the_color_t result(*this);
    result += c;
    return result;
  }
  
  inline the_color_t operator - (const the_color_t & c) const
  {
    the_color_t result(*this);
    result -= c;
    return result;
  }
  
  inline the_color_t & operator *= (const the_color_t & c)
  {
    rgba_[0] *= c.rgba_[0];
    rgba_[1] *= c.rgba_[1];
    rgba_[2] *= c.rgba_[2];
    rgba_[3] *= c.rgba_[3];
    return *this;
  }
  
  inline the_color_t operator * (const the_color_t & c) const
  {
    the_color_t result(*this);
    result *= c;
    return result;
  }
  
  inline the_color_t & operator *= (const float & scale)
  {
    rgba_[0] *= scale;
    rgba_[1] *= scale;
    rgba_[2] *= scale;
    rgba_[3] *= scale;
    return *this;
  }
  
  inline the_color_t operator * (const float & scale) const
  {
    the_color_t result(*this);
    result *= scale;
    return result;
  }
  
  inline the_color_t operator ! () const
  {
    the_color_t result(*this);
    result.normalize();
    return result;
  }
  
  // scale this color so that its largest channel (not alpha) is set to 1.0:
  void normalize();
  
  inline bool operator == (const the_color_t & c) const
  {
    return (rgba_[0] == c.rgba_[0] &&
	    rgba_[1] == c.rgba_[1] &&
	    rgba_[2] == c.rgba_[2] &&
	    rgba_[3] == c.rgba_[3]);
  }
  
  inline bool operator < (const the_color_t & c) const
  {
    return (rgba_[0] <= c.rgba_[0] &&
	    rgba_[1] <= c.rgba_[1] &&
	    rgba_[2] <= c.rgba_[2] &&
	    rgba_[3] <= c.rgba_[3] &&
	    !(*this == c));
  }
  
  void dielectric_attenuation_to_color(the_color_t & color) const;
  void color_to_dielectric_attenuation(the_color_t & attenuation) const;
  
  inline the_color_t dielectric_attenuation_to_color() const
  {
    the_color_t color;
    dielectric_attenuation_to_color(color);
    return color;
  }
  
  inline the_color_t color_to_dielectric_attenuation() const
  {
    the_color_t attenuation;
    color_to_dielectric_attenuation(attenuation);
    return attenuation;
  }
  
  inline the_color_t mul3(const float & scale) const
  {
    the_color_t result(*this);
    result.rgba_[0] *= scale;
    result.rgba_[1] *= scale;
    result.rgba_[2] *= scale;
    return result;
  }
  
  inline the_color_t mul3(const the_color_t & c) const
  {
    the_color_t result(*this);
    result.rgba_[0] *= c.rgba_[0];
    result.rgba_[1] *= c.rgba_[1];
    result.rgba_[2] *= c.rgba_[2];
    return result;
  }
  
  inline the_color_t add3(const the_color_t & c) const
  {
    the_color_t result(*this);
    result.rgba_[0] += c.rgba_[0];
    result.rgba_[1] += c.rgba_[1];
    result.rgba_[2] += c.rgba_[2];
    return result;
  }
  
  inline the_color_t sub3(const the_color_t & c) const
  {
    the_color_t result(*this);
    result.rgba_[0] -= c.rgba_[0];
    result.rgba_[1] -= c.rgba_[1];
    result.rgba_[2] -= c.rgba_[2];
    return result;
  }
  
  // accessors:
  inline float * rgba()
  { return rgba_; }
  
  inline const float * rgba() const
  { return rgba_; }
  
  inline float r() const { return rgba_[0]; }
  inline float g() const { return rgba_[1]; }
  inline float b() const { return rgba_[2]; }
  inline float a() const { return rgba_[3]; }
  
  inline float & r() { return rgba_[0]; }
  inline float & g() { return rgba_[1]; }
  inline float & b() { return rgba_[2]; }
  inline float & a() { return rgba_[3]; }
  
  // more accessors to the individual color channels:
  inline float operator [] (unsigned int channel) const
  { return rgba_[channel]; }
  
  inline float & operator [] (unsigned int channel)
  { return rgba_[channel]; }
  
  // For debugging, dumps the color:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
private:
  // red-green-blue-alpha (greater or equal to 0, less then or equal to 1):
  float rgba_[4];
  
public:
  // some predefined colors:
  static const the_color_t RED;
  static const the_color_t GREEN;
  static const the_color_t BLUE;
  static const the_color_t CYAN;
  static const the_color_t MAGENTA;
  static const the_color_t YELLOW;
  static const the_color_t BLACK;
  static const the_color_t WHITE;
  static const the_color_t ORANGE;
  static const the_color_t GRAPE;
  static const the_color_t BROWN;
  static const the_color_t GOLD;
  static const the_color_t AMPAD_DARK;
  static const the_color_t AMPAD_LIGHT;
  static const the_color_t AMPAD_PENCIL;
  static const the_color_t LIGHT_GREY;
  static const the_color_t GREY;
  static const the_color_t BLANK;
};

//----------------------------------------------------------------
// operator *
// 
inline the_color_t
operator * (float scale, const the_color_t & c)
{
  return c * scale;
}

//----------------------------------------------------------------
// operator <<
// 
inline ostream &
operator << (ostream & strm, const the_color_t & c)
{
  c.dump(strm);
  return strm;
}

//----------------------------------------------------------------
// hsv_to_rgb
// 
extern the_color_t hsv_to_rgb(const the_color_t & HSV);

//----------------------------------------------------------------
// rgb_to_hsv
// 
extern the_color_t rgb_to_hsv(const the_color_t & RGB);

//----------------------------------------------------------------
// make_rainbow
// 
extern void
make_rainbow(const unsigned int & num_colors,
	     the_color_t * colors,
	     const bool scrambled = true,
	     const float scale = 1.0);


//----------------------------------------------------------------
// the_rgba_word_t
// 
class the_rgba_word_t
{
public:
  the_rgba_word_t(unsigned char r = 0x00,
		  unsigned char g = 0x00,
		  unsigned char b = 0x00,
		  unsigned char a = 0xFF)
  { setup(r, g, b, a); }
  
  the_rgba_word_t(const the_color_t & rgba)
  {
    setup((unsigned char)std::max(0.0, std::min(255.0, 255.0 * rgba.r())),
	  (unsigned char)std::max(0.0, std::min(255.0, 255.0 * rgba.g())),
	  (unsigned char)std::max(0.0, std::min(255.0, 255.0 * rgba.b())),
	  (unsigned char)std::max(0.0, std::min(255.0, 255.0 * rgba.a())));
  }
  
  inline bool operator == (const the_rgba_word_t & rgba) const
  {
    return (data_[0] == rgba.data_[0] &&
	    data_[1] == rgba.data_[1] &&
	    data_[2] == rgba.data_[2] &&
	    data_[3] == rgba.data_[3]);
  }
  
  inline bool operator < (const the_rgba_word_t & rgba) const
  {
    double i0 = (double(r()) +
		double(g()) +
		double(b())) * double(a()) / 3.0;
    double i1 = (double(rgba.r()) +
		double(rgba.g()) +
		double(rgba.b())) * double(a()) / 3.0;
    
    return i0 < i1;
  }
  
  // constructor helper function:
  inline void setup(unsigned char r,
		    unsigned char g,
		    unsigned char b,
		    unsigned char a)
  {
    data_[0] = r;
    data_[1] = g;
    data_[2] = b;
    data_[3] = a;
  }
  
  // accessor:
  const unsigned char * data() const
  { return data_; }
  
  // color component accessors:
  inline unsigned char r() const { return data_[0]; }
  inline unsigned char g() const { return data_[1]; }
  inline unsigned char b() const { return data_[2]; }
  inline unsigned char a() const { return data_[3]; }
  
  inline void setR(unsigned char r) { data_[0] = r; }
  inline void setG(unsigned char g) { data_[1] = g; }
  inline void setB(unsigned char b) { data_[2] = b; }
  inline void setA(unsigned char a) { data_[3] = a; }
  
  // return color in format 0xAABBGGRR:
  inline unsigned int rgba() const
  {
    unsigned int rgba_ = r() | (g() << 8) | (b() << 16) | (a() << 24);
    return rgba_;
  }
  
  // return color in format 0xRRGGBBAA:
  inline unsigned int abgr() const
  {
    unsigned int abgr_ = a() | (b() << 8) | (g() << 16) | (r() << 24);
    return abgr_;
  }

  // return color in format 0xAARRGGBB:
  inline unsigned int bgra() const
  {
    unsigned int bgra_ = b() | (g() << 8) | (r() << 16) | (a() << 24);
    return bgra_;
  }
  
  // return color in format 0xAARRGGBB, where RGB are scaled by AA/255:
  inline unsigned int bgra_premultiplied() const
  {
    float a_ = float(a()) / 255;
    float r_ = a_ * float(r()) / 255;
    float g_ = a_ * float(g()) / 255;
    float b_ = a_ * float(b()) / 255;
    return the_rgba_word_t(the_color_t(r_, g_, b_, a_)).bgra();
  }
  
  // For debugging, dumps the color:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
private:
  // the word:
  unsigned char data_[4];
};

//----------------------------------------------------------------
// operator <<
// 
inline ostream &
operator << (ostream & strm, const the_rgba_word_t & rgba)
{
  rgba.dump(strm);
  return strm;
}


#endif // THE_COLOR_HXX_

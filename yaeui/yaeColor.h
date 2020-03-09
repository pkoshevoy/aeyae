// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_COLOR_H_
#define YAE_COLOR_H_

// standard libraries:
#include <algorithm>
#include <map>

// Qt library:
#if defined(YAE_USE_QT4) || defined(YAE_USE_QT5)
#include <QColor>
#endif

// yae includes:
#include "yae/api/yae_api.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_utils.h"

// local interfaces:
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // TVec4D
  //
  struct TVec4D : public Vec<double, 4>
  {
    typedef Vec<double, 4> TBase;

    inline TVec4D(double a = 0.0,
                  double r = 0.0,
                  double g = 0.0,
                  double b = 0.0)
    {
      TBase::coord_[0] = a;
      TBase::coord_[1] = r;
      TBase::coord_[2] = g;
      TBase::coord_[3] = b;
    }

    inline TVec4D(const TBase & v): TBase(v) {}
    inline TVec4D & operator = (const TBase & v)
    {
      TBase::operator = (v);
      return *this;
    }

    inline const double & a() const { return TBase::coord_[0]; }
    inline const double & r() const { return TBase::coord_[1]; }
    inline const double & g() const { return TBase::coord_[2]; }
    inline const double & b() const { return TBase::coord_[3]; }

    inline double & a() { return TBase::coord_[0]; }
    inline double & r() { return TBase::coord_[1]; }
    inline double & g() { return TBase::coord_[2]; }
    inline double & b() { return TBase::coord_[3]; }

    inline TVec4D & set_a(double v) { TBase::coord_[0] = v; return *this; }
    inline TVec4D & set_r(double v) { TBase::coord_[1] = v; return *this; }
    inline TVec4D & set_g(double v) { TBase::coord_[2] = v; return *this; }
    inline TVec4D & set_b(double v) { TBase::coord_[3] = v; return *this; }

    inline TVec4D & operator *= (const TVec4D & scale)
    {
      TBase::operator *= (scale);
      return *this;
    }

    inline TVec4D operator * (const TVec4D & s) const
    {
      TVec4D v(*this);
      v *= s;
      return v;
    }

    inline TVec4D & operator += (const TVec4D & translate)
    {
      TBase::operator += (translate);
      return *this;
    }

    inline TVec4D operator + (const TVec4D & t) const
    {
      TVec4D v(*this);
      v += t;
      return v;
    }

    inline TVec4D & clamp(double t0 = 0.0, double t1 = 1.0)
    {
      TBase::coord_[0] = std::min(t1, std::max(t0, TBase::coord_[0]));
      TBase::coord_[1] = std::min(t1, std::max(t0, TBase::coord_[1]));
      TBase::coord_[2] = std::min(t1, std::max(t0, TBase::coord_[2]));
      TBase::coord_[3] = std::min(t1, std::max(t0, TBase::coord_[3]));
      return *this;
    }

    inline TVec4D clamped(double t0 = 0.0, double t1 = 1.0) const
    {
      TVec4D v(*this);
      v.clamp(t0, t1);
      return v;
    }
  };

  //----------------------------------------------------------------
  // Color
  //
  struct Color
  {
    Color(unsigned int rgb = 0, double a = 1.0):
      argb_(rgb)
    {
      this->a() = (unsigned char)(std::max(0.0, std::min(255.0, 255.0 * a)));
    }

    Color(const TVec4D & v)
    {
      YAE_ASSERT(v.coord_[0] >= 0.0 && v.coord_[0] <= 1.0);
      YAE_ASSERT(v.coord_[1] >= 0.0 && v.coord_[1] <= 1.0);
      YAE_ASSERT(v.coord_[2] >= 0.0 && v.coord_[2] <= 1.0);
      YAE_ASSERT(v.coord_[3] >= 0.0 && v.coord_[3] <= 1.0);

      this->operator[](0) = (unsigned char)(v.coord_[0] * 255.0);
      this->operator[](1) = (unsigned char)(v.coord_[1] * 255.0);
      this->operator[](2) = (unsigned char)(v.coord_[2] * 255.0);
      this->operator[](3) = (unsigned char)(v.coord_[3] * 255.0);
    }

    inline operator TVec4D() const
    {
      TVec4D v;
      v.coord_[0] = double(this->operator[](0)) / 255.0;
      v.coord_[1] = double(this->operator[](1)) / 255.0;
      v.coord_[2] = double(this->operator[](2)) / 255.0;
      v.coord_[3] = double(this->operator[](3)) / 255.0;
      return v;
    }

#if defined(YAE_USE_QT4) || defined(YAE_USE_QT5)
    Color(const QColor & c)
    {
      set_a((unsigned char)(c.alpha()));
      set_r((unsigned char)(c.red()));
      set_g((unsigned char)(c.green()));
      set_b((unsigned char)(c.blue()));
    }

    inline operator QColor() const
    {
      return QColor(r(), g(), b(), a());
    }
#endif

    inline const unsigned char & a() const { return this->operator[](0); }
    inline const unsigned char & r() const { return this->operator[](1); }
    inline const unsigned char & g() const { return this->operator[](2); }
    inline const unsigned char & b() const { return this->operator[](3); }

    inline unsigned char & a() { return this->operator[](0); }
    inline unsigned char & r() { return this->operator[](1); }
    inline unsigned char & g() { return this->operator[](2); }
    inline unsigned char & b() { return this->operator[](3); }

    inline Color & set_a(unsigned char v) { a() = v; return *this; }
    inline Color & set_r(unsigned char v) { r() = v; return *this; }
    inline Color & set_g(unsigned char v) { g() = v; return *this; }
    inline Color & set_b(unsigned char v) { b() = v; return *this; }

    inline Color & set_a(double v) { return set_a((unsigned char)(v * 255)); }
    inline Color & set_r(double v) { return set_r((unsigned char)(v * 255)); }
    inline Color & set_g(double v) { return set_g((unsigned char)(v * 255)); }
    inline Color & set_b(double v) { return set_b((unsigned char)(v * 255)); }

    inline const unsigned char & operator[] (unsigned int i) const
    {
      const unsigned char * argb = (const unsigned char *)&argb_;
#if __BIG_ENDIAN__
      return argb[i];
#else
      return argb[3 - i];
#endif
    }

    inline unsigned char & operator[] (unsigned int i)
    {
      unsigned char * argb = (unsigned char *)&argb_;
#if __BIG_ENDIAN__
      return argb[i];
#else
      return argb[3 - i];
#endif
    }

    Color & operator *= (double scale)
    {
      unsigned char * argb = (unsigned char *)&argb_;
      argb[0] = (unsigned char)(std::min(255.0, double(argb[0]) * scale));
      argb[1] = (unsigned char)(std::min(255.0, double(argb[1]) * scale));
      argb[2] = (unsigned char)(std::min(255.0, double(argb[2]) * scale));
      argb[3] = (unsigned char)(std::min(255.0, double(argb[3]) * scale));
      return *this;
    }

    Color & operator += (double translate)
    {
      unsigned char * argb = (unsigned char *)&argb_;

      argb[0] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[0]) + translate)));

      argb[1] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[1]) + translate)));

      argb[2] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[2]) + translate)));

      argb[3] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[3]) + translate)));

      return *this;
    }

    inline static unsigned char
    transform(unsigned char c, double s, double t = 0.0)
    {
      double v = s * double(c) + t * 255.0;
      return (unsigned char)(std::max(0.0, std::min(255.0, v)));
    }

    inline Color & scale_alpha(double s, double t = 0.0)
    {
      unsigned char sa = Color::transform(this->a(), s, t);
      this->set_a(sa);
      return *this;
    }

    inline Color a_scaled(double s, double t = 0.0) const
    {
      Color result(*this);
      result.scale_alpha(s, t);
      return result;
    }

    inline Color transparent() const
    {
      return a_scaled(0.0);
    }

    inline Color opaque(double alpha = 1.0) const
    {
      Color result(*this);
      result.set_a(alpha);
      return result;
    }

    Color bw_contrast() const
    {
      TVec4D v(*this);
      double m = std::max(std::max(v.r(), v.g()), v.b());
      double c = (m < 0.5) ? 1.0 : 0.0;
      v.set_r(c);
      v.set_g(c);
      v.set_b(c);
      return Color(v);
    }

    Color premultiplied_transparent() const
    {
      TVec4D v(*this);
      double a = v.a();
      v.set_r(v.r() * a);
      v.set_g(v.g() * a);
      v.set_b(v.b() * a);
      v.set_a(0.0);
      return Color(v);
    }

    unsigned int argb_;
  };

  //----------------------------------------------------------------
  // TGradient
  //
  typedef std::map<double, Color> TGradient;

  //----------------------------------------------------------------
  // TGradientPtr
  //
  typedef yae::shared_ptr<TGradient> TGradientPtr;

  //----------------------------------------------------------------
  // make_gradient
  //
  // see https://github.com/d3/d3-scale-chromatic
  //
  inline static void
  make_gradient(TGradient & g,
                // d3.schemeCategory10 from
                // https://github.com/d3/d3-scale-chromatic/
                // blob/master/src/categorical/category10.js
                const char * colors =
                "1f77b4"
                "ff7f0e"
                "2ca02c"
                "d62728"
                "9467bd"
                "8c564b"
                "e377c2"
                "7f7f7f"
                "bcbd22"
                "17becf")
  {
    g.clear();

    std::size_t l = strlen(colors);
    YAE_ASSERT(l % 6 == 0);

    std::size_t n = l / 6;
    for (std::size_t i = 0; i < n; i++)
    {
      double t = double(i) / double(n - 1);
      g[t] = Color(hex_to_u24(colors + i * 6));
    }
  }

  //----------------------------------------------------------------
  // make_gradient
  //
  inline static TGradient
  make_gradient(// d3.schemeCategory10 from
                // https://github.com/d3/d3-scale-chromatic/
                // blob/master/src/categorical/category10.js
                const char * colors =
                "1f77b4"
                "ff7f0e"
                "2ca02c"
                "d62728"
                "9467bd"
                "8c564b"
                "e377c2"
                "7f7f7f"
                "bcbd22"
                "17becf")
  {
    TGradient g;
    make_gradient(g, colors);
    return g;
  }

  //----------------------------------------------------------------
  // get_ordinal
  //
  inline const Color & get_ordinal(const TGradient & g, std::size_t i)
  {
    std::size_t z = g.size();
    if (g.empty())
    {
      throw std::runtime_error("invalid gradient color map");
    }

    std::size_t x = i % z;
    double t = double(x) / double(z - 1);
    TGradient::const_iterator found = g.lower_bound(t);
    if (found == g.end())
    {
      throw std::runtime_error("invalid ordinal color map");
    }

    return found->second;
  }

}


#endif // YAE_COLOR_H_

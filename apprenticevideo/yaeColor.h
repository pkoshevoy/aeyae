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

// boost includes:
#include <boost/shared_ptr.hpp>

// Qt library:
#include <QColor>

// yae includes:
#include "yae/api/yae_api.h"

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

    inline TVec4D & operator += (const TVec4D & translate)
    {
      TBase::operator += (translate);
      return *this;
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

    Color scale_a(double s, double t = 0.0) const
    {
      Color result(*this);
      double sa = s * double(this->a()) + t * 255.0;
      result.set_a((unsigned char)(std::max(0.0, std::min(255.0, sa))));
      return result;
    }

    inline Color transparent() const
    {
      return scale_a(0.0);
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
  typedef boost::shared_ptr<TGradient> TGradientPtr;

}


#endif // YAE_COLOR_H_

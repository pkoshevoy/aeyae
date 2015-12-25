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

// Qt library:
#include <QColor>

// yae includes:
#include "yae/api/yae_api.h"

// local interfaces:
#include "yaeVec.h"


namespace yae
{

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

    Color(const Vec<double, 4> & v)
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

    inline operator Vec<double, 4>() const
    {
      Vec<double, 4> v;
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

    unsigned int argb_;
  };
}


#endif // YAE_COLOR_H_

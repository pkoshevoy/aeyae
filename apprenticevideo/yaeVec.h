// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VEC_H_
#define YAE_VEC_H_

// standard libraries:
#include <algorithm>
#include <cmath>
#include <limits>


namespace yae
{

  //----------------------------------------------------------------
  // Vec
  //
  template <typename TData, unsigned int Cardinality>
  struct Vec
  {
    enum { kCardinality = Cardinality };
    enum { kDimension = Cardinality };
    typedef TData value_type;
    typedef Vec<TData, Cardinality> TVec;
    TData coord_[Cardinality];

    inline TVec & operator *= (const TData & scale)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] *= scale;
      }

      return *this;
    }

    inline TVec operator * (const TData & scale) const
    {
      TVec result(*this);
      result *= scale;
      return result;
    }

    inline TVec & operator += (const TData & normDelta)
    {
      TData n0 = norm();
      if (n0 > 0.0)
      {
        TData n1 = n0 + normDelta;
        TData scale = n1 / n0;
        return this->operator *= (scale);
      }

      const TData v = normDelta / std::sqrt(TData(Cardinality));
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] = v;
      }
      return *this;
    }

    inline TVec operator + (const TData & normDelta) const
    {
      TVec result(*this);
      result += normDelta;
      return result;
    }

    inline TVec & operator -= (const TData & normDelta)
    {
      return this->operator += (-normDelta);
    }

    inline TVec operator - (const TData & normDelta) const
    {
      return this->operator + (-normDelta);
    }

    inline TData operator * (const TVec & other) const
    {
      TData result = TData(0);

      for (unsigned int i = 0; i < Cardinality; i++)
      {
        result += (coord_[i] * other.coord_[i]);
      }

      return result;
    }

    inline TVec & operator += (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] += other.coord_[i];
      }

      return *this;
    }

    inline TVec operator + (const TVec & other) const
    {
      TVec result(*this);
      result += other;
      return result;
    }

    inline TVec & operator -= (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] -= other.coord_[i];
      }

      return *this;
    }

    inline TVec operator - (const TVec & other) const
    {
      TVec result(*this);
      result -= other;
      return result;
    }

    inline TVec & negate()
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] = -coord_[i];
      }

      return *this;
    }

    inline TVec negated() const
    {
      TVec result(*this);
      result.negate();
      return result;
    }

    inline TData normSqrd() const
    {
      TData result = TData(0);

      for (unsigned int i = 0; i < Cardinality; i++)
      {
        result += (coord_[i] * coord_[i]);
      }

      return result;
    }

    inline TData norm() const
    {
      return std::sqrt(this->normSqrd());
    }

    inline bool
    normalize(const TData & epsilon = std::numeric_limits<TData>::min())
    {
      TData n = this->norm();
      if (n > epsilon)
      {
        this->operator *= (TData(1) / n);
        return true;
      }

      this->operator *= (TData(0));
      return false;
    }

    inline TVec
    normalized(const TData & epsilon = std::numeric_limits<TData>::min()) const
    {
      TVec result(*this);
      result.normalize(epsilon);
      return result;
    }

    inline TVec
    resized(const TData & newsize)
    {
      TVec result(*this);
      double size = result.norm();
      double scale = size ? (newsize / size) : 1.0;
      result *= (scale);
      return result;
    }

    inline TVec & operator < (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        if (!(coord_[i] < other.coord_[i]))
        {
          return false;
        }
      }

      return true;
    }
  };

  //----------------------------------------------------------------
  // operator -
  //
  template <typename TData, unsigned int Cardinality>
  inline static Vec<TData, Cardinality>
  operator - (const Vec<TData, Cardinality> & vec)
  {
    return vec.negated();
  }

  //----------------------------------------------------------------
  // operator *
  //
  template <typename TData, unsigned int Cardinality>
  inline static Vec<TData, Cardinality>
  operator * (const TData & scale, const Vec<TData, Cardinality> & vec)
  {
    return vec * scale;
  }

  //----------------------------------------------------------------
  // TVec2D
  //
  struct TVec2D : public Vec<double, 2>
  {
    typedef Vec<double, 2> TBase;

    inline TVec2D(double x = 0.0, double y = 0.0)
    {
      TBase::coord_[0] = x;
      TBase::coord_[1] = y;
    }

    inline TVec2D(const TBase & v): TBase(v) {}
    inline TVec2D & operator = (const TBase & v)
    {
      TBase::operator = (v);
      return *this;
    }

    inline const double & x() const { return TBase::coord_[0]; }
    inline const double & y() const { return TBase::coord_[1]; }

    inline double & x() { return TBase::coord_[0]; }
    inline double & y() { return TBase::coord_[1]; }

    inline void set_x(double v) { TBase::coord_[0] = v; }
    inline void set_y(double v) { TBase::coord_[1] = v; }
  };

  //----------------------------------------------------------------
  // wcs_to_lcs
  //
  inline TVec2D
  wcs_to_lcs(const TVec2D & origin,
             const TVec2D & u_axis,
             const TVec2D & v_axis,
             const TVec2D & wcs_pt)
  {
    TVec2D wcs_vec = wcs_pt - origin;
    double u = (u_axis * wcs_vec) / (u_axis * u_axis);
    double v = (v_axis * wcs_vec) / (v_axis * v_axis);
    return TVec2D(u, v);
  }

  //----------------------------------------------------------------
  // lcs_to_wcs
  //
  inline TVec2D
  lcs_to_wcs(const TVec2D & origin,
             const TVec2D & u_axis,
             const TVec2D & v_axis,
             const TVec2D & lcs_pt)
  {
    return origin + u_axis * lcs_pt.x() + v_axis * lcs_pt.y();
  }

}


#endif // YAE_VEC_H_

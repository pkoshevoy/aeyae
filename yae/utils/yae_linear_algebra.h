// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 21 12:37:47 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LINEAR_ALGEBRA_H_
#define YAE_LINEAR_ALGEBRA_H_

// standard:
#include <algorithm>
#include <assert.h>
#include <cmath>


namespace yae
{

  //----------------------------------------------------------------
  // Matrix
  //
  template <int rows, int cols, typename TData = double>
  struct Matrix
  {
    typedef TData value_type;
    typedef Matrix<rows, cols, TData> TSelf;
    typedef Matrix<cols, rows, TData> TTranspose;

    enum { kRows = rows, kCols = cols, kSize = rows * cols };

    inline TData operator[](int index) const
    {
      assert(index < kSize);
      return data_[index];
    }

    inline TData & operator[](int index)
    {
      assert(index < kSize);
      return data_[index];
    }

    inline TData at(int row, int col) const
    { return this->operator[](col + row * cols); }

    inline TData & at(int row, int col)
    { return this->operator[](col + row * cols); }

    inline TData * begin()
    { return &(data_[0]); }

    inline const TData * begin() const
    { return &(data_[0]); }

    inline const TData * end() const
    { return begin() + kSize; }

    //----------------------------------------------------------------
    // init
    //
    TSelf &
    init(TData v)
    {
      TData * dst = this->begin();
      const TData * end = this->end();

      for (; dst < end; ++dst)
      {
        *dst = v;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // init_diagonal
    //
    TSelf &
    init_diagonal(TData v)
    {
      int diagonal = rows < cols ? rows : cols;
      for (int i = 0; i < diagonal; i++)
      {
        at(i, i) = v;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator +=
    //
    TSelf &
    operator += (const TSelf & m)
    {
      const TData * src = m.begin();
      const TData * end = m.end();
      TData * dst = begin();

      for (; src < end; ++src, ++dst)
      {
        *dst += *src;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator -=
    //
    TSelf &
    operator -= (const TSelf & m)
    {
      const TData * src = m.begin();
      const TData * end = m.end();
      TData * dst = begin();

      for (; src < end; ++src, ++dst)
      {
        *dst -= *src;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator *=
    //
    TSelf &
    operator *= (TData s)
    {
      TData * dst = this->begin();
      const TData * end = this->end();

      for (; dst < end; ++dst)
      {
        *dst *= s;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator +=
    //
    TSelf &
    operator += (TData t)
    {
      TData * dst = this->begin();
      const TData * end = this->end();

      for (; dst < end; ++dst)
      {
        *dst += t;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator -=
    //
    inline TSelf & operator -= (TData t)
    { return this->operator += (-t); }

    //----------------------------------------------------------------
    // operator /=
    //
    TSelf &
    operator /= (TData s)
    {
      TData * dst = this->begin();
      const TData * end = this->end();

      for (; dst < end; ++dst)
      {
        *dst /= s;
      }

      return *this;
    }

    //----------------------------------------------------------------
    // operator +
    //
    inline TSelf operator + (const TSelf & m) const
    {
      TSelf out(*this);
      out += m;
      return out;
    }

    //----------------------------------------------------------------
    // operator -
    //
    inline TSelf operator - (const TSelf & m) const
    {
      TSelf out(*this);
      out -= m;
      return out;
    }

    //----------------------------------------------------------------
    // operator +
    //
    inline TSelf operator + (TData t) const
    {
      TSelf out(*this);
      out += t;
      return out;
    }

    //----------------------------------------------------------------
    // operator -
    //
    inline TSelf operator - (TData t) const
    { return this->operator + (-t); }

    //----------------------------------------------------------------
    // operator *
    //
    inline TSelf operator * (TData s) const
    {
      TSelf out(*this);
      out *= s;
      return out;
    }

    //----------------------------------------------------------------
    // operator /
    //
    inline TSelf operator / (TData s) const
    {
      TSelf out(*this);
      out /= s;
      return out;
    }

    //----------------------------------------------------------------
    // operator *
    //
    template <int other_cols>
    Matrix<rows, other_cols, TData>
    operator * (const Matrix<cols, other_cols, TData> & m) const
    {
      Matrix<rows, other_cols, TData> out;

      for (int out_col = 0; out_col < other_cols; out_col++)
      {
        for (int row = 0; row < rows; row++)
        {
          TData & sum = out.at(row, out_col);
          sum = 0;

          for (int col = 0; col < cols; col++)
          {
            sum += at(row, col) * m.at(col, out_col);
          }
        }
      }

      return out;
    }

    //----------------------------------------------------------------
    // transpose
    //
    Matrix<cols, rows, TData>
    transpose() const
    {
      Matrix<cols, rows, TData> out;

      for (int row = 0; row < rows; row++)
      {
        for (int col = 0; col < cols; col++)
        {
          out.at(col, row) = at(row, col);
        }
      }

      return out;
    }

    //----------------------------------------------------------------
    // operator -
    //
    TSelf
    operator - () const
    {
      TSelf out;
      TData * dst = out.begin();

      const TData * src = this->begin();
      const TData * end = this->end();

      for (; src < end; ++src, ++dst)
      {
        *dst = -(*src);
      }

      return out;
    }

    //----------------------------------------------------------------
    // sub
    //
    Matrix<rows - 1, cols - 1, TData>
    sub(int row, int col) const
    {
      Matrix<rows - 1, cols - 1, TData> out;

      for (int r = 0; r < rows; r++)
      {
        if (r == row)
        {
          continue;
        }

        int r_out = (r < row) ? r : r - 1;
        for (int c = 0; c < cols; c++)
        {
          if (c == col)
          {
            continue;
          }

          int c_out = (c < col) ? c : c - 1;
          out.at(r_out, c_out) = at(r, c);
        }
      }

      return out;
    }

  protected:
    TData data_[rows * cols];
  };

  //----------------------------------------------------------------
  // m3x3_t
  //
  typedef Matrix<3, 3> m3x3_t;

  //----------------------------------------------------------------
  // m3x3
  //
  inline m3x3_t
  make_m3x3(double m00, double m01, double m02,
            double m10, double m11, double m12,
            double m20, double m21, double m22)
  {
    m3x3_t m;
    m[0] = m00;
    m[1] = m01;
    m[2] = m02;
    m[3] = m10;
    m[4] = m11;
    m[5] = m12;
    m[6] = m20;
    m[7] = m21;
    m[8] = m22;
    return m;
  }

  //----------------------------------------------------------------
  // make_diagonal_m3x3
  //
  inline m3x3_t
  make_diagonal_m3x3(double m00, double m11, double m22)
  {
    return make_m3x3(m00, 0.0, 0.0,
                     0.0, m11, 0.0,
                     0.0, 0.0, m22);
  }

  //----------------------------------------------------------------
  // make_identity_m3x3
  //
  inline m3x3_t
  make_identity_m3x3()
  {
    return make_diagonal_m3x3(1.0, 1.0, 1.0);
  }


  //----------------------------------------------------------------
  // v3x1_t
  //
  typedef Matrix<3, 1> v3x1_t;

  //----------------------------------------------------------------
  // make_diagonal_m3x3
  //
  inline m3x3_t
  make_diagonal_m3x3(const v3x1_t & v)
  {
    return make_diagonal_m3x3(v[0], v[1], v[2]);
  }

  //----------------------------------------------------------------
  // make_m3x3
  //
  // https://en.wikipedia.org/wiki/Rotation_matrix
  //
  inline m3x3_t
  make_m3x3(// axis of rotation:
            double x,
            double y,
            double z,
            // angle of rotation:
            double theta_radians)
  {
    const double ct = cos(theta_radians);
    const double st = sin(theta_radians);
    const double vt = 1.0 - ct; // versine of theta
    return make_m3x3
      (x * x * vt + ct,     x * y * vt - z * st, x * z * vt + y * st,
       x * y * vt + z * st, y * y * vt + ct,     y * z * vt - x * st,
       x * z * vt - y * st, y * z * vt + x * st, z * z * vt + ct);
  }

  //----------------------------------------------------------------
  // v3x1
  //
  inline v3x1_t
  make_v3x1(double x, double y, double z)
  {
    v3x1_t v;
    v[0] = x;
    v[1] = y;
    v[2] = z;
    return v;
  }

  //----------------------------------------------------------------
  // cross_product
  //
  inline v3x1_t
  cross_product(const v3x1_t & a, const v3x1_t & b)
  {
    v3x1_t c;
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    return c;
  }

  //----------------------------------------------------------------
  // dot_product
  //
  inline double
  dot_product(const v3x1_t & a, const v3x1_t & b)
  { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

  //----------------------------------------------------------------
  // norm_squared
  //
  inline double
  norm_squared(const v3x1_t & v)
  { return dot_product(v, v); }

  //----------------------------------------------------------------
  // norm
  //
  inline double
  norm(const v3x1_t & v)
  { return sqrt(norm_squared(v)); }

  //----------------------------------------------------------------
  // pow
  //
  inline v3x1_t
  pow(const v3x1_t & v, double t)
  { return make_v3x1(std::pow(v[0], t),
                     std::pow(v[1], t),
                     std::pow(v[2], t)); }

  //----------------------------------------------------------------
  // clip
  //
  inline v3x1_t
  clip(const v3x1_t & v, double v_min = 0.0, double v_max = 1.0)
  {
    return make_v3x1(std::min(1.0, std::max(0.0, v[0])),
                     std::min(1.0, std::max(0.0, v[1])),
                     std::min(1.0, std::max(0.0, v[2])));
  }

  //----------------------------------------------------------------
  // det
  //
  // https://en.wikipedia.org/wiki/Determinant
  //
  template <typename TData>
  TData
  det(const Matrix<2, 2, TData> & m)
  {
    TData det_m = m.at(0, 0) * m.at(1, 1) - m.at(0, 1) * m.at(1, 0);
    return det_m;
  }

  //----------------------------------------------------------------
  // det
  //
  // https://en.wikipedia.org/wiki/Determinant
  //
  template <int rows, typename TData>
  TData
  det(const Matrix<rows, rows, TData> & m)
  {
    TData det_m = 0;
    TData sign = 1;

    for (int col = 0; col < rows; col++)
    {
      TData a_ij = m.at(0, col);
      TData d_ij = det(m.sub(0, col));
      det_m += sign * a_ij * d_ij;
      sign = -sign;
    }

    return det_m;
  }

  //----------------------------------------------------------------
  // inv
  //
  // https://en.wikipedia.org/wiki/Cramer%27s_rule
  // https://en.wikipedia.org/wiki/Adjugate_matrix
  //
  // computes det(m) * inv(m)
  // returns det(m)
  //
  template <int rows, typename TData>
  TData
  inv(const Matrix<rows, rows, TData> & m,
      Matrix<rows, rows, TData> & inv_m)
  {
    TData det_m = 0;
    TData sign = 1;

    for (int row = 0; row < rows; row++)
    {
      for (int col = 0; col < rows; col++)
      {
        TData d_ji = det(m.sub(col, row));
        inv_m.at(row, col) = sign * d_ji;

        if (!col)
        {
          TData a_ji = m.at(col, row);
          det_m += sign * a_ji * d_ji;
        }

        sign = -sign;
      }
    }

    return det_m;
  }

  //----------------------------------------------------------------
  // inv
  //
  // return inverse of m
  //
  template <int rows, typename TData>
  inline Matrix<rows, rows, TData>
  inv(const Matrix<rows, rows, TData> & m)
  {
    Matrix<rows, rows, TData> inv_m;
    TData det_m = inv(m, inv_m);
    inv_m /= det_m;
    return inv_m;
  }

}


#endif // YAE_LINEAR_ALGEBRA_H_

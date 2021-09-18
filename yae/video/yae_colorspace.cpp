// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 21 12:37:47 MDT 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <algorithm>
#include <cmath>
#include <map>
#include <string>

// ffmpeg includes:
extern "C"
{
#include <libavutil/pixdesc.h>
}

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_assert.h"
#include "yae/video/yae_colorspace.h"


namespace yae
{

  //----------------------------------------------------------------
  // get_xyz_to_xyz
  //
  // http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
  //
  m3x3_t
  get_xyz_to_xyz(// CIE XYZ source reference white point:
                 const v3x1_t & ws,
                 // CIE XYZ destination reference white point:
                 const v3x1_t & wd)
  {
#if 1
    // The Bradford method is the newest of the three methods,
    // and is considered by most experts to be the best of them.
    static const m3x3_t Ma =
      make_m3x3( 0.8951000,  0.2664000, -0.1614000,
                -0.7502000,  1.7135000,  0.0367000,
                 0.0389000, -0.0685000,  1.0296000);

    static const m3x3_t inv_Ma =
      make_m3x3( 0.9869929, -0.1470543, 0.1599627,
                 0.4323053,  0.5183603, 0.0492912,
                -0.0085287,  0.0400428, 0.9684867);
#elif 1
    // Von Kries:
    static const m3x3_t Ma =
      make_m3x3( 0.4002400, 0.7076000, -0.0808100,
                -0.2263000, 1.1653200,  0.0457000,
                 0.0000000, 0.0000000,  0.9182200);

    static const m3x3_t inv_Ma =
      make_m3x3(1.8599364, -1.1293816,  0.2198974,
                0.3611914,  0.6388125, -0.0000064,
                0.0000000,  0.0000000,  1.0890636);
#else // XYZ Scaling
    static const m3x3_t Ma = make_identity_m3x3();
    static const m3x3_t inv_Ma = make_identity_m3x3();
#endif
    v3x1_t src = Ma * ws;
    v3x1_t dst = Ma * wd;

    m3x3_t S = make_diagonal_m3x3(dst[0] / src[0],
                                  dst[1] / src[1],
                                  dst[2] / src[2]);

    m3x3_t M = inv_Ma * S * Ma;
    return M;
  }


  //----------------------------------------------------------------
  // linear::TransferFunc
  //
  namespace linear
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {};
  }

  //----------------------------------------------------------------
  // xvYCC::TransferFunc
  //
  namespace xvYCC
  {
    // https://en.wikipedia.org/wiki/Rec._709
    static const double beta = 0.018053968510807; // 0.018
    static const double alpha = 1.0 + 5.5 * beta; // 1.099
    static const double alpha1 = alpha - 1; // alpha1

    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V =
          (fabs(L) < 0.018) ? (4.5 * L) :
          (0.018 <= L) ? (alpha * std::pow(L, 0.45) - alpha1) :
          // for xvYCC, https://en.wikipedia.org/wiki/XvYCC
          (-alpha) * std::pow(-L, 0.45) + alpha1;
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L =
          (fabs(V) < 0.081) ? (V / 4.5) :
          (0.081 <= V) ? std::pow((V + alpha1) / alpha, 1.0 / 0.45) :
          // for xvYCC, https://en.wikipedia.org/wiki/XvYCC
          -std::pow((V - alpha1) / -alpha, 1.0 / 0.45);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // smpte240m::TransferFunc
  //
  namespace smpte240m
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V =
          (L < 0.0228) ? (4.0 * L) :
          1.1115 * std::pow(L, 0.45) - 0.1115;
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L =
          (V < 4.0 * 0.0228) ? (V / 4.0) :
          std::pow((V + 0.1115) / 1.1115, 1.0 / 0.45);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // smpte428_1::TransferFunc
  //
  // from https://developer.apple.com/library/archive/documentation/
  //      QuickTime/QTFF/QTFFChap3/qtff3.html
  //
  // SMPTE ST 428-1
  //
  //  Ew’ = (48 W ÷ 52.37)^( 1 ÷ 2.6 ) for 0 <= W <= 1
  //
  // for which W equal to 1 for peak white is ordinarily intended
  // to correspond to a display luminance level of 48 candelas
  // per square meter.
  //
  namespace smpte428_1
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V = std::pow((48.0 * L) / 52.37, 1.0 / 2.6);
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        V = clip(V, 0.0, 1.0);
        double L = (std::pow(V, 2.6) * 52.37) / 48.0;
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // bt1361::TransferFunc
  //
  namespace bt1361
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V =
          (L <= -0.0045) ? -(1.099 * std::pow(-4.0 * L, 0.45) - 0.099) / 4.0 :
          (L < 0.018) ? 4.5 * L :
          (1.099 * std::pow(L, 0.45) - 0.099);
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L =
          (V <= -0.0045 * 4.5) ? std::pow(((-4.0 * V) + 0.099) / 1.099,
                                          1.0 / 0.45) / -4.0 :
          (V < 0.018 * 4.5) ? (V / 4.5) :
          std::pow((V + 0.099) / 1.099, 1.0 / 0.45);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // sRGB::TransferFunc
  //
  namespace sRGB
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V =
          (L < 0.0031308) ? (12.92 * L) :
          1.055 * std::pow(L, 1.0 / 2.4) - 0.055;
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L =
          (V < 12.92 * 0.0031308) ? (V / 12.92) :
          std::pow((V + 0.055) / 1.055, 2.4);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // bt2020_12::TransferFunc
  //
  namespace bt2020_12
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V =
          (fabs(L) < 0.0181) ? (4.5 * L) :
          (0.0181 <= L) ? (1.0993 * std::pow(L, 0.45) - 0.0993) :
          (-1.0993) * std::pow(-L, 0.45) + 0.0993;
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L =
          (fabs(V) < 4.5 * 0.0181) ? (V / 4.5) :
          (4.5 * 0.0181 <= V) ? std::pow((V + 0.0993) / 1.0993, 1.0 / 0.45) :
          -std::pow((V - 0.0993) / -4.5, 1.0 / 0.45);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // gamma::TransferFunc
  //
  namespace gamma
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      const double gamma_;

      TransferFunc(double gamma):
        gamma_(gamma)
      {}

      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V = std::pow(L, 1.0 / gamma_);
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        V = clip(V, 0.0, 1.0);
        double L = std::pow(V, gamma_);
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // logarithmic::TransferFunc
  //
  // map [0, N] to [0, 1]
  //
  namespace logarithmic
  {
    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      const double scale_;

      TransferFunc(double N):
        scale_(std::log(1.0 + N))
      {}

      // linear RGB to non-linear R'G'B'
      virtual double oetf(double L) const
      {
        double V = std::log(1.0 + L) / scale_;
        return V;
      }

      // inv(oetf):
      virtual double eotf(double V) const
      {
        double L = exp(V * scale_) - 1.0;
        return L;
      }
    };
  }

  //----------------------------------------------------------------
  // smpte2084::TransferFunc
  //
  // aka PQ, see BT.2100-2 2018
  //
  namespace smpte2084
  {
    // variables defined in the standard:
    static const double m1 = (2610.0 / 16384.0);
    static const double m2 = (2523.0 / 32.0);
    static const double inv_m1 = (16384.0 / 2610.0);
    static const double inv_m2 = (32.0 / 2523.0);
    static const double c1 = (3424.0 / 4096.0);
    static const double c2 = (2413.0 / 128.0);
    static const double c3 = (2392.0 / 128.0);

    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear RGB to non-linear R'G'B'
      virtual double oetf(double Y) const
      {
        double Y_m1 = std::pow(Y, 2610.0 / 16384.0);
        double V = std::pow((c1 + c2 * Y_m1) /
                            (1.0 + c3 * Y_m1),
                            m2);
        return V;
      }

      // inv(oetf):
      virtual double eotf(double E) const
      {
        E = clip(E, 0.0, 1.0);
        double E_inv_m2 = std::pow(E, inv_m2);
        double Y = std::pow(std::max(E_inv_m2 - c1, 0.0) /
                            (c2 - c3 * E_inv_m2),
                            inv_m1);
        return Y;
      }
    };
  }

  //----------------------------------------------------------------
  // get_hlg_gamma
  //
  double
  get_hlg_gamma(double Lw)
  {
    static const double k = 1.111;
    static const double log_2 = std::log(2.0);

    return
      (400.0 <= Lw && Lw <= 2000.0) ?
      (1.2 + 0.42 * std::log10(Lw / 1000.0)) :
      (1.2 * std::pow(k, std::log(Lw / 1000.0) / log_2));
  }

  //----------------------------------------------------------------
  // get_hlg_beta
  //
  double
  get_hlg_beta(double Lw, double Lb, double gamma)
  {
    return sqrt(3.0 * std::pow(Lb / Lw, 1.0 / gamma));
  }


  //----------------------------------------------------------------
  // hlg::TransferFunc
  //
  // aka ARIB STD-B67
  //
  //   Yd = alpha * (Ys ^ gamma)
  //
  //   Ys in [0.0, 1.0], normalized linear scene luminance
  //   Yd in [ Lb,  Lw]
  //
  namespace hlg
  {
    // variables defined in the standard:
    static const double a = 0.17883277;
    static const double b = 1.0 - 4.0 * a;
    static const double c = 0.5 - a * std::log(4.0 * a);

    struct TransferFunc : yae::Colorspace::TransferFunc
    {
      // linear cd/m2 RGB components to non-linear encoded R'G'B'.
      // default implementation rescales and delegates to normalized oetf:
      virtual void oetf_rgb(const Colorspace & csp,
                            const Context & ctx,
                            const double * rgb_cdm2,
                            double * rgb) const
      {
        const double Yd = (csp.kr_ * rgb_cdm2[0] +
                           csp.kg_ * rgb_cdm2[1] +
                           csp.kb_ * rgb_cdm2[2]);

        const double Yd_alpha_to_1_minus_gamma_over_gamma_over_alpha =
          std::pow(Yd / ctx.Lw_, (1.0 - ctx.gamma_) / ctx.gamma_) /
          ctx.Lw_;

        const double Rs =
          Yd_alpha_to_1_minus_gamma_over_gamma_over_alpha * rgb_cdm2[0];

        const double Gs =
          Yd_alpha_to_1_minus_gamma_over_gamma_over_alpha * rgb_cdm2[1];

        const double Bs =
          Yd_alpha_to_1_minus_gamma_over_gamma_over_alpha * rgb_cdm2[2];

        rgb[0] = this->oetf(Rs, ctx.beta_);
        rgb[1] = this->oetf(Gs, ctx.beta_);
        rgb[2] = this->oetf(Bs, ctx.beta_);
      }

      // normalized [0, 1] linear RGB to non-linear R'G'B'
      virtual double oetf(double v, double beta) const
      {
        double x =
          (v <= 1.0 / 12.0) ? sqrt(3.0 * v) :
          a * std::log(12.0 * v - b) + c;
        double E = std::max(0.0, (x - beta) / (1.0 - beta));
        return E;
      }

      // non-linear encoded R'G'B' to linear cd/m2 RGB components.
      // default implementation delegates to normalized eotf and rescales:
      virtual void eotf_rgb(const Colorspace & csp,
                            const Context & ctx,
                            const double * rgb,
                            double * rgb_cdm2) const
      {
        // inv(OETF):
        const double Rs = this->oetf_inv(clip(rgb[0], 0.0, 1.0), ctx.beta_);
        const double Gs = this->oetf_inv(clip(rgb[1], 0.0, 1.0), ctx.beta_);
        const double Bs = this->oetf_inv(clip(rgb[2], 0.0, 1.0), ctx.beta_);

        // OOTF:
        const double Ys = (csp.kr_ * Rs +
                           csp.kg_ * Gs +
                           csp.kb_ * Bs);

        const double alpha_times_Ys_to_gamma_minus_1 =
          ctx.Lw_ * std::pow(Ys, ctx.gamma_ - 1.0);

        rgb_cdm2[0] = alpha_times_Ys_to_gamma_minus_1 * Rs;
        rgb_cdm2[1] = alpha_times_Ys_to_gamma_minus_1 * Gs;
        rgb_cdm2[2] = alpha_times_Ys_to_gamma_minus_1 * Bs;
      }

      inline static double oetf_inv(double E, double beta)
      {
        double x = std::max(0.0, (1.0 - beta) * E + beta);
        double v =
          (x <= 0.5) ? (x * x) / 3.0 :
          (exp((x - c) / a) + b) / 12.0;
        return v;
      }
    };
  }

  //----------------------------------------------------------------
  // get_transfer_func
  //
  const Colorspace::TransferFunc &
  get_transfer_func(AVColorTransferCharacteristic av_trc)
  {
    if (av_trc == AVCOL_TRC_BT709 ||
        av_trc == AVCOL_TRC_SMPTE170M ||
        av_trc == AVCOL_TRC_IEC61966_2_4 ||
        av_trc == AVCOL_TRC_BT2020_10)
    {
      static const xvYCC::TransferFunc xvYCC_709_;
      return xvYCC_709_;
    }

    if (av_trc == AVCOL_TRC_GAMMA22)
    {
      static const gamma::TransferFunc gamma22_(2.2);
      return gamma22_;
    }

    if (av_trc == AVCOL_TRC_GAMMA28)
    {
      static const gamma::TransferFunc gamma28_(2.8);
      return gamma28_;
    }

    if (av_trc == AVCOL_TRC_SMPTE240M)
    {
      static const smpte240m::TransferFunc smpte240m_;
      return smpte240m_;
    }

    if (av_trc == AVCOL_TRC_LOG)
    {
      // IDK if these are correct:
      static const logarithmic::TransferFunc log100_(100.0);
      return log100_;
    }

    if (av_trc == AVCOL_TRC_LOG_SQRT)
    {
      // IDK if these are correct:
      static const logarithmic::TransferFunc log316_(100.0 * sqrt(10.0));
      return log316_;
    }

    if (av_trc == AVCOL_TRC_BT1361_ECG)
    {
      static const bt1361::TransferFunc bt1361_;
      return bt1361_;
    }

    if (av_trc == AVCOL_TRC_IEC61966_2_1)
    {
      static const sRGB::TransferFunc srgb_;
      return srgb_;
    }

    if (av_trc == AVCOL_TRC_BT2020_12)
    {
      static const bt2020_12::TransferFunc bt2020_12_;
      return bt2020_12_;
    }

    if (av_trc == AVCOL_TRC_SMPTE2084)
    {
      static const smpte2084::TransferFunc st2084_;
      return st2084_;
    }

    if (av_trc == AVCOL_TRC_SMPTE428)
    {
      static const smpte428_1::TransferFunc smpte428_1_;
      return smpte428_1_;
    }

    if (av_trc == AVCOL_TRC_ARIB_STD_B67)
    {
      static const hlg::TransferFunc hlg_;
      return hlg_;
    }

    YAE_ASSERT(av_trc == AVCOL_TRC_RESERVED0 ||
               av_trc == AVCOL_TRC_UNSPECIFIED ||
               av_trc == AVCOL_TRC_RESERVED ||
               av_trc == AVCOL_TRC_LINEAR);
    static const linear::TransferFunc linear_;
    return linear_;
  }


  //----------------------------------------------------------------
  // xy_t
  //
  struct xy_t
  {
    xy_t(double x = 0.0, double y = 0.0): x_(x), y_(y) {}

    double x_;
    double y_;
  };

  //----------------------------------------------------------------
  // Primaries
  //
  struct Primaries
  {
    xy_t r_;
    xy_t g_;
    xy_t b_;
    xy_t w_;
  };

  //----------------------------------------------------------------
  // get_primaries
  //
  static bool
  get_primaries(AVColorPrimaries av_pri,
                Primaries & p)
  {
    // various white point chromaticity coordinates:
    static const xy_t wp_c(0.31, 0.316);
    static const xy_t wp_d65(0.31271, 0.32902);
    static const xy_t wp_dci(0.314, 0.351);
    static const xy_t wp_e(1.0 / 3.0, 1.0 / 3.0);

    // various RGB primaries:
    static const xy_t rp_709(0.640, 0.330);
    static const xy_t gp_709(0.300, 0.600);
    static const xy_t bp_709(0.150, 0.060);

    static const xy_t rp_470m(0.67, 0.33);
    static const xy_t gp_470m(0.21, 0.71);
    static const xy_t bp_470m(0.14, 0.08);

    static const xy_t rp_470bg(0.64, 0.33);
    static const xy_t gp_470bg(0.29, 0.60);
    static const xy_t bp_470bg(0.15, 0.06);

    static const xy_t rp_170m(0.630, 0.340);
    static const xy_t gp_170m(0.310, 0.595);
    static const xy_t bp_170m(0.155, 0.070);

    static const xy_t rp_film(0.681, 0.319);
    static const xy_t gp_film(0.243, 0.692);
    static const xy_t bp_film(0.145, 0.049);

    static const xy_t rp_2020(0.708, 0.292);
    static const xy_t gp_2020(0.170, 0.797);
    static const xy_t bp_2020(0.131, 0.046);

    static const xy_t rp_428(0.735, 0.265);
    static const xy_t gp_428(0.274, 0.718);
    static const xy_t bp_428(0.167, 0.009);

    static const xy_t rp_431(0.680, 0.320);
    static const xy_t gp_431(0.265, 0.690);
    static const xy_t bp_431(0.150, 0.060);

    static const xy_t rp_p22(0.630, 0.340);
    static const xy_t gp_p22(0.295, 0.605);
    static const xy_t bp_p22(0.155, 0.077);

    if (av_pri == AVCOL_PRI_BT470M)
    {
      p.w_ = wp_c;
      p.r_ = rp_470m;
      p.g_ = gp_470m;
      p.b_ = bp_470m;
    }
    else if (av_pri == AVCOL_PRI_BT470BG)
    {
      p.w_ = wp_d65;
      p.r_ = rp_470bg;
      p.g_ = gp_470bg;
      p.b_ = bp_470bg;
    }
    else if (av_pri == AVCOL_PRI_SMPTE170M ||
             av_pri == AVCOL_PRI_SMPTE240M)
    {
      p.w_ = wp_d65;
      p.r_ = rp_170m;
      p.g_ = gp_170m;
      p.b_ = bp_170m;
    }
    else if (av_pri == AVCOL_PRI_FILM)
    {
      p.w_ = wp_c;
      p.r_ = rp_film;
      p.g_ = gp_film;
      p.b_ = bp_film;
    }
    else if (av_pri == AVCOL_PRI_BT2020)
    {
      p.w_ = wp_d65;
      p.r_ = rp_2020;
      p.g_ = gp_2020;
      p.b_ = bp_2020;
    }
    else if (av_pri == AVCOL_PRI_SMPTE428)
    {
      p.w_ = wp_e;
      p.r_ = rp_428;
      p.g_ = gp_428;
      p.b_ = bp_428;
    }
    else if (av_pri == AVCOL_PRI_SMPTE431 ||
             av_pri == AVCOL_PRI_SMPTE432)
    {
      p.w_ = (av_pri == AVCOL_PRI_SMPTE431) ? wp_dci : wp_d65;
      p.r_ = rp_431;
      p.g_ = gp_431;
      p.b_ = bp_431;
    }
    else if (av_pri == AVCOL_PRI_JEDEC_P22)
    {
      p.w_ = wp_d65;
      p.r_ = rp_p22;
      p.g_ = gp_p22;
      p.b_ = bp_p22;
    }
    else
    {
      YAE_ASSERT(av_pri == AVCOL_PRI_BT709);
      p.w_ = wp_d65;
      p.r_ = rp_709;
      p.g_ = gp_709;
      p.b_ = bp_709;
      return (av_pri == AVCOL_PRI_BT709);
    }

    return true;
  }


  //----------------------------------------------------------------
  // Colorspace::get
  //
  const Colorspace *
  Colorspace::get(AVColorSpace av_csp,
                  AVColorPrimaries av_pri,
                  AVColorTransferCharacteristic av_trc)
  {
    static boost::mutex mutex_;

    std::string key;
    key += av_color_space_name(av_csp);
    key += "/";
    key += av_color_primaries_name(av_pri);
    key += "/";
    key += av_color_transfer_name(av_trc);

    boost::unique_lock<boost::mutex> lock(mutex_);

    static std::map<std::string, yae::shared_ptr<Colorspace> > cache_;
    yae::shared_ptr<Colorspace> & csp_ptr = cache_[key];
    if (csp_ptr)
    {
      return csp_ptr.get();
    }

    const Colorspace::TransferFunc & transfer_func = get_transfer_func(av_trc);

    Primaries primaries;
    get_primaries(av_pri, primaries);

    csp_ptr.reset(new Colorspace(// name:
                                 key.c_str(),

                                 // ffmpeg color specs:
                                 av_csp,
                                 av_pri,
                                 av_trc,

                                 // primaries chromaticity coordinates:
                                 primaries.r_.x_, primaries.r_.y_,
                                 primaries.g_.x_, primaries.g_.y_,
                                 primaries.b_.x_, primaries.b_.y_,

                                 // white point chromaticity coordinates:
                                 primaries.w_.x_, primaries.w_.y_,

                                 // transfer functions:
                                 transfer_func));

    // shortcut:
    Colorspace * colorspace = csp_ptr.get();

    if (av_trc == AVCOL_TRC_SMPTE170M)
    {
      colorspace->set_luma_coefficients(0.299, 0.587, 0.114);
    }

    return colorspace;
  }

  //----------------------------------------------------------------
  // Colorspace::Colorspace
  //
  Colorspace::Colorspace(const char * name,

                         // corresponding ffmpeg color specs, if known:
                         AVColorSpace av_csp,
                         AVColorPrimaries av_pri,
                         AVColorTransferCharacteristic av_trc,

                         // xyY coordinates of the primaries:
                         double rx, double ry,
                         double gx, double gy,
                         double bx, double by,

                         // xyY coordinates of the reference white point:
                         double wx, double wy,

                         const TransferFunc & transfer):
    name_(name),
    av_csp_(av_csp),
    av_pri_(av_pri),
    av_trc_(av_trc),
    r_(make_v3x1(rx, ry, 1.0)),
    g_(make_v3x1(gx, gy, 1.0)),
    b_(make_v3x1(bx, by, 1.0)),
    w_(make_v3x1(wx, wy, 1.0)),
    transfer_(transfer)
  {
    // from SMPTE RP 177-1993:
    double rz = 1.0 - (rx + ry);
    double gz = 1.0 - (gx + gy);
    double bz = 1.0 - (bx + by);
    double wz = 1.0 - (wx + wy);

    m3x3_t p = make_m3x3(rx, gx, bx,
                         ry, gy, by,
                         rz, gz, bz);

    v3x1_t w = make_v3x1(wx / wy,
                         1.0,
                         wz / wy);

    m3x3_t c = make_diagonal_m3x3(inv(p) * w);

    rgb_to_xyz_ = p * c;
    xyz_to_rgb_ = inv(rgb_to_xyz_);

    set_luma_coefficients(rgb_to_xyz_.at(1, 0),
                          rgb_to_xyz_.at(1, 1),
                          rgb_to_xyz_.at(1, 2));
  }

  //----------------------------------------------------------------
  // Colorspace::override_luma_coefficients
  //
  void
  Colorspace::set_luma_coefficients(double kr, double kg, double kb)
  {
    kr_ = kr;
    kg_ = kg;
    kb_ = kb; // kb_ = 1 - (kr_ + kg_)

    ypbpr_to_rgb_ = get_ypbpr_to_rgb(kr_, kg_, kb_);
    rgb_to_ypbpr_ = get_rgb_to_ypbpr(kr_, kg_, kb_);
  }
}

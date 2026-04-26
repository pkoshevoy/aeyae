// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Apr 23 09:20:18 PM MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_RANSAC_H_
#define YAE_RANSAC_H_

// boost:
#ifndef Q_MOC_RUN
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#endif

// standard:
#include <algorithm>
#include <vector>


namespace yae
{

  //----------------------------------------------------------------
  // RANSAC
  //
  // see https://en.wikipedia.org/wiki/Random_sample_consensus
  //
  template <typename TData>
  struct RANSAC
  {
    typedef TData data_t;
    typedef RANSAC<TData> ransac_t;
    typedef std::vector<const TData *> TSubSet;

    boost::random::uniform_int_distribution<std::size_t> random_;
    mutable boost::random::mt19937 rng_;

    const TData * dataset_;
    std::size_t dataset_size_;

    RANSAC(const TData * dataset, std::size_t dataset_size):
      random_(0, dataset_size - 1),
      dataset_(dataset),
      dataset_size_(dataset_size)
    {}

    //----------------------------------------------------------------
    // Model
    //
    struct Model
    {
      typedef typename ransac_t::TSubSet TSubSet;
      virtual ~Model() {}
      virtual void reset(const TSubSet & data) = 0;
      virtual double fit(const TData & sample) const = 0;
    };

    //----------------------------------------------------------------
    // Mean
    //
    struct Mean : Model
    {
      typedef typename ransac_t::TSubSet TSubSet;

      Mean(): mean_(0) {}

      // virtual:
      void reset(const TSubSet & data)
      {
        double sum = 0.0;
        std::size_t num = data.size();
        for (std::size_t i = 0; i < num; ++i)
        {
          sum += *(data[i]);
        }

        mean_ = num ? (sum / double(num)) : 0.0;
      }

      // virtual:
      double fit(const TData & sample) const
      {
        double abs_diff = ::fabs(mean_ - sample);
        return abs_diff;
      }

      double mean_;
    };

    //----------------------------------------------------------------
    // Median
    //
    struct Median : Model
    {
      typedef typename ransac_t::TSubSet TSubSet;

      Median(): median_(0) {}

      // virtual:
      void reset(const TSubSet & data)
      {
        std::size_t num = data.size();
        std::vector<TData> sorted(num);
        for (std::size_t i = 0; i < num; ++i)
        {
          sorted[i] = *(data[i]);
        }

        std::sort(sorted.begin(), sorted.end());
        median_ = sorted[sorted.size() >> 1];
      }

      // virtual:
      double fit(const TData & sample) const
      {
        double abs_diff = ::fabs(median_ - sample);
        return abs_diff;
      }

      TData median_;
    };

    void select_random_samples(std::size_t num_samples,
                               TSubSet & selected) const
    {
      for (std::size_t i = 0; i < num_samples; ++i)
      {
        std::size_t r = random_(rng_);
        selected[i] = &(dataset_[r]);
      }
    }

    // return bestfit error average, and pass back
    // the bestfit subset of dataset inliers:
    double find_inliers(Model & model,
                        double inlier_fit_threshold,
                        TSubSet & bestfit) const
    {
      std::size_t k = 2;
      std::size_t n = dataset_size_ / k;
      while (n > (k >> 1))
      {
        n >>= 1;
        k <<= 1;
      }

      TSubSet maybe_inliers(n);
      std::size_t d = n;
      double best_fit_error = std::numeric_limits<double>::max();

      for (std::size_t i = 0; i < k; ++i)
      {
        this->select_random_samples(n, maybe_inliers);
        model.reset(maybe_inliers);

        TSubSet confirmed;
        double sum_fit_error = 0;

        const TData * dataset_end = dataset_ + dataset_size_;
        for (const TData * s = dataset_; s < dataset_end; ++s)
        {
          double fit_error = model.fit(*s);
          if (fit_error < inlier_fit_threshold)
          {
            confirmed.push_back(s);
            sum_fit_error += fit_error;
          }
        }

        if (confirmed.size() < d)
        {
          continue;
        }

        double avg_fit_error = sum_fit_error / double(confirmed.size());
        if (avg_fit_error < best_fit_error)
        {
          best_fit_error = avg_fit_error;
          bestfit.swap(confirmed);
        }
      }

      return best_fit_error;
    }
  };

}


#endif // YAE_RANSAC_H_

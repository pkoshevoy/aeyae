// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <iostream>
#include <sstream>
#include <thread>

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/utils/aeyae_benchmark.hxx"


//----------------------------------------------------------------
// func_a
//
static void func_a()
{
  YAE_BENCHMARK(benchmark, "func_a");
}

//----------------------------------------------------------------
// func_b
//
static void func_b()
{
  YAE_BENCHMARK(benchmark, "func_b");

  for (int i = 0; i < 1000; i++)
  {
    func_a();
  }
}

//----------------------------------------------------------------
// to_string
//
template <typename TData>
inline static std::string
to_string(const TData & data)
{
  std::ostringstream oss;
  oss << data;
  return std::string(oss.str().c_str());
}


BOOST_AUTO_TEST_CASE(aeyae_benchmark)
{
  YAE_BENCHMARK_CLEAR();
  {
    std::ostringstream oss;
    YAE_BENCHMARK_SHOW(oss);
    BOOST_CHECK_EQUAL(oss.str().c_str(),
                      "\nBenchmark timesheets per thread:\n\n");
  }

  std::thread t1(&func_b);
  std::string t1_id = to_string(t1.get_id());

  std::thread t2(&func_b);
  std::string t2_id = to_string(t2.get_id());

  t1.join();
  t2.join();

  std::ostringstream oss;
  YAE_BENCHMARK_SHOW(oss);
  std::string result(oss.str().c_str());

  std::string::size_type found_t1 = result.find(t1_id, 0);
  BOOST_CHECK(found_t1 != std::string::npos);

  std::string::size_type found_t2 = result.find(t1_id, 0);
  BOOST_CHECK(found_t2 != std::string::npos);

  std::string::size_type found_b1 =
    result.find(" func_b                                   :        1  call,",
                found_t1);
  BOOST_CHECK(found_b1 != std::string::npos);

  std::string::size_type found_a1 =
    result.find("  func_a                                  :     1000 calls,",
                found_b1);
  BOOST_CHECK(found_a1 != std::string::npos);

  std::string::size_type found_b2 =
    result.find(" func_b                                   :        1  call,",
                found_t2);
  BOOST_CHECK(found_b2 != std::string::npos);

  std::string::size_type found_a2 =
    result.find("  func_a                                  :     1000 calls,",
                found_b2);
  BOOST_CHECK(found_a2 != std::string::npos);

  YAE_BENCHMARK_CLEAR();
  {
    std::ostringstream oss;
    YAE_BENCHMARK_SHOW(oss);
    BOOST_CHECK_EQUAL(oss.str().c_str(),
                      "\nBenchmark timesheets per thread:\n\n");
  }
}

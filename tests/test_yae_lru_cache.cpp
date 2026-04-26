// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jan 14 12:47:05 MST 2018
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_lru_cache.h"

// standard:
#include <map>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/test/unit_test.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// shortcut:
using namespace yae;


// keep track of how many times the factory method was called:
static std::map<unsigned int, int> factory_call_count;

//----------------------------------------------------------------
// factory
//
static bool
factory(void * context, const int & key, unsigned int & value)
{
  factory_call_count[key] += 1;
  value = ((unsigned int)key) << 8;
  return true;
}

BOOST_AUTO_TEST_CASE(yae_lru_cache)
{
  typedef LRUCache<int, unsigned int> TCache;

  TCache cache;
  cache.set_capacity(16);

  // first round, cache is empty, every referenced value
  // has to be constructed by the factory:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 1);
  }

  // second round, cached values are unreferenced
  // and reused, no new factory calls are required:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 1);
  }

  // third round, unreferenced cached values are purged
  // and new values are constructed by the factory:
  for (int i = 0; i < 16; i++)
  {
    int j = i + cache.capacity();
    TCache::TRefPtr ref = cache.get(j, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)j) << 8));
    BOOST_CHECK(factory_call_count[j] == 1);
  }

  // final round, cached values are unreferenced
  // and pured, new factory calls are required:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 2);
  }

  // cache 0..16 and hold on to the references:
  std::list<TCache::TRefPtr> refs;
  for (int i = 0; i < cache.capacity(); i++)
  {
    refs.push_back(cache.get(i, &factory));
  }

#if 0
  // this should block because the cache is full:
  cache.get(0, &factory);
  BOOST_CHECK(false);
#else
  // purge 15 to make room for another 0:
  refs.pop_back();
  cache.get(0, &factory);
  BOOST_CHECK(factory_call_count[0] == 3);
#endif

  // release all references:
  refs.clear();

  // cache 16 instances of 100 and hold on to the references:
  for (int i = 0; i < cache.capacity(); i++)
  {
    refs.push_back(cache.get(100, &factory));
  }
  BOOST_CHECK_EQUAL(factory_call_count[100], cache.capacity());

  // release all references:
  refs.clear();

  // cache 16 instances of 100, hold on to the references:
  for (int i = 0; i < cache.capacity(); i++)
  {
    refs.push_back(cache.get(100, &factory));
  }

  // verify that unreferenced instances were reused:
  BOOST_CHECK_EQUAL(factory_call_count[100], cache.capacity());

  // release all references:
  refs.clear();

  // access 16 instances of 200, releasing reference immediately:
  for (int i = 0; i < cache.capacity(); i++)
  {
    cache.get(200, &factory);
  }

  // verify that 200 was instantiated only once:
  BOOST_CHECK_EQUAL(factory_call_count[200], 1);
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <iostream>
#include <list>
#include <set>
#include <vector>

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_type_name.h"


//----------------------------------------------------------------
// aaa
//
struct aaa
{
#if !(__cplusplus < 201103L)
  aaa(std::string&& data):
    data_(std::move(data))
  {
    // std::cerr << data_ << ", aaa(std::string&&)" << std::endl;
  }

  aaa(aaa&& rhs):
    data_(std::move(rhs.data_))
  {
    // std::cerr << data_ << ", aaa(aaa&&)" << std::endl;
  }
#endif

  aaa(const std::string & data = std::string("aaa")):
    data_(data)
  {
    // std::cerr << data_ << ", aaa(const std::string &)" << std::endl;
  }

  aaa(const aaa & rhs):
    data_(rhs.data_)
  {
    // std::cerr << data_ << ", aaa(const aaa &)" << std::endl;
  }

  virtual ~aaa()
  {
    // std::cerr << data_ << ", ~aaa()" << std::endl;
  }

  std::string data_;
};

//----------------------------------------------------------------
// bbb
//
struct bbb : public aaa
{
  bbb(const char * msg = "bbb"):
    aaa(std::string(msg))
  {}

  bbb(const bbb & rhs):
    aaa(rhs)
  {}

  virtual ~bbb()
  {}
};

//----------------------------------------------------------------
// ccc
//
struct ccc : public bbb
{
  ccc(const char * msg = "ccc"):
    bbb(msg)
  {}
};

//----------------------------------------------------------------
// ddd
//
struct ddd : public bbb
{
  ddd(const char * msg = "ddd"):
    bbb(msg)
  {}
};

//----------------------------------------------------------------
// eee
//
struct eee : public aaa
{
  eee(const char * msg = "eee"):
    aaa(msg)
  {}
};

BOOST_AUTO_TEST_CASE(yae_shared_ptr)
{
  typedef yae::shared_ptr<aaa> APtr;
  typedef yae::shared_ptr<bbb, aaa> BPtr;
  typedef yae::shared_ptr<ccc, aaa> CPtr;
  typedef yae::shared_ptr<ddd, aaa> DPtr;
  typedef yae::shared_ptr<eee, aaa> EPtr;

  typedef APtr::weak_ptr_type AWPtr;
  typedef BPtr::weak_ptr_type BWPtr;
  typedef CPtr::weak_ptr_type CWPtr;
  typedef DPtr::weak_ptr_type DWPtr;
  typedef EPtr::weak_ptr_type EWPtr;

  AWPtr wa;
  BWPtr wb;
  CWPtr wc;
  DWPtr wd;
  EWPtr we;

  // test sizeof:
  BOOST_CHECK_EQUAL(sizeof(APtr), sizeof(aaa *));

  // test pointer casts:
  {
    APtr aa(new aaa("aa"));
    BOOST_CHECK(aa.cast<aaa>());
    BOOST_CHECK(!aa.cast<bbb>());
    BOOST_CHECK(!aa.cast<ccc>());
    BOOST_CHECK(!aa.cast<ddd>());
    BOOST_CHECK(!aa.cast<eee>());

    wa = aa;

    APtr ab(new bbb("ab"));
    BOOST_CHECK(ab.cast<aaa>());
    BOOST_CHECK(ab.cast<bbb>());
    BOOST_CHECK(!ab.cast<ccc>());
    BOOST_CHECK(!ab.cast<ddd>());
    BOOST_CHECK(!ab.cast<eee>());

    wb = ab;

    APtr ac(new ccc("ac"));
    BOOST_CHECK(ac.cast<aaa>());
    BOOST_CHECK(ac.cast<bbb>());
    BOOST_CHECK(ac.cast<ccc>());
    BOOST_CHECK(!ac.cast<ddd>());
    BOOST_CHECK(!ac.cast<eee>());

    wc = ac;

    APtr ad(new ddd("ad"));
    BOOST_CHECK(ad.cast<aaa>());
    BOOST_CHECK(ad.cast<bbb>());
    BOOST_CHECK(!ad.cast<ccc>());
    BOOST_CHECK(ad.cast<ddd>());
    BOOST_CHECK(!ad.cast<eee>());

    wd = ad;

    APtr ae(new eee("ae"));
    BOOST_CHECK(ae.cast<aaa>());
    BOOST_CHECK(!ae.cast<bbb>());
    BOOST_CHECK(!ae.cast<ccc>());
    BOOST_CHECK(!ae.cast<ddd>());
    BOOST_CHECK(ae.cast<eee>());

    we = ae;
  }

  APtr a(new aaa("AAA"));
  BPtr b(new bbb("BBB"));
  CPtr c(new ccc("CCC"));
  DPtr d(new ddd("DDD"));

  APtr aa = b;
  BPtr bb = aa;
  BOOST_CHECK_EQUAL(bb.get(), aa.get());

  EPtr ee = APtr(new eee("EEE"));
  BOOST_CHECK_EQUAL(ee->data_, std::string("EEE"));

  DPtr().swap(d);
  BOOST_CHECK(!d);

  d.reset(new ddd("DDD.2"));
  BOOST_CHECK(d);
}


//----------------------------------------------------------------
// Node
//
struct Node
{
  Node(std::size_t id = 0):
    id_(id)
  {}

  yae::weak_ptr<Node> self_;
  yae::weak_ptr<Node> prev_;
  std::size_t id_;
};

BOOST_AUTO_TEST_CASE(yae_weak_ptr)
{
  // verify node self-reference via weak_ptr works as expected:
  std::vector<yae::shared_ptr<Node> > nodes;
  for (std::size_t i = 0; i < 1000000; i++)
  {
    yae::shared_ptr<Node> node(new Node(i));
    node->self_ = node;
    if (!nodes.empty())
    {
      node->prev_ = nodes.back();
    }

    nodes.push_back(node);
  }

  std::list<yae::shared_ptr<Node> > node_list(nodes.begin(), nodes.end());
  std::set<yae::shared_ptr<Node> > node_set(nodes.begin(), nodes.end());
  nodes.clear();
  node_list.clear();
  node_set.clear();
  BOOST_CHECK(true);
}

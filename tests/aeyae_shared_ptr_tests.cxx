// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 15 18:30:37 MDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#include "yae_shared_ptr.hxx"
#include "yae_type_name.hxx"

#include <iostream>


//----------------------------------------------------------------
// aaa
//
struct aaa
{
  aaa(std::string&& data):
    data_(std::move(data))
  {
    // std::cerr << data_ << ", aaa(std::string&&)" << std::endl;
  }

  aaa(const std::string & data = std::string("aaa")):
    data_(data)
  {
    // std::cerr << data_ << ", aaa(const std::string &)" << std::endl;
  }

  aaa(aaa&& rhs):
    data_(std::move(rhs.data_))
  {
    // std::cerr << data_ << ", aaa(aaa&&)" << std::endl;
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

//----------------------------------------------------------------
// main
//
int
main(int argc, const char ** argv)
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

  using yae::dump_smart_ptr;

  // test pointer casts:
  {
    APtr aa(new aaa("aa"));
    dump_smart_ptr(std::cerr << "\naa: ", aa);
    dump_smart_ptr(std::cerr << "aa.cast<aaa>: ", aa.cast<aaa>());
    assert(aa.cast<aaa>());
    dump_smart_ptr(std::cerr << "aa.cast<bbb>: ", aa.cast<bbb>());
    assert(!aa.cast<bbb>());
    dump_smart_ptr(std::cerr << "aa.cast<ccc>: ", aa.cast<ccc>());
    assert(!aa.cast<ccc>());
    dump_smart_ptr(std::cerr << "aa.cast<ddd>: ", aa.cast<ddd>());
    assert(!aa.cast<ddd>());
    dump_smart_ptr(std::cerr << "aa.cast<eee>: ", aa.cast<eee>());
    assert(!aa.cast<eee>());
    std::cerr << std::endl;

    wa = aa;
    dump_smart_ptr(std::cerr << "wa.lock(): ", wa.lock()) << std::endl;

    APtr ab(new bbb("ab"));
    dump_smart_ptr(std::cerr << "\nab: ", ab);
    dump_smart_ptr(std::cerr << "ab.cast<aaa>: ", ab.cast<aaa>());
    assert(ab.cast<aaa>());
    dump_smart_ptr(std::cerr << "ab.cast<bbb>: ", ab.cast<bbb>());
    assert(ab.cast<bbb>());
    dump_smart_ptr(std::cerr << "ab.cast<ccc>: ", ab.cast<ccc>());
    assert(!ab.cast<ccc>());
    dump_smart_ptr(std::cerr << "ab.cast<ddd>: ", ab.cast<ddd>());
    assert(!ab.cast<ddd>());
    dump_smart_ptr(std::cerr << "ab.cast<eee>: ", ab.cast<eee>());
    assert(!ab.cast<eee>());
    std::cerr << std::endl;

    wb = ab;
    dump_smart_ptr(std::cerr << "wb.lock(): ", wb.lock()) << std::endl;

    APtr ac(new ccc("ac"));
    dump_smart_ptr(std::cerr << "\nac: ", ac);
    dump_smart_ptr(std::cerr << "ac.cast<aaa>: ", ac.cast<aaa>());
    assert(ac.cast<aaa>());
    dump_smart_ptr(std::cerr << "ac.cast<bbb>: ", ac.cast<bbb>());
    assert(ac.cast<bbb>());
    dump_smart_ptr(std::cerr << "ac.cast<ccc>: ", ac.cast<ccc>());
    assert(ac.cast<ccc>());
    dump_smart_ptr(std::cerr << "ac.cast<ddd>: ", ac.cast<ddd>());
    assert(!ac.cast<ddd>());
    dump_smart_ptr(std::cerr << "ac.cast<eee>: ", ac.cast<eee>());
    assert(!ac.cast<eee>());
    std::cerr << std::endl;

    wc = ac;
    dump_smart_ptr(std::cerr << "wc.lock(): ", wc.lock()) << std::endl;

    APtr ad(new ddd("ad"));
    dump_smart_ptr(std::cerr << "\nad: ", ad);
    dump_smart_ptr(std::cerr << "ad.cast<aaa>: ", ad.cast<aaa>());
    assert(ad.cast<aaa>());
    dump_smart_ptr(std::cerr << "ad.cast<bbb>: ", ad.cast<bbb>());
    assert(ad.cast<bbb>());
    dump_smart_ptr(std::cerr << "ad.cast<ccc>: ", ad.cast<ccc>());
    assert(!ad.cast<ccc>());
    dump_smart_ptr(std::cerr << "ad.cast<ddd>: ", ad.cast<ddd>());
    assert(ad.cast<ddd>());
    dump_smart_ptr(std::cerr << "ad.cast<eee>: ", ad.cast<eee>());
    assert(!ad.cast<eee>());
    std::cerr << std::endl;

    wd = ad;
    dump_smart_ptr(std::cerr << "wd.lock(): ", wd.lock()) << std::endl;

    APtr ae(new eee("ae"));
    dump_smart_ptr(std::cerr << "\nae: ", ae);
    dump_smart_ptr(std::cerr << "ae.cast<aaa>: ", ae.cast<aaa>());
    assert(ae.cast<aaa>());
    dump_smart_ptr(std::cerr << "ae.cast<bbb>: ", ae.cast<bbb>());
    assert(!ae.cast<bbb>());
    dump_smart_ptr(std::cerr << "ae.cast<ccc>: ", ae.cast<ccc>());
    assert(!ae.cast<ccc>());
    dump_smart_ptr(std::cerr << "ae.cast<ddd>: ", ae.cast<ddd>());
    assert(!ae.cast<ddd>());
    dump_smart_ptr(std::cerr << "ae.cast<eee>: ", ae.cast<eee>());
    assert(ae.cast<eee>());
    std::cerr << std::endl;

    we = ae;
    dump_smart_ptr(std::cerr << "we.lock(): ", we.lock()) << std::endl;
  }

  dump_smart_ptr(std::cerr << "wa.lock(): ", wa.lock());
  dump_smart_ptr(std::cerr << "wb.lock(): ", wb.lock());
  dump_smart_ptr(std::cerr << "wc.lock(): ", wc.lock());
  dump_smart_ptr(std::cerr << "wd.lock(): ", wd.lock());
  dump_smart_ptr(std::cerr << "we.lock(): ", we.lock());
  std::cerr << std::endl;

  APtr a(new aaa("AAA"));
  BPtr b(new bbb("BBB"));
  CPtr c(new ccc("CCC"));
  DPtr d(new ddd("DDD"));

  APtr aa = b;
  BPtr bb = aa;
  EPtr ee = APtr(new eee("EEE"));

  dump_smart_ptr(std::cerr << "wa = d, wa.lock(): ", (wa = d, wa.lock()));
  dump_smart_ptr(std::cerr << "wd = wa, wd.lock(): ", (wd = wa, wd.lock()));
  dump_smart_ptr(std::cerr << "wa = wb, wa.lock(): ", (wa = wd, wa.lock()));

  DPtr().swap(d);
  dump_smart_ptr(std::cerr << "DPtr().swap(d); wd.lock(): ", wd.lock());
  std::cerr << std::endl;

  dump_smart_ptr(std::cerr << "a: ", a) << std::endl;
  dump_smart_ptr(std::cerr << "b: ", b) << std::endl;
  dump_smart_ptr(std::cerr << "c: ", c) << std::endl;
  d.reset(new ddd("DDD.2"));
  dump_smart_ptr(std::cerr << "d: ", d) << std::endl;
  dump_smart_ptr(std::cerr << "aa: ", aa) << std::endl;
  dump_smart_ptr(std::cerr << "bb: ", bb) << std::endl;
  dump_smart_ptr(std::cerr << "ee: ", ee) << std::endl;

  return 0;
}

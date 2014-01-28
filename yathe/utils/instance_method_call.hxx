// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

/*
Copyright 2008 Pavel Koshevoy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : instance_method_call.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 14 11:17:00 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : Generic trail record/replay API.

#ifndef INSTANCE_METHOD_CALL_HXX_
#define INSTANCE_METHOD_CALL_HXX_

// system includes:
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <assert.h>

// boost includes:
#include <boost/shared_ptr.hpp>

// local includes:
#include "io/io_base.hxx"


//----------------------------------------------------------------
// instance_t
//
class instance_t : public io_base_t
{
public:
  instance_t();
  instance_t(const std::string & signature);
  instance_t(const std::string & signature, const void * address);
  instance_t(const void * address);

  static io_base_t * create()
  { return new instance_t(); }

  // instance pointer accessor:
  inline void * address() const
  { return address_; }

  // instance signature accessor:
  inline const std::string & signature() const
  { return signature_; }

  // virtual:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);

  // check whether the instance has been saved before:
  bool was_saved() const;

  // initialize this instance from an old address by looking up
  // a matching signature and new address in the maps:
  bool init(uint64_t old_address);

protected:
  // instance signature:
  std::string signature_;

  // current address of the instance:
  void * address_;

  // map from loaded signature to loaded address:
  static std::map<std::string, uint64_t> map_load_;

  // map from instance signature to current address:
  static std::map<std::string, void *> map_save_;

  // map from the current address to instance signature:
  static std::map<void *, std::string> map_signature_;

  // map from loaded address to current address:
  static std::map<uint64_t, void *> map_address_;
};


//----------------------------------------------------------------
// type_instance_t
//
template <typename class_t>
class type_instance_t : public instance_t
{
public:
  type_instance_t(): instance_t() {}

  type_instance_t(const char * sig, const void * addr, bool increment = true)
  {
    init(sig, addr, increment);
  }

  inline type_instance_t<class_t> &
  init(const char * sig, const void * addr, bool increment = true)
  {
    if (increment) index_++;
    std::ostringstream oss;
    oss << sig << ".v" << index_;
    instance_t::signature_ = oss.str().c_str();

    address_ = const_cast<void *>(addr);
    return *this;
  }

  static size_t index_;
};


//----------------------------------------------------------------
// DECLARE_INSTANCE
//
#ifndef DECLARE_INSTANCE
#define DECLARE_INSTANCE( CLASS, INSTANCE )	\
  type_instance_t<CLASS> INSTANCE
#endif

//----------------------------------------------------------------
// DECLARED_INSTANCE_INIT
//
#ifndef DEFINE_INSTANCE
#define DEFINE_INSTANCE( INSTANCE, CLASS, ADDRESS )	\
  INSTANCE.init(#CLASS, ADDRESS)
#endif


//----------------------------------------------------------------
// type_instance_t::index_
//
template <typename class_t>
size_t
type_instance_t<class_t>::index_ = 0;


//----------------------------------------------------------------
// args_t
//
class args_t : public io_base_t
{
public:
  // virtual:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);
};


//----------------------------------------------------------------
// arg1_t
//
template <typename arg_t>
class arg1_t : public args_t
{
public:
  arg1_t() {}
  arg1_t(arg_t arg): arg_(arg) {}

  // virtual:
  void save(std::ostream & so) const
  {
    so << "arg1_t ";
    ::save(so, arg_);
  }

  bool load(std::istream & si, const std::string & magic)
  {
    if (magic != "arg1_t")
    {
      return false;
    }

    bool ok = ::load(si, arg_);
    return ok;
  }

  arg_t arg_;
};


//----------------------------------------------------------------
// method_t
//
class method_t
{
public:
  method_t(const char * signature);
  virtual ~method_t() {}

  // lookup a method by its signature:
  static const method_t * lookup(const std::string & signature);

  // instance signature accessor:
  inline const std::string & signature() const
  { return signature_; }

  // API for executing a loaded method call:
  virtual void execute(const instance_t & instance,
		       const boost::shared_ptr<args_t> & args) const = 0;

  // each method must know how to load its arguments:
  virtual bool load(std::istream & si,
		    boost::shared_ptr<args_t> & args) const = 0;

protected:
  const std::string signature_;
  static std::map<std::string, const method_t *> methods_;
};


//----------------------------------------------------------------
// method_void_t
//
template <typename class_t, typename func_t>
class method_void_t : public method_t
{
public:
  method_void_t(const char * signature, func_t func):
    method_t(signature),
    func_(func)
  {}

  // virtual:
  void execute(const instance_t & instance,
	       const boost::shared_ptr<args_t> & /* args */) const
  {
    class_t * c = (class_t *)(instance.address());
    if (!c)
    {
      assert(false);
      return;
    }

    // call the member function:
    assert(func_);
    if (func_)
    {
      (c->*func_)();
    }
  }

  // virtual:
  bool load(std::istream & /* si */, boost::shared_ptr<args_t> & args) const
  {
    args = boost::shared_ptr<args_t>();
    return true;
  }

protected:
  func_t func_;
};

//----------------------------------------------------------------
// void_method_void_t
//
template <typename class_t>
class void_method_void_t : public method_t
{
public:
  typedef void (class_t::*func_t)();
  typedef void (class_t::*const_func_t)() const;

  void_method_void_t(const char * signature, func_t func):
    method_t(signature),
    func_(func),
    const_func_(NULL)
  {}

  void_method_void_t(const char * signature, const_func_t func):
    method_t(signature),
    func_(NULL),
    const_func_(func)
  {}

  // virtual:
  void execute(const instance_t & instance,
	       const boost::shared_ptr<args_t> & /* args */) const
  {
    class_t * c = (class_t *)(instance.address());
    if (!c)
    {
      assert(false);
      return;
    }

    // call the member function:
    if (func_)
    {
      (c->*func_)();
    }
    else if (const_func_)
    {
      (c->*const_func_)();
    }
    else
    {
      assert(false);
    }
  }

  // virtual:
  bool load(std::istream & /* si */, boost::shared_ptr<args_t> & args) const
  {
    args = boost::shared_ptr<args_t>();
    return true;
  }

protected:
  func_t func_;
  const_func_t const_func_;
};


//----------------------------------------------------------------
// method_arg1_t
//
template <typename class_t, typename func_t, typename arg_t>
class method_arg1_t : public method_t
{
public:
  method_arg1_t(const char * signature, func_t func):
    method_t(signature),
    func_(func)
  {}

  // virtual:
  void execute(const instance_t & instance,
	       const boost::shared_ptr<args_t> & args) const
  {
    class_t * c = (class_t *)(instance.address());
    if (!c)
    {
      assert(false);
      return;
    }

    // call the member function:
    const arg1_t<arg_t> * arg1 = (const arg1_t<arg_t> *)(args.get());
    assert(func_);
    if (func_)
    {
      (c->*func_)(arg1->arg_);
    }
  }

  // virtual:
  bool load(std::istream & si, boost::shared_ptr<args_t> & args) const
  {
    std::string magic;
    si >> magic;

    arg1_t<arg_t> * arg1 = new arg1_t<arg_t>();
    bool ok = arg1->load(si, magic);

    args = boost::shared_ptr<args_t>(arg1);
    return ok;
  }

protected:
  func_t func_;
};


//----------------------------------------------------------------
// void_method_arg1_t
//
template <typename class_t, typename arg_t>
class void_method_arg1_t : public method_t
{
public:
  typedef void (class_t::*func_t)(arg_t);
  typedef void (class_t::*func_cref_t)(const arg_t &);
  typedef void (class_t::*const_func_t)(arg_t) const;
  typedef void (class_t::*const_func_cref_t)(const arg_t &) const;

  void_method_arg1_t(const char * signature, func_t func):
    method_t(signature),
    func_(func),
    func_cref_(NULL),
    const_func_(NULL),
    const_func_cref_(NULL)
  {}

  void_method_arg1_t(const char * signature, func_cref_t func):
    method_t(signature),
    func_(NULL),
    func_cref_(func),
    const_func_(NULL),
    const_func_cref_(NULL)
  {}

  void_method_arg1_t(const char * signature, const_func_t func):
    method_t(signature),
    func_(NULL),
    func_cref_(NULL),
    const_func_(func),
    const_func_cref_(NULL)
  {}

  void_method_arg1_t(const char * signature, const_func_cref_t func):
    method_t(signature),
    func_(NULL),
    func_cref_(NULL),
    const_func_(NULL),
    const_func_cref_(func)
  {}

  // virtual:
  void execute(const instance_t & instance,
	       const boost::shared_ptr<args_t> & args) const
  {
    class_t * c = (class_t *)(instance.address());
    if (!c)
    {
      assert(false);
      return;
    }

    // call the member function:
    const arg1_t<arg_t> * arg1 = (const arg1_t<arg_t> *)(args.get());
    if (func_)
    {
      (c->*func_)(arg1->arg_);
    }
    else if (func_cref_)
    {
      (c->*func_cref_)(arg1->arg_);
    }
    else if (const_func_)
    {
      (c->*const_func_)(arg1->arg_);
    }
    else if (const_func_cref_)
    {
      (c->*const_func_cref_)(arg1->arg_);
    }
    else
    {
      assert(false);
    }
  }

  // virtual:
  bool load(std::istream & si, boost::shared_ptr<args_t> & args) const
  {
    std::string magic;
    si >> magic;

    arg1_t<arg_t> * arg1 = new arg1_t<arg_t>();
    bool ok = arg1->load(si, magic);

    args = boost::shared_ptr<args_t>(arg1);
    return ok;
  }

protected:
  func_t func_;
  func_cref_t func_cref_;
  const_func_t const_func_;
  const_func_cref_t const_func_cref_;
};


//----------------------------------------------------------------
// REGISTER_METHOD_VOID
//
#ifndef REGISTER_METHOD_VOID
#ifdef WIN32
#define REGISTER_METHOD_VOID( CLASS, METHOD )				\
  static void_method_void_t<CLASS>					\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"()", &CLASS::METHOD)
#else
#define REGISTER_METHOD_VOID( CLASS, METHOD )				\
  static method_void_t<CLASS, typeof(&CLASS::METHOD) >			\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"()", &CLASS::METHOD)
#endif
#endif


//----------------------------------------------------------------
// REGISTER_METHOD_ARG1
//
#ifndef REGISTER_METHOD_ARG1
#ifdef WIN32
#define REGISTER_METHOD_ARG1( CLASS, METHOD, ARG )			\
  static void_method_arg1_t<CLASS, ARG>					\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"("#ARG")", &CLASS::METHOD)
#else
#define REGISTER_METHOD_ARG1( CLASS, METHOD, ARG )			\
  static method_arg1_t<CLASS, typeof(&CLASS::METHOD), ARG>		\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"("#ARG")", &CLASS::METHOD)
#endif
#endif


//----------------------------------------------------------------
// REGISTER_OVERLOADED_METHOD_ARG1
//
#ifndef REGISTER_OVERLOADED_METHOD_ARG1
#define REGISTER_OVERLOADED_METHOD_ARG1( CLASS, METHOD, ARG )		\
  static void_method_arg1_t<CLASS, ARG>					\
  void_method_##CLASS##_##METHOD(#CLASS"::"#METHOD"("#ARG")",		\
				 &CLASS::METHOD)
#endif


//----------------------------------------------------------------
// REGISTER_METHOD_VOID_EXPLICITLY
//
#ifndef REGISTER_METHOD_VOID_EXPLICITLY
#define REGISTER_METHOD_VOID_EXPLICITLY( CLASS, METHOD, TYPE )		\
  static method_void_t<CLASS, TYPE>					\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"()", &CLASS::METHOD)
#endif


//----------------------------------------------------------------
// REGISTER_METHOD_ARG1_EXPLICITLY
//
#ifndef REGISTER_METHOD_ARG1_EXPLICITLY
#define REGISTER_METHOD_ARG1_EXPLICITLY( CLASS, METHOD, ARG, TYPE )    	\
  static method_arg1_t<CLASS, TYPE, ARG>	       			\
  method_##CLASS##_##METHOD(#CLASS"::"#METHOD"("#ARG")", &CLASS::METHOD)
#endif


//----------------------------------------------------------------
// call_t
//
class call_t : public io_base_t
{
public:
  call_t(const instance_t & instance = instance_t(),
	 const char * method_signature = NULL,
	 const boost::shared_ptr<args_t> & args = boost::shared_ptr<args_t>());

  call_t(void * address,
	 const char * method_signature);

  // helpers:
  template <typename arg_t>
  call_t & init(const instance_t & instance,
		const char * method_signature,
		const arg_t & arg)
  {
    assert(instance.signature().size());
    instance_ = instance;

    // lookup the method:
    if (method_signature)
    {
      method_ = method_t::lookup(method_signature);
      assert(method_);
    }

    typedef arg1_t<arg_t> one_arg_t;
    args_ = boost::shared_ptr<one_arg_t>(new one_arg_t(arg));
    return *this;
  }

  template <typename arg_t>
  call_t & init(const void * address,
		const char * method_signature,
		const arg_t & arg)
  {
    return init<arg_t>(instance_t(address), method_signature, arg);
  }

  // virtual:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);

  // execute the call:
  void execute() const;

  static io_base_t * create()
  { return new call_t(); }

protected:
  instance_t instance_;
  const method_t * method_;
  boost::shared_ptr<args_t> args_;
};


#endif // INSTANCE_METHOD_CALL_HXX_

// File         : the_boost_mutex.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu Oct 23 21:49:25 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : A Boost mutex wrapper class.

#ifndef THE_BOOST_MUTEX_HXX_
#define THE_BOOST_MUTEX_HXX_

// local includes:
#include "thread/the_mutex_interface.hxx"

// Boost includes:
#include <boost/thread/mutex.hpp>


//----------------------------------------------------------------
// the_boost_mutex_t
// 
class the_boost_mutex_t : public the_mutex_interface_t
{
public:
  the_boost_mutex_t();
  
  // the destructor is protected on purpose,
  // see delete_this for details:
  virtual ~the_boost_mutex_t();
  
  // In order to avoid memory management problems with shared libraries,
  // whoever provides this interface instance (via it's creator), has to
  // provide a way to delete the instance as well.  This will avoid
  // issues with multiple-instances of C runtime libraries being
  // used by the app and whatever libraries it links against that
  // either use or provide this interface:
  virtual void delete_this();
  
  // the creation method:
  static the_mutex_interface_t * create();
  
  // virtual:
  void lock();
  
  // virtual:
  void unlock();
  
  // virtual:
  bool try_lock();
  
private:
  boost::mutex mutex_;
};


#endif // THE_BOOST_MUTEX_HXX_

// File         : the_qt_mutex.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt mutex wrapper class.

#ifndef THE_QT_MUTEX_HXX_
#define THE_QT_MUTEX_HXX_

// Qt includes:
#include <qmutex.h>

// local includes:
#include "thread/the_mutex_interface.hxx"


//----------------------------------------------------------------
// the_qt_mutex_t
// 
class the_qt_mutex_t : public QMutex, public the_mutex_interface_t
{
public:
  the_qt_mutex_t();
  
  // the destructor is protected on purpose,
  // see delete_this for details:
  virtual ~the_qt_mutex_t();
  
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
};


#endif // THE_QT_MUTEX_HXX_

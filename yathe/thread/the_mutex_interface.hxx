// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_mutex_interface.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : An abstract mutex class interface.

#ifndef THE_MUTEX_INTERFACE_HXX_
#define THE_MUTEX_INTERFACE_HXX_


//----------------------------------------------------------------
// the_mutex_interface_t
// 
class the_mutex_interface_t
{
protected:
  // the destructor is protected on purpose,
  // see delete_this for details:
  virtual ~the_mutex_interface_t();
  
public:
  // In order to avoid memory management problems with shared libraries,
  // whoever provides this interface instance (via it's creator), has to
  // provide a way to delete the instance as well.  This will avoid
  // issues with multiple-instances of C runtime libraries being
  // used by the app and whatever libraries it links against that
  // either use or provide this interface:
  virtual void delete_this() = 0;
  
  // mutex controls:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool try_lock() = 0;
  
  //----------------------------------------------------------------
  // creator_t
  // 
  typedef the_mutex_interface_t *(*creator_t)();
  
  // specify a thread creation method:
  static void set_creator(creator_t creator);
  
  // create a new instance of a thread:
  static the_mutex_interface_t * create();
  
protected:
  // an abstract mutex creator:
  static creator_t creator_;
};


#endif // THE_MUTEX_INTERFACE_HXX_

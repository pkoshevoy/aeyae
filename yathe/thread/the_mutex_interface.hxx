// File         : the_mutex_interface.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : 
// Description  : 

#ifndef THE_MUTEX_INTERFACE_HXX_
#define THE_MUTEX_INTERFACE_HXX_


//----------------------------------------------------------------
// the_mutex_interface_t
// 
class the_mutex_interface_t
{
public:
  virtual ~the_mutex_interface_t() {}
  
  // mutex controls:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool try_lock() = 0;
  
  //----------------------------------------------------------------
  // creator_t
  // 
  typedef the_mutex_interface_t*(*creator_t)();
  
  // specify a thread creation method:
  static void set_creator(creator_t creator);
  
  // create a new instance of a thread:
  static the_mutex_interface_t * create();
  
protected:
  // an abstract mutex creator:
  static creator_t creator_;
};


#endif // THE_MUTEX_INTERFACE_HXX_

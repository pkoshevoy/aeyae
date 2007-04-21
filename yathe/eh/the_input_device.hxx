// File         : the_input_device.hxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : GPL.
// Description  : base class for mouse, keyboard and tablet devices.

#ifndef THE_INPUT_DEVICE_HXX_
#define THE_INPUT_DEVICE_HXX_


//----------------------------------------------------------------
// the_input_device_t
// 
class the_input_device_t
{
public:
  the_input_device_t()
  { time_stamp_ = latest_time_stamp_; }
  
  inline void update_time_stamp()
  { time_stamp_ = latest_time_stamp_; }
  
  inline bool current() const
  { return (time_stamp_ == latest_time_stamp_); }
  
  inline static void advance_time_stamp()
  { latest_time_stamp_++; }
  
  inline const unsigned int & time_stamp() const
  { return time_stamp_; }
  
  inline static unsigned int latest_time_stamp()
  { return latest_time_stamp_; }
  
protected:
  unsigned int time_stamp_;
  
  static unsigned int latest_time_stamp_;
};


//----------------------------------------------------------------
// the_btn_state_t
// 
typedef enum
{
  THE_BTN_UP_E = 0,
  THE_BTN_DN_E = 1
} the_btn_state_t;


#endif // THE_INPUT_DEVICE_HXX_

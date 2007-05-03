/*
Copyright 2004-2007 University of Utah

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


// File         : the_input_device.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_mouse_device.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : mouse device abstraction classes.

#ifndef THE_MOUSE_DEVICE_HXX_
#define THE_MOUSE_DEVICE_HXX_

// system includes:
#include <list>

// local includes:
#include "eh/the_input_device.hxx"
#include "eh/the_input_device_event.hxx"
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// the_mouse_btn_t
// 
// Mouse button state machine:
class the_mouse_btn_t : public the_input_device_t
{
public:
  the_mouse_btn_t(unsigned int button = 0);
  
  void update(const the_mouse_event_t & e);
  
  // this is necessary for searching:
  inline bool operator == (const the_mouse_btn_t & btn) const
  { return (btn_ == btn.btn_); }
  
  // this is necessary for sorting:
  inline bool operator < (const the_mouse_btn_t & btn) const
  { return (btn_ < btn.btn_); }
  
  // conversion operator: construct on-the-fly a linked list containing
  // this mouse button as its only element:
  inline operator std::list<the_mouse_btn_t>() const
  {
    std::list<the_mouse_btn_t> l;
    l.push_back(*this);
    return l;
  }
  
  // accessors:
  inline the_btn_state_t curr_state() const
  { return curr_state_; }
  
  inline the_btn_state_t prev_state() const
  { return prev_state_; }
  
  inline const unsigned int & btn() const
  { return btn_; }
  
  inline void set_btn(unsigned int btn)
  { btn_ = btn; }
  
  inline const bool & moved_while_down() const
  { return moved_while_down_; }
  
  inline bool double_click() const
  { return dbl_clk_; }
  
  // helpers:
  inline bool up() const
  { return (curr_state_ == THE_BTN_UP_E); }
  
  inline bool down() const
  { return (curr_state_ == THE_BTN_DN_E); }
  
  inline bool single_click() const
  { return (up() && changed() && !moved_while_down_); }
  
  inline bool changed() const
  { return (current() && (prev_state_ != curr_state_)); }
private:
  // previuos/current state of the button (up/down):
  the_btn_state_t prev_state_;
  the_btn_state_t curr_state_;
  
  // flag indicating whether the mouse was moved while this button was down:
  bool moved_while_down_;
  
  // true -> double click with this button:
  bool dbl_clk_;
  
  // id of the button we are supposed to watch for:
  unsigned int btn_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & sout, const the_mouse_btn_t & btn);


//----------------------------------------------------------------
// the_mouse_t
// 
// This class represents the current state of the mouse.
// 
class the_mouse_t
{
public:
  the_mouse_t();
  
  void update(const the_mouse_event_t & me);
  
  // accessors:
  inline const the_mouse_btn_t & l_btn() const { return l_btn_; }
  inline const the_mouse_btn_t & m_btn() const { return m_btn_; }
  inline const the_mouse_btn_t & r_btn() const { return r_btn_; }
  
  // find all buttons that are currently pressed,
  // return false if no buttons are pressed:
  unsigned int collect_pressed_btns(std::list<the_mouse_btn_t> & btns) const;
  
  // verify that the passed in list of btns matches the
  // currently pressed buttons:
  bool verify_pressed_btns(const std::list<the_mouse_btn_t> & btns) const;
  
  // check that there are no pressed buttons:
  inline bool verify_no_pressed_btns() const
  {
    std::list<the_mouse_btn_t> btns;
    return (collect_pressed_btns(btns) == 0);
  }
  
  // setup the left, middle and right mouse button detectors:
  void setup_buttons(unsigned int l, unsigned int m, unsigned int r)
  {
    l_btn_.set_btn(l);
    m_btn_.set_btn(m);
    r_btn_.set_btn(r);
  }
  
private:
  the_mouse_btn_t l_btn_; // left mouse button.
  the_mouse_btn_t m_btn_; // middle mouse button.
  the_mouse_btn_t r_btn_; // right mouse button.
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & sout, const the_mouse_t & mouse);

//----------------------------------------------------------------
// dump
// 
extern void dump(const the_mouse_t & mouse);


// shortcuts to the input devices:
#define THE_MOUSE THE_TRAIL.mouse()


#endif // THE_MOUSE_DEVICE_HXX_

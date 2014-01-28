// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_mouse_device.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : mouse device abstraction classes.

// local includes:
#include "eh/the_mouse_device.hxx"
#include "utils/the_utils.hxx"

// system includes:
#include <iostream>


//----------------------------------------------------------------
// the_mouse_btn_t::the_mouse_btn_t
//
the_mouse_btn_t::the_mouse_btn_t(unsigned int button):
  prev_state_(THE_BTN_UP_E),
  curr_state_(THE_BTN_UP_E),
  moved_while_down_(false),
  dbl_clk_(false),
  btn_(button)
{}

//----------------------------------------------------------------
// the_mouse_btn_t::update
//
void
the_mouse_btn_t::update(const the_mouse_event_t & e)
{
  update_time_stamp();
  prev_state_ = curr_state_;

  if ((e.btns() & btn_) &&
      (e.tran() == the_mouse_event_t::tran_up_))
  {
    curr_state_ = THE_BTN_UP_E;
  }
  else if ((e.btns() & btn_) &&
	   (e.tran() == the_mouse_event_t::tran_down_))
  {
    curr_state_ = THE_BTN_DN_E;
  }

  // detect double click:
  dbl_clk_ = ((e.btns() & btn_) && e.double_click_);

  // detect drag while the button is down:
  moved_while_down_ = down() && !changed();
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & sout, const the_mouse_btn_t & btn)
{
  sout << btn.btn() << " { ";

  if (btn.moved_while_down())
  {
    sout << "moved while down, ";
  }

  if (btn.double_click())
  {
    sout << "double click, ";
  }

  if (btn.single_click())
  {
    sout << "single click, ";
  }

  if (btn.changed())
  {
    sout << "changed, ";
  }

  if (btn.curr_state() == THE_BTN_DN_E)
  {
    if (btn.prev_state() == THE_BTN_DN_E)
    {
      sout << "stayed down }";
    }
    else
    {
      sout << "went down }";
    }
  }
  else
  {
    if (btn.prev_state() == THE_BTN_UP_E)
    {
      sout << "stayed up }";
    }
    else
    {
      sout << "went up }";
    }
  }
  sout << endl;

  return sout;
}


//----------------------------------------------------------------
// the_mouse_t::the_mouse_t
//
the_mouse_t::the_mouse_t():
  l_btn_(),
  m_btn_(),
  r_btn_()
{}

//----------------------------------------------------------------
// the_mouse_t::update
//
void
the_mouse_t::update(const the_mouse_event_t & me)
{
  l_btn_.update(me);
  m_btn_.update(me);
  r_btn_.update(me);
}

//----------------------------------------------------------------
// the_mouse_t::collect_pressed_btns
//
unsigned int
the_mouse_t::collect_pressed_btns(std::list<the_mouse_btn_t> & btns) const
{
  unsigned int n_pressed = 0;
  if (l_btn().down())
  {
    btns.push_back(l_btn());
    n_pressed++;
  }

  if (m_btn().down())
  {
    btns.push_back(m_btn());
    n_pressed++;
  }

  if (r_btn().down())
  {
    btns.push_back(r_btn());
    n_pressed++;
  }

  return n_pressed;
}

//----------------------------------------------------------------
// the_mouse_t::verify_pressed_btns
//
bool
the_mouse_t::
verify_pressed_btns(const std::list<the_mouse_btn_t> & btn_list) const
{
  std::list<the_mouse_btn_t> pressed_btns;
  collect_pressed_btns(pressed_btns);

  // easy test:
  if (btn_list.size() != pressed_btns.size()) return false;

  // make sure that both lists have the same elements,
  // the list order does not matter:
  for (std::list<the_mouse_btn_t>::const_iterator i = pressed_btns.begin();
       i != pressed_btns.end(); ++i)
  {
    if (has(btn_list, *i) == false) return false;
  }

  return true;
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & sout, const the_mouse_t & mouse)
{
  sout << "l_btn: " << mouse.l_btn()
       << "m_btn: " << mouse.m_btn()
       << "r_btn: " << mouse.r_btn() << endl;
  return sout;
}

//----------------------------------------------------------------
// dump
//
void
dump(const the_mouse_t & mouse)
{
  std::cerr << mouse;
}

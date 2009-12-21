// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : the_keybd_device.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : keyboard device abstraction classes.

#ifndef THE_KEYBD_DEVICE_HXX_
#define THE_KEYBD_DEVICE_HXX_

// system includes:
#include <list>

// local includes:
#include "eh/the_input_device.hxx"
#include "eh/the_input_device_event.hxx"
#include "utils/the_bit_tree.hxx"


//----------------------------------------------------------------
// the_keybd_key_t
// 
// Keyboard key state machine:
class the_keybd_key_t : public the_input_device_t
{
public:
  the_keybd_key_t();
  
  void setup(unsigned int key, unsigned int mask = 0x0);
  
  void reset();
  
  void update(const the_mouse_event_t & e);
  void update(const the_keybd_event_t & e);
  
  inline bool up() const
  { return (curr_state_ == THE_BTN_UP_E); }
  
  inline bool down() const
  { return (curr_state_ == THE_BTN_DN_E); }
  
  inline the_btn_state_t state() const
  { return curr_state_; }
  
  inline bool changed() const
  { return (current() && (prev_state_ != curr_state_)); }
  
  // this is necessary for searching:
  inline bool operator == (const the_keybd_key_t & key) const
  { return (key_ == key.key_); }
  
  // this is necessary for sorting:
  inline bool operator < (const the_keybd_key_t & key) const
  { return (key_ < key.key_); }
  
  // conversion operator: construct on-the-fly a linked list containing
  // this keyboard key as its only element:
  inline operator std::list<the_keybd_key_t>() const
  {
    std::list<the_keybd_key_t> l;
    l.push_back(*this);
    return l;
  }

  // accessors:
  inline const the_btn_state_t & prev_state() const
  { return prev_state_; }
  
  inline const the_btn_state_t & curr_state() const
  { return curr_state_; }
  
  inline const unsigned int & key() const
  { return key_; }
  
  inline const unsigned int & mask() const
  { return mask_; }
  
  inline bool auto_repeat() const
  { return auto_repeat_; }
  
private:
  the_btn_state_t prev_state_;
  the_btn_state_t curr_state_;
  
  // the key that this state machine has to track:
  unsigned int key_;
  
  // key mask (modifier keys only - Alt, Ctrl, Shift):
  unsigned int mask_;
  
  // auto repeat flag:
  bool auto_repeat_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & sout, const the_keybd_key_t & key);

#ifdef DELETE
#undef DELETE
#endif

//----------------------------------------------------------------
// the_keybd_t
// 
class the_keybd_t
{
public:
  //----------------------------------------------------------------
  // key_id_t
  // 
  typedef enum {
    SHIFT,
    ALT,
    CONTROL,
    META,
    
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    
    PAGE_UP,
    PAGE_DOWN,
    
    HOME,
    END,
    
    INSERT,
    DELETE,
    
    ESCAPE,
    TAB,
    BACKSPACE,
    RETURN,
    ENTER,
    SPACE,
    
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    NUMLOCK,
    CAPSLOCK,
    
    INVALID_KEY
  } key_id_t;
  
  the_keybd_t();
  
  // generic key accessors:
  the_keybd_key_t & key(unsigned int id);
  
  inline const the_keybd_key_t & key(unsigned int id) const
  { return const_cast<the_keybd_t *>(this)->key(id); }
  
  inline const the_keybd_key_t & operator [] (unsigned int id) const
  { return key(id); }
  
  // common modifier key accessors:
  inline const the_keybd_key_t & shf() const
  { return *key_[the_keybd_t::SHIFT]; }
  
  inline const the_keybd_key_t & alt() const
  { return *key_[the_keybd_t::ALT]; }
  
  inline const the_keybd_key_t & ctl() const
  { return *key_[the_keybd_t::CONTROL]; }
  
  inline the_keybd_key_t & shf()
  { return *key_[the_keybd_t::SHIFT]; }
  
  inline  the_keybd_key_t & alt()
  { return *key_[the_keybd_t::ALT]; }
  
  inline the_keybd_key_t & ctl()
  { return *key_[the_keybd_t::CONTROL]; }
  
  inline the_keybd_key_t * by_id(key_id_t id)
  { return (id < INVALID_KEY) ? key_[id] : NULL; }
  
  void update(const the_keybd_event_t & e);
  void update(const the_mouse_event_t & e);
  
  // find all keys that are currently pressed,
  // return false if no keys are pressed:
  size_t collect_pressed_keys(std::list<the_keybd_key_t> & keys) const;
  
  // verify that the passed in list of keys matches the
  // currently pressed keys:
  bool verify_pressed_keys(const std::list<the_keybd_key_t> & keys) const;
  
  // check that there are no pressed keys:
  inline bool verify_no_pressed_keys() const
  {
    std::list<the_keybd_key_t> keys;
    return (collect_pressed_keys(keys) == 0);
  }
  
  // this is a workaround for those few cases when I miss the key release
  // event and think that the key is pressed even when it is not:
  void forget_pressed_keys();
  
  // bind toolkit dependent key codes to the local key id:
  void init_key(const key_id_t & id,
		unsigned int key_code,
		unsigned int modifier = 0);
  
  // bind toolkit dependent key codes to the corresponding ascii code:
  void init_ascii(unsigned char ascii, unsigned int key_code);
  
  // lookup a key by it's ascii code:
  the_keybd_key_t & ascii(unsigned char id);
  
  inline const the_keybd_key_t & key(unsigned char id) const
  { return const_cast<the_keybd_t *>(this)->ascii(id); }
  
private:
  the_bit_tree_t<the_keybd_key_t> key_tree_;
  
  // shortcuts to the modifier keys:
  the_keybd_key_t * key_[INVALID_KEY];
  
  // shortcuts to the commonly used keys:
  the_keybd_key_t * ascii_[128];
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & sout, const the_keybd_t & keybd);

//----------------------------------------------------------------
// dump
// 
extern void dump(const the_keybd_t & keybd);


// shortcuts to the input devices:
#define THE_KEYBD THE_TRAIL.keybd()


#endif // THE_KEYBD_DEVICE_HXX_

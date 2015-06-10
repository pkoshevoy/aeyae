// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_keybd_device.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : keyboard device abstraction classes.

// local includes:
#include "eh/the_keybd_device.hxx"
#include "utils/the_utils.hxx"


//----------------------------------------------------------------
// the_keybd_key_t::the_keybd_key_t
//
the_keybd_key_t::the_keybd_key_t():
  prev_state_(THE_BTN_UP_E),
  curr_state_(THE_BTN_UP_E),
  key_(0x0),
  mask_(0x0),
  auto_repeat_(false)
{}

//----------------------------------------------------------------
// the_keybd_key_t::setup
//
void
the_keybd_key_t::setup(unsigned int k, unsigned int m)
{
  prev_state_ = THE_BTN_UP_E;
  curr_state_ = THE_BTN_UP_E;
  key_  = k;
  mask_ = m;
}

//----------------------------------------------------------------
// the_keybd_key_t::reset
//
void
the_keybd_key_t::reset()
{
  prev_state_ = THE_BTN_UP_E;
  curr_state_ = THE_BTN_UP_E;
  auto_repeat_ = false;
}

//----------------------------------------------------------------
// the_keybd_key_t::update
//
void
the_keybd_key_t::update(const the_mouse_event_t & e)
{
  update_time_stamp();
  prev_state_ = curr_state_;

  if (mask_ & e.mods())
  {
    curr_state_ = THE_BTN_DN_E;
  }
  else
  {
    curr_state_ = THE_BTN_UP_E;
  }
}

//----------------------------------------------------------------
// the_keybd_key_t::update
//
void
the_keybd_key_t::update(const the_keybd_event_t & e)
{
  if (key_ != e.key()) return;

  update_time_stamp();
  prev_state_ = curr_state_;
  auto_repeat_ = e.autorepeat();

  if (auto_repeat_)
  {
    // ignore the oscillations:
    return;
  }

  if (e.tran() == the_keybd_event_t::tran_up_)
  {
    curr_state_ = THE_BTN_UP_E;
  }
  else if (e.tran() == the_keybd_event_t::tran_down_)
  {
    curr_state_ = THE_BTN_DN_E;
  }
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & sout, const the_keybd_key_t & key)
{
  if (key.key() < 128)
  {
    sout << char(key.key());
  }
  else
  {
    sout << key.key();
  }

  sout << " { " << key.mask()
       << ", autorepeat: " << std::boolalpha << key.auto_repeat()
       << ", ";

  if (key.curr_state() == THE_BTN_DN_E)
  {
    if (key.prev_state() == THE_BTN_DN_E)
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
    if (key.prev_state() == THE_BTN_UP_E)
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
// the_keybd_t::the_keybd_t
//
the_keybd_t::the_keybd_t():
  key_tree_(32, 4)
{
  for (int i = 0; i < INVALID_KEY; i++)
  {
    key_[i] = NULL;
  }

  for (unsigned int i = 0; i < 128; i++)
  {
    ascii_[i] = NULL;
  }
}

//----------------------------------------------------------------
// the_keybd_t::key
//
the_keybd_key_t &
the_keybd_t::key(unsigned int id)
{
  the_bit_tree_leaf_t<the_keybd_key_t> * leaf = key_tree_.get(id);
  if (leaf != NULL) return leaf->elem;

  leaf = key_tree_.add(id);
  leaf->elem.setup(id);
  return leaf->elem;
}

//----------------------------------------------------------------
// the_keybd_t::update
//
void
the_keybd_t::update(const the_keybd_event_t & ke)
{
  if (ke.key() == 0) return;
  key(ke.key()).update(ke);
}

//----------------------------------------------------------------
// the_keybd_t::update
//
void
the_keybd_t::update(const the_mouse_event_t & e)
{
  shf().update(e);
  alt().update(e);
  ctl().update(e);
}

//----------------------------------------------------------------
// the_keybd_t::collect_pressed_keys
//
size_t
the_keybd_t::collect_pressed_keys(std::list<the_keybd_key_t> & keys) const
{
  std::list<the_keybd_key_t> known_keys;
  key_tree_.collect_leaf_contents(known_keys);

  for (std::list<the_keybd_key_t>::iterator i = known_keys.begin();
       i != known_keys.end(); ++i)
  {
    const the_keybd_key_t & key = *i;
    if (key.down()) keys.push_back(key);
  }

  return keys.size();
}

//----------------------------------------------------------------
// the_keybd_t::verify_pressed_keys
//
bool
the_keybd_t::
verify_pressed_keys(const std::list<the_keybd_key_t> & key_list) const
{
  std::list<the_keybd_key_t> pressed_keys;
  collect_pressed_keys(pressed_keys);

  // easy test:
  if (key_list.size() != pressed_keys.size())
  {
    return false;
  }

  // make sure that both lists have the same elements,
  // the list order does not matter:
  for (std::list<the_keybd_key_t>::const_iterator i = pressed_keys.begin();
       i != pressed_keys.end(); ++i)
  {
    if (has(key_list, *i) == false)
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// the_keybd_t::forget_pressed_keys
//
void
the_keybd_t::forget_pressed_keys()
{
  typedef the_bit_tree_leaf_t<the_keybd_key_t> node_t;

  std::list<node_t *> known_keys;
  key_tree_.collect_leaf_nodes(known_keys);

  for (std::list<node_t *>::iterator i = known_keys.begin();
       i != known_keys.end(); ++i)
  {
    the_keybd_key_t & key = (*i)->elem;
    key.reset();
  }
}

//----------------------------------------------------------------
// the_keybd_t::init_key
//
void
the_keybd_t::init_key(const key_id_t & id,
		      unsigned int key,
		      unsigned int mod)
{
  if (key_tree_.get(key) == NULL)
  {
    key_tree_.add(key);
  }

  key_[id] = &(key_tree_.get(key)->elem);
  key_[id]->setup(key, mod);
}

//----------------------------------------------------------------
// the_keybd_t::init_ascii
//
void
the_keybd_t::init_ascii(unsigned char ascii, unsigned int key)
{
  if (key_tree_.get(key) == NULL)
  {
    key_tree_.add(key);
  }

  ascii_[ascii] = &(key_tree_.get(key)->elem);
  ascii_[ascii]->setup(key);
}

//----------------------------------------------------------------
// the_keybd_t::ascii
//
the_keybd_key_t &
the_keybd_t::ascii(unsigned char id)
{
  if (ascii_[id] == NULL)
  {
    assert(false);
    static the_keybd_key_t dummy;
    return dummy;
  }

  return *ascii_[id];
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & sout, const the_keybd_t & keybd)
{
  std::list<the_keybd_key_t> pressed_keys;
  keybd.collect_pressed_keys(pressed_keys);

  sout << "pressed keys: " << endl;
  for (std::list<the_keybd_key_t>::iterator iter = pressed_keys.begin();
       iter != pressed_keys.end(); ++iter)
  {
    sout << *iter;
  }
  sout << endl;
  return sout;
}

//----------------------------------------------------------------
// dump
//
void
dump(const the_keybd_t & keybd)
{
  cerr << keybd;
}

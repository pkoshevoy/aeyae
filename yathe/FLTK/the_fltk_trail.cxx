// File         : the_fltk_trail.cxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003, 2004
// License      : GPL.
// Description  : event trail recoring/playback (regression testing).

// local includes:
#include "FLTK/the_fltk_trail.hxx"
#include "FLTK/the_fltk_input_device_event.hxx"
#include "utils/the_utils.hxx"
#include "utils/the_text.hxx"

// FLTK includes:
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Enumerations.H>


//----------------------------------------------------------------
// the_fltk_trail_t::the_fltk_trail_t
// 
the_fltk_trail_t::
the_fltk_trail_t(int & argc, char ** argv, bool record_by_default):
  the_trail_t(argc, argv, record_by_default)
{
  the_mouse_event_t::setup_transition_detectors(FL_PUSH, FL_RELEASE);
  mouse_.setup_buttons(FL_BUTTON1,
		       FL_BUTTON2,
		       FL_BUTTON3);
  
  the_keybd_event_t::setup_transition_detectors(FL_KEYDOWN, FL_KEYUP);
  
  keybd_.init_key(the_keybd_t::SHIFT, FL_Shift_L, FL_SHIFT);
  keybd_.init_key(the_keybd_t::ALT, FL_Alt_L, FL_ALT);
  keybd_.init_key(the_keybd_t::CONTROL, FL_Control_L, FL_CTRL);
  keybd_.init_key(the_keybd_t::META, FL_Meta_L, FL_META);
  
  keybd_.init_key(the_keybd_t::ARROW_UP, FL_Up);
  keybd_.init_key(the_keybd_t::ARROW_DOWN, FL_Down);
  keybd_.init_key(the_keybd_t::ARROW_LEFT, FL_Left);
  keybd_.init_key(the_keybd_t::ARROW_RIGHT, FL_Right);
  
  keybd_.init_key(the_keybd_t::PAGE_UP, FL_Page_Up);
  keybd_.init_key(the_keybd_t::PAGE_DOWN, FL_Page_Down);
  
  keybd_.init_key(the_keybd_t::HOME, FL_Home);
  keybd_.init_key(the_keybd_t::END, FL_End);
  
  keybd_.init_key(the_keybd_t::INSERT, FL_Insert);
  keybd_.init_key(the_keybd_t::DELETE, FL_Delete);
  
  keybd_.init_key(the_keybd_t::ESCAPE, FL_Escape);
  keybd_.init_key(the_keybd_t::TAB, FL_Tab);
  keybd_.init_key(the_keybd_t::BACKSPACE, FL_BackSpace);
  keybd_.init_key(the_keybd_t::RETURN, FL_Enter);
  keybd_.init_key(the_keybd_t::ENTER, FL_Enter);
  keybd_.init_key(the_keybd_t::SPACE, ' ');
  
  keybd_.init_key(the_keybd_t::F1, FL_F + 1);
  keybd_.init_key(the_keybd_t::F2, FL_F + 2);
  keybd_.init_key(the_keybd_t::F3, FL_F + 3);
  keybd_.init_key(the_keybd_t::F4, FL_F + 4);
  keybd_.init_key(the_keybd_t::F5, FL_F + 5);
  keybd_.init_key(the_keybd_t::F6, FL_F + 6);
  keybd_.init_key(the_keybd_t::F7, FL_F + 7);
  keybd_.init_key(the_keybd_t::F8, FL_F + 8);
  keybd_.init_key(the_keybd_t::F9, FL_F + 9);
  keybd_.init_key(the_keybd_t::F10, FL_F + 10);
  keybd_.init_key(the_keybd_t::F11, FL_F + 11);
  keybd_.init_key(the_keybd_t::F12, FL_F + 12);
  
  keybd_.init_key(the_keybd_t::NUMLOCK, FL_Num_Lock);
  keybd_.init_key(the_keybd_t::CAPSLOCK, FL_Caps_Lock);
  
  keybd_.init_ascii('~', '~');
  keybd_.init_ascii('-', '-');
  keybd_.init_ascii('=', '=');
  keybd_.init_ascii('\\', '\\');
  keybd_.init_ascii('\t', FL_Tab);
  keybd_.init_ascii('[', '[');
  keybd_.init_ascii(']', ']');
  keybd_.init_ascii('\n', FL_Enter);
  keybd_.init_ascii('\r', FL_Enter);
  keybd_.init_ascii(';', ';');
  keybd_.init_ascii('\'', '\'');
  keybd_.init_ascii(',', ',');
  keybd_.init_ascii('.', '.');
  keybd_.init_ascii('/', '/');
  keybd_.init_ascii(' ', ' ');
  
  keybd_.init_ascii('0', '0');
  keybd_.init_ascii('1', '1');
  keybd_.init_ascii('2', '2');
  keybd_.init_ascii('3', '3');
  keybd_.init_ascii('4', '4');
  keybd_.init_ascii('5', '5');
  keybd_.init_ascii('6', '6');
  keybd_.init_ascii('7', '7');
  keybd_.init_ascii('8', '8');
  keybd_.init_ascii('9', '9');
  
  keybd_.init_ascii('A', 'a');
  keybd_.init_ascii('B', 'b');
  keybd_.init_ascii('C', 'c');
  keybd_.init_ascii('D', 'd');
  keybd_.init_ascii('E', 'e');
  keybd_.init_ascii('F', 'f');
  keybd_.init_ascii('G', 'g');
  keybd_.init_ascii('H', 'h');
  keybd_.init_ascii('I', 'i');
  keybd_.init_ascii('J', 'j');
  keybd_.init_ascii('K', 'k');
  keybd_.init_ascii('L', 'l');
  keybd_.init_ascii('M', 'm');
  keybd_.init_ascii('N', 'n');
  keybd_.init_ascii('O', 'o');
  keybd_.init_ascii('P', 'p');
  keybd_.init_ascii('Q', 'q');
  keybd_.init_ascii('R', 'r');
  keybd_.init_ascii('S', 's');
  keybd_.init_ascii('T', 't');
  keybd_.init_ascii('U', 'u');
  keybd_.init_ascii('V', 'v');
  keybd_.init_ascii('W', 'w');
  keybd_.init_ascii('X', 'x');
  keybd_.init_ascii('Y', 'y');
  keybd_.init_ascii('Z', 'z');
  
}

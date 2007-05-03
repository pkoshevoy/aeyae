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


// File         : the_fltk_std_widgets.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Oct 04 15:00:20 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Convinence functions for laying out standard FLTK widgets.

#ifndef THE_FLTK_STD_WIDGETS_HXX_
#define THE_FLTK_STD_WIDGETS_HXX_

// system includes:
#include <stddef.h>

// FLTK includes:
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Roller.H>
#include <FL/fl_draw.H>


// some constant dimensions common to all dialogs:
extern const unsigned int FS; // font size
extern const unsigned int BH; // button height
extern const unsigned int BW; // button width
extern const unsigned int PD; // padding
extern const unsigned int PH; // half the padding


extern Fl_Box *
std_spacer(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h);

extern Fl_Group *
std_groupbox(const unsigned int & x,
	     const unsigned int & y,
	     const unsigned int & w,
	     const unsigned int & h,
	     const char * title = NULL);

extern Fl_Button *
std_button(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const char * title,
	   Fl_Callback cb,
	   const size_t & event);

extern Fl_Check_Button *
std_checkbox(const unsigned int & x,
	     const unsigned int & y,
	     const unsigned int & w,
	     const unsigned int & h,
	     const char * title,
	     Fl_Callback cb,
	     const size_t & event,
	     const bool & value = false);

extern Fl_Round_Button *
std_radiobutton(const unsigned int & x,
		const unsigned int & y,
		const unsigned int & w,
		const unsigned int & h,
		const char * title,
		Fl_Callback cb,
		const size_t & event,
		const bool & value = false);

extern Fl_Box *
std_label(const unsigned int & x,
	  const unsigned int & y,
	  const unsigned int & w,
	  const unsigned int & h,
	  const char * title);

extern Fl_Output *
std_output(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const char * value);

extern Fl_Float_Input *
std_float_input(const unsigned int & x,
		const unsigned int & y,
		const unsigned int & w,
		const unsigned int & h,
		const float & value,
		Fl_Callback cb,
		const size_t & event);

extern Fl_Int_Input *
std_int_input(const unsigned int & x,
	      const unsigned int & y,
	      const unsigned int & w,
	      const unsigned int & h,
	      const int & value,
	      Fl_Callback cb,
	      const size_t & event);

extern Fl_Select_Browser *
std_listbox(const unsigned int & x,
	    const unsigned int & y,
	    const unsigned int & w,
	    const unsigned int & h,
	    Fl_Callback cb,
	    const size_t & event);

extern Fl_Slider *
std_slider(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const unsigned char & type,
	   Fl_Callback cb,
	   const size_t & event);

extern Fl_Roller *
std_roller(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const unsigned char & type,
	   Fl_Callback cb,
	   const size_t & event);

extern void
wdg_enable(Fl_Widget * widget, const bool & enable);


#endif // THE_FLTK_STD_WIDGETS_HXX_

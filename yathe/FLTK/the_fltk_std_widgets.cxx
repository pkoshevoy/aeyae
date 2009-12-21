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


// File         : the_fltk_std_widgets.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Oct 04 15:07:10 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Convinence functions for laying out standard FLTK widgets.

// local includes:
#include "FLTK/the_fltk_std_widgets.hxx"
#include "utils/the_text.hxx"

// some constant dimensions common to all dialogs:
const unsigned int FS = 11; // font size
const unsigned int BH = FS + 8; // button height
const unsigned int BW = FS * 6; // button width
const unsigned int PD = FS - 1; // padding
const unsigned int PH = PD / 2; // smaller padding


//----------------------------------------------------------------
// std_spacer
// 
Fl_Box *
std_spacer(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h)
{
  Fl_Box * spacer = new Fl_Box(x, y, w, h);
#ifdef DEBUG_LAYOUT
  spacer->box(FL_FLAT_BOX);
  spacer->color(FL_WHITE);
#endif
  
  return spacer;
}

//----------------------------------------------------------------
// estimate_text_pixels
// 
static unsigned int
estimate_text_pixels(const char * text, const unsigned int & font_size)
{
  if (text == NULL) return 0;
  
  int saved_ff = fl_font();
  int saved_fs = fl_size();
  fl_font(saved_ff, font_size);
  
  int w = 0;
  int h = 0;
  fl_measure(text, w, h);
  
  fl_font(saved_ff, saved_fs);
  return w;
}

//----------------------------------------------------------------
// std_groupbox
// 
Fl_Group *
std_groupbox(const unsigned int & x,
	     const unsigned int & y,
	     const unsigned int & w,
	     const unsigned int & h,
	     const char * title)
{
  Fl_Group * g = new Fl_Group(x, y, w, h);
#ifdef DEBUG_LAYOUT
  g->box(FL_FLAT_BOX);
  g->color(FL_BLUE);
#endif
  
  // make a title like in Qt:
  if (title != NULL)
  {
#ifndef DEBUG_LAYOUT
    g->box(FL_ENGRAVED_FRAME);
#endif
    
    unsigned int tx = x + PH;
    unsigned int ty = y - PD / 2;
    unsigned int tw = estimate_text_pixels(title, FS) + PH;
    
    Fl_Group * gt = new Fl_Group(x, ty, w, PD);
#ifdef DEBUG_LAYOUT
    gt->box(FL_FLAT_BOX);
    gt->color(FL_RED);
#endif
    
    Fl_Box * label = new Fl_Box(FL_FLAT_BOX, tx, ty, tw, PD, title);
    label->labelsize(FS);
    label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    gt->resizable(std_spacer(tx + tw, ty, w - ((tx - x) + tw), PD));
    gt->end();
  }
  
  return g;
}

//----------------------------------------------------------------
// std_button
// 
Fl_Button *
std_button(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const char * title,
	   Fl_Callback cb,
	   const size_t & event)
{
  Fl_Button * button = new Fl_Button(x, y, w, h, title);
  button->labelsize(FS);
  button->callback(cb, (void *)event);
  button->box(FL_PLASTIC_UP_BOX);
  return button;
}

//----------------------------------------------------------------
// std_checkbox
// 
Fl_Check_Button *
std_checkbox(const unsigned int & x,
	     const unsigned int & y,
	     const unsigned int & w,
	     const unsigned int & h,
	     const char * title,
	     Fl_Callback cb,
	     const size_t & event,
	     const bool & value)
{
  Fl_Check_Button * checkbox = new Fl_Check_Button(x, y, w, h, title);
  checkbox->labelsize(FS);
  checkbox->callback(cb, (void *)event);
  checkbox->value(int(value));
  return checkbox;
}

//----------------------------------------------------------------
// std_radiobutton
// 
Fl_Round_Button *
std_radiobutton(const unsigned int & x,
		const unsigned int & y,
		const unsigned int & w,
		const unsigned int & h,
		const char * title,
		Fl_Callback cb,
		const size_t & event,
		const bool & value)
{
  Fl_Round_Button * radiobtn = new Fl_Round_Button(x, y, w, h, title);
  radiobtn->labelsize(FS);
  radiobtn->type(FL_RADIO_BUTTON);
  radiobtn->selection_color(FL_RED);
  radiobtn->value(int(value));
  radiobtn->callback(cb, (void *)event);
  return radiobtn;
}

//----------------------------------------------------------------
// std_label
// 
Fl_Box *
std_label(const unsigned int & x,
	  const unsigned int & y,
	  const unsigned int & w,
	  const unsigned int & h,
	  const char * title)
{
  Fl_Box * label = new Fl_Box(x, y, w, h, title);
  label->labelsize(FS);
  label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  return label;
}

//----------------------------------------------------------------
// std_output
// 
Fl_Output *
std_output(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const char * value)
{
  Fl_Output * output = new Fl_Output(x, y, w, h);
  output->textsize(FS);
  output->value(value);
  return output;
}

//----------------------------------------------------------------
// std_float_input
// 
Fl_Float_Input *
std_float_input(const unsigned int & x,
		const unsigned int & y,
		const unsigned int & w,
		const unsigned int & h,
		const float & value,
		Fl_Callback cb,
		const size_t & event)
{
  Fl_Float_Input * input = new Fl_Float_Input(x, y, w, h);
  input->textsize(FS);
  input->value(the_text_t::number(value));
  input->callback(cb, (void *)event);
  return input;
}

//----------------------------------------------------------------
// std_int_input
// 
Fl_Int_Input *
std_int_input(const unsigned int & x,
		const unsigned int & y,
		const unsigned int & w,
		const unsigned int & h,
		const int & value,
		Fl_Callback cb,
		const size_t & event)
{
  Fl_Int_Input * input = new Fl_Int_Input(x, y, w, h);
  input->textsize(FS);
  input->value(the_text_t::number(value));
  input->callback(cb, (void *)event);
  return input;
}

//----------------------------------------------------------------
// std_listbox
// 
Fl_Select_Browser *
std_listbox(const unsigned int & x,
	    const unsigned int & y,
	    const unsigned int & w,
	    const unsigned int & h,
	    Fl_Callback cb,
	    const size_t & event)
{
  Fl_Select_Browser * listbox = new Fl_Select_Browser(x, y, w, h);
  listbox->textsize(FS);
  listbox->callback(cb, (void *)event);
  return listbox;
}

//----------------------------------------------------------------
// std_slider
// 
extern Fl_Slider *
std_slider(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const unsigned char & type,
	   Fl_Callback cb,
	   const size_t & event)
{
  Fl_Slider * slider = new Fl_Slider(x, y, w, h);
  slider->labelsize(FS);
  slider->callback(cb, (void *)event);
  slider->type(type);
  slider->selection_color(FL_RED);
  return slider;
}

//----------------------------------------------------------------
// std_roller
// 
extern Fl_Roller *
std_roller(const unsigned int & x,
	   const unsigned int & y,
	   const unsigned int & w,
	   const unsigned int & h,
	   const unsigned char & type,
	   Fl_Callback cb,
	   const size_t & event)
{
  Fl_Roller * roller = new Fl_Roller(x, y, w, h);
  roller->labelsize(FS);
  roller->callback(cb, (void *)event);
  roller->type(type);
  roller->box(FL_BORDER_FRAME);
  roller->selection_color(FL_RED);
  
  return roller;
}


//----------------------------------------------------------------
// the_violator_t
// 
// FIXME: 2004/10/06: all this to avoid an infinite loop:
// 
class the_violator_t : public Fl_Widget
{
public:
  the_violator_t(): Fl_Widget(0, 0, 0, 0, NULL) {}
  void draw() {}
  
  void wdg_enable(Fl_Widget * widget, const bool & enable)
  {
    if (widget == NULL) return;
    if (enable == (widget->active() != 0)) return;
    
    if (enable)
    {
      widget->activate();
      return;
    }
    
    // avoid calling fl_fix_focus() - it causes an infinite loop:
    the_violator_t * violator = (the_violator_t *)(widget);
    violator->set_flag(INACTIVE);
    violator->redraw();
    violator->redraw_label();
    violator->handle(FL_DEACTIVATE);
  }
};

//----------------------------------------------------------------
// wdg_enable
// 
void
wdg_enable(Fl_Widget * widget, const bool & enable)
{
  static the_violator_t violator;
  violator.wdg_enable(widget, enable);
}

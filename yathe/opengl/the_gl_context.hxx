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


// File         : the_gl_context.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Jan 31 11:07:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : An OpenGL rendering context wrapper class.

#ifndef THE_GL_CONTEXT_HXX_
#define THE_GL_CONTEXT_HXX_

// forward declaration:
class the_view_t;


//----------------------------------------------------------------
// the_gl_context_t
// 
// Abstract class for toolkit-independent opengl context API.
// A subclass must be made for each toolkit.
// 
class the_gl_context_t
{
public:
  the_gl_context_t(the_view_t * view = NULL);
  ~the_gl_context_t();
  
  void make_current();
  void done_current();
  bool is_valid() const;
  void invalidate();
  
  static the_gl_context_t current();
  
protected:
  the_view_t * view_;
};


#endif // THE_GL_CONTEXT_HXX_

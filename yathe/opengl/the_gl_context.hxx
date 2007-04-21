// File         : the_gl_context.hxx
// Author       : Paul A. Koshevoy
// Created      : Wed Jan 31 11:07:00 MST 2007
// Copyright    : (C) 2007
// License      : GPL.
// Description  : 

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

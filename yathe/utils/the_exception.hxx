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


// File         : the_exception.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 18:06:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : an exception convenience class

#ifndef THE_EXCEPTION_HXX_
#define THE_EXCEPTION_HXX_

// system includes:
#include <exception>
#include <string>
#include <sstream>


//----------------------------------------------------------------
// the_exception_t
// 
class the_exception_t : public std::exception
{
public:
  the_exception_t(const char * description = NULL,
		  const char * file = NULL,
		  const unsigned int & line = 0)
  {
    std::ostringstream os;
    
    if (file != NULL)
    {
      os << file << ':' << line << " -- ";
    }
    
    if (description != NULL)
    {
      os << description;
    }
    
    what_ = os.str();
  }
  
  virtual ~the_exception_t() throw ()
  {}
  
  // virtual:
  const char * what() const throw()
  { return what_.c_str(); }
  
  // data:
  std::string what_;
};


#endif // THE_EXCEPTION_HXX_

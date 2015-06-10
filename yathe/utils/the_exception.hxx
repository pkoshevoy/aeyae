// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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

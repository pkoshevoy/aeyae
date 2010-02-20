// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_indentation.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A convenience class used for text indentation.

// local includes:
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_indent_str_t
// 
class the_indent_str_t
{
public:
  the_indent_str_t(const char & ch,
			 const unsigned int & size):
    string_(NULL),
    size_(size)
  {
    string_ = new char [size_ + 1];
    for (unsigned int i = 0; i < size_; i++) string_[i] = ch;
    string_[size_] = '\0';
  }
  
  ~the_indent_str_t()
  {
    delete [] string_;
  }
  
  inline const char * indent(const unsigned int & tabs) const
  {
    return &string_[size_ - tabs];
  }
  
private:
  char * string_;
  unsigned int size_;
};


//----------------------------------------------------------------
// operator <<
// 
std::ostream &
operator << (std::ostream & stream, const indtkn_t & t)
{
  static const the_indent_str_t ind_space(' ', 7);
  static const the_indent_str_t ind_tab('\t', 64);
  
  unsigned int tabs = t.ind / 8;
  unsigned int spcs = t.ind % 8;
  return stream << ind_tab.indent(tabs) << ind_space.indent(spcs);
}

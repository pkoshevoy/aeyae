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

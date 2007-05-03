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

#ifndef THE_INDENTATION_HXX_
#define THE_INDENTATION_HXX_

// system includes:
#include <iostream>
#include <iomanip>

// namespace access:
using std::ios;
using std::istream;
using std::ostream;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::setw;
using std::cout;
using std::cerr;
using std::endl;
using std::ws;


//----------------------------------------------------------------
// indtkn_t
// 
class indtkn_t
{
public:
  indtkn_t(const unsigned int & i): ind(i) {}
  ~indtkn_t() {}

  unsigned int ind;

private:
  indtkn_t(): ind(0) {}
};

//----------------------------------------------------------------
// operator <<
// 
extern std::ostream &
operator << (std::ostream & stream, const indtkn_t & t);


//----------------------------------------------------------------
// INDSTP
//
static const unsigned int INDSTP = 2;

//----------------------------------------------------------------
// INDSTR
// 
#define INDSTR indtkn_t(indent + INDSTP)

//----------------------------------------------------------------
// INDSCP
// 
#define INDSCP indtkn_t(indent)

//----------------------------------------------------------------
// INDNXT
// 
#define INDNXT (indent + INDSTP)


#endif // THE_INDENTATION_HXX_

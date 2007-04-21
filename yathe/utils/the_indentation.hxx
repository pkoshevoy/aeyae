// File         : the_indentation.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  : 

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

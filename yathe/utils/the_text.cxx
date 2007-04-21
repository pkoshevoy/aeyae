// File         : the_text.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Aug 29 14:53:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : ASCII text.

// local includes:
#include "utils/the_text.hxx"

// system includes:
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <iostream>
#include <list>


//----------------------------------------------------------------
// the_text_t::assign
// 
void
the_text_t::assign(const char * text, const size_t & text_size)
{
  char * text_copy = new char [text_size + 1];
  for (unsigned int i = 0; i < text_size; i++)
  {
    text_copy[i] = text[i];
  }
  
  text_copy[text_size] = '\0';
  
  delete [] text_;
  text_ = text_copy;
  size_ = text_size;
}

//----------------------------------------------------------------
// the_text_t::append
// 
void
the_text_t::append(const char * text, const size_t & text_size)
{
  char * text_copy = new char [size_ + text_size + 1];
  
  for (unsigned int i = 0; i < size_; i++)
  {
    text_copy[i] = text_[i];
  }
  
  for (unsigned int i = 0; i < text_size; i++)
  {
    text_copy[i + size_] = text[i];
  }
  
  text_copy[size_ + text_size] = '\0';
  
  delete [] text_;
  text_ = text_copy;
  size_ += text_size;
}

//----------------------------------------------------------------
// the_text_t::toShort
// 
short int
the_text_t::toShort(bool * ok, int base) const
{
  long int num = toULong(ok, base);
  if (ok != NULL) *ok &= ((num >= SHRT_MIN) && (num <= SHRT_MAX));
  return num;
}

//----------------------------------------------------------------
// the_text_t::toUShort
// 
unsigned short int
the_text_t::toUShort(bool * ok, int base) const
{
  unsigned long int num = toULong(ok, base);
  if (ok != NULL) *ok &= (num <= USHRT_MAX);
  return num;
}

//----------------------------------------------------------------
// the_text_t::toInt
// 
int
the_text_t::toInt(bool * ok, int base) const
{
  long int num = toULong(ok, base);
  if (ok != NULL) *ok &= ((num >= INT_MIN) && (num <= INT_MAX));
  return num;
}

//----------------------------------------------------------------
// the_text_t::toUInt
// 
unsigned int
the_text_t::toUInt(bool * ok, int base) const
{
  unsigned long int num = toULong(ok, base);
  if (ok != NULL) *ok &= (num <= UINT_MAX);
  return num;
}

//----------------------------------------------------------------
// the_text_t::toLong
// 
long int
the_text_t::toLong(bool * ok, int base) const
{
  char * endptr = NULL;
  long int num = strtol(text_, &endptr, base);
  if (ok != NULL) *ok = !(text_ == endptr || errno == ERANGE);
  return num;
}

//----------------------------------------------------------------
// the_text_t::toULong
// 
unsigned long int
the_text_t::toULong(bool * ok, int base) const
{
  char * endptr = NULL;
  unsigned long int num = strtoul(text_, &endptr, base);
  if (ok != NULL) *ok = !(text_ == endptr || errno == ERANGE);
  return num;
}

//----------------------------------------------------------------
// the_text_t::toFloat
// 
float
the_text_t::toFloat(bool * ok) const
{
  char * endptr = NULL;
  float num = strtod(text_, &endptr);
  if (ok != NULL) *ok = !(text_ == endptr || errno == ERANGE);
  return num;
}

//----------------------------------------------------------------
// the_text_t::toDouble
// 
double
the_text_t::toDouble(bool * ok) const
{
  char * endptr = NULL;
  double num = strtod(text_, &endptr);
  if (ok != NULL) *ok = !(text_ == endptr || errno == ERANGE);
  return num;
}

//----------------------------------------------------------------
// the_text_t::to_lower
// 
void
the_text_t::to_lower()
{
  for (unsigned int i = 0; i < size_; i++)
  {
    text_[i] = tolower(text_[i]);
  }
}

//----------------------------------------------------------------
// the_text_t::to_upper
// 
void
the_text_t::to_upper()
{
  for (unsigned int i = 0; i < size_; i++)
  {
    text_[i] = toupper(text_[i]);
  }
}

//----------------------------------------------------------------
// the_text_t::fill
// 
void
the_text_t::fill(const char & c, const unsigned int size)
{
  char * text = new char [size + 1];
  for (unsigned int i = 0; i < size; i++)
  {
    text[i] = c;
  }
  text[size] = '\0';
  
  delete [] text_;
  text_ = text;
  size_ = size;
}

//----------------------------------------------------------------
// the_text_t::match_head
// 
bool
the_text_t::match_head(const the_text_t & t, bool ignore_case) const
{
  if (t.size_ > size_) return false;
  return match_text(t, 0, ignore_case);
}

//----------------------------------------------------------------
// the_text_t::match_tail
// 
bool
the_text_t::match_tail(const the_text_t & t, bool ignore_case) const
{
  if (t.size_ > size_) return false;
  return match_text(t, size_ - t.size_, ignore_case);
}

//----------------------------------------------------------------
// the_text_t::match_text
// 
bool
the_text_t::match_text(const the_text_t & t,
		       const unsigned int & start,
		       bool ignore_case) const
{
  unsigned int end = start + t.size_;
  if (end > size_) return false;
  
  for (unsigned int i = start; i < end; i++)
  {
    char a = text_[i];
    char b = t.text_[i - start];
    
    if (ignore_case)
    {
      a = tolower(a);
      b = tolower(b);
    }
    
    if (a != b) return false;
  }
  
  return true;
}

//----------------------------------------------------------------
// the_text_t::simplify_ws
// 
the_text_t
the_text_t::simplify_ws() const
{
  // find the first non-whitespace character:
  int start = 0;
  for (; start < int(size_) && isspace(text_[start]); start++);
  
  // NOTE: an all-whitespace string will simplify to an empty string:
  if (start == int(size_)) return the_text_t("");
  
  // find the last non-whitespace character:
  int finish = size_ - 1;
  for (; finish >= start && isspace(text_[finish]); finish--);
  
  // intermediate storage:
  the_text_t tmp;
  tmp.fill('\0', (finish + 1) - start);
  
  unsigned int j = 0;
  bool prev_ws = false;
  for (int i = start; i <= finish; i++)
  {
    char c = isspace(text_[i]) ? ' ' : text_[i];
    if (c == ' ' && prev_ws) continue;
    
    prev_ws = (c == ' ');
    tmp.text_[j] = c;
    j++;
  }
  
  the_text_t out(tmp.text(), j);
  return out;
}

//----------------------------------------------------------------
// the_text_t::split
// 
unsigned int
the_text_t::split(std::vector<the_text_t> & tokens,
		  const char & separator,
		  const bool & empty_ok) const
{
  if (size_ == 0)
  {
    tokens.resize(0);
    return 0;
  }
  
  // find the separators:
  typedef std::list<unsigned int> list_t;
  list_t separators;
  for (unsigned int i = 0; i < size_; i++)
  {
    if (text_[i] != separator) continue;
    separators.push_back(i);
  }
  separators.push_back(size_);
  
  std::list<the_text_t> tmp;
  
  typedef std::list<unsigned int>::iterator iter_t;
  unsigned int a = 0;
  for (iter_t i = separators.begin(); i != separators.end(); ++i)
  {
    unsigned int b = *i;
    
    if (b - a == 0 && empty_ok)
    {
      tmp.push_back(the_text_t(""));
    }
    else if (b - a > 0)
    {
      tmp.push_back(the_text_t(&text_[a], b - a));
    }
    
    a = b + 1;
  }
  
  tokens.resize(tmp.size());
  a = 0;
  for (std::list<the_text_t>::iterator i = tmp.begin(); i != tmp.end(); ++i)
  {
    tokens[a] = *i;
    a++;
  }
  
  return tmp.size();
}

//----------------------------------------------------------------
// the_text_t::contains
// 
unsigned int
the_text_t::contains(const char & symbol) const
{
  unsigned int count = 0;
  for (unsigned int i = 0; i < size_; i++)
  {
    if (text_[i] != symbol) continue;
    count++;
  }
  
  return count;
}


//----------------------------------------------------------------
// operator <<
// 
std::ostream &
operator << (std::ostream & out, const the_text_t & text)
{
  std::string tmp(text.text(), text.size());
  out << tmp;
  return out;
}

//----------------------------------------------------------------
// operator >>
// 
std::istream &
operator >> (std::istream & in, the_text_t & text)
{
  std::string tmp;
  in >> tmp;
  text.assign(tmp.data(), tmp.size());
  return in;
}

//----------------------------------------------------------------
// getline
// 
std::istream &
getline(std::istream & in, the_text_t & text)
{
  std::string tmp;
  getline(in, tmp);
  text.assign(tmp.data(), tmp.size());
  return in;
}

//----------------------------------------------------------------
// to_binary
// 
the_text_t
to_binary(const unsigned char & byte, unsigned int lsb_first)
{
  the_text_t str;
  str.fill('0', 8);
  
  unsigned char mask = 1;
  if (lsb_first)
  {
    for (int i = 0; i < 8; i++, mask *= 2)
    {
      str[i] = (byte & mask) ? '1' : '0';
    }
  }
  else
  {
    for (int i = 7; i > -1; i--, mask *= 2)
    {
      str[i] = (byte & mask) ? '1' : '0';
    }
  }
  
  return str;
}

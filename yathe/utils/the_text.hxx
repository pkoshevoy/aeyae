// File         : the_text.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Aug 29 14:53:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : ASCII text.

#ifndef THE_TEXT_HXX_
#define THE_TEXT_HXX_

// system includes:
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>


//----------------------------------------------------------------
// the_text_t
// 
class the_text_t
{
public:
  the_text_t(const char * text = ""):
    text_(NULL),
    size_(0)
  { assign(text); }
  
  the_text_t(const char * text, const size_t & size):
    text_(NULL),
    size_(0)
  { assign(text, size); }
  
  the_text_t(const the_text_t & text):
    text_(NULL),
    size_(0)
  { assign(text.text_, text.size_); }
  
  ~the_text_t()
  { clear(); }
  
  // assignment operator:
  inline the_text_t & operator = (const the_text_t & text)
  {
    if (this != &text)
    {
      assign(text.text_, text.size_);
    }
    
    return *this;
  }
  
  // clear the string:
  inline void clear()
  {
    delete [] text_;
    text_ = NULL;
    size_ = 0;
  }
  
  // shorthand:
  inline bool is_empty() const
  { return size_ == 0; }
  
  // assign a new string to this text:
  inline void assign(const char * text)
  { assign(text, strlen(text)); }
  
  void assign(const char * text, const size_t & text_size);
  
  // append a new string to this text:
  inline void append(const char * text)
  { append(text, strlen(text)); }
  
  void append(const char * text, const size_t & text_size);
  
  // equality/inequality tests:
  inline bool operator == (const the_text_t & text) const
  { return ((size_ == text.size_) && (strcmp(text_, text.text_) == 0)); }
  
  inline bool operator != (const the_text_t & text) const
  { return !(*this == text); }
  
  inline bool operator == (const char * text) const
  { return (*this == the_text_t(text)); }
  
  inline bool operator != (const char * text) const
  { return !(*this == text); }
  
  inline bool operator < (const the_text_t & text) const
  { return (strcmp(text_, text.text_) < 0); }
  
  inline bool operator > (const the_text_t & text) const
  { return (strcmp(text_, text.text_) > 0); }
  
  // arithmetic:
  inline the_text_t & operator += (const the_text_t & text)
  {
    append(text.text_, text.size_);
    return *this;
  }
  
  inline the_text_t operator + (const the_text_t & text) const
  {
    the_text_t text_sum(*this);
    text_sum += text;
    return text_sum;
  }
  
  // access operators:
  template <typename index_t>
  inline const char & operator [] (const index_t & index) const
  { return text_[index]; }
  
  template <typename index_t>
  inline char & operator [] (const index_t & index)
  { return text_[index]; }
  
  // accessors:
  inline const char * text() const
  { return text_; }
  
  inline const size_t & size() const
  { return size_; }
  
  // conversion operator:
  inline operator const char * () const
  { return text_; }
  
  // helpers:
  template <class number_t>
  static the_text_t number(const number_t & number,
			   const unsigned int width = 0,
			   const char pad = ' ',
			   const bool pad_left = true)
  {
    std::ostringstream os;
    os << number;
    
    std::string str = os.str();
    the_text_t num_txt(str.data(), str.size());
    
    if (width > num_txt.size())
    {
      the_text_t padding;
      padding.fill(pad, width - num_txt.size());
      num_txt = pad_left ? padding + num_txt : num_txt + padding;
    }
    
    return num_txt;
  }
  
  short int		toShort(bool * ok = 0, int base = 10) const;
  unsigned short int	toUShort(bool * ok = NULL, int base = 10) const;
  
  int			toInt(bool * ok = NULL, int base = 10) const;
  unsigned int		toUInt(bool * ok = NULL, int base = 10) const;
  
  long int		toLong(bool * ok = NULL, int base = 10) const;
  unsigned long int	toULong(bool * ok = NULL, int base = 10) const;
  
  float			toFloat(bool * ok = NULL) const;
  double		toDouble(bool * ok = NULL) const;
  
  void to_lower();
  void to_upper();
  void fill(const char & c, const unsigned int size);
  
  void fill(const char & c)
  { fill(c, size_); }
  
  // return true if the given text matches the head/tail of this text:
  bool match_head(const the_text_t & t, bool ignore_case = false) const;
  bool match_tail(const the_text_t & t, bool ignore_case = false) const;
  
  bool match_text(const the_text_t & t,
		  const unsigned int & index,
		  bool ignore_case = false) const;
  
  // remove leading/tailing white space, replace internal white space
  // with a single space:
  the_text_t simplify_ws() const;
  
  // split the text into a set of tokens, return the number of tokens:
  unsigned int split(std::vector<the_text_t> & tokens,
		     const char & separator,
		     const bool & empty_ok = false) const;
  
  // count the number of occurrences of a given symbol in the text:
  unsigned int contains(const char & symbol) const;
  
  // extract a portion of the string:
  void extract(the_text_t & to,
	       const unsigned int & from,
	       const unsigned int & size) const
  {
    assert(from + size < size_);
    to.assign(&text_[from], size);
  }
  
  inline the_text_t extract(const unsigned int & from,
			    const unsigned int & size) const
  {
    the_text_t to;
    extract(to, from, size);
    return to;
  }
  
  inline the_text_t reverse() const
  {
    the_text_t rev(*this);
    for (unsigned int i = 0; i < size_; i++)
    {
      rev.text_[i] = text_[size_ - i - 1];
    }
    
    return rev;
  }
  
  inline the_text_t cut(const char & separator,
			unsigned int f0,
			unsigned int f1 = 0) const
  {
    const char sep_str[2] = { separator, '\0' };
    
    std::vector<the_text_t> fields;
    split(fields, separator, true);
    unsigned int num_fields = fields.size();
    
    if (f1 < f0)
    {
      f1 = f0;
    }
    else if (f1 >= num_fields)
    {
      f1 = num_fields - 1;
    }
    
    the_text_t out;
    for (unsigned int f = f0; f <= f1; f++)
    {
      out += fields[f];
      if (f + 1 <= f1)
      {
	out += sep_str;
      }
    }
    
    return out;
  }
  
private:
  // the text itself:
  char * text_;
  
  // the length of the text:
  size_t size_;
};

extern std::ostream &
operator << (std::ostream & out, const the_text_t & text);

extern std::istream &
operator >> (std::istream & in, the_text_t & text);

extern std::istream &
getline(std::istream & in, the_text_t & text);

//----------------------------------------------------------------
// to_binary
//
// return a 0 and 1 string representation of a byte
// 
extern the_text_t
to_binary(const unsigned char & byte, unsigned int lsb_first = true);


#endif // THE_TEXT_HXX_

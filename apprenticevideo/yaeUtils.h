// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:34:13 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_H_
#define YAE_UTILS_H_

// std includes:
#include <list>
#include <string.h>
#include <cstdio>
#include <sstream>

// yae includes:
#include <yaeAPI.h>

// Qt includes:
#include <QString>


namespace yae
{

  YAE_API int
  renameUtf8(const char * fnOldUtf8, const char * fnNewUtf8);

  YAE_API std::FILE *
  fopenUtf8(const char * filenameUtf8, const char * mode);

  YAE_API int
  fileOpenUtf8(const char * filenameUTF8, int accessMode, int permissions);

  YAE_API int64
  fileSeek64(int fd, int64 offset, int whence);

  YAE_API int64
  fileSize64(int fd);

  //----------------------------------------------------------------
  // TOpenHere
  //
  // open an instance of TOpenable in the constructor,
  // close it in the destructor:
  //
  template <typename TOpenable>
  struct TOpenHere
  {
    TOpenHere(TOpenable & something):
      something_(something)
    {
      something_.open();
    }

    ~TOpenHere()
    {
      something_.close();
    }

  protected:
    TOpenable & something_;
  };

  //----------------------------------------------------------------
  // isSizeOne
  //
  template <typename TContainer>
  inline bool
  isSizeOne(const TContainer & c)
  {
    typename TContainer::const_iterator i = c.begin();
    typename TContainer::const_iterator e = c.end();
    return (i != e) && (++i == e);
  }

  //----------------------------------------------------------------
  // isSizeTwoOrMore
  //
  template <typename TContainer>
  inline bool
  isSizeTwoOrMore(const TContainer & c)
  {
    typename TContainer::const_iterator i = c.begin();
    typename TContainer::const_iterator e = c.end();
    return (i != e) && (++i != e);
  }

  //----------------------------------------------------------------
  // compare
  //
  template <typename TData>
  int compare(const TData & a, const TData & b)
  {
    return memcmp(&a, &b, sizeof(TData));
  }

  //----------------------------------------------------------------
  // toQString
  //
  extern QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace = false);

  //----------------------------------------------------------------
  // splitOnCamelCase
  //
  extern void
  splitOnCamelCase(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  extern void
  splitOnGroupTags(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitOnSeparators
  //
  extern void
  splitOnSeparators(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitIntoWords
  //
  extern void
  splitIntoWords(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // toWords
  //
  extern QString toWords(const std::list<QString> & keys);

  //----------------------------------------------------------------
  // toWords
  //
  extern QString toWords(const QString & key);

  //----------------------------------------------------------------
  // isNumeric
  //
  // if the key consists exclusively of numeric characters
  // returns the key length;
  // otherwise returns 0
  //
  extern int isNumeric(const QString & key);

  //----------------------------------------------------------------
  // prepareForSorting
  //
  extern QString prepareForSorting(const QString & key);

  //----------------------------------------------------------------
  // floor_log2
  //
  template <typename TScalar>
  unsigned int
  floor_log2(TScalar given)
  {
    unsigned int n = 0;
    while (given >= 2)
    {
      given /= 2;
      n++;
    }

    return n;
  }

  //----------------------------------------------------------------
  // toText
  //
  template <typename TData>
  std::string
  toText(const TData & data)
  {
    std::ostringstream os;
    os << data;
    return std::string(os.str().c_str());
  }
}


#endif // YAE_UTILS_H_

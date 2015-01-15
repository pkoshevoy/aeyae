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
  // has
  //
  template <typename TContainer>
  bool
  has(const TContainer & container, const typename TContainer::value_type & v)
  {
    typename TContainer::const_iterator iter =
      std::find(container.begin(), container.end(), v);
    return iter != container.end();
  }

  //----------------------------------------------------------------
  // indexOf
  //
  template <typename TData>
  std::size_t
  indexOf(const TData & item, const TData * items, std::size_t numItems)
  {
    for (std::size_t i = 0; i < numItems; ++i, ++items)
    {
      if (item == *items)
      {
        return i;
      }
    }

    return numItems;
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

  //----------------------------------------------------------------
  // toScalar
  //
  template <typename TScalar, typename TText>
  TScalar
  toScalar(const TText & text)
  {
    std::istringstream is;
    is.str(std::string(text));

    TScalar v = (TScalar)0;
    is >> v;

    return v;
  }

  //----------------------------------------------------------------
  // stripHtmlTags
  //
  YAE_API std::string
  stripHtmlTags(const std::string & in);

  //----------------------------------------------------------------
  // assaToPlainText
  //
  YAE_API std::string
  assaToPlainText(const std::string & in);

  //----------------------------------------------------------------
  // convertEscapeCodes
  //
  YAE_API std::string
  convertEscapeCodes(const std::string & in);

  //----------------------------------------------------------------
  // parse_hhmmss_xxx
  //
  // parse hh mm ss xxx timecode string, return time expressed in seconds
  //
  YAE_API double
  parse_hhmmss_xxx(const char * hhmmss,
                   const char * separator = ":",
                   const char * separator_xxx = NULL,
                   const double frameRate = 0.0);

}


#endif // YAE_UTILS_H_

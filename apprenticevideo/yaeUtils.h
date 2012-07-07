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
#include <QPainter>

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
  YAE_API QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace = false);

  //----------------------------------------------------------------
  // splitOnCamelCase
  //
  YAE_API void
  splitOnCamelCase(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  YAE_API void
  splitOnGroupTags(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitOnSeparators
  //
  YAE_API void
  splitOnSeparators(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // splitIntoWords
  //
  YAE_API void
  splitIntoWords(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // toWords
  //
  YAE_API QString toWords(const std::list<QString> & keys);

  //----------------------------------------------------------------
  // toWords
  //
  YAE_API QString toWords(const QString & key);

  //----------------------------------------------------------------
  // isNumeric
  //
  // if the key consists exclusively of numeric characters
  // returns the key length;
  // otherwise returns 0
  //
  YAE_API int isNumeric(const QString & key);

  //----------------------------------------------------------------
  // prepareForSorting
  //
  YAE_API QString prepareForSorting(const QString & key);

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
  // overlapExists
  //
  YAE_API bool
  overlapExists(const QRect & a, const QRect & b);

  //----------------------------------------------------------------
  // overlapExists
  //
  YAE_API bool
  overlapExists(const QRect & a, const QPoint & b);

  //----------------------------------------------------------------
  // shortenTextToFit
  //
  YAE_API bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight);

  //----------------------------------------------------------------
  // drawTextToFit
  //
  YAE_API void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText = NULL);

  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  //
  YAE_API void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & bgPen,
                          int shadowOffset = 1,
                          QRect * bboxText = NULL);

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

}


#endif // YAE_UTILS_H_

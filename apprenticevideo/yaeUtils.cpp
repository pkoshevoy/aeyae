// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:37:50 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

//----------------------------------------------------------------
// __STDC_CONSTANT_MACROS
//
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

//----------------------------------------------------------------
// _FILE_OFFSET_BITS
//
// turn on 64-bit file offsets:
//
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

// system includes:
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <vector>

// yae includes:
#include <yaeAPI.h>
#include <yaeUtils.h>

// Qt includes:
#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QPainter>
#include <QStringList>
#include <QXmlStreamReader>

namespace yae
{

#if defined(_WIN32) && !defined(__MINGW32__)
  //----------------------------------------------------------------
  // cstr_to_utf16
  //
  static wchar_t *
  cstr_to_utf16(const char * cstr, unsigned int code_page = CP_UTF8)
  {
    int sz = MultiByteToWideChar(code_page, 0, cstr, -1, NULL, 0);
    wchar_t * wstr = (wchar_t *)malloc(sz * sizeof(wchar_t));
    MultiByteToWideChar(code_page, 0, cstr, -1, wstr, sz);
    return wstr;
  }

  //----------------------------------------------------------------
  // utf16_to_cstr
  //
  static char *
  utf16_to_cstr(const wchar_t * wstr, unsigned int code_page = CP_UTF8)
  {
    int sz = WideCharToMultiByte(code_page, 0, wstr, -1, NULL, 0, NULL, NULL);
    char * cstr = (char *)malloc(sz);
    WideCharToMultiByte(code_page, 0, wstr, -1, cstr, sz, NULL, NULL);
    return cstr;
  }
#endif

  //----------------------------------------------------------------
  // renameUtf8
  //
  int
  renameUtf8(const char * fnOld, const char * fnNew)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    wchar_t * wold = cstr_to_utf16(fnOld);
    wchar_t * wnew = cstr_to_utf16(fnNew);

    int ret = _wrename(wold, wnew);

    free(wold);
    free(wnew);
#else
    int ret = rename(fnOld, fnNew);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // fopenUtf8
  //
  std::FILE *
  fopenUtf8(const char * filenameUtf8, const char * mode)
  {
    std::FILE * file = NULL;

#if defined(_WIN32) && !defined(__MINGW32__)
    wchar_t * wname = cstr_to_utf16(filenameUtf8);
    wchar_t * wmode = cstr_to_utf16(mode);

    _wfopen_s(&file, wname, wmode);

    free(wname);
    free(wmode);
#else
    file = fopen(filenameUtf8, mode);
#endif

    return file;
  }

  //----------------------------------------------------------------
  // fileOpenUtf8
  //
  int
  fileOpenUtf8(const char * filenameUtf8, int accessMode, int permissions)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    accessMode |= O_BINARY;

    wchar_t * wname = cstr_to_utf16(filenameUtf8);
    int fd = -1;
    int sh = accessMode & (_O_RDWR | _O_WRONLY) ? _SH_DENYWR : _SH_DENYNO;

    errno_t err = _wsopen_s(&fd, wname, accessMode, sh, permissions);
    free(wname);
#else
    int fd = open(filenameUtf8, accessMode, permissions);
#endif

    return fd;
  }

  //----------------------------------------------------------------
  // fileSeek64
  //
  int64
  fileSeek64(int fd, int64 offset, int whence)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    __int64 pos = _lseeki64(fd, offset, whence);
#elif defined(__APPLE__)
    off_t pos = lseek(fd, offset, whence);
#else
    off64_t pos = lseek64(fd, offset, whence);
#endif

    return pos;
  }

  //----------------------------------------------------------------
  // fileSize64
  //
  int64
  fileSize64(int fd)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    struct _stati64 st;
    __int64 ret = _fstati64(fd, &st);
#else
    struct stat st;
    int ret = fstat(fd, &st);
#endif

    if (ret < 0)
    {
      return ret;
    }

    return st.st_size;
  }

  //----------------------------------------------------------------
  // toQString
  //
  QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace)
  {
    QString path;

    for (std::list<QString>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
    {
      if (i != keys.begin())
      {
        path += QString::fromUtf8(" ");
      }

      const QString & key = *i;
      path += trimWhiteSpace ? key.trimmed() : key;
    }

    return path;
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
  // isNumeric
  //
  bool
  isNumeric(const QChar & c)
  {
    return c >= QChar('0') && c <= QChar('9');
  }

  //----------------------------------------------------------------
  // isNumeric
  //
  int
  isNumeric(const QString & key)
  {
    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];
      if (!isNumeric(c))
      {
        return 0;
      }
    }

    return size;
  }

  //----------------------------------------------------------------
  // capitalize
  //
  static QString
  capitalize(const QString & word)
  {
    if (word.isEmpty())
    {
      YAE_ASSERT(false);
      return word;
    }

    QString out = word;
    out[0] = out[0].toUpper();
    return out;
  }

  //----------------------------------------------------------------
  // splitOnVersion
  //
  static void
  splitOnVersion(const QString & key, std::list<QString> & tokens)
  {
    static const QChar kVersionTags[] = {
      QChar::fromAscii('v')
      // QChar::fromAscii('n'),
      // QChar::fromAscii('p')
    };

    static const std::size_t numTags = sizeof(kVersionTags) / sizeof(QChar);

    // attempt to split on camel case:
    QString token;
    QChar versionTag = 0;
    QChar c0 = 0;

    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];

      if (versionTag != 0)
      {
        if (!isNumeric(c))
        {
          token += versionTag;
        }
        else
        {
          if (!token.isEmpty())
          {
            tokens.push_back(capitalize(token));
          }

          token = QString(versionTag);
        }

        versionTag = 0;
        token += c;
      }
      else
      {
        std::size_t tagIndex =
          isNumeric(c0) ?
          indexOf(c.toLower(), kVersionTags, numTags) :
          numTags;

        if (tagIndex < numTags)
        {
          versionTag = c;
        }
        else
        {
          token += c;
        }
      }

      c0 = c;
    }

    if (versionTag != 0)
    {
      token += versionTag;
    }

    if (!token.isEmpty())
    {
      tokens.push_back(capitalize(token));
    }
  }

  //----------------------------------------------------------------
  // splitOnCamelCase
  //
  void
  splitOnCamelCase(const QString & key, std::list<QString> & tokens)
  {
    // attempt to split on camel case:
    QString token;

    QChar c0 = key[0];
    token += c0;

    const int size = key.size();
    for (int i = 1; i < size; i++)
    {
      QChar c1 = key[i];

      if (c0.isLetter() && c1.isLetter() && c0.isLower() && !c1.isLower())
      {
        splitOnVersion(token, tokens);
        token = QString();
      }
      else if (isNumeric(token) && !isNumeric(c1))
      {
        tokens.push_back(token);
        token = QString();
      }

      token += c1;
      c0 = c1;
    }

    if (!token.isEmpty())
    {
      splitOnVersion(token, tokens);
    }
  }

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  void
  splitOnGroupTags(const QString & key, std::list<QString> & tokens)
  {
    static const QChar kOpenTag[] = {
      QChar::fromAscii('<'),
      QChar::fromAscii('['),
      QChar::fromAscii('{'),
      QChar::fromAscii('('),
      QChar::fromAscii('"')
    };

    static const QChar kCloseTag[] = {
      QChar::fromAscii('>'),
      QChar::fromAscii(']'),
      QChar::fromAscii('}'),
      QChar::fromAscii(')'),
      QChar::fromAscii('"')
    };

    // attempt to split on open/close tags:
    QString token;

    std::size_t numTags = sizeof(kOpenTag) / sizeof(kOpenTag[0]);
    std::size_t tagIndex0 = numTags;
    std::size_t tagIndex1 = numTags;
    std::size_t tagSize = 0;

    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];

      if (tagIndex0 == numTags)
      {
        tagIndex0 = indexOf(c, kOpenTag, numTags);
        tagIndex1 = numTags;
        tagSize = 0;
      }
      else
      {
        tagIndex1 = indexOf(c, kCloseTag, numTags);
        tagIndex1 = (tagIndex1 == tagIndex0) ? tagIndex0 : numTags;
      }

      if (tagIndex0 < numTags && !tagSize)
      {
        if (!token.isEmpty())
        {
          tokens.push_back(token);
        }

        token = QString(c);
        tagSize = 1;
      }
      else
      {
        token += c;

        if (tagIndex0 < numTags)
        {
          tagSize++;
        }

        if (tagIndex1 < numTags)
        {
          tokens.push_back(token);
          token = QString();
          tagIndex0 = numTags;
          tagIndex1 = numTags;
          tagSize = 0;
        }
      }
    }

    if (!token.isEmpty())
    {
      tokens.push_back(token);
    }
  }

  //----------------------------------------------------------------
  // splitOnSeparators
  //
  void
  splitOnSeparators(const QString & key, std::list<QString> & tokens)
  {
    static const QChar kUnderscore = QChar::fromAscii('_');
    static const QChar kHyphen = QChar::fromAscii('-');
    static const QChar kSpace = QChar::fromAscii(' ');
    static const QChar kPeriod = QChar::fromAscii('.');
    static const QChar kNumber = QChar::fromAscii('#');

    // attempt to split based on separator character:
    QString token;

    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];
      if (c == kUnderscore ||
          c == kHyphen ||
          c == kSpace ||
          c == kPeriod ||
          c == kNumber)
      {
        if (!token.isEmpty())
        {
          splitOnCamelCase(token, tokens);
          token = QString();
        }
      }
      else
      {
        token += c;
      }
    }

    if (!token.isEmpty())
    {
      splitOnCamelCase(token, tokens);
    }
  }

  //----------------------------------------------------------------
  // splitIntoWords
  //
  void
  splitIntoWords(const QString & key, std::list<QString> & words)
  {
    std::list<QString> groups;
    splitOnGroupTags(key, groups);

    for (std::list<QString>::const_iterator i = groups.begin();
         i != groups.end(); ++i)
    {
      const QString & group = *i;
      splitOnSeparators(group, words);
    }
  }

  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const std::list<QString> & keys)
  {
    std::list<QString> words;
    for (std::list<QString>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
    {
      const QString & key = *i;
      splitIntoWords(key, words);
    }

    return toQString(words, true);
  }

  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words);
    return toQString(words, false);
  }

  //----------------------------------------------------------------
  // prepareForSorting
  //
  QString
  prepareForSorting(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words);

    QString out;

    for (std::list<QString>::const_iterator i = words.begin();
         i != words.end(); ++i)
    {
      if (!out.isEmpty())
      {
        out += QChar::fromAscii(' ');
      }

      QString word = *i;

      // if the string is all numerical then pad it on the front so that
      // it would be properly sorted (2.avi would be before 10.avi)
      int numDigits = isNumeric(word);

      if (numDigits && numDigits < 8)
      {
        QString padding(8 - numDigits, QChar::fromAscii(' '));
        out += padding;
      }

      out += word;
    }

    return out;
  }

  //----------------------------------------------------------------
  // overlapExists
  //
  bool
  overlapExists(const QRect & a, const QRect & b)
  {
    if (a.height() && a.width() &&
        b.height() && b.width())
    {
      return a.intersects(b);
    }

    // ignore overlap with an empty region:
    return false;
  }

  //----------------------------------------------------------------
  // overlapExists
  //
  bool
  overlapExists(const QRect & a, const QPoint & b)
  {
    if (a.height() && a.width())
    {
      return a.contains(b);
    }

    // ignore overlap with an empty region:
    return false;
  }

  //----------------------------------------------------------------
  // shortenTextToFit
  //
  bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight)
  {
    static const QString ellipsis("...");

    // in case style sheet is used, get fontmetrics from painter:
    QFontMetrics fm = painter.fontMetrics();

    const int bboxWidth = bbox.width();

    textLeft.clear();
    textRight.clear();

    QSize sz = fm.size(Qt::TextSingleLine, text);
    int textWidth = sz.width();
    if (textWidth <= bboxWidth || bboxWidth <= 0)
    {
      // text fits, nothing to do:
      if (textAlignment & Qt::AlignLeft)
      {
        textLeft = text;
      }
      else
      {
        textRight = text;
      }

      return false;
    }

    // scale back the estimate to avoid cutting out too much of text,
    // because not all characters have the same width:
    const double stepScale = 0.78;
    const int textLen = text.size();

    int numToRemove = 0;
    int currLen = textLen - numToRemove;
    int aLen = currLen / 2;
    int bLen = currLen - aLen;

    while (currLen > 1)
    {
      // estimate (conservatively) how much text to remove:
      double excess = double(textWidth) / double(bboxWidth) - 1.0;
      if (excess <= 0.0)
      {
        break;
      }

      double excessLen =
        std::max<double>(1.0,
                         stepScale * double(currLen) *
                         excess / (excess + 1.0));

      numToRemove += int(excessLen);
      currLen = textLen - numToRemove;

      aLen = currLen / 2;
      bLen = currLen - aLen;
      QString tmp = text.left(aLen) + ellipsis + text.right(bLen);

      sz = fm.size(Qt::TextSingleLine, tmp);
      textWidth = sz.width();
    }

    if (currLen < 2)
    {
      // too short, give up:
      aLen = 0;
      bLen = 0;
    }

    if (textAlignment & Qt::AlignLeft)
    {
      textLeft = text.left(aLen) + ellipsis;
      textRight = text.right(bLen);
    }
    else
    {
      textLeft = text.left(aLen);
      textRight = ellipsis + text.right(bLen);
    }

    return true;
  }

  //----------------------------------------------------------------
  // drawTextToFit
  //
  void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText)
  {
    QString textLeft;
    QString textRight;

    if ((textAlignment & Qt::TextWordWrap) ||
        !shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }

    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;

    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);

    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);

    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
  }

  //----------------------------------------------------------------
  // drawTextShadow
  //
  static void
  drawTextShadow(QPainter & painter,
                 const QRect & bbox,
                 int textAlignment,
                 const QString & text,
                 bool outline,
                 int offset)
  {
    if (outline)
    {
      painter.drawText(bbox.translated(-offset, 0), textAlignment, text);
      painter.drawText(bbox.translated(offset, 0), textAlignment, text);
      painter.drawText(bbox.translated(0, -offset), textAlignment, text);
    }

    painter.drawText(bbox.translated(0, offset), textAlignment, text);
  }

  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  //
  void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & bgPen,
                          bool outlineShadow,
                          int shadowOffset,
                          QRect * bboxText)
  {
    QPen fgPen = painter.pen();

    QRect bbox(bboxBig.x() + shadowOffset,
               bboxBig.y() + shadowOffset,
               bboxBig.width() - shadowOffset,
               bboxBig.height() - shadowOffset);

    QString textLeft;
    QString textRight;

    if ((textAlignment & Qt::TextWordWrap) ||
        !shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.setPen(bgPen);
      drawTextShadow(painter,
                     bbox,
                     textAlignment,
                     text,
                     outlineShadow,
                     shadowOffset);

      painter.setPen(fgPen);
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }

    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;

    painter.setPen(bgPen);
    drawTextShadow(painter,
                   bbox,
                   vertAlignment | Qt::AlignLeft,
                   textLeft,
                   outlineShadow,
                   shadowOffset);

    drawTextShadow(painter,
                   bbox,
                   vertAlignment | Qt::AlignRight,
                   textRight,
                   outlineShadow,
                   shadowOffset);

    painter.setPen(fgPen);
    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);

    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);

    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
  }

  //----------------------------------------------------------------
  // stripHtmlTags
  //
  std::string
  stripHtmlTags(const std::string & in)
  {
    // count open/close angle brackets:
    int brackets[] = { 0, 0 };

    std::size_t inLen = in.size();
    for (std::size_t i = 0; i < inLen; i++)
    {
      if (in[i] == '<')
      {
        brackets[0]++;
      }
      else if (in[i] == '>')
      {
        brackets[1]++;
      }

      if (brackets[0] >= 2 && brackets[1] >= 2)
      {
        break;
      }
    }

    if (brackets[0] < 2 || brackets[1] < 2)
    {
      // insufficient number of brackets, probably not an html string:
      return std::string(in);
    }

    std::vector<char> tmp(inLen, 0);
    std::size_t j = 0;

    enum TState
    {
      kInText,
      kInTag
    };
    TState s = kInText;

    for (std::size_t i = 0; i < inLen; i++)
    {
      char c = in[i];

      if (s == kInText)
      {
        if (c == '<')
        {
          s = kInTag;
        }
        else
        {
          tmp[j++] = c;
        }
      }
      else if (s == kInTag)
      {
        if (c == '>')
        {
          s = kInText;
          tmp[j++] = ' ';
        }
      }
    }

    std::string out;
    if (j > 0)
    {
        out.assign(&(tmp[0]), &(tmp[0]) + j);
    }
 
    return out;
  }

  //----------------------------------------------------------------
  // assaToPlainText
  //
  std::string
  assaToPlainText(const std::string & in)
  {
    std::string out;

    std::size_t inLen = in.size();
    const char * ssa = in.c_str();
    const char * end = ssa + inLen;

    while (ssa && ssa < end)
    {
      ssa = strstr(ssa, "Dialogue:");
      if (!ssa)
      {
        break;
      }

      const char * lEnd = strstr(ssa, "\n");
      if (!lEnd)
      {
        lEnd = end;
      }

      ssa += 9;
      for (int i = 0; i < 9; i++)
      {
        ssa = strstr(ssa, ",");
        if (!ssa)
        {
          break;
        }

        ssa++;
      }

      if (!ssa)
      {
        break;
      }

      // skip override:
      std::string tmp;

      while (true)
      {
        const char * override = strstr(ssa, "{");
        if (!override || override >= lEnd)
        {
          break;
        }

        if (ssa < override)
        {
          tmp += std::string(ssa, override);
        }

        override = strstr(override, "}");
        if (!override || override >= lEnd)
        {
          break;
        }

        ssa = override + 1;
      }

      if (!tmp.empty() || (ssa && ssa < lEnd))
      {
        if (!out.empty())
        {
          out += "\n";
        }

        if (!tmp.empty())
        {
          out += tmp;
        }

        if (ssa && (ssa < lEnd))
        {
          out += std::string(ssa, lEnd);
        }
      }
    }

    return out;
  }

  //----------------------------------------------------------------
  // convertEscapeCodes
  //
  std::string
  convertEscapeCodes(const std::string & in)
  {
    std::size_t inLen = in.size();
    std::vector<char> tmp(inLen, 0);
    std::size_t j = 0;

    enum TState
    {
      kInText,
      kInEsc
    };
    TState s = kInText;

    for (std::size_t i = 0; i < inLen; i++)
    {
      char c = in[i];

      if (s == kInText)
      {
        if (c == '\\')
        {
          s = kInEsc;
        }
        else
        {
          tmp[j++] = c;
        }
      }
      else if (s == kInEsc)
      {
        if (c == 'n' || c == 'N')
        {
          tmp[j++] = '\n';
        }
        else if (c == 'r' || c == 'R')
        {
          tmp[j++] = '\r';
        }
        else if (c == 't' || c == 'T')
        {
          tmp[j++] = '\t';
        }
        else
        {
          tmp[j++] = '\\';
          tmp[j++] = c;
        }

        s = kInText;
      }
    }

    std::string out;
    if (j > 0)
    {
        out.assign(&(tmp[0]), &(tmp[0]) + j);
    }

    return out;
  }

  //----------------------------------------------------------------
  // readNextValidToken
  //
  static QXmlStreamReader::TokenType
  readNextValidToken(QXmlStreamReader & xml)
  {
    QXmlStreamReader::TokenType tt = xml.tokenType();

    for (int i = 0; i < 10; i++)
    {
      xml.readNext();
      tt = xml.tokenType();

      if (tt != QXmlStreamReader::NoToken &&
          tt != QXmlStreamReader::Invalid)
      {
        break;
      }

      QXmlStreamReader::Error xerr = xml.error();
      if (xml.atEnd() && xerr == QXmlStreamReader::NoError)
      {
        break;
      }
    }

    return tt;
  }

  //----------------------------------------------------------------
  // parseXmlTag
  //
  static bool
  parseXmlTag(QXmlStreamReader & xml,
              std::string & name,
              QString & value)
  {
    name.clear();
    value.clear();

    QXmlStreamReader::TokenType tt = xml.tokenType();
    while (!xml.atEnd())
    {
      if (tt == QXmlStreamReader::StartElement)
      {
        name = xml.name().toString().toLower().toUtf8().constData();

        tt = readNextValidToken(xml);
        if (tt == QXmlStreamReader::StartElement)
        {
          return false;
        }

        if (tt == QXmlStreamReader::Characters)
        {
          value = xml.text().toString();
          tt = readNextValidToken(xml);
        }

        if (tt == QXmlStreamReader::StartElement)
        {
          bool emptyValue = value.trimmed().isEmpty();
          YAE_ASSERT(emptyValue);
          value.clear();
          return false;
        }

        if (tt != QXmlStreamReader::EndElement)
        {
          YAE_ASSERT(false);
          value.clear();
          return false;
        }

        tt = readNextValidToken(xml);
        if (tt == QXmlStreamReader::Characters)
        {
          std::string t = xml.text().toString().trimmed().toUtf8().constData();
          YAE_ASSERT(t.empty());
          tt = readNextValidToken(xml);
        }

        return true;
      }

      tt = readNextValidToken(xml);
    }

    return false;
  }

  //----------------------------------------------------------------
  // same
  //
  static bool
  same(const std::list<std::string> & nodePath,
       const char * testPath)
  {
    const char * startHere = testPath;

    for (std::list<std::string>::const_iterator i = nodePath.begin();
         i != nodePath.end(); )
    {
      const std::string & node = *i;
      std::size_t size = node.size();
      if (strncmp(&node[0], startHere, size) != 0)
      {
        return false;
      }

      // skip to the next node:
      startHere += size;
      ++i;

      if (i == nodePath.end())
      {
        break;
      }
      else if (startHere[0] != '/')
      {
        return false;
      }

      // skip the '/' path separator:
      startHere++;
    }

    return startHere && startHere[0] == '\0';
  }

  //----------------------------------------------------------------
  // parseEyetvInfo
  //
  bool
  parseEyetvInfo(const QString & eyetvPath,
                 QString & program,
                 QString & episode,
                 QString & timestamp)
  {
    static const QString kExtEyetvR = QString::fromUtf8("eyetvr");

    QStringList extFilters;
    QDirIterator iter(eyetvPath,
                      extFilters,
                      QDir::NoDotAndDotDot |
                      QDir::AllEntries |
                      QDir::Readable,
                      QDirIterator::FollowSymlinks);

    while (iter.hasNext())
    {
      iter.next();

      QFileInfo fi = iter.fileInfo();
      QString fn = fi.absoluteFilePath();
      QString ext = fi.suffix();

      if (ext == kExtEyetvR)
      {
        QFile xmlFile(fn);
        if (!xmlFile.open(QIODevice::ReadOnly))
        {
          return false;
        }
        std::string filename = fn.toUtf8().constData();
        std::string name;
        QString value;
        std::list<std::string> nodePath;

        QXmlStreamReader xml(&xmlFile);
        while (!xml.atEnd())
        {
          bool nameValue = parseXmlTag(xml, name, value);
          if (!nameValue)
          {
            if (name.empty())
            {
              break;
            }

            nodePath.push_back(name);
          }

          if (nameValue && name == "key")
          {
            std::string v = value.toLower().toUtf8().constData();

            if (v == "recording title" && same(nodePath, "plist/dict/dict"))
            {
              if (!parseXmlTag(xml, name, value))
              {
                YAE_ASSERT(false);
                return false;
              }

              program = value;
            }
            else if (v == "episode title" && same(nodePath, "plist/dict/dict"))
            {
              if (!parseXmlTag(xml, name, value))
              {
                YAE_ASSERT(false);
                return false;
              }

              episode = value;
            }
            else if (v == "start" && same(nodePath, "plist/dict/dict"))
            {
              if (!parseXmlTag(xml, name, value))
              {
                YAE_ASSERT(false);
                return false;
              }

              QDateTime t = QDateTime::fromString(value, Qt::ISODate);
              timestamp = t.toLocalTime().toString("yyyyMMdd hhmm");
            }
          }

          QXmlStreamReader::TokenType tt = xml.tokenType();
          if (tt == QXmlStreamReader::EndElement)
          {
            const std::string & top = nodePath.back();
            name = xml.name().toString().toLower().toUtf8().constData();
            if (top != name)
            {
              YAE_ASSERT(false);
              return false;
            }

            nodePath.pop_back();
          }

          if (!program.isEmpty() &&
              !episode.isEmpty() &&
              !timestamp.isEmpty())
          {
            // done:
            break;
          }
        }
      }
    }

    bool done = ((!program.isEmpty() ||
                  !episode.isEmpty()) &&
                 !timestamp.isEmpty());
    return done;
  }
}

#if defined(_WIN32) && !defined(__MINGW32__)
extern "C"
{
  //----------------------------------------------------------------
  // strtoll
  //
  long long int
  strtoll(const char * nptr, char ** endptr, int base)
  {
    YAE_ASSERT(sizeof(long long int) == sizeof(__int64));
    return _strtoi64(nptr, endptr, base);
  }
}

#endif

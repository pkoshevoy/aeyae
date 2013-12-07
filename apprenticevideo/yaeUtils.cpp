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
#include <math.h>

// yae includes:
#include <yaeAPI.h>
#include <yaeUtils.h>

// Qt includes:
#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QPainter>
#include <QStringList>
#include <QSettings>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace yae
{

#ifdef __APPLE__
  static QString kOrganization = QString::fromUtf8("sourceforge.net");
  static QString kApplication = QString::fromUtf8("apprenticevideo");
#else
  static QString kOrganization = QString::fromUtf8("PavelKoshevoy");
  static QString kApplication = QString::fromUtf8("ApprenticeVideo");
#endif


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

  static const QChar kUnderscore = QChar::fromAscii('_');
  static const QChar kHyphen = QChar::fromAscii('-');
  static const QChar kSpace = QChar::fromAscii(' ');
  static const QChar kPeriod = QChar::fromAscii('.');
  static const QChar kComma = QChar::fromAscii(',');
  static const QChar kSemicolon = QChar::fromAscii(';');
  static const QChar kExclamation = QChar::fromAscii('!');
  static const QChar kQuestionmark = QChar::fromAscii('?');

  //----------------------------------------------------------------
  // isPunctuation
  //
  static inline bool
  isPunctuation(const QChar & ch)
  {
    return (ch == kPeriod ||
            ch == kComma ||
            ch == kSemicolon ||
            ch == kExclamation ||
            ch == kQuestionmark);
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
      const QString & key = *i;

      if (i != keys.begin() && !(key.size() == 1 && isPunctuation(key[0])))
      {
        path += kSpace;
      }

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
  static inline bool
  isNumeric(const QChar & c)
  {
    return c >= QChar('0') && c <= QChar('9');
  }

  //----------------------------------------------------------------
  // isNumeric
  //
  static int
  isNumeric(const QString & key, int start = 0)
  {
    const int size = key.size();
    for (int i = start; i < size; i++)
    {
      QChar c = key[i];
      if (!isNumeric(c))
      {
        return 0;
      }
    }

    return (start < size) ? (size - start) : 0;
  }

  //----------------------------------------------------------------
  // isVaguelyNumeric
  //
  static int
  isVaguelyNumeric(const QString & key, int start = 0)
  {
    int numDigits = 0;
    const int size = key.size();

    for (int i = start; i < size; i++)
    {
      QChar ch = key[i];

      if (!ch.isLetterOrNumber() && !isPunctuation(ch))
      {
        return 0;
      }

      if (isNumeric(ch))
      {
        numDigits++;
      }
    }

    return (start < size && numDigits) ? (size - start) : 0;
  }

  //----------------------------------------------------------------
  // isVersionNumber
  //
  static bool
  isVersionNumber(const QString & word, int start = 0)
  {
    if (!word.isEmpty())
    {
      QChar l0 = word[start].toLower();
      if ((l0 == QChar::fromAscii('v') ||
           l0 == QChar::fromAscii('p') ||
           l0 == QChar::fromAscii('#')) &&
          isNumeric(word, start + 1))
      {
        return true;
      }
    }

    return false;
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

    if (isVersionNumber(word))
    {
      QChar l0 = word[0].toLower();
      out[0] = l0;
    }
    else if (word.size() > 2 && !isVaguelyNumeric(word))
    {
      QChar u0 = word[0].toUpper();
      out[0] = u0;
    }

    return out;
  }

  //----------------------------------------------------------------
  // TPostprocessToken
  //
  typedef QString(*TPostprocessToken)(const QString &);

  //----------------------------------------------------------------
  // TSplitStringIntoTokens
  //
  typedef void(*TSplitStringIntoTokens)(const QString &,
                                        std::list<QString> &,
                                        TPostprocessToken);

  //----------------------------------------------------------------
  // splitVaguelyNumericTokens
  //
  static void
  splitVaguelyNumericTokens(const QString & key,
                            std::list<QString> & tokens,
                            TPostprocessToken postprocess = NULL)
  {
    const QChar * text = key.constData();
    const int size = key.size();
    int i = 0;
    int numDigits = 0;

    for (; i < size; i++, numDigits++)
    {
      const QChar & ch = text[i];
      if (!isNumeric(ch) && !isPunctuation(ch))
      {
        break;
      }
    }

    int numOthers = 0;
    for (; i < size; i++, numOthers++)
    {
      const QChar & ch = text[i];
      if (isNumeric(ch))
      {
        break;
      }
    }

    if (numDigits && (numDigits + numOthers == size))
    {
      QString digits(text, numDigits);
      tokens.push_back(digits);

      if (numDigits < size)
      {
        QString others(text + numDigits, size - numDigits);
        tokens.push_back(others);
      }
    }
    else if (postprocess)
    {
      QString token = postprocess(key);
      tokens.push_back(token);
    }
    else
    {
      tokens.push_back(key);
    }
  }

  //----------------------------------------------------------------
  // splitOnCamelCase
  //
  static void
  splitOnCamelCase(const QString & key,
                   std::list<QString> & tokens,
                   TPostprocessToken postprocess = NULL)
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
        splitVaguelyNumericTokens(token, tokens, postprocess);
        token = QString();
      }
      else if (isNumeric(token) && !isNumeric(c1) &&
               (c1.isNumber() || isVersionNumber(key, i)))
      {
        tokens.push_back(token);
        token = QString();
      }

      token += c1;
      c0 = c1;
    }

    if (!token.isEmpty())
    {
      splitVaguelyNumericTokens(token, tokens, postprocess);
    }
  }

  //----------------------------------------------------------------
  // kSeparatorTags
  //
  static const QChar kSeparatorTags[] = {
    QChar::fromAscii(' '),
    QChar::fromAscii('.'),
    QChar::fromAscii('-')
  };

  //----------------------------------------------------------------
  // kSeparatorTagsNum
  //
  static const std::size_t kSeparatorTagsNum =
    sizeof(kSeparatorTags) / sizeof(kSeparatorTags[0]);

  //----------------------------------------------------------------
  // indexOfSeparatorTag
  //
  std::size_t
  indexOfSeparatorTag(const QChar & ch)
  {
    for (std::size_t i = 0; i < kSeparatorTagsNum; ++i)
    {
      if (kSeparatorTags[i] == ch)
      {
        return i;
      }
    }

    return kSeparatorTagsNum;
  }

  //----------------------------------------------------------------
  // splitOnSeparators
  //
  static void
  splitOnSeparators(const QString & key,
                    std::list<QString> & tokens,
                    TPostprocessToken postprocess = NULL)
  {
    // first replace all underscores with spaces:
    QString cleanKey = key;
    const int size = cleanKey.size();

    for (int i = 0; i < size; i++)
    {
      QChar ch = cleanKey[i];
      if (ch == kUnderscore)
      {
        cleanKey[i] = kSpace;
      }
    }

    // split based on separator character:
    QString token;
    QString prev;
    for (int i = 0; i < size; i++)
    {
      QChar ch = cleanKey[i];
      QChar ch1 = ((i + 1) < size) ? cleanKey[i + 1] : QChar(0);
      QChar ch2 = ((i + 2) < size) ? cleanKey[i + 2] : QChar(0);

      std::size_t foundSeparatorTag = indexOfSeparatorTag(ch);
      if (foundSeparatorTag < kSeparatorTagsNum &&

          (ch != kPeriod ||
           (// avoid breaking up ellipsis and numbered list punctuation:
            ch1 != kPeriod &&
            ch1 != kSpace &&
            (ch1 != 0 || !token.endsWith(kPeriod)) &&
            // avoid breaking up YYYY.MM.DD dates:
            !(isNumeric(prev) && isNumeric(ch1)) &&
            // avoid breaking up A.B.C initials:
            !(prev.size() == 1 && prev[0].isLetter() &&
              ch1.isLetter() && (ch2 == kPeriod || ch2 == kSpace)))))
      {
        if (!token.isEmpty())
        {
          splitOnCamelCase(token, tokens, postprocess);
        }

        token = QString();
        prev = QString();

        if (ch == kHyphen &&
            (// preserve proper hyphenation:
             ch1 == kHyphen ||
             ch1 == kSpace) &&
            ch1 != 0)
        {
          if (!tokens.empty() && tokens.back().endsWith(kHyphen))
          {
            tokens.back() += QString(kHyphen);
          }
          else
          {
            tokens.push_back(QString(kHyphen));
          }
        }
      }
      else
      {
        token += ch;

        if (foundSeparatorTag < kSeparatorTagsNum)
        {
          prev = QString();
        }
        else
        {
          prev += QString(ch);
        }
      }
    }

    if (!token.isEmpty())
    {
      splitOnCamelCase(token, tokens, postprocess);
    }
  }

  //----------------------------------------------------------------
  // kGroupTags
  //
  static const QChar kGroupTags[][2] = {
    { QChar::fromAscii('<'), QChar::fromAscii('>') },
    { QChar::fromAscii('['), QChar::fromAscii(']') },
    { QChar::fromAscii('{'), QChar::fromAscii('}') },
    { QChar::fromAscii('('), QChar::fromAscii(')') },
    { QChar::fromAscii('"'), QChar::fromAscii('"') }
  };

  //----------------------------------------------------------------
  // kGroupTagsNum
  //
  static const std::size_t kGroupTagsNum =
    sizeof(kGroupTags) / sizeof(kGroupTags[0]);

  //----------------------------------------------------------------
  // indexOfGroupTag
  //
  std::size_t
  indexOfGroupTag(const QChar & ch, std::size_t columnIndex)
  {
    for (std::size_t i = 0; i < kGroupTagsNum; ++i)
    {
      if (kGroupTags[i][columnIndex] == ch)
      {
        return i;
      }
    }

    return kGroupTagsNum;
  }

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  static bool
  splitOnGroupTags(const QString & key,
                   int & keyPos,
                   const int keySize,
                   const QChar & closingTag,
                   std::list<QString> & tokens,
                   TSplitStringIntoTokens splitFurther = NULL,
                   TPostprocessToken postprocess = NULL)
  {
    // attempt to split on nested open/close tags:
    QString token;
    QChar ch;
    bool foundClosingTag = false;

    while (keyPos < keySize && !foundClosingTag)
    {
      ch = key[keyPos];
      ++keyPos;

      if (ch == closingTag)
      {
        foundClosingTag = true;
        break;
      }

      std::size_t foundNestedTag = indexOfGroupTag(ch, 0);
      if (foundNestedTag < kGroupTagsNum)
      {
        std::list<QString> nestedTokens;
        if (splitOnGroupTags(key,
                             keyPos,
                             keySize,
                             kGroupTags[foundNestedTag][1],
                             nestedTokens,
                             splitFurther,
                             postprocess))
        {
          if (!token.isEmpty())
          {
            if (splitFurther)
            {
              std::list<QString> furtherTokens;
              splitFurther(token, furtherTokens, postprocess);
              tokens.splice(tokens.end(), furtherTokens);
            }
            else
            {
              tokens.push_back(token);
            }
          }

          token = QString(ch);
          token += nestedTokens.front();
          nestedTokens.pop_front();

          tokens.push_back(token);
          tokens.splice(tokens.end(), nestedTokens);

          token = QString();
        }
        else
        {
          token += ch;

          if (!nestedTokens.empty())
          {
            token += nestedTokens.front();
            nestedTokens.pop_front();

            if (splitFurther)
            {
              std::list<QString> furtherTokens;
              splitFurther(token, furtherTokens, postprocess);
              tokens.splice(tokens.end(), furtherTokens);
            }
            else
            {
              tokens.push_back(token);
            }

            tokens.splice(tokens.end(), nestedTokens);

            token = QString();
          }
        }
      }
      else
      {
        token += ch;
      }
    }

    if (!token.isEmpty())
    {
      if (splitFurther)
      {
        std::list<QString> furtherTokens;
        splitFurther(token, furtherTokens, postprocess);
        tokens.splice(tokens.end(), furtherTokens);
      }
      else
      {
        tokens.push_back(token);
      }
    }

    if (foundClosingTag && ch != 0)
    {
      if (!tokens.empty())
      {
        tokens.back() += QString(ch);
      }
      else
      {
        tokens.push_back(QString(ch));
      }
    }

    return foundClosingTag;
  }

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  static void
  splitOnGroupTags(const QString & key,
                   std::list<QString> & tokens,
                   TSplitStringIntoTokens splitFurther = NULL,
                   TPostprocessToken postprocess = NULL)
  {
    int keyPos = 0;
    const int keySize = key.size();
    const QChar closingTag = 0;
    splitOnGroupTags(key,
                     keyPos,
                     keySize,
                     closingTag,
                     tokens,
                     splitFurther,
                     postprocess);
  }

  //----------------------------------------------------------------
  // splitIntoWords
  //
  static void
  splitIntoWords(const QString & key,
                 std::list<QString> & words,
                 TPostprocessToken postprocess)
  {
    splitOnGroupTags(key, words, &splitOnSeparators, postprocess);
  }

  //----------------------------------------------------------------
  // splitIntoWords
  //
  void
  splitIntoWords(const QString & key, std::list<QString> & words)
  {
    splitOnGroupTags(key, words, &splitOnSeparators, &capitalize);
  }

  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words, NULL);
    return toQString(words, false);
  }

  //----------------------------------------------------------------
  // prepareForSorting
  //
  QString
  prepareForSorting(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words, &capitalize);

    int nwords = 0;
    QString out;
    QString prev;

    for (std::list<QString>::const_iterator i = words.begin();
         i != words.end(); ++i)
    {
      QString word = *i;

      if (word.size() == 1 && word[0] == kHyphen)
      {
        if (nwords == 1 && isNumeric(prev))
        {
          // hyde numeric list separator hyphen:
          continue;
        }

        std::list<QString>::const_iterator j = i;
        ++j;
        if (j != words.end())
        {
          const QString & next = *j;
          if (isVaguelyNumeric(next))
          {
            // hyde numeric list separator hyphen:
            continue;
          }
        }
      }

      if (!out.isEmpty() && !(word.size() == 1 && isPunctuation(word[0])))
      {
        out += kSpace;
      }

      // if the string is all numerical then pad it on the front so that
      // it would be properly sorted (2.avi would be before 10.avi)
      int numDigits = isVaguelyNumeric(word);

      if (numDigits && numDigits < 8)
      {
        QString padding(8 - numDigits, kSpace);
        out += padding;
      }

      out += word;
      nwords++;

      prev = word;
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
    const char * ssa = inLen ? in.c_str() : NULL;
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

      if (!tmp.empty() || (ssa < lEnd))
      {
        if (!out.empty())
        {
          out += "\n";
        }

        if (!tmp.empty())
        {
          out += tmp;
        }

        if (ssa < lEnd)
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

  //----------------------------------------------------------------
  // xmlEncode
  //
  QString
  xmlEncode(const QString & text)
  {
    QString output;
    {
      QXmlStreamWriter stream(&output);
      stream.writeCharacters(text);
    }

    return output;
  }

  //----------------------------------------------------------------
  // saveSetting
  //
  bool
  saveSetting(const QString & key, const QString & value)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    settings.setValue(key, value);

    bool ok = (settings.status() == QSettings::NoError);
    return ok;
  }

  //----------------------------------------------------------------
  // loadSetting
  //
  bool
  loadSetting(const QString & key, QString & value)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    if (!settings.contains(key))
    {
      return false;
    }

    value = settings.value(key).toString();
    return true;
  }

  //----------------------------------------------------------------
  // loadSettingOrDefault
  //
  QString
  loadSettingOrDefault(const QString & key, const QString & defaultValue)
  {
    QString value;
    if (loadSetting(key, value))
    {
      return value;
    }

    return defaultValue;
  }

  //----------------------------------------------------------------
  // removeSetting
  //
  bool
  removeSetting(const QString & key)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    settings.remove(key);

    bool ok = (settings.status() == QSettings::NoError);
    return ok;
  }

  //----------------------------------------------------------------
  // parse_hhmmss_xxx
  //
  double
  parse_hhmmss_xxx(const char * hhmmss,
                   const char * separator,
                   const char * separator_xxx,
                   const double frameRate)
  {
    const std::size_t len = hhmmss ? strlen(hhmmss) : 0;
    if (!len)
    {
      return 0.0;
    }

    const bool has_xxx_separator = separator_xxx && *separator_xxx;
    YAE_ASSERT(frameRate == 0.0 || has_xxx_separator);

    std::vector<std::string> tokens;
    {
      std::list<std::string> token_list;
      std::size_t num_tokens = 0;
      std::list<char> token;
      std::size_t token_len = 0;

      // read from the tail:
      for (const char * i = hhmmss + len - 1; i >= hhmmss; i--)
      {
        // decide which separator to check for:
        const char * sep =
          has_xxx_separator && token_list.empty() ?
          separator_xxx :
          separator;

        const bool has_separator = sep && *sep;

        bool token_ready = false;
        if (*i >= '0' && *i <= '9')
        {
          token.push_front(*i);
          token_len++;
          token_ready = (!has_separator && token_len == 2);
        }
        else
        {
          YAE_ASSERT(has_separator && *i == *sep);
          token_ready = !token.empty();
        }

        if (token_ready)
        {
          token_list.push_front(std::string());
          token_list.front().assign(token.begin(), token.end());
          token.clear();
          token_len = 0;
          num_tokens++;
        }
      }

      if (!token.empty())
      {
        token_list.push_front(std::string());
        token_list.front().assign(token.begin(), token.end());
        num_tokens++;
      }

      tokens.assign(token_list.begin(), token_list.end());
    }

    std::size_t numTokens = tokens.size();
    std::size_t ixxx =
      has_xxx_separator ? numTokens - 1 :
      numTokens > 3 ? numTokens - 1 :
      numTokens;

    std::size_t iss = ixxx > 0 ? ixxx - 1 : numTokens;
    std::size_t imm = iss > 0 ? iss - 1 : numTokens;
    std::size_t ihh = imm > 0 ? imm - 1 : numTokens;
    std::size_t idd = ihh > 0 ? ihh - 1 : numTokens;
    YAE_ASSERT(idd == numTokens || idd == 0);

    int64_t t = 0;

    if (idd < numTokens)
    {
      t = toScalar<int64_t>(tokens[idd]);
    }

    if (ihh < numTokens)
    {
      t = t * 24 + toScalar<int64_t>(tokens[ihh]);
    }

    if (imm < numTokens)
    {
      t = t * 60 + toScalar<int64_t>(tokens[imm]);
    }

    if (iss < numTokens)
    {
      t = t * 60 + toScalar<int64_t>(tokens[iss]);
    }

    double seconds = double(t);
    if (ixxx < numTokens)
    {
      double xxx = toScalar<double>(tokens[ixxx]);
      std::size_t xxx_len = tokens[ixxx].size();

      if (frameRate > 0.0)
      {
        // it's a frame number:
        seconds += xxx / frameRate;
      }
      else if (xxx_len == 2)
      {
        // centiseconds:
        seconds += xxx * 1e-2;
      }
      else if (xxx_len == 3)
      {
        // milliseconds:
        seconds += xxx * 1e-3;
      }
      else if (xxx_len == 6)
      {
        // microseconds:
        seconds += xxx * 1e-6;
      }
      else if (xxx_len == 9)
      {
        // nanoseconds:
        seconds += xxx * 1e-9;
      }
      else if (xxx_len)
      {
        YAE_ASSERT(false);
        seconds += xxx * pow(10.0, -double(xxx_len));
      }
    }

    return seconds;
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

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
#include <stdint.h>
#include <vector>

// yae includes:
#include <yaeAPI.h>
#include <yaeUtils.h>

namespace yae
{
  
#ifdef _WIN32
  //----------------------------------------------------------------
  // utf8_to_utf16
  // 
  static wchar_t * utf8_to_utf16(const char * str_utf8)
  {
    int nchars = MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, NULL, 0);
    wchar_t * str_utf16 = (wchar_t *)malloc(nchars * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, str_utf16, nchars);
    return str_utf16;
  }
#endif

  //----------------------------------------------------------------
  // renameUtf8
  //
  int
  renameUtf8(const char * fnOld, const char * fnNew)
  {
#ifdef _WIN32
    wchar_t * wold = utf8_to_utf16(fnOld);
    wchar_t * wnew = utf8_to_utf16(fnNew);
    
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
    
#ifdef _WIN32
    wchar_t * wname = utf8_to_utf16(filenameUtf8);
    wchar_t * wmode = utf8_to_utf16(mode);
    
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
#ifdef _WIN32
    accessMode |= O_BINARY;
    
    wchar_t * wname = utf8_to_utf16(filenameUtf8);
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
#ifdef _WIN32
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
#ifdef _WIN32
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
        if (!c.isNumber())
        {
          token += versionTag;
        }
        else
        {
          if (!token.isEmpty())
          {
            tokens.push_back(token);
          }
          
          token = QString(versionTag);
        }
        
        versionTag = 0;
        token += c;
      }
      else
      {
        std::size_t tagIndex =
          c0.isNumber() ?
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
      tokens.push_back(token);
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
      
      if (// c0.isNumber() &&
          // c1.isLetter() ||
          c0.isLetter() &&
          (// c1.isNumber() ||
           c1.isLetter() && c0.isLower() && !c1.isLower()))
      {
        splitOnVersion(token, tokens);
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
  // isNumeric
  // 
  int
  isNumeric(const QString & key)
  {
    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];
      if (!c.isNumber())
      {
        return 0;
      }
    }
    
    return size;
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
}

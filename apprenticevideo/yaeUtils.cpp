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
  
  //----------------------------------------------------------------
  // fileOpenUtf8
  // 
  int
  fileOpenUtf8(const char * filenameUTF8, int accessMode, int permissions)
  {
#ifdef _WIN32
    accessMode |= O_BINARY;
    
    int wcsSize =
      MultiByteToWideChar(CP_UTF8, // source string encoding
                          0,       // flags (precomposed, composite, etc...)
                          filenameUTF8, // source string
                          -1,      // source string size
                          NULL,    // output string destination buffer
                          0);      // output string destination buffer size
    
    std::vector<wchar_t> filenameUTF16(wcsSize + 1);
    MultiByteToWideChar(CP_UTF8,
                        0,
                        filenameUTF8,
                        -1,
                        &filenameUTF16[0],
                        wcsSize);
    
    int fd = _wopen(&filenameUTF16[0], accessMode, permissions);
    
#else
    
    int fd = open(filenameUTF8, accessMode, permissions);
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
  toQString(const std::list<QString> & keys)
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
      path += key;
    }
    
    return path;
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
        tokens.push_back(token);
        token = QString();
      }
      
      token += c1;
      c0 = c1;
    }
    
    if (!token.isEmpty())
    {
      tokens.push_back(token);
    }
  }
  
  //----------------------------------------------------------------
  // splitIntoWords
  // 
  void
  splitIntoWords(const QString & key, std::list<QString> & tokens)
  {
    static const QChar kUnderscore = QChar::fromAscii('_');
    static const QChar kHyphen = QChar::fromAscii('-');
    static const QChar kSpace = QChar::fromAscii(' ');
    
    // attempt to split based on separator character:
    QString token;
    
    const int size = key.size();
    for (int i = 0; i < size; i++)
    {
      QChar c = key[i];
      if (c == kUnderscore ||
          c == kHyphen ||
          c == kSpace)
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
      token = QString();
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

    return toQString(words);
  }
  
  //----------------------------------------------------------------
  // toWords
  // 
  QString
  toWords(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words);
    return toQString(words);
  }
  
}

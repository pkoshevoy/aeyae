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
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <math.h>

// aeyae:
#include "yae_utils.h"
#include "../video/yae_video.h"


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

  //----------------------------------------------------------------
  // utf8_to_utf16
  //
  static std::wstring
  utf8_to_utf16(const char * str)
  {
    wchar_t * tmp = cstr_to_utf16(str, CP_UTF8);
    std::wstring wstr(tmp);
    free(tmp);
    return wstr;
  }

  //----------------------------------------------------------------
  // utf16_to_utf8
  //
  static std::string
  utf16_to_utf8(const wchar_t * wstr)
  {
    char * tmp = utf16_to_cstr(wstr, CP_UTF8);
    std::string str(tmp);
    free(tmp);
    return str;
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
  // data
  //
  static const char *
  data(const std::string & str)
  {
    return str.size() ? &str[0] : NULL;
  }

  //----------------------------------------------------------------
  // trimFromTail
  //
  static std::string
  trimFromTail(char c, const char * str, std::size_t strLen)
  {
    if (!strLen && str)
    {
      strLen = strlen(str);
    }

    const char * end = str ? str + strLen : NULL;
    while (str < end)
    {
      if (*(end - 1) != c)
      {
        return std::string(str, end);
      }

      end--;
    }

    return std::string();
  }

  //----------------------------------------------------------------
  // trimFromHead
  //
  static std::string
  trimFromHead(char c, const char * str, std::size_t strLen)
  {
    if (!strLen && str)
    {
      strLen = strlen(str);
    }

    const char * end = str ? str + strLen : NULL;
    while (str < end)
    {
      if (*str != c)
      {
        return std::string(str, end);
      }

      str++;
    }

    return std::string();
  }

  //----------------------------------------------------------------
  // joinPaths
  //
  std::string
  joinPaths(const std::string & a, const std::string & b, char pathSeparator)
  {
    std::size_t aLen = a.size();
    std::size_t bLen = b.size();

    std::string aTrimmed =
      aLen ? trimFromTail(pathSeparator, data(a), aLen) : std::string("");

    std::string bTrimmed =
      bLen ? trimFromHead(pathSeparator, data(b), bLen) : std::string("");

    std::string ab = aTrimmed + pathSeparator + bTrimmed;

    return ab;
  }

  //----------------------------------------------------------------
  // parseFilePath
  //
  bool
  parseFilePath(const std::string & filePath,
                std::string & folder,
                std::string & name)
  {
    std::size_t	indexName = filePath.rfind(kDirSeparator);

    std::size_t	indexNameUnix =
      (kDirSeparator != '/') ?
      filePath.rfind('/') :
      indexName;

    if (indexNameUnix != std::string::npos &&
        (indexName == std::string::npos ||
         indexName < indexNameUnix))
    {
      // Unix directory separator used before file name
      indexName = indexNameUnix;
    }

    if (indexName != std::string::npos)
    {
      folder = filePath.substr(0, indexName);
      name = filePath.substr(indexName + 1, filePath.size());
      return true;
    }

    folder = std::string();
    name = filePath;
    return false;
  }

  //----------------------------------------------------------------
  // getCurrentExecutablePath
  //
  bool
  getCurrentExecutablePath(std::string & exePathUtf8)
  {
    bool ok = false;

#ifdef _WIN32
    wchar_t path[_MAX_PATH] = { 0 };
    unsigned long pathLen = sizeof(path) / sizeof(wchar_t);

    if (GetModuleFileNameW(0, path, pathLen) != 0)
    {
      exe_path_utf8 = utf16_to_utf8(path);
      ok = true;
    }

#elif defined(__APPLE__)
    CFBundleRef bundleRef = CFBundleGetMainBundle();
    if (bundleRef)
    {
      CFURLRef urlRef = CFBundleCopyExecutableURL(bundleRef);
      if (urlRef)
      {
        CFStringRef stringRef = CFURLCopyFileSystemPath(urlRef,
                                                        kCFURLPOSIXPathStyle);
        if (stringRef)
        {
          char txt[1024] = { 0 };
          if (CFStringGetCString(stringRef,
                                 txt,
                                 sizeof(txt),
                                 kCFStringEncodingUTF8))
          {
            exePathUtf8.assign(txt);
            ok = true;
          }

          CFRelease(stringRef);
        }

        CFRelease(urlRef);
      }
    }

#else
    char path[PATH_MAX + 1] = { 0 };

    if (readlink("/proc/self/exe", path, sizeof(path)) > 0)
    {
        exePathUtf8.assign(path);
        ok = true;
    }

#endif

    YAE_ASSERT(!exePathUtf8.empty());
    return ok;
  }

  //----------------------------------------------------------------
  // getCurrentExecutableFolder
  //
  bool
  getCurrentExecutableFolder(std::string & exeFolderPathUtf8)
  {
    std::string exePathUtf8;
    if (!getCurrentExecutablePath(exePathUtf8))
    {
      return false;
    }

    std::string name;
    return parseFilePath(exePathUtf8, exeFolderPathUtf8, name);
  }

  //----------------------------------------------------------------
  // TOpenFolder::Private
  //
  struct TOpenFolder::Private
  {
    //----------------------------------------------------------------
    // Dirent
    //
    // Some systems don't define the dName element sufficiently long.
    // In this case the user has to provide additional space.
    // There must be room for at least NAME_MAX + 1 characters
    // in the d_name array
    //
    typedef union
    {
      struct dirent d;
      char b[offsetof(struct dirent, d_name) + NAME_MAX + 1];

    } Dirent;

    //----------------------------------------------------------------
    // Private
    //
    Private(const std::string & folderPath):
      dir_(NULL)
    {
      memset(&dirent_, 0, sizeof(dirent_));

      // cleanup the folder path:
      {
        char * tmp = ::realpath(folderPath.c_str(), NULL);
        folderPath_ = tmp;
        ::free(tmp);
      }

      dir_ = ::opendir(folderPath_.c_str());
      if (!dir_)
      {
        std::ostringstream oss;
        oss << "opendir failed for \"" << folderPath_ << "\"";
        throw std::runtime_error(oss.str().c_str());
      }

      struct dirent * found = NULL;
      int err = ::readdir_r(dir_, &dirent_.d, &found);
      if (err)
      {
        std::ostringstream oss;
        oss << "readdir_r failed for \"" << folderPath_ << "\"";
        throw std::runtime_error(oss.str().c_str());
      }

      if (dirent_.d.d_type == DT_UNKNOWN)
      {
        recoverItemType(folderPath_, dirent_.d);
      }
    }

    ~Private()
    {
      if (dir_ != NULL)
      {
        ::closedir(dir_);
        dir_ = NULL;
      }
    }

    bool parseNextItem()
    {
      if (dir_ == NULL)
      {
        return false;
      }

      struct dirent * found = NULL;
      int error = ::readdir_r(dir_, &dirent_.d, &found);
      bool ok = (error == 0) && (found != NULL);

      if (ok && dirent_.d.d_type == DT_UNKNOWN)
      {
        recoverItemType(folderPath_, dirent_.d);
      }

      return ok;
    }

    inline const std::string & folderPath() const
    {
      return folderPath_;
    }

    inline bool itemIsFolder() const
    {
      return
        (dir_ != NULL) &&
        (dirent_.d.d_type == DT_DIR);
    }

    inline std::string itemName() const
    {
      return std::string(dirent_.d.d_name);
    }

    inline std::string itemPath() const
    {
      return joinPaths(folderPath_, itemName());
    }

  protected:

    //----------------------------------------------------------------
    // recoverItemType
    //
    static void
    recoverItemType(const std::string & folderPath, struct dirent & d)
    {
      std::string path = joinPaths(folderPath, std::string(d.d_name));

      struct stat st;
      int err = ::stat(path.c_str(), &st);
      if (err)
      {
        return;
      }

      if (S_ISDIR(st.st_mode))
      {
        d.d_type = DT_DIR;
      }
      else if (S_ISREG(st.st_mode))
      {
        d.d_type = DT_REG;
      }
      else if (S_ISCHR(st.st_mode))
      {
        d.d_type = DT_CHR;
      }
      else if (S_ISBLK(st.st_mode))
      {
        d.d_type = DT_BLK;
      }
      else if (S_ISFIFO(st.st_mode))
      {
        d.d_type = DT_FIFO;
      }
      else if (S_ISLNK(st.st_mode))
      {
        d.d_type = DT_LNK;
      }
      else if (S_ISSOCK(st.st_mode))
      {
        d.d_type = DT_SOCK;
      }
    }

    std::string folderPath_;
    Dirent dirent_;
    DIR * dir_;
  };

  //----------------------------------------------------------------
  // TOpenFolder::TOpenFolder
  //
  TOpenFolder::TOpenFolder(const std::string & folderPath):
    private_(new Private(folderPath))
  {}

  //----------------------------------------------------------------
  // TOpenFolder::~TOpenFolder
  //
  TOpenFolder::~TOpenFolder()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TOpenFolder::folderPath
  //
  const std::string &
  TOpenFolder::folderPath() const
  {
    return private_->folderPath();
  }

  //----------------------------------------------------------------
  // TOpenFolder::parseNextItem
  //
  bool
  TOpenFolder::parseNextItem()
  {
    return private_->parseNextItem();
  }

  //----------------------------------------------------------------
  // TOpenFolder::itemIsFolder
  //
  bool
  TOpenFolder::itemIsFolder() const
  {
    return private_->itemIsFolder();
  }

  //----------------------------------------------------------------
  // TOpenFolder::itemName
  //
  std::string
  TOpenFolder::itemName() const
  {
    return private_->itemName();
  }

  //----------------------------------------------------------------
  // TOpenFolder::temPath
  //
  std::string
  TOpenFolder::itemPath() const
  {
    return private_->itemPath();
  }


  //----------------------------------------------------------------
  // TOpenFile::TOpenFile
  //
  TOpenFile::TOpenFile(const char * filenameUtf8, const char * mode):
    file_(fopenUtf8(filenameUtf8, mode))
  {
    if (!file_)
    {
      std::ostringstream oss;
      oss << "fopenUtf8 failed for \"" << filenameUtf8 << "\"";
      throw std::runtime_error(oss.str().c_str());
    }
  }

  //----------------------------------------------------------------
  // TOpenFile::~TOpenFile
  //
  TOpenFile::~TOpenFile()
  {
    if (file_)
    {
      ::fclose(file_);
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
  // parse_hhmmss_xxx
  //
  double
  parse_hhmmss_xxx(const char * hhmmss,
                   const char * separator,
                   const char * separator_xxx,
                   const double frameRate)
  {
    std::size_t len = hhmmss ? strlen(hhmmss) : 0;
    if (!len)
    {
      return 0.0;
    }

    bool negative = false;
    while (*hhmmss == '-')
    {
      negative = !negative;
      hhmmss++;
      len--;

      if (!len)
      {
        return 0.0;
      }
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

    return negative ? -seconds : seconds;
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

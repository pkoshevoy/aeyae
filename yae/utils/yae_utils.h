// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:34:13 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_H_
#define YAE_UTILS_H_

// std includes:
#include <cstdio>
#include <list>
#include <map>
#include <math.h>
#include <set>
#include <stdexcept>
#include <string.h>
#include <sstream>
#include <vector>

// boost:
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#endif

// yae includes:
#include "../api/yae_api.h"

// namespace shortcut:
namespace fs = boost::filesystem;
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // parity_lut
  //
  extern YAE_API const bool parity_lut[256];

  //----------------------------------------------------------------
  // renameUtf8
  //
  YAE_API int
  renameUtf8(const char * fnOldUtf8, const char * fnNewUtf8);

  //----------------------------------------------------------------
  // fopenUtf8
  //
  YAE_API std::FILE *
  fopenUtf8(const char * filenameUtf8, const char * mode);

  //----------------------------------------------------------------
  // fileOpenUtf8
  //
  YAE_API int
  fileOpenUtf8(const char * filenameUTF8, int accessMode, int permissions);

  //----------------------------------------------------------------
  // fileSeek64
  //
  YAE_API int64
  fileSeek64(int fd, int64 offset, int whence);

  //----------------------------------------------------------------
  // fileSize64
  //
  YAE_API int64
  fileSize64(int fd);

  //----------------------------------------------------------------
  // kDirSeparator
  //
#ifdef _WIN32
#define kDirSeparator '\\'
#else
#define kDirSeparator '/'
#endif

  //----------------------------------------------------------------
  // joinPaths
  //
  YAE_API std::string
  joinPaths(const std::string & a,
            const std::string & b,
            char pathSeparator = kDirSeparator);

  //----------------------------------------------------------------
  // parseFilePath
  //
  YAE_API bool
  parseFilePath(const std::string & filePath,
                std::string & folder,
                std::string & name);

  //----------------------------------------------------------------
  // parseFileName
  //
  YAE_API bool
  parseFileName(const std::string & fileName,
                std::string & name,
                std::string & ext);

  //----------------------------------------------------------------
  // getModuleFilename
  //
  YAE_API bool
  getModuleFilename(const void * symbol, std::string & filepathUtf8);

  //----------------------------------------------------------------
  // getCurrentExecutablePath
  //
  YAE_API bool
  getCurrentExecutablePath(std::string & exePathUtf8);

  //----------------------------------------------------------------
  // getCurrentExecutableFolder
  //
  YAE_API bool
  getCurrentExecutableFolder(std::string & exeFolderPathUtf8);

  //----------------------------------------------------------------
  // getCurrentExecutablePluginsFolder
  //
  YAE_API bool
  getCurrentExecutablePluginsFolder(std::string & pluginsFolderPathUtf8);

  //----------------------------------------------------------------
  // loadLibrary
  //
  YAE_API void *
  loadLibrary(const char * filepathUtf8);

  //----------------------------------------------------------------
  // getSymbol
  //
  YAE_API void *
  getSymbol(void * module, const char * symbol);

  //----------------------------------------------------------------
  // TOpenFolder
  //
  struct YAE_API TOpenFolder
  {
    // NOTE: this will throw an exceptions if the folder doesn't exist
    // or can not be opened.
    TOpenFolder(const std::string & folderPath);
    ~TOpenFolder();

    // accessor to the folder path:
    std::string folderPath() const;

    // parse the next item in the folder:
    bool parseNextItem();

    // accessors to current item:
    bool itemIsFolder() const;
    std::string itemName() const;
    std::string itemPath() const;

  private:
    TOpenFolder(const TOpenFolder &);
    TOpenFolder & operator = (const TOpenFolder &);

  protected:
    struct Private;
    Private * private_;
  };


  //----------------------------------------------------------------
  // for_each_file_at
  //
  template <typename TVisitor>
  static void
  for_each_file_at(const std::string & pathUtf8, TVisitor & callback)
  {
    try
    {
      TOpenFolder folder(pathUtf8);
      while (folder.parseNextItem())
      {
        std::string name = folder.itemName();
        std::string path = folder.itemPath();
        bool isSubFolder = folder.itemIsFolder();

        if (isSubFolder)
        {
          if (name == "." || name == "..")
          {
            continue;
          }
        }

        if (!callback(isSubFolder, name, path))
        {
          return;
        }

        if (isSubFolder)
        {
          for_each_file_at(path, callback);
        }
      }
    }
    catch (...)
    {
      std::string name = fs::path(pathUtf8).filename().string();
      callback(false, name, pathUtf8);
    }
  }


  //----------------------------------------------------------------
  // CollectMatchingFiles
  //
  struct CollectMatchingFiles
  {
    CollectMatchingFiles(std::set<std::string> & dst,
                         const std::string & regex):
      pattern_(regex, boost::regex::icase),
      files_(dst)
    {}

    bool operator()(bool isFolder,
                    const std::string & name,
                    const std::string & path)
    {
      if (!isFolder && boost::regex_match(name, pattern_))
      {
        files_.insert(path);
      }

      return true;
    }

  protected:
    boost::regex pattern_;
    std::set<std::string> & files_;
  };


  //----------------------------------------------------------------
  // read
  //
  YAE_API std::size_t
  read(FILE * file, void * buffer, std::size_t buffer_size);

  //----------------------------------------------------------------
  // read
  //
  YAE_API std::string read(FILE * file);

  //----------------------------------------------------------------
  // load
  //
  YAE_API std::size_t
  load(FILE * file, std::vector<unsigned char> & out);

  //----------------------------------------------------------------
  // write
  //
  YAE_API bool
  write(FILE * file, const void * buffer, std::size_t buffer_size);

  //----------------------------------------------------------------
  // write
  //
  YAE_API bool
  write(FILE * file, const std::string & text);

  //----------------------------------------------------------------
  // write
  //
  YAE_API bool
  write(FILE * file, const char * text);


  //----------------------------------------------------------------
  // TOpenFile
  //
  struct YAE_API TOpenFile
  {
    // NOTE: this will throw an exceptions if the file doesn't exist
    // or can not be opened.
    TOpenFile(const char * filenameUtf8, const char * mode);
    ~TOpenFile();

    inline bool is_open() const
    {
      return file_ != NULL;
    }

    inline bool is_eof() const
    {
      return !file_ || feof(file_);
    }

    inline bool write(const void * buffer, std::size_t buffer_size)
    {
      return yae::write(this->file_, buffer, buffer_size);
    }

    inline std::size_t read(void * buffer, std::size_t buffer_size)
    {
      return yae::read(this->file_, buffer, buffer_size);
    }

    inline std::string read()
    {
      return yae::read(this->file_);
    }

    inline std::size_t load(std::vector<unsigned char> & out)
    {
      return yae::load(this->file_, out);
    }

    inline bool write(const std::string & text)
    {
      return this->write(text.c_str(), text.size());
    }

    inline bool write(const char * text)
    {
      return this->write(text, ::strlen(text));
    }

    // the file handle:
    FILE * file_;
  };

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

#if defined(_MSC_VER) && _MSC_VER < 1800
  //----------------------------------------------------------------
  // round
  //
  inline double round(double number)
  {
    return number < 0.0 ? ::ceil(number - 0.5) : ::floor(number + 0.5);
  }
#else
  inline double round(double number)
  {
    return ::round(number);
  }
#endif

  //----------------------------------------------------------------
  // at
  //
  template <typename TValue>
  inline static const TValue &
  at(const std::vector<TValue> & v, int offset)
  {
    std::size_t i = offset < 0 ? v.size() - offset : offset;
    return v.at(i);
  }

  //----------------------------------------------------------------
  // at
  //
  template <typename TValue>
  inline static TValue &
  at(std::vector<TValue> & v, int offset)
  {
    std::size_t i = offset < 0 ? v.size() - offset : offset;
    return v.at(i);
  }

  //----------------------------------------------------------------
  // at
  //
  template <typename TKey, typename TValue>
  inline static const TValue &
  at(const std::map<TKey, TValue> & lut, const TKey & key)
  {
    typename std::map<TKey, TValue>::const_iterator found = lut.find(key);

    if (found == lut.end())
    {
      YAE_ASSERT(false);
      throw std::out_of_range("key not found");
    }

    return found->second;
  }

  //----------------------------------------------------------------
  // at
  //
  template <typename TKey, typename TValue>
  inline static TValue &
  at(std::map<TKey, TValue> & lut, const TKey & key)
  {
    typename std::map<TKey, TValue>::iterator found = lut.find(key);

    if (found == lut.end())
    {
      YAE_ASSERT(false);
      throw std::out_of_range("key not found");
    }

    return found->second;
  }

  //----------------------------------------------------------------
  // has
  //
  template <typename TKey, typename TValue>
  inline static bool
  has(const std::map<TKey, TValue> & lut, const TKey & key)
  {
    typename std::map<TKey, TValue>::const_iterator found = lut.find(key);
    return found != lut.end();
  }

  //----------------------------------------------------------------
  // get
  //
  template <typename TKey, typename TValue>
  inline static TValue
  get(const std::map<TKey, TValue> & lut,
      const TKey & key,
      const TValue & defaultValue = TValue())
  {
    typename std::map<TKey, TValue>::const_iterator found = lut.find(key);
    return found == lut.end() ? defaultValue : found->second;
  }

  //----------------------------------------------------------------
  // get
  //
  template <typename TValue>
  inline static TValue
  get(const std::map<std::string, TValue> & lut,
      const char * key,
      const TValue & defaultValue = TValue())
  {
    return get<std::string, TValue>(lut, std::string(key), defaultValue);
  }

  //----------------------------------------------------------------
  // put
  //
  template <typename TValue>
  inline static void
  put(std::map<std::string, TValue> & lut,
      const char * key,
      const TValue & value)
  {
    lut[std::string(key)] = value;
  }

  //----------------------------------------------------------------
  // has
  //
  template <typename TValue>
  inline bool
  has(const std::set<TValue> & values, const TValue & value)
  {
    typename std::set<TValue>::const_iterator found = values.find(value);
    return found != values.end();
  }

  //----------------------------------------------------------------
  // has
  //
  template <typename TValue>
  inline bool
  has(const std::list<TValue> & values, const TValue & value)
  {
    typename std::list<TValue>::const_iterator found =
      std::find(values.begin(), values.end(), value);
    return found != values.end();
  }

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
  // remove_one
  //
  template <typename TData>
  void
  remove_one(std::list<TData> & c, const TData & v)
  {
    typename std::list<TData>::iterator
      found = std::find(c.begin(), c.end(), v);

    if (found != c.end())
    {
      c.erase(found);
    }
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
  // str
  //
  template <typename TData>
  inline static std::string
  str(const std::string & a, const TData & b)
  {
    std::ostringstream oss;
    oss << a << b;
    return oss.str();
  }

  //----------------------------------------------------------------
  // str
  //
  template <typename TData>
  inline static std::string
  str(const char * a, const TData & b)
  {
    std::ostringstream oss;
    oss << a << b;
    return oss.str();
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

  //----------------------------------------------------------------
  // to_lower
  //
  YAE_API std::string
  to_lower(const std::string & in);

  //----------------------------------------------------------------
  // TDictionary
  //
  typedef std::map<std::string, std::string> TDictionary;

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const TDictionary & dict);

  //----------------------------------------------------------------
  // extend
  //
  YAE_API void
  extend(TDictionary & dst, const TDictionary & src);

  //----------------------------------------------------------------
  // unhex
  //
  inline unsigned char unhex(char ch)
  {
    char shift =
      (ch >= '0' && ch <= '9') ? '0' :
      (ch >= 'A' && ch <= 'F') ? 'A' :
      (ch >= 'a' && ch <= 'f') ? 'a' :
      0;

    YAE_ASSERT(shift);

    unsigned char offset =
      ((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) ? 10 : 0;

    return offset + (ch - shift);
  }

  //----------------------------------------------------------------
  // hex_to_byte
  //
  inline unsigned char hex_to_byte(const char * h)
  {
    return ((unhex(h[0]) << 4) | unhex(h[1]));
  }

  //----------------------------------------------------------------
  // unhex
  //
  inline std::vector<unsigned char> unhex(const char * h)
  {
    std::size_t l = strlen(h);
    YAE_ASSERT(l % 2 == 0);

    const char * src = h;
    const char * end = src + l;

    std::vector<unsigned char> b(l / 2);
    unsigned char * dst = b.empty() ? NULL : &(b[0]);

    for (; src < end; src += 2, dst += 1)
    {
      *dst = hex_to_byte(src);
    }

    return b;
  }

  //----------------------------------------------------------------
  // hex_to_u24
  //
  inline unsigned int hex_to_u24(const char * h)
  {
    unsigned char b0 = hex_to_byte(h);
    unsigned char b1 = hex_to_byte(h + 2);
    unsigned char b2 = hex_to_byte(h + 4);
    unsigned int u24 = (b0 << 16) | (b1 << 8) | b2;
    return u24;
  }
}


#endif // YAE_UTILS_H_

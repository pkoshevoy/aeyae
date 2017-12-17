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
#include <map>
#include <string.h>
#include <cstdio>
#include <sstream>

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

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
          forEachFileAt(path, callback);
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
  // TOpenFile
  //
  struct YAE_API TOpenFile
  {
    // NOTE: this will throw an exceptions if the file doesn't exist
    // or can not be opened.
    TOpenFile(const char * filenameUtf8, const char * mode);
    ~TOpenFile();

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

  //----------------------------------------------------------------
  // get
  //
  template <typename TKey, typename TValue>
  static TValue
  get(const std::map<TKey, TValue> & lut,
      const TKey & key,
      const TValue & defaultValue = TValue())
  {
    typename std::map<TKey, TValue>::const_iterator found = lut.find(key);
    return found == lut.end() ? defaultValue : found->second;
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

  //----------------------------------------------------------------
  // to_lower
  //
  YAE_API std::string
  to_lower(const std::string & in);

}


#endif // YAE_UTILS_H_

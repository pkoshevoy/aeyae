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
#include <stdarg.h>
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

// jsoncpp:
#include "json/json.h"

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
  // get_main_args_utf8
  //
  YAE_API void
  get_main_args_utf8(int & argc, char **& argv);

  //----------------------------------------------------------------
  // set_console_output_utf8
  //
  YAE_API void
  set_console_output_utf8();

  //----------------------------------------------------------------
  // getenv_utf8
  //
  YAE_API bool
  getenv_utf8(const char * var_utf8, std::string & value);

  //----------------------------------------------------------------
  // get_home_path_utf8
  //
  // NOTE: this may throw a runtime_error if $HOME is not set
  //
  YAE_API std::string
  get_home_path_utf8();

  //----------------------------------------------------------------
  // get_user_folder_path
  //
  // NOTE: this relies on get_home_path_utf8, so same exceptions apply
  //
  YAE_API std::string
  get_user_folder_path(const char * folder_utf8);

  //----------------------------------------------------------------
  // mkdir_utf8
  //
  YAE_API bool
  mkdir_utf8(const char * path_utf8);

  //----------------------------------------------------------------
  // mkdir_p
  //
  YAE_API bool
  mkdir_p(const std::string & path_utf8);

  //----------------------------------------------------------------
  // mkdir_p
  //
  // like `mkdir -p /some/new/path`
  //
  YAE_API bool
  mkdir_p(const char * path_utf8);

  //----------------------------------------------------------------
  // rename_utf8
  //
  YAE_API int
  rename_utf8(const char * fn_old_utf8, const char * fn_new_utf8);

  //----------------------------------------------------------------
  // remove_utf8
  //
  YAE_API bool
  remove_utf8(const char * fn_utf8);

  //----------------------------------------------------------------
  // remove_utf8
  //
  YAE_API bool
  remove_utf8(const std::string & fn_utf8);

  //----------------------------------------------------------------
  // fopen_utf8
  //
  YAE_API std::FILE *
  fopen_utf8(const char * filename_utf8, const char * mode);

  //----------------------------------------------------------------
  // ftell64
  //
  YAE_API uint64_t
  ftell64(const FILE * file);

  //----------------------------------------------------------------
  // fseek64
  //
  YAE_API int
  fseek64(FILE * file, int64_t offset, int whence);

  //----------------------------------------------------------------
  // open_utf8
  //
  YAE_API int
  open_utf8(const char * filename_utf8, int access_mode, int permissions);

  //----------------------------------------------------------------
  // file_seek64
  //
  YAE_API int64
  file_seek64(int fd, int64 offset, int whence);

  //----------------------------------------------------------------
  // file_size64
  //
  YAE_API int64
  file_size64(int fd);

  //----------------------------------------------------------------
  // kDirSeparator
  //
#ifdef _WIN32
#define kDirSeparator "\\"
#else
#define kDirSeparator "/"
#endif

  //----------------------------------------------------------------
  // join_paths
  //
  YAE_API std::string
  join_paths(const std::string & a,
             const std::string & b,
             const char * pathSeparator = kDirSeparator);

  //----------------------------------------------------------------
  // parse_file_path
  //
  YAE_API bool
  parse_file_path(const std::string & file_path,
                  std::string & folder,
                  std::string & name);

  //----------------------------------------------------------------
  // parse_file_name
  //
  YAE_API bool
  parse_file_name(const std::string & fileName,
                  std::string & name,
                  std::string & ext);

  //----------------------------------------------------------------
  // get_module_filename
  //
  YAE_API bool
  get_module_filename(const void * symbol, std::string & filepath_utf8);

  //----------------------------------------------------------------
  // get_current_executable_path
  //
  YAE_API bool
  get_current_executable_path(std::string & exe_path_utf8);

  //----------------------------------------------------------------
  // get_current_executable_folder
  //
  YAE_API bool
  get_current_executable_folder(std::string & exe_folder_path_utf8);

  //----------------------------------------------------------------
  // get_current_executable_plugins_folder
  //
  YAE_API bool
  get_current_executable_plugins_folder(std::string & plugins_folder_path_utf8);

  //----------------------------------------------------------------
  // load_library
  //
  YAE_API void *
  load_library(const char * filepath_utf8);

  //----------------------------------------------------------------
  // get_symbol
  //
  YAE_API void *
  get_symbol(void * module, const char * symbol);

  //----------------------------------------------------------------
  // TOpenFolder
  //
  struct YAE_API TOpenFolder
  {
    TOpenFolder();

    // NOTE: this will throw an exceptions if the folder doesn't exist,
    // or can not be opened, or the folder is empty:
    TOpenFolder(const std::string & folder_path);
    ~TOpenFolder();

    // return false if the folder doesn't exist
    // or can not be opened.
    bool open(const std::string & folder_path);

    // accessor to the folder path:
    std::string folder_path() const;

    // parse the next item in the folder:
    bool parse_next_item();

    // accessors to current item:
    bool item_is_folder() const;
    std::string item_name() const;
    std::string item_path() const;

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
  for_each_file_at(const std::string & path_utf8, TVisitor & callback)
  {
    try
    {
      TOpenFolder folder(path_utf8);
      while (folder.parse_next_item())
      {
        std::string name = folder.item_name();
        std::string path = folder.item_path();
        bool is_subfolder = folder.item_is_folder();

        if (is_subfolder)
        {
          if (name == "." || name == "..")
          {
            continue;
          }
        }

        if (!callback(is_subfolder, name, path))
        {
          return;
        }

        if (is_subfolder)
        {
          for_each_file_at(path, callback);
        }
      }
    }
    catch (...)
    {
      std::string name = fs::path(path_utf8).filename().string();
      callback(false, name, path_utf8);
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

    bool operator()(bool is_folder,
                    const std::string & name,
                    const std::string & path)
    {
      if (!is_folder && boost::regex_match(name, pattern_))
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
    // NOTE: file open can silently fail, so the caller is responsible
    // to check whether the file is_open:
    TOpenFile();
    TOpenFile(const char * filename_utf8, const char * mode);
    TOpenFile(const std::string & filename_utf8, const char * mode);
    ~TOpenFile();

    bool open(const char * filename_utf8, const char * mode);
    bool open(const std::string & filename_utf8, const char * mode);
    void close();

    inline bool is_open() const
    { return file_ != NULL; }

    inline bool is_eof() const
    { return !file_ || feof(file_); }

    inline bool write(const void * buffer, std::size_t buffer_size)
    { return yae::write(this->file_, buffer, buffer_size); }

    inline std::size_t read(void * buffer, std::size_t buffer_size)
    { return yae::read(this->file_, buffer, buffer_size); }

    inline std::string read()
    { return yae::read(this->file_); }

    inline std::size_t load(std::vector<unsigned char> & out)
    { return yae::load(this->file_, out); }

    inline std::size_t load(std::string & out)
    {
      out.clear();
      out = yae::read(this->file_);
      return out.size();
    }

    inline bool write(const std::string & text)
    { return this->write(text.c_str(), text.size()); }

    inline bool write(const char * text)
    { return text ? this->write(text, ::strlen(text)) : true; }

    inline bool load(Json::Value & v)
    {
      std::string document;
      return this->load(document) > 0 && Json::Reader().parse(document, v);
    }

    inline bool save(const Json::Value & v)
    {
      std::ostringstream oss;
      Json::StyledStreamWriter().write(oss, v);
      return this->write(oss.str());
    }

    inline void flush()
    { if (file_) fflush(file_); }

    // the file handle:
    FILE * file_;

  private:
    // intentionally disabled:
    TOpenFile(const TOpenFile &);
    TOpenFile & operator = (const TOpenFile &);
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
      const TValue & default_value = TValue())
  {
    return get<std::string, TValue>(lut, std::string(key), default_value);
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
  // is_size_one
  //
  template <typename TContainer>
  inline bool
  is_size_one(const TContainer & c)
  {
    typename TContainer::const_iterator i = c.begin();
    typename TContainer::const_iterator e = c.end();
    return (i != e) && (++i == e);
  }

  //----------------------------------------------------------------
  // is_size_two_or_more
  //
  template <typename TContainer>
  inline bool
  is_size_two_or_more(const TContainer & c)
  {
    typename TContainer::const_iterator i = c.begin();
    typename TContainer::const_iterator e = c.end();
    return (i != e) && (++i != e);
  }

  //----------------------------------------------------------------
  // index_of
  //
  template <typename TData>
  std::size_t
  index_of(const TData & item, const TData * items, std::size_t num_items)
  {
    for (std::size_t i = 0; i < num_items; ++i, ++items)
    {
      if (item == *items)
      {
        return i;
      }
    }

    return num_items;
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
  int
  compare(const TData & a, const TData & b)
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
  // bitmask_width<64>
  //
  template <unsigned int width>
  inline unsigned int
  bitmask_width(uint64_t m)
  {
    unsigned int w_hi = bitmask_width<(width >> 1)>(m >> (width >> 1));
    return w_hi ? (w_hi + (width >> 1)) : bitmask_width<(width >> 1)>(m);
  }

  //----------------------------------------------------------------
  // bitmask_width<4>
  //
  template <>
  inline unsigned int
  bitmask_width<4>(uint64_t m)
  {
    static const unsigned char w[16] = {
      0, 1, 2, 2, 3, 3, 3, 3,
      4, 4, 4, 4, 4, 4, 4, 4
    };

    return w[m & 0xF];
  }

  //----------------------------------------------------------------
  // bitmask_width
  //
  inline unsigned int
  bitmask_width(uint64_t m)
  {
    return bitmask_width<64>(m);
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
  // to_text
  //
  template <typename TData>
  std::string
  to_text(const TData & data)
  {
    std::ostringstream os;
    os << data;
    return std::string(os.str().c_str());
  }

  //----------------------------------------------------------------
  // to_scalar
  //
  template <typename TScalar, typename TText>
  TScalar
  to_scalar(const TText & text)
  {
    std::istringstream is;
    is.str(std::string(text));

    TScalar v = (TScalar)0;
    is >> v;

    return v;
  }

  //----------------------------------------------------------------
  // strip_html_tags
  //
  YAE_API std::string
  strip_html_tags(const std::string & in);

  //----------------------------------------------------------------
  // assa_to_plain_text
  //
  YAE_API std::string
  assa_to_plain_text(const std::string & in);

  //----------------------------------------------------------------
  // convert_escape_codes
  //
  YAE_API std::string
  convert_escape_codes(const std::string & in);

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
  // to_hex
  //
  YAE_API std::string
  to_hex(const unsigned char * src, std::size_t src_size, std::size_t word_size = 0);

  //----------------------------------------------------------------
  // load_from_hex
  //
  YAE_API void
  from_hex(unsigned char * dst, std::size_t dst_size, const char * hex_str);

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

  //----------------------------------------------------------------
  // vstrfmt
  //
  YAE_API std::string
  vstrfmt(const char * format, va_list args);

  //----------------------------------------------------------------
  // strfmt
  //
  YAE_API std::string
  strfmt(const char * format, ...);

  //----------------------------------------------------------------
  // get_open_file
  //
  // NOTE: if file open fails this will throw a std::runtime_error
  //
  YAE_API boost::shared_ptr<TOpenFile>
  get_open_file(const char * path, const char * mode);

  //----------------------------------------------------------------
  // dump
  //
  template <typename TString>
  inline void
  dump(const TString & path, const void * data, std::size_t size)
  {
    std::string p(path);
    boost::shared_ptr<TOpenFile> file = yae::get_open_file(p.c_str(), "wb");
    file->write(data, size);
    file->flush();
  }

  //----------------------------------------------------------------
  // jot
  //
  template <typename TString>
  inline void
  jot(const TString & path, const char * format, ...)
  {
    va_list arglist;
    va_start(arglist, format);
    std::string txt = yae::vstrfmt(format, arglist);
    va_end(arglist);

    std::string p(path);
    boost::shared_ptr<TOpenFile> file = yae::get_open_file(p.c_str(), "wb");
    file->write(txt);
    file->flush();
  }
}


#endif // YAE_UTILS_H_

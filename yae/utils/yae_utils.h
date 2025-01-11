// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:34:13 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_H_
#define YAE_UTILS_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_assert.h"
#include "yae/utils/yae_data.h"

// standard:
#include <cstdio>
#include <iterator>
#include <list>
#include <map>
#include <math.h>
#include <set>
#include <stdarg.h>
#include <stdexcept>
#include <string.h>
#include <sstream>
#include <vector>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// jsoncpp:
#include "json/json.h"

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
  // get_temp_dir_utf8
  //
  // NOTE: this may throw a runtime exception:
  //
  YAE_API std::string
  get_temp_dir_utf8();

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
  // rename_utf8
  //
  inline int
  rename_utf8(const std::string & fn_old_utf8,
              const std::string & fn_new_utf8)
  { return rename_utf8(fn_old_utf8.c_str(), fn_new_utf8.c_str()); }

  //----------------------------------------------------------------
  // atomic_rename_utf8
  //
  // This uses ReplaceFile WIN32 API on windows,
  // otherwise this is the same as rename_utf8.
  //
  YAE_API int
  atomic_rename_utf8(const char * fn_old_utf8,
                     const char * fn_new_utf8);

  //----------------------------------------------------------------
  // atomic_rename_utf8
  //
  inline int
  atomic_rename_utf8(const std::string & fn_old_utf8,
                     const std::string & fn_new_utf8)
  { return atomic_rename_utf8(fn_old_utf8.c_str(), fn_new_utf8.c_str()); }

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
  // stat_lastmod
  //
  // NOTE: uses stat to return (unix) time of last modification,
  // returns std::numeric_limits<int64_t>::min() if stat call fails.
  //
  YAE_API int64_t
  stat_lastmod(const char * path_utf8);

  //----------------------------------------------------------------
  // stat_filesize
  //
  YAE_API uint64_t
  stat_filesize(const char * path_utf8);

  //----------------------------------------------------------------
  // stat_diskspace
  //
  YAE_API bool
  stat_diskspace(const char * path_utf8,
                 uint64_t & filesystem_bytes,
                 uint64_t & filesystem_bytes_free,
                 uint64_t & available_bytes);

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
  // NOTE: passed back filename extension (if any) is ext, not .ext
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
  for_each_file_at(const std::string & path_utf8,
                   TVisitor & callback,
                   bool skip_subfolders = false)
  {
    TOpenFolder folder;
    if (folder.open(path_utf8))
    {
      do
      {
        std::string name = folder.item_name();
        std::string path = folder.item_path();
        bool is_subfolder = false;

        try
        {
          is_subfolder = folder.item_is_folder();
        }
        catch (const std::exception & e)
        {
          continue;
        }

        if (is_subfolder)
        {
          if (name == "." || name == "..")
          {
            continue;
          }

          if (skip_subfolders)
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
      while (folder.parse_next_item());
    }
    else
    {
      std::string name = fs::path(path_utf8).filename().string();
      callback(false, name, path_utf8);
    }
  }


  //----------------------------------------------------------------
  // CollectMatchingFiles
  //
  struct YAE_API CollectMatchingFiles
  {
    CollectMatchingFiles(std::set<std::string> & dst,
                         const std::string & regex);

    bool operator()(bool is_folder,
                    const std::string & name,
                    const std::string & path);

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

    inline bool write(const std::vector<unsigned char> & data)
    { return data.empty() ? true : this->write(&(data[0]), data.size()); }

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

    inline int seek(int64 offset, int whence)
    { return file_ ? yae::fseek64(file_, offset, whence) : -1; }

    // the file handle:
    FILE * file_;

  private:
    // intentionally disabled:
    TOpenFile(const TOpenFile &);
    TOpenFile & operator = (const TOpenFile &);
  };

  //----------------------------------------------------------------
  // TOpenFilePtr
  //
  typedef boost::shared_ptr<TOpenFile> TOpenFilePtr;


  //----------------------------------------------------------------
  // generate_uuid
  //
  // thread-safe UUID generator function
  //
  YAE_API std::string generate_uuid();

  //----------------------------------------------------------------
  // atomic_save
  //
  // thread-safe
  //
  YAE_API bool atomic_save(const std::string & path,
                           const Json::Value & data);

  //----------------------------------------------------------------
  // attempt_load
  //
  // attempt to load JSON data from the specified file path and retry
  // upto max_attempts, sleeping for msec_sleep * attempt in between
  //
  YAE_API bool attempt_load(const std::string & path,
                            Json::Value & data,
                            unsigned int max_attempts = 3,
                            unsigned int msec_sleep = 10);


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
  // close_enough
  //
  inline static bool
  close_enough(const double & ref,
               const double & given,
               const double tolerance = 1e-6)
  {
    double err = fabs(given - ref);
    return err < tolerance;
  }

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
  // not_whitespace
  //
  YAE_API bool
  not_whitespace(char c);

  //----------------------------------------------------------------
  // strip_head_ws
  //
  YAE_API void
  strip_head_ws(std::string & str);

  //----------------------------------------------------------------
  // strip_tail_ws
  //
  YAE_API void
  strip_tail_ws(std::string & str);

  //----------------------------------------------------------------
  // strip_ws
  //
  YAE_API void
  strip_ws(std::string & str);

  //----------------------------------------------------------------
  // trim_head_ws
  //
  YAE_API std::string
  trim_head_ws(const std::string & str);

  //----------------------------------------------------------------
  // trim_tail_ws
  //
  YAE_API std::string
  trim_tail_ws(const std::string & str);

  //----------------------------------------------------------------
  // trim_ws
  //
  YAE_API std::string
  trim_ws(const std::string & str);

  //----------------------------------------------------------------
  // trim_from_head
  //
  YAE_API std::string
  trim_from_head(const char * c, const char * str, std::size_t str_len);

  //----------------------------------------------------------------
  // trim_from_tail
  //
  YAE_API std::string
  trim_from_tail(const char * c, const char * str, std::size_t str_len);


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
  to_hex(const void * src,
         std::size_t src_size,
         std::size_t word_size = 0);

  //----------------------------------------------------------------
  // to_hex
  //
  template <typename TData>
  std::string
  to_hex(TData data)
  {
    static const char * alphabet = "0123456789ABCDEF";
    static const std::size_t max = sizeof(TData) * 2;
    char tmp[sizeof(TData) * 2] = { '0' };
    std::size_t i = 0;
    while (data)
    {
      unsigned char h = data & 0xF;
      data >>= 4;
      tmp[max - i - 1] = alphabet[h];
      i++;
    }

    return std::string(tmp + (max - i), tmp + max);
  }

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
  // unicode_to_utf8
  //
  template <typename TContainer>
  static bool
  unicode_to_utf8(unsigned int codepoint, TContainer & out)
  {
    typedef typename TContainer::value_type TByte;

    assert(codepoint < 0x110000);

    if (codepoint < 0x0080)
    {
      out.push_back((unsigned char)codepoint);
    }
    else if (codepoint < 0x0800)
    {
      unsigned char b1 = (0x3 << 6) | ((codepoint >> 6) & 0x1F);
      unsigned char b2 = (0x1 << 7) | (codepoint & 0x3F);
      out.push_back(TByte(b1));
      out.push_back(TByte(b2));
    }
    else if (codepoint < 0x10000)
    {
      unsigned char b1 = (0x7 << 5) | ((codepoint >> 12) & 0xF);
      unsigned char b2 = (0x1 << 7) | ((codepoint >> 6) & 0x3F);
      unsigned char b3 = (0x1 << 7) | (codepoint & 0x3F);
      out.push_back(TByte(b1));
      out.push_back(TByte(b2));
      out.push_back(TByte(b3));
    }
    else if (codepoint < 0x110000)
    {
      unsigned char b1 = (0xF << 4) | ((codepoint >> 18) & 0x7);
      unsigned char b2 = (0x1 << 7) | ((codepoint >> 12) & 0x3F);
      unsigned char b3 = (0x1 << 7) | ((codepoint >> 6) & 0x3F);
      unsigned char b4 = (0x1 << 7) | (codepoint & 0x3F);
      out.push_back(TByte(b1));
      out.push_back(TByte(b2));
      out.push_back(TByte(b3));
      out.push_back(TByte(b4));
    }
    else
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // utf8_to_unicode
  //
  YAE_API bool
  utf8_to_unicode(const char *& src, const char * end, uint32_t & uc);

  //----------------------------------------------------------------
  // utf16_to_unicode
  //
  YAE_API bool
  utf16_to_unicode(const uint16_t *& src, const uint16_t * end, uint32_t & uc);

  //----------------------------------------------------------------
  // utf16_to_unicode
  //
  template <std::size_t i0, std::size_t i1>
  bool
  utf16_to_unicode(const uint8_t *& src, const uint8_t * end, uint32_t & uc)
  {
    std::size_t n_bytes = end - src;
    std::size_t i2 = i0 + 2;
    std::size_t i3 = i1 + 2;

    if ((n_bytes > 3) &&
        (0xD8 <= src[i0]) && (src[i0] <= 0xDB) &&
        (0xDC <= src[i2]) && (src[i2] <= 0xDF))
    {
      // decode surrogate pair:
      uint16_t hi = (src[i0] << 8) | src[i1];
      uint16_t lo = (src[i2] << 8) | src[i3];
      uc = 0x10000;
      uc += (hi & 0x03FF) << 10;
      uc += (lo & 0x03FF);
      src += 4;
      return true;
    }

    if ((n_bytes > 1) && (src[i0] < 0xD8 || 0xE0 <= src[i0]))
    {
      uc = (src[i0] << 8) | src[i1];
      src += 2;
      return true;
    }

    uc = 0;
    return false;
  }

  //----------------------------------------------------------------
  // utf16_to_utf8
  //
  template <typename TContainer>
  bool
  utf16_to_utf8(const uint16_t * src, const uint16_t * end, TContainer & out)
  {
    unsigned int uc = 0;

    while (src < end)
    {
      if (!utf16_to_unicode(src, end, uc))
      {
        return false;
      }

      if (!unicode_to_utf8<TContainer>(uc, out))
      {
        return false;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // utf16be_to_unicode
  //
  YAE_API bool
  utf16be_to_unicode(const uint8_t *& src, const uint8_t * end, uint32_t & uc);

  //----------------------------------------------------------------
  // utf16le_to_unicode
  //
  YAE_API bool
  utf16le_to_unicode(const uint8_t *& src, const uint8_t * end, uint32_t & uc);

  //----------------------------------------------------------------
  // utf16_to_utf8
  //
  template <std::size_t i0, std::size_t i1, typename TContainer>
  bool
  utf16_to_utf8(const uint8_t * src, const uint8_t * end, TContainer & out)
  {
    unsigned int uc = 0;

    while (src < end)
    {
      if (!utf16_to_unicode<i0, i1>(src, end, uc))
      {
        return false;
      }

      if (!unicode_to_utf8<TContainer>(uc, out))
      {
        return false;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // utf16be_to_utf8
  //
  template <typename TContainer>
  bool
  utf16be_to_utf8(const uint8_t * src, const uint8_t * end, TContainer & out)
  {
    return utf16_to_utf8<0, 1, TContainer>(src, end, out);
  }

  //----------------------------------------------------------------
  // utf16le_to_utf8
  //
  template <typename TContainer>
  bool
  utf16le_to_utf8(const uint8_t * src, const uint8_t * end, TContainer & out)
  {
    return utf16_to_utf8<1, 0, TContainer>(src, end, out);
  }

  //----------------------------------------------------------------
  // unescape
  //
  // convert \t \n \r \xHH \unnnn \Unnnnnnnn to UTF-8 byte sequence
  //
  template <typename TContainer>
  void
  unescape(const std::string & str, TContainer & out)
  {
    typedef typename TContainer::value_type TByte;

    const char * src = str.empty() ? NULL : &str[0];
    const char * end = src + str.size();

    while (src < end)
    {
      char c = *src++;

      if (c == '\\' && src < end)
      {
        c = *src++;

        if (c == 'a')
        {
          c = '\a';
        }
        else if (c == 'b')
        {
          c = '\b';
        }
        else if (c == 'f')
        {
          c = '\f';
        }
        else if (c == 'n')
        {
          c = '\n';
        }
        else if (c == 'r')
        {
          c = '\r';
        }
        else if (c == 't')
        {
          c = '\t';
        }
        else if (c == 'v')
        {
          c = '\v';
        }
        else if (c >= '0' && c <= '9' && (end - src) > 1)
        {
          // octal byte code:
          unsigned int o[3] = { 0 };
          o[0] = c - '0';
          o[1] = (*src++) - '0';
          o[2] = (*src++) - '0';

          assert(o[0] < 4 && o[1] < 8 && o[2] < 8);
          c = char((o[0] << 6) |
                   (o[1] << 3) |
                   (o[2]));
        }
        else if (c == 'x' && (end - src) > 1)
        {
          // hex byte code:
          unsigned int h[2] = { 0 };
          h[0] = unhex(*src++);
          h[1] = unhex(*src++);

          c = char((h[0] << 4) | h[1]);
        }
        else if (c == 'u' && (end - src) > 3)
        {
          // unicode:
          unsigned int h[4] = { 0 };
          h[0] = unhex(*src++);
          h[1] = unhex(*src++);
          h[2] = unhex(*src++);
          h[3] = unhex(*src++);

          unsigned int codepoint = ((h[0] << 12) |
                                    (h[1] << 8) |
                                    (h[2] << 4) |
                                    (h[3]));
          unicode_to_utf8(codepoint, out);
          continue;
        }
        else if (c == 'U' && (end - src) > 7)
        {
          // unicode:
          unsigned int h[8] = { 0 };
          h[0] = unhex(*src++);
          h[1] = unhex(*src++);
          h[2] = unhex(*src++);
          h[3] = unhex(*src++);
          h[4] = unhex(*src++);
          h[5] = unhex(*src++);
          h[6] = unhex(*src++);
          h[7] = unhex(*src++);

          unsigned int codepoint = ((h[0] << 28) |
                                    (h[1] << 24) |
                                    (h[2] << 20) |
                                    (h[3] << 16) |
                                    (h[4] << 12) |
                                    (h[5] << 8) |
                                    (h[6] << 4) |
                                    (h[7]));
          unicode_to_utf8(codepoint, out);
          continue;
        }
      }

      out.push_back((TByte)c);
    }
  }

  //----------------------------------------------------------------
  // split
  //
  YAE_API std::size_t
  split(std::vector<std::string> & tokens,
        const char * separators,
        const char * src,
        const char * end = NULL);

  //----------------------------------------------------------------
  // sanitize_filename_utf8
  //
  YAE_API std::string
  sanitize_filename_utf8(const std::string & fn);

  //----------------------------------------------------------------
  // replace_inplace
  //
  YAE_API void
  replace_inplace(std::string & text,
                  const std::string & search_text,
                  const std::string & replacement);

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
  YAE_API TOpenFilePtr
  get_open_file(const char * path, const char * mode);

  //----------------------------------------------------------------
  // dump
  //
  template <typename TString>
  inline void
  dump(const TString & path, const void * data, std::size_t size)
  {
    std::string p(path);
    TOpenFilePtr file = yae::get_open_file(p.c_str(), "wb");
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
    TOpenFilePtr file = yae::get_open_file(p.c_str(), "wb");
    file->write(txt);
    file->flush();
  }


  //----------------------------------------------------------------
  // ContextCallback
  //
  struct YAE_API ContextCallback
  {
    typedef void(*TFuncPtr)(void *);

    ContextCallback(TFuncPtr func = NULL, void * context = NULL);

    void reset(TFuncPtr func = NULL, void * context = NULL);
    bool is_null() const;
    void operator()() const;

    inline bool operator < (const ContextCallback & other) const
    {
      return (func_ == other.func_ ?
              context_ < other.context_ :
              std::size_t(func_) < std::size_t(other.func_));
    }

  protected:
    TFuncPtr func_;
    void * context_;
  };


  //----------------------------------------------------------------
  // ContextQuery
  //
  template <typename TData>
  struct ContextQuery
  {
    typedef bool(*TFuncPtr)(void *, TData &);

    ContextQuery(TFuncPtr func = NULL, void * context = NULL)
    {
      reset(func, context);
    }

    inline void reset(TFuncPtr func = NULL, void * context = NULL)
    {
      func_ = func;
      context_ = context;
    }

    inline bool is_null() const
    { return !func_; }

    inline bool operator()(TData & result) const
    {
      bool ok = func_ ? func_(context_, result) : false;
      return ok;
    }

  protected:
    TFuncPtr func_;
    void * context_;
  };

  //----------------------------------------------------------------
  // next
  //
  template <typename TIter>
  TIter
  next(const TIter & iter, std::size_t n = 1)
  {
    TIter i = iter;
    std::advance(i, n);
    return i;
  }

}


#endif // YAE_UTILS_H_

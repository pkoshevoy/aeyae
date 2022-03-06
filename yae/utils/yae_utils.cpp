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
#include <sys/param.h>
#include <sys/mount.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <share.h>
#else
#include <dirent.h>
#include <dlfcn.h>
#include <sys/statvfs.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <math.h>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

// aeyae:
#include "yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // parity_lut
  //
  // http://graphics.stanford.edu/~seander/bithacks.html
  //
  const bool parity_lut[256] =
  {
#   define P2(n) n, n^1, n^1, n
#   define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#   define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
    P6(0), P6(1), P6(1), P6(0)
  };

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
  // get_main_args_utf8
  //
  void
  get_main_args_utf8(int & argc, char **& argv)
  {
#ifdef _WIN32
    argc = 0;
    wchar_t * cmd = GetCommandLineW();
    wchar_t ** wargv = CommandLineToArgvW(cmd, &argc);
    for (int i = 0; i < argc; i++)
    {
      argv[i] = utf16_to_cstr(wargv[i], CP_UTF8);
    }
    LocalFree(wargv);
#else
    (void)argc;
    (void)argv;
#endif
  }

  //----------------------------------------------------------------
  // set_console_output_utf8
  //
  void
  set_console_output_utf8()
  {
#ifdef _WIN32
    /* Configure console for UTF-8. */
    SetConsoleOutputCP(CP_UTF8);
#endif
  }

  //----------------------------------------------------------------
  // getenv_utf8
  //
  bool
  getenv_utf8(const char * var_utf8, std::string & value)
  {
#ifdef _WIN32
    std::size_t size = 0;
    std::wstring var = utf8_to_utf16(var_utf8);

    errno_t err = _wgetenv_s(&size, NULL, 0, var.c_str());
    if (err || !size)
    {
      return false;
    }

    std::vector<wchar_t> ret(size);
    err = _wgetenv_s(&size, &(ret[0]), ret.size(), var.c_str());
    if (err)
    {
      assert(false);
      return false;
    }

    value = utf16_to_utf8(&ret[0]);
#else
    char * ret = getenv(var_utf8);
    if (ret)
    {
      value = std::string(ret);
    }
#endif

    return !value.empty();
  }

  //----------------------------------------------------------------
  // get_home_path_utf8
  //
  std::string
  get_home_path_utf8()
  {
    std::string home;
    if (getenv_utf8("HOME", home))
    {
      return home;
    }

#ifdef _WIN32
    if (getenv_utf8("USERPROFILE", home))
    {
      return home;
    }

    std::string homedrive;
    std::string homepath;
    if (getenv_utf8("HOMEDRIVE", homedrive) &&
        getenv_utf8("HOMEPATH", homepath))
    {
    }
#endif

    throw std::runtime_error("unable to determine HOME path");
    return std::string();
  }

  //----------------------------------------------------------------
  // get_user_folder_path
  //
  std::string
  get_user_folder_path(const char * folder_utf8)
  {
    std::string home = get_home_path_utf8();
    std::string path = (fs::path(home) / fs::path(folder_utf8)).string();
    return path;
  }

  //----------------------------------------------------------------
  // get_temp_dir_utf8
  //
  std::string
  get_temp_dir_utf8()
  {
#ifdef _WIN32
    WCHAR wstr[MAX_PATH];
    DWORD wlen = GetTempPathW(MAX_PATH, wstr);
    YAE_THROW_IF(!wlen || wlen > MAX_PATH);

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    YAE_THROW_IF(!len);

    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], len, NULL, NULL);

    if (len > 1 && str[len - 2] == '\\')
    {
      // truncate the trailing separator:
      str[len - 2] = 0;
      str = str.c_str();
    }

    return str;
#else
    return std::string("/tmp");
#endif
  }

  //----------------------------------------------------------------
  // mkdir_utf8
  //
  bool
  mkdir_utf8(const char * path_utf8)
  {
#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(path_utf8);
    int err = _wmkdir(path_utf16.c_str());
#else
    int err = mkdir(path_utf8, S_IRWXU);
#endif
    return !err;
  }

  //----------------------------------------------------------------
  // mkdir_p
  //
  bool
  mkdir_p(const std::string & path_utf8)
  {
    fs::path path(path_utf8);
    try
    {
      fs::create_directories(path);
    }
    catch (...)
    {
      return false;
    }

    bool exists = fs::is_directory(path);
    return exists;
  }

  //----------------------------------------------------------------
  // mkdir_p
  //
  bool
  mkdir_p(const char * path_utf8)
  {
    return path_utf8 && *path_utf8 && yae::mkdir_p(std::string(path_utf8));
  }

  //----------------------------------------------------------------
  // rename_utf8
  //
  int
  rename_utf8(const char * fn_old, const char * fn_new)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    wchar_t * wold = cstr_to_utf16(fn_old);
    wchar_t * wnew = cstr_to_utf16(fn_new);

    int ret = _wrename(wold, wnew);

    free(wold);
    free(wnew);
#else
    int ret = rename(fn_old, fn_new);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // atomic_rename_utf8
  //
  int
  atomic_rename_utf8(const char * fn_old,
                     const char * fn_new)
  {
#ifndef _WIN32
    return rename_utf8(fn_old, fn_new);
#else
    wchar_t * wold = cstr_to_utf16(fn_old);
    wchar_t * wnew = cstr_to_utf16(fn_new);

    BOOL ok = ReplaceFileW(wnew, // to
                           wold, // from
                           NULL, // backup file name
                           0, // flags
                           NULL, // lpExclude
                           NULL);// lpReserved
    int err = ok ? 0 : GetLastError();

    free(wold);
    free(wnew);

    if (err == ERROR_FILE_NOT_FOUND)
    {
      err = rename_utf8(fn_old, fn_new);
    }

    return err;
#endif
  }

  //----------------------------------------------------------------
  // remove_utf8
  //
  bool
  remove_utf8(const char * fn_utf8)
  {
#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(fn_utf8);
    int err = _wremove(path_utf16.c_str());
#else
    int err = ::remove(fn_utf8);
#endif

    return err == 0;
  }

  //----------------------------------------------------------------
  // remove_utf8
  //
  bool
  remove_utf8(const std::string & fn_utf8)
  {
    return remove_utf8(fn_utf8.c_str());
  }

  //----------------------------------------------------------------
  // fopen_utf8
  //
  std::FILE *
  fopen_utf8(const char * filename_utf8, const char * mode)
  {
    std::FILE * file = NULL;

#if defined(_WIN32) && !defined(__MINGW32__)
    wchar_t * wname = cstr_to_utf16(filename_utf8);
    wchar_t * wmode = cstr_to_utf16(mode);

    // type of sharing allowed:
    int shflag =
      (std::strstr(mode, "w") != NULL ||
       std::strstr(mode, "a") != NULL ||
       std::strstr(mode, "r+") != NULL) ?
      _SH_DENYWR : // deny write access to the file
      _SH_DENYNO; // permit read and write access

    file = _wfsopen(wname, wmode, shflag);
    free(wname);
    free(wmode);
#else
    file = fopen(filename_utf8, mode);
#endif

    return file;
  }

  //----------------------------------------------------------------
  // ftell64
  //
  uint64_t
  ftell64(const FILE * file)
  {
#ifdef _WIN32
    uint64_t pos = _ftelli64(const_cast<FILE *>(file));
#else
    uint64_t pos = ftello(const_cast<FILE *>(file));
#endif

    return pos;
  }

  //----------------------------------------------------------------
  // fseek64
  //
  int
  fseek64(FILE * file, int64_t offset, int whence)
  {
#ifdef _WIN32
    int ret = _fseeki64(file, offset, whence);
#else
    int ret = fseeko(file, offset, whence);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // open_utf8
  //
  int
  open_utf8(const char * filename_utf8, int access_mode, int permissions)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    access_mode |= O_BINARY;

    wchar_t * wname = cstr_to_utf16(filename_utf8);
    int fd = -1;
    int sh = access_mode & (_O_RDWR | _O_WRONLY) ? _SH_DENYWR : _SH_DENYNO;

    errno_t err = _wsopen_s(&fd, wname, access_mode, sh, permissions);
    free(wname);
#else
    int fd = open(filename_utf8, access_mode, permissions);
#endif

    return fd;
  }

  //----------------------------------------------------------------
  // file_seek64
  //
  int64
  file_seek64(int fd, int64 offset, int whence)
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
  // file_size64
  //
  int64
  file_size64(int fd)
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
  // stat_lastmod
  //
  int64_t
  stat_lastmod(const char * path_utf8)
  {
#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(path_utf8);
    struct __stat64 st = { 0 };
    int ret = _wstat64(path_utf16.c_str(), &st);
    return ret == 0 ? st.st_mtime : std::numeric_limits<int64_t>::min();
#else
    struct stat st = { 0 };
    int ret = stat(path_utf8, &st);
#ifdef __APPLE__
    return ret == 0 ? st.st_mtime : std::numeric_limits<int64_t>::min();
#else
    return ret == 0 ? st.st_mtim.tv_sec : std::numeric_limits<int64_t>::min();
#endif
#endif
  }

  //----------------------------------------------------------------
  // stat_filesize
  //
  uint64_t
  stat_filesize(const char * path_utf8)
  {
#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(path_utf8);
    struct __stat64 st = { 0 };
    int ret = _wstat64(path_utf16.c_str(), &st);
    return ret == 0 ? st.st_size : 0;
#else
    struct stat st = { 0 };
    int ret = stat(path_utf8, &st);
    return ret == 0 ? st.st_size : 0;
#endif
  }

  //----------------------------------------------------------------
  // stat_diskspace
  //
  bool
  stat_diskspace(const char * path_utf8,
                 uint64_t & filesystem_bytes,
                 uint64_t & filesystem_bytes_free,
                 uint64_t & available_bytes)
  {
    filesystem_bytes = 0;
    filesystem_bytes_free = 0;
    available_bytes = 0;

#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(path_utf8);
    ULARGE_INTEGER bytes_available_to_caller;
    ULARGE_INTEGER total_number_of_bytes;
    ULARGE_INTEGER total_number_of_bytes_free;
    BOOL ok = GetDiskFreeSpaceExW(path_utf16.c_str(),
                                  &bytes_available_to_caller,
                                  &total_number_of_bytes,
                                  &total_number_of_bytes_free);
    if (!ok)
    {
      return false;
    }

    filesystem_bytes = total_number_of_bytes.QuadPart;
    filesystem_bytes_free = total_number_of_bytes_free.QuadPart;
    available_bytes = bytes_available_to_caller.QuadPart;

#elif defined(__APPLE__) && defined(__DARWIN_STRUCT_STATFS64)
    struct statfs64 stat = { 0 };
    int err = statfs64(path_utf8, &stat);
    if (err)
    {
      return false;
    }

    filesystem_bytes = uint64_t(stat.f_bsize) * uint64_t(stat.f_blocks);
    filesystem_bytes_free = uint64_t(stat.f_bsize) * uint64_t(stat.f_bfree);
    available_bytes = uint64_t(stat.f_bsize) * uint64_t(stat.f_bavail);

#else
    struct statvfs stat = { 0 };
    int err = statvfs(path_utf8, &stat);
    if (err)
    {
      return false;
    }

    // the available size is f_bsize * f_bavail
    filesystem_bytes = uint64_t(stat.f_frsize) * uint64_t(stat.f_blocks);
    filesystem_bytes_free = uint64_t(stat.f_frsize) * uint64_t(stat.f_bfree);
    available_bytes = uint64_t(stat.f_frsize) * uint64_t(stat.f_bavail);
#endif

    return true;
  }

  //----------------------------------------------------------------
  // not_whitespace
  //
  bool
  not_whitespace(char c)
  {
    return isspace(c) == 0;
  }

  //----------------------------------------------------------------
  // strip_head_ws
  //
  void
  strip_head_ws(std::string & str)
  {
    std::string::iterator found =
      std::find_if(str.begin(), str.end(), &not_whitespace);
    str.erase(str.begin(), found);
  }

  //----------------------------------------------------------------
  // strip_tail_ws
  //
  void
  strip_tail_ws(std::string & str)
  {
    std::string::reverse_iterator found =
      std::find_if(str.rbegin(), str.rend(), &not_whitespace);
    str.erase(found.base(), str.end());
  }

  //----------------------------------------------------------------
  // strip_ws
  //
  void
  strip_ws(std::string & str)
  {
    strip_tail_ws(str);
    strip_tail_ws(str);
  }

  //----------------------------------------------------------------
  // trim_head
  //
  std::string
  trim_head_ws(const std::string & str)
  {
    std::string out = str;
    strip_head_ws(out);
    return out;
  }

  //----------------------------------------------------------------
  // trim_tail_ws
  //
  std::string
  trim_tail_ws(const std::string & str)
  {
    std::string out = str;
    strip_tail_ws(out);
    return out;
  }

  //----------------------------------------------------------------
  // trim_ws
  //
  std::string
  trim_ws(const std::string & str)
  {
    std::string out = str;
    strip_ws(out);
    return out;
  }

  //----------------------------------------------------------------
  // trim_from_head
  //
  std::string
  trim_from_head(const char * c, const char * str, std::size_t str_len)
  {
    if (!str_len && str)
    {
      str_len = strlen(str);
    }

    const char * end = str ? str + str_len : NULL;
    while (str < end)
    {
      if (*str != *c)
      {
        return std::string(str, end);
      }

      str++;
    }

    return std::string();
  }

  //----------------------------------------------------------------
  // trim_from_tail
  //
  std::string
  trim_from_tail(const char * c, const char * str, std::size_t str_len)
  {
    if (!str_len && str)
    {
      str_len = strlen(str);
    }

    const char * end = str ? str + str_len : NULL;
    while (str < end)
    {
      if (*(end - 1) != *c)
      {
        return std::string(str, end);
      }

      end--;
    }

    return std::string();
  }

  //----------------------------------------------------------------
  // split
  //
  std::size_t
  split(std::vector<std::string> & tokens,
        const char * separators,
        const char * src,
        const char * end)
  {
    if (!end)
    {
      end = src + strlen(src);
    }

    std::set<uint32_t> uc_separators;
    {
      const char * sep_src = separators;
      const char * sep_end = sep_src + strlen(separators);

      while (sep_src < sep_end)
      {
        uint32_t uc = 0;
        bool valid_utf8 = true;
        YAE_ASSERT(valid_utf8 = utf8_to_unicode(sep_src, sep_end, uc));

        if (!valid_utf8)
        {
          // separators must be valid UTF8
          return 0;
        }

        uc_separators.insert(uc);
      }
    }

    std::size_t num_tokens = 0;
    const char * token_pos = src;
    while (src < end)
    {
      uint32_t uc = 0;
      const char * token_end = src;
      if (!utf8_to_unicode(src, end, uc))
      {
        src++;
        continue;
      }

      if (yae::has(uc_separators, uc))
      {
        if (token_pos < token_end)
        {
          tokens.push_back(std::string(token_pos, token_end));
          num_tokens++;
        }

        token_pos = src;
      }
    }

    if (token_pos < end)
    {
      tokens.push_back(std::string(token_pos, end));
      num_tokens++;
    }

    return num_tokens;
  }

  //----------------------------------------------------------------
  // sanitize_file_name
  //
  std::string
  sanitize_filename_utf8(const std::string & fn)
  {
    std::string out;
    const char * src = fn.empty() ? "" : &(fn[0]);
    const char * end = src + strlen(src);
    bool valid_utf8 = true;
    std::size_t nq = 0;

    while (src < end)
    {
      uint32_t uc = 0;
      YAE_EXPECT((valid_utf8 = utf8_to_unicode(src, end, uc)));
      if (valid_utf8)
      {
        if (uc == '/')
        {
          // replace with FULLWIDTH SOLIDUS
          uc = 0xFF0F;
        }
        else if (uc == '\\')
        {
          // replace with FULLWIDTH REVERSE SOLIDUS
          uc = 0xFF3C;
        }
        else if (uc == ':')
        {
          // replace with FULLWIDTH COLON
          uc = 0xFF1A;
        }
        else if (uc == '*')
        {
          // replace with FULLWIDTH ASTERISK
          uc = 0xFF0A;
        }
        else if (uc == '"')
        {
          static const uint32_t left_double_quotation_mark = 0x201C;
          static const uint32_t right_double_quotation_mark = 0x201D;
          uc = ((nq & 1) == 0) ?
            left_double_quotation_mark :
            right_double_quotation_mark;
          nq++;
        }

        unicode_to_utf8(uc, out);
      }
      else
      {
        src++;
        out += '_';
      }
    }

    return out;
  }

  //----------------------------------------------------------------
  // replace_inplace
  //
  void
  replace_inplace(std::string & text,
                  const std::string & search_text,
                  const std::string & replacement)
  {
    std::string::size_type pos = 0;
    while (true)
    {
      pos = text.find(search_text, pos);
      if (pos == std::string::npos)
      {
        break;
      }

      text.replace(pos, search_text.size(), replacement);
      pos += replacement.size();
    }
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
  // join_paths
  //
  std::string
  join_paths(const std::string & a,
             const std::string & b,
             const char * path_separator)
  {
    std::size_t a_len = a.size();
    std::size_t b_len = b.size();

    std::string a_trimmed =
      a_len ? trim_from_tail(path_separator, data(a), a_len) : std::string("");

    std::string b_trimmed =
      b_len ? trim_from_head(path_separator, data(b), b_len) : std::string("");

    std::string ab = a_trimmed + path_separator + b_trimmed;

    return ab;
  }

  //----------------------------------------------------------------
  // parse_file_path
  //
  bool
  parse_file_path(const std::string & file_path,
                  std::string & folder,
                  std::string & name)
  {
    std::size_t index_name = file_path.rfind(kDirSeparator);

    std::size_t	index_name_unix =
#ifdef _WIN32
      file_path.rfind('/')
#else
      index_name
#endif
      ;

    if (index_name_unix != std::string::npos &&
        (index_name == std::string::npos ||
         index_name < index_name_unix))
    {
      // _unix directory separator used before file name
      index_name = index_name_unix;
    }

    if (index_name != std::string::npos)
    {
      name = file_path.substr(index_name + 1, file_path.size());
      folder = file_path.substr(0, index_name);
      return true;
    }

    name = file_path;
    folder = std::string();
    return false;
  }

  //----------------------------------------------------------------
  // parse_file_name
  //
  bool
  parse_file_name(const std::string & file_name,
                  std::string & name,
                  std::string & ext)
  {
    std::size_t indexExt = file_name.rfind('.');
    if (indexExt != std::string::npos)
    {
      ext = file_name.substr(indexExt + 1, file_name.size());
      name = file_name.substr(0, indexExt);
      return true;
    }

    name = file_name;
    ext = std::string();
    return false;
  }

  //----------------------------------------------------------------
  // get_module_filename
  //
  bool
  get_module_filename(const void * symbol, std::string & filename_utf8)
  {
#if defined(_WIN32)
    DWORD flags =
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;

    HMODULE module;
    BOOL ok = ::GetModuleHandleExW(flags,
                                   (LPCWSTR)(const_cast<void *>(symbol)),
                                   &module);
    if (!ok)
    {
      return false;
    }

    wchar_t wpath[_MAX_PATH] = { 0 };
    DWORD nameLen = ::GetModuleFileNameW(module, wpath, _MAX_PATH);
    if (nameLen >= _MAX_PATH)
    {
      return false;
    }

    filename_utf8 = utf16_to_utf8(wpath);
    return true;

#elif defined(__APPLE__) || defined(__linux__)

    Dl_info dlInfo;

    if (!dladdr(symbol, &dlInfo))
    {
      return false;
    }

    filename_utf8.assign(dlInfo.dli_fname);
    return true;

#endif

    // FIXME: write me!
    YAE_ASSERT(false);
    return false;
  }

  //----------------------------------------------------------------
  // get_current_executable_path
  //
  bool
  get_current_executable_path(std::string & filepath_utf8)
  {
    bool ok = false;

#ifdef _WIN32
    wchar_t path[_MAX_PATH] = { 0 };
    unsigned long pathLen = sizeof(path) / sizeof(wchar_t);

    if (GetModuleFileNameW(0, path, pathLen) != 0)
    {
      filepath_utf8 = utf16_to_utf8(path);
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
            filepath_utf8.assign(txt);
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
      filepath_utf8.assign(path);
      ok = true;
    }

#endif

    YAE_ASSERT(!filepath_utf8.empty());
    return ok;
  }

  //----------------------------------------------------------------
  // get_current_executable_folder
  //
  bool
  get_current_executable_folder(std::string & folderpath_utf8)
  {
    std::string filepath_utf8;
    if (!get_current_executable_path(filepath_utf8))
    {
      return false;
    }

    std::string name;
    return parse_file_path(filepath_utf8, folderpath_utf8, name);
  }

  //----------------------------------------------------------------
  // get_current_executable_plugins_folder
  //
  bool
  get_current_executable_plugins_folder(std::string & plugin_path_utf8)
  {
    std::string exe_folder_utf8;
    if (!get_current_executable_folder(exe_folder_utf8))
    {
      return false;
    }

#ifdef __APPLE__
    std::string macos;
    if (!parse_file_path(exe_folder_utf8, plugin_path_utf8, macos) ||
        macos != "MacOS")
    {
      return false;
    }

    plugin_path_utf8 += "/PlugIns";
#else
    plugin_path_utf8 = exe_folder_utf8;

    std::string parent;
    std::string bin;
    if (parse_file_path(exe_folder_utf8, parent, bin) && bin == "bin")
    {
      std::string lib = (fs::path(parent) / "lib").string();
      if (fs::is_directory(lib))
      {
        plugin_path_utf8 = lib;
      }
    }
    else if (bin != "bin" && al::ends_with(parent, "bin"))
    {
      std::string subfolder = bin;
      if (parse_file_path(parent, parent, bin))
      {
        std::string lib = (fs::path(parent) / "lib" / subfolder).string();
        if (fs::is_directory(lib))
        {
          plugin_path_utf8 = lib;
        }
      }
    }
#endif

    return true;
  }

  //----------------------------------------------------------------
  // load_library
  //
  void *
  load_library(const char * filepath_utf8)
  {
#if defined(_WIN32)

    std::wstring wpath = utf8_to_utf16(filepath_utf8);
    HMODULE module = (HMODULE)LoadLibraryW(wpath.c_str());
    return (void *)module;

#elif defined(__APPLE__) || defined(__linux__)

    void * module = dlopen(filepath_utf8, RTLD_NOW);
    return module;

#endif

    // FIXME: write me!
    YAE_ASSERT(false);
    return NULL;
  }

  //----------------------------------------------------------------
  // get_symbol
  //
  void *
  get_symbol(void * module, const char * symbol)
  {
#if defined(_WIN32)
    return ::GetProcAddress((HMODULE)module, symbol);

#elif defined(__APPLE__) || defined(__linux__)
    return ::dlsym(module, symbol);

#endif

    // FIXME: write me!
    YAE_ASSERT(false);
    return NULL;
  }

  //----------------------------------------------------------------
  // TOpenFolder::Private
  //
  struct TOpenFolder::Private
  {
    Private()
    {}

    Private(const std::string & folder_path_utf8):
      path_(fs::absolute(fs::path(folder_path_utf8))),
      iter_(path_)
    {
      if (iter_ == fs::directory_iterator())
      {
        std::ostringstream oss;
        oss << "\"" << path_.string() << "\" folder is empty";
        throw std::runtime_error(oss.str().c_str());
      }
    }

    bool open(const std::string & folder_path_utf8)
    {
      path_ = fs::absolute(fs::path(folder_path_utf8));

      boost::system::error_code err;
      iter_ = fs::directory_iterator(path_, err);
      return !err && iter_ != fs::directory_iterator();
    }

    bool parse_next_item()
    {
      ++iter_;
      bool ok = iter_ != fs::directory_iterator();
      return ok;
    }

    inline std::string folder_path() const
    {
      return path_.string();
    }

    inline bool item_is_folder() const
    {
      return
        (iter_ != fs::directory_iterator()) &&
        (fs::is_directory(iter_->path()));
    }

    inline std::string item_name() const
    {
      return iter_->path().filename().string();
    }

    inline std::string item_path() const
    {
      return iter_->path().string();
    }

  protected:

    fs::path path_;
    fs::directory_iterator iter_;
  };

  //----------------------------------------------------------------
  // TOpenFolder::TOpenFolder
  //
  TOpenFolder::TOpenFolder():
    private_(new Private())
  {}

  TOpenFolder::TOpenFolder(const std::string & folder_path):
    private_(new Private(folder_path))
  {}

  //----------------------------------------------------------------
  // TOpenFolder::~TOpenFolder
  //
  TOpenFolder::~TOpenFolder()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TOpenFolder::open
  //
  bool
  TOpenFolder::open(const std::string & folder_path)
  {
    return private_->open(folder_path);
  }

  //----------------------------------------------------------------
  // TOpenFolder::folder_path
  //
  std::string
  TOpenFolder::folder_path() const
  {
    return private_->folder_path();
  }

  //----------------------------------------------------------------
  // TOpenFolder::parse_next_item
  //
  bool
  TOpenFolder::parse_next_item()
  {
    return private_->parse_next_item();
  }

  //----------------------------------------------------------------
  // TOpenFolder::item_is_folder
  //
  bool
  TOpenFolder::item_is_folder() const
  {
    return private_->item_is_folder();
  }

  //----------------------------------------------------------------
  // TOpenFolder::item_name
  //
  std::string
  TOpenFolder::item_name() const
  {
    return private_->item_name();
  }

  //----------------------------------------------------------------
  // TOpenFolder::tem_path
  //
  std::string
  TOpenFolder::item_path() const
  {
    return private_->item_path();
  }


  //----------------------------------------------------------------
  // CollectMatchingFiles::CollectMatchingFiles
  //
  CollectMatchingFiles::CollectMatchingFiles(std::set<std::string> & dst,
                                             const std::string & regex):
    pattern_(regex, boost::regex::icase),
    files_(dst)
  {}

  //----------------------------------------------------------------
  // CollectMatchingFiles::operator
  //
  bool
  CollectMatchingFiles::operator()(bool is_folder,
                                   const std::string & name,
                                   const std::string & path)
  {
    if (!is_folder && boost::regex_match(name, pattern_))
    {
      files_.insert(path);
    }

    return true;
  }


  //----------------------------------------------------------------
  // read
  //
  std::size_t
  read(FILE * file, void * buffer, std::size_t buffer_size)
  {
    std::size_t done_size = 0;
    unsigned char * dst = (unsigned char *)buffer;

    while (file && done_size < buffer_size)
    {
      std::size_t z = buffer_size - done_size;
      std::size_t n = fread(dst, 1, z, file);
      done_size += n;
      dst += n;

      if (n < z)
      {
        break;
      }
    }

    return done_size;
  }

  //----------------------------------------------------------------
  // read
  //
  std::string
  read(FILE * file)
  {
    std::string str;
    char tmp[1024] = { 0 };

    while (file)
    {
      std::size_t n = fread(tmp, 1, sizeof(tmp), file);
      if (n > 0)
      {
        str += std::string(tmp, tmp + n);
      }

      if (n < sizeof(tmp))
      {
        break;
      }
    }

    return str;
  }

  //----------------------------------------------------------------
  // load
  //
  std::size_t
  load(FILE * file, std::vector<unsigned char> & out)
  {
    out = std::vector<unsigned char>();

    unsigned char tmp[1024] = { 0 };
    while (file)
    {
      std::size_t n = fread(tmp, 1, sizeof(tmp), file);
      if (n > 0)
      {
        out.insert(out.end(), tmp, tmp + n);
      }

      if (n < sizeof(tmp))
      {
        break;
      }
    }

    return out.size();
  }

  //----------------------------------------------------------------
  // write
  //
  bool
  write(FILE * file, const void * buffer, std::size_t buffer_size)
  {
    if (!file)
    {
      return false;
    }

    const unsigned char * src = (const unsigned char *)buffer;
    const unsigned char * end = src + buffer_size;

    while (src < end)
    {
      std::size_t bytes_written = ::fwrite(src, 1, end - src, file);
      if (!bytes_written)
      {
        return false;
      }

      src += bytes_written;
    }

    return true;
  }

  //----------------------------------------------------------------
  // write
  //
  bool
  write(FILE * file, const std::string & text)
  {
    return yae::write(file, text.c_str(), text.size());
  }

  //----------------------------------------------------------------
  // write
  //
  bool
  write(FILE * file, const char * text)
  {
    return yae::write(file, text, ::strlen(text));
  }


  //----------------------------------------------------------------
  // TOpenFile::TOpenFile
  //
  TOpenFile::TOpenFile():
    file_(NULL)
  {}

  //----------------------------------------------------------------
  // TOpenFile::TOpenFile
  //
  TOpenFile::TOpenFile(const char * filepath_utf8, const char * mode):
    file_(NULL)
  {
    this->open(filepath_utf8, mode);
  }

  //----------------------------------------------------------------
  // TOpenFile::TOpenFile
  //
  TOpenFile::TOpenFile(const std::string & filepath_utf8, const char * mode):
    file_(NULL)
  {
    this->open(filepath_utf8.c_str(), mode);
  }

  //----------------------------------------------------------------
  // TOpenFile::~TOpenFile
  //
  TOpenFile::~TOpenFile()
  {
    this->close();
  }

  //----------------------------------------------------------------
  // TOpenFile::open
  //
  bool
  TOpenFile::open(const char * filepath_utf8, const char * mode)
  {
    if (filepath_utf8 && *filepath_utf8)
    {
      this->close();
      this->file_ = fopen_utf8(filepath_utf8, mode);
      return this->file_ != NULL;
    }

    return false;
  }

  //----------------------------------------------------------------
  // TOpenFile::open
  //
  bool
  TOpenFile::open(const std::string & filepath_utf8, const char * mode)
  {
    return this->open(filepath_utf8.c_str(), mode);
  }

  //----------------------------------------------------------------
  // TOpenFile::close
  //
  void
  TOpenFile::close()
  {
    if (file_)
    {
      ::fclose(file_);
      file_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // uuid_generator_t
  //
  struct uuid_generator_t
  {
    boost::mutex mutex_;
    boost::uuids::random_generator rng_;

    inline std::string generate()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return boost::lexical_cast<std::string>(rng_());
    }
  };

  //----------------------------------------------------------------
  // generate_uuid
  //
  std::string
  generate_uuid()
  {
    static uuid_generator_t uuid;
    return uuid.generate();
  }

  //----------------------------------------------------------------
  // atomic_save
  //
  bool
  atomic_save(const std::string & path,
              const Json::Value & data)
  {
    std::string dir_name;
    std::string filename;
    if (!parse_file_path(path, dir_name, filename))
    {
      yae_elog("atomic_save: failed to parse the file path: %s", path.c_str());
      return false;
    }

    if (!yae::mkdir_p(dir_name))
    {
      yae_elog("atomic_save: failed to mkdir_p: %s", dir_name.c_str());
      return false;
    }

    std::string uuid = generate_uuid();
    std::string tmp_path = (fs::path(dir_name) / (".tmp." + uuid)).string();
    {
      yae::TOpenFile file;
      if (!file.open(tmp_path, "wb"))
      {
        yae_elog("atomic_save: failed to open temp file: %s",
                 tmp_path.c_str());
        return false;
      }

      if (!file.save(data))
      {
        yae_elog("atomic_save: failed to save temp file: %s",
                 tmp_path.c_str());
        YAE_ASSERT(remove_utf8(tmp_path) == 0);
        return false;
      }
    }

    int err = yae::atomic_rename_utf8(tmp_path, path);
    if (err != 0)
    {
      yae_elog("atomic_save: failed to rename %s to %s",
               tmp_path.c_str(),
               path.c_str());
      YAE_ASSERT(remove_utf8(tmp_path) == 0);
      return false;
    }

    return !err;
  }

  //----------------------------------------------------------------
  // attempt_load
  //
  bool
  attempt_load(const std::string & path,
               Json::Value & data,
               unsigned int max_attempts,
               unsigned int msec_sleep)
  {
    for (unsigned int i = 0; i < max_attempts; i++)
    {
      if (yae::TOpenFile(path, "rb").load(data))
      {
        return true;
      }

      if (i + 1 < max_attempts)
      {
        // read/write race condition with atomic_save, perhaps:
        boost::this_thread::sleep_for
          (boost::chrono::milliseconds((i + 1) * msec_sleep));
      }
    }

    return false;
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
      t = to_scalar<int64_t>(tokens[idd]);
    }

    if (ihh < numTokens)
    {
      t = t * 24 + to_scalar<int64_t>(tokens[ihh]);
    }

    if (imm < numTokens)
    {
      t = t * 60 + to_scalar<int64_t>(tokens[imm]);
    }

    if (iss < numTokens)
    {
      t = t * 60 + to_scalar<int64_t>(tokens[iss]);
    }

    double seconds = double(t);
    if (ixxx < numTokens)
    {
      double xxx = to_scalar<double>(tokens[ixxx]);
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

  //----------------------------------------------------------------
  // to_lower
  //
  std::string
  to_lower(const std::string & in)
  {
    std::string out(in);
    boost::algorithm::to_lower(out);
    return out;
  }

  //----------------------------------------------------------------
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & oss, const TDictionary & dict)
  {
    for (TDictionary::const_iterator i = dict.begin(); i != dict.end(); ++i)
    {
      const std::string & key = i->first;
      const std::string & value = i->second;
      oss << key << ": " << value << '\n';
    }

    return oss;
  }

  //----------------------------------------------------------------
  // extend
  //
  void
  extend(TDictionary & dst, const TDictionary & src)
  {
    for (TDictionary::const_iterator i = src.begin(); i != src.end(); ++i)
    {
      const std::string & key = i->first;
      const std::string & value = i->second;

      TDictionary::const_iterator found = dst.find(key);
      if (found != dst.end())
      {
        continue;
      }

      dst[key] = value;
    }
  }

  //----------------------------------------------------------------
  // to_hex
  //
  std::string
  to_hex(const void * data,
         std::size_t src_size,
         std::size_t word_size)
  {
    static const char * alphabet = "0123456789ABCDEF";

    std::ostringstream oss;
    const unsigned char * src = static_cast<const unsigned char *>(data);
    for (const unsigned char * i = src, * end = src + src_size; i < end; ++i)
    {
      if (word_size && (i > src) && ((i - src) % word_size == 0))
      {
        oss << ' ';
      }

      unsigned char hi = (*i) >> 4;
      unsigned char lo = (*i) & 0xF;
      oss << alphabet[hi] << alphabet[lo];
    }

    return std::string(oss.str().c_str());
  }

  //----------------------------------------------------------------
  // from_hex
  //
  void
  from_hex(unsigned char * dst, std::size_t dst_size, const char * hex_str)
  {
    const char * src = hex_str;
    for (std::size_t i = 0, n = std::min(dst_size, strlen(hex_str) / 2);
         i < n; i++, src += 2)
    {
      dst[i] = (unhex(src[0]) << 4) | unhex(src[1]);
    }
  }

  //----------------------------------------------------------------
  // utf8_to_unicode
  //
  bool
  utf8_to_unicode(const char *& src, const char * end, uint32_t & uc)
  {
    std::size_t sz = (end - src);
    if (sz > 0 && ((src[0] & 0x80) == 0x00))
    {
      uc = src[0];
      src++;
    }
    else if (sz > 1 && ((src[0] & 0xE0) == 0xC0 &&
                        (src[1] & 0xC0) == 0x80))
    {
      uc = (((src[0] & 0x1F) << 6) |
            ((src[1] & 0x3F)));
      src += 2;
    }
    else if (sz > 2 && ((src[0] & 0xF0) == 0xE0 &&
                        (src[1] & 0xC0) == 0x80 &&
                        (src[2] & 0xC0) == 0x80))
    {
      uc = (((src[0] & 0x0F) << 12) |
            ((src[1] & 0x3F) << 6) |
            ((src[2] & 0x3F)));
      src += 3;
    }
    else if (sz > 3 && ((src[0] & 0xF8) == 0xF0 &&
                        (src[1] & 0xC0) == 0x80 &&
                        (src[2] & 0xC0) == 0x80 &&
                        (src[3] & 0xC0) == 0x80))
    {
      uc = (((src[0] & 0x07) << 18) |
            ((src[1] & 0x3F) << 12) |
            ((src[2] & 0x3F) << 6) |
            ((src[3] & 0x3F)));
      src += 4;
    }
    else
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // utf16_to_unicode
  //
  bool
  utf16_to_unicode(const uint16_t *& src, const uint16_t * end, uint32_t & uc)
  {
    if ((0xD800 <= src[0] && src[0] <= 0xDBFF) && (end - src > 1) &&
        (0xDC00 <= src[1] && src[1] <= 0xDFFF))
    {
      // decode surrogate pair:
      uc = 0x10000;
      uc += (src[0] & 0x03FF) << 10;
      uc += (src[1] & 0x03FF);
      src += 2;
      return true;
    }

    if (src[0] < 0xD800 || 0xE000 <= src[0])
    {
      uc = src[0];
      src++;
      return true;
    }

    uc = 0;
    return false;
  }

  //----------------------------------------------------------------
  // utf16be_to_unicode
  //
  bool
  utf16be_to_unicode(const uint8_t *& src, const uint8_t * end, uint32_t & uc)
  {
    return utf16_to_unicode<0, 1>(src, end, uc);
  }

  //----------------------------------------------------------------
  // utf16le_to_unicode
  //
  bool
  utf16le_to_unicode(const uint8_t *& src, const uint8_t * end, uint32_t & uc)
  {
    return utf16_to_unicode<1, 0>(src, end, uc);
  }

  //----------------------------------------------------------------
  // vstrfmt
  //
  std::string
  vstrfmt(const char * format, va_list arglist)
  {
    // make a backup copy of arglist for the 2nd call to vsnprintf:
    va_list args;
#if defined(__GNUC__) || defined(__clang__) || defined(va_copy)
    va_copy(args, arglist);
#else
    args = arglist;
#endif

    // Determine how big the buffer should be
    int len = vsnprintf(NULL, 0, format, arglist) + 1;
    std::string ret(len, '\0');
    char * buffer = &ret[0];
    vsnprintf(buffer, len, format, args);

    va_end(args);
    return std::string(ret.c_str());
  }

  //----------------------------------------------------------------
  // strfmt
  //
  std::string
  strfmt(const char * format, ...)
  {
    va_list arglist;
    va_start(arglist, format);
    std::string ret = yae::vstrfmt(format, arglist);
    va_end(arglist);
    return ret;
  }

  //----------------------------------------------------------------
  // get_open_file
  //
  boost::shared_ptr<TOpenFile>
  get_open_file(const char * path, const char * mode)
  {
    std::string key(path);

    typedef boost::shared_ptr<TOpenFile> TFilePtr;
    static std::map<std::string, TFilePtr> files;
    static boost::mutex mutex;

    boost::lock_guard<boost::mutex> lock(mutex);
    boost::shared_ptr<TOpenFile> & file = files[key];

    if (!file)
    {
      file.reset(new TOpenFile(path, mode));
      if (!file->is_open())
      {
        throw std::runtime_error(yae::strfmt("failed to open file: %s", path));
      }
    }

    return file;
  }


  //----------------------------------------------------------------
  // ContextCallback::ContextCallback
  //
  ContextCallback::ContextCallback(ContextCallback::TFuncPtr func,
                                   void * context)
  {
    reset(func, context);
  }

  //----------------------------------------------------------------
  // ContextCallback::reset
  //
  void
  ContextCallback::reset(ContextCallback::TFuncPtr func,
                         void * context)
  {
    func_ = func;
    context_ = context;
  }

  //----------------------------------------------------------------
  // ContextCallback::is_null
  //
  bool
  ContextCallback::is_null() const
  {
    return !func_;
  }

  //----------------------------------------------------------------
  // ContextCallback::operator
  //
  void
  ContextCallback::operator()() const
  {
    if (func_)
    {
      func_(context_);
    }
  }

}

#if defined(_WIN32) && !defined(__MINGW32__)
extern "C"
{
  //----------------------------------------------------------------
  // strtoll
  //
  YAE_API long long int
  strtoll(const char * nptr, char ** endptr, int base)
  {
    YAE_ASSERT(sizeof(long long int) == sizeof(__int64));
    return _strtoi64(nptr, endptr, base);
  }
}

#endif

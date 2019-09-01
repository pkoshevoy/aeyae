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
#else
#include <dirent.h>
#include <dlfcn.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <math.h>

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
      argv[i] = utf16_to_utf8(wargv[i]);
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
    err = _wgetenv_s(&size, &(ref[0]), ret.size(), var.c_str());
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
    TOpenFolder folder;
    if (folder.open(path_utf8))
    {
      return true;
    }

    std::string dirname;
    std::string basename;
    if (!parse_file_path(path_utf8, dirname, basename))
    {
      return false;
    }

    if (!mkdir_p(dirname))
    {
      return false;
    }

    return mkdir_utf8(path_utf8.c_str());
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
  // fopen_utf8
  //
  std::FILE *
  fopen_utf8(const char * filename_utf8, const char * mode)
  {
    std::FILE * file = NULL;

#if defined(_WIN32) && !defined(__MINGW32__)
    wchar_t * wname = cstr_to_utf16(filename_utf8);
    wchar_t * wmode = cstr_to_utf16(mode);

    _wfopen_s(&file, wname, wmode);

    free(wname);
    free(wmode);
#else
    file = fopen(filename_utf8, mode);
#endif

    return file;
  }

  //----------------------------------------------------------------
  // open_utf8
  //
  int
  open_utf8(const char * filename_utf8, int access_mode, int permissions)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    accessMode |= O_BINARY;

    wchar_t * wname = cstr_to_utf16(filename_utf8);
    int fd = -1;
    int sh = accessMode & (_O_RDWR | _O_WRONLY) ? _SH_DENYWR : _SH_DENYNO;

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
  // data
  //
  static const char *
  data(const std::string & str)
  {
    return str.size() ? &str[0] : NULL;
  }

  //----------------------------------------------------------------
  // trim_from_tail
  //
  static std::string
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
  // trim_from_head
  //
  static std::string
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
      return !err;
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
  TOpenFile::TOpenFile(const char * filepath_utf8, const char * mode):
    file_(NULL)
  {
    this->open(filepath_utf8, mode);
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
  // strip_html_tags
  //
  std::string
  strip_html_tags(const std::string & in)
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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

/*
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : the_utils.cxx
// Author       : Pavel A. Koshevoy
// Created      : Sun Jan  4 16:08:35 MST 2009
// Copyright    : (C) 2009
// License      : MIT
// Description  : utility functions

// system includes:
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <uuid/uuid.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdexcept>
#include <string>

// local includes:
#include "the_utils.hxx"

// forward declarations:
#ifndef _WIN32
extern "C" int main(int argc, char ** argv);
#endif

//----------------------------------------------------------------
// THE_PATH_SEPARATOR
//
#ifdef _WIN32
const char * THE_PATH_SEPARATOR = "\\";
#else
const char * THE_PATH_SEPARATOR = "/";
#endif

//----------------------------------------------------------------
// THE_UNIX_PATH_SEPARATOR
//
static const char * THE_UNIX_PATH_SEPARATOR = "/";


//----------------------------------------------------------------
// sleep_msec
//
void
sleep_msec(size_t msec)
{
#ifdef _WIN32
  Sleep((DWORD)(msec));
#else
  usleep(msec * 1000);
#endif
}

//----------------------------------------------------------------
// restore_console_stdio
//
bool
restore_console_stdio()
{
#ifdef _WIN32
  AllocConsole();

#pragma warning(push)
#pragma warning(disable: 4996)

  freopen("conin$", "r", stdin);
  freopen("conout$", "w", stdout);
  freopen("conout$", "w", stderr);

#pragma warning(pop)

  HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (std_out_handle == INVALID_HANDLE_VALUE)
  {
    return false;
  }

  COORD console_buffer_size;
  console_buffer_size.X = 80;
  console_buffer_size.Y = 9999;
  SetConsoleScreenBufferSize(std_out_handle,
                             console_buffer_size);
#endif

  return true;
}


namespace the
{
#ifdef _WIN32
  //----------------------------------------------------------------
  // utf8_to_utf16
  //
  std::wstring
  utf8_to_utf16(const std::string & str_utf8)
  {
    int sizeUTF16 =
      MultiByteToWideChar(CP_UTF8,          // encoding (ansi, utf, etc...)
                          0,                // flags (precomposed, composite)
                          str_utf8.c_str(), // source multi-byte characters
                          -1,               // number of bytes in the source
                          NULL,             // wide-character destination
                          0);               // destination buffer size

    std::wstring str_utf16(sizeUTF16 - 1, 0);
    MultiByteToWideChar(CP_UTF8,
                        0,
                        str_utf8.c_str(),
                        -1,
                        &str_utf16[0],
                        sizeUTF16);
    return str_utf16;
  }

  //----------------------------------------------------------------
  // utf16_to_utf8
  //
  std::string
  utf16_to_utf8(const std::wstring & str_utf16)
  {
    int size_utf8 =
      WideCharToMultiByte(CP_UTF8,           // encoding (ansi, utf, etc...)
                          0,                 // flags (precomposed, composite)
                          str_utf16.c_str(), // source multi-byte characters
                          -1,                // number of bytes in the source
                          NULL,              // wide-character destination
                          0,                 // destination buffer size
                          NULL,              // unmappable char replacement
                          NULL);             // flag to set if replacement used

    std::string str_utf8(size_utf8 - 1, 0);
    WideCharToMultiByte(CP_UTF8,
                        0,
                        str_utf16.c_str(),
                        -1,
                        &str_utf8[0],
                        size_utf8,
                        0,
                        0);
    return str_utf8;
  }
#endif

  //----------------------------------------------------------------
  // open_utf8
  //
  int
  open_utf8(const char * filename_utf8, int oflag, int pmode)
  {
    int fd = -1;

#ifdef _WIN32
    // on windows utf-8 has to be converted to utf-16
    std::wstring filename_utf16 = utf8_to_utf16(std::string(filename_utf8));

    int sflag = _SH_DENYNO;
    _wsopen_s(&fd, filename_utf16.c_str(), oflag, sflag, pmode);

#else
    // assume utf-8 is supported natively:
    fd = open(filename_utf8, oflag, pmode);
#endif

    return fd;
  }

  //----------------------------------------------------------------
  // open_utf8
  //
  void
  open_utf8(std::fstream & fstream_to_open,
            const char * filename_utf8,
            std::ios_base::openmode mode)
  {
#ifdef _WIN32
    // on windows utf-8 has to be converted to utf-16
    std::wstring filename_utf16 = utf8_to_utf16(std::string(filename_utf8));

    fstream_to_open.open(filename_utf16.c_str(), mode);

#else
    // assume utf-8 is supported natively:
    fstream_to_open.open(filename_utf8, mode);
#endif
  }

  //----------------------------------------------------------------
  // fopen_utf8
  //
  FILE *
  fopen_utf8(const char * filename_utf8, const char * mode)
  {
    FILE * file = NULL;

#ifdef _WIN32
    std::wstring filename_utf16 = utf8_to_utf16(std::string(filename_utf8));
    std::wstring mode_utf16 = utf8_to_utf16(std::string(mode));

    _wfopen_s(&file, filename_utf16.c_str(), mode_utf16.c_str());

#else
    file = fopen(filename_utf8, mode);
#endif

    return file;
  }

  //----------------------------------------------------------------
  // fseek64
  //
  int
  fseek64(FILE * file, off_t offset, int whence)
  {
#ifdef _WIN32
    int ret = _fseeki64(file, offset, whence);
#else
    int ret = fseeko(file, offset, whence);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // ftell64
  //
  off_t
  ftell64(const FILE * file)
  {
#ifdef _WIN32
    off_t pos = _ftelli64(const_cast<FILE *>(file));
#else
    off_t pos = ftello(const_cast<FILE *>(file));
#endif

    return pos;
  }

  //----------------------------------------------------------------
  // rename_utf8
  //
  int
  rename_utf8(const char * old_utf8, const char * new_utf8)
  {
#ifdef _WIN32
    std::wstring old_utf16 = utf8_to_utf16(std::string(old_utf8));
    std::wstring new_utf16 = utf8_to_utf16(std::string(new_utf8));

    int ret = _wrename(old_utf16.c_str(), new_utf16.c_str());

#else

    int ret = rename(old_utf8, new_utf8);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // remove_utf8
  //
  int
  remove_utf8(const char * filename_utf8)
  {
#ifdef _WIN32
    std::wstring filename_utf16 = utf8_to_utf16(std::string(filename_utf8));
    int ret = _wremove(filename_utf16.c_str());
#else

    int ret = remove(filename_utf8);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // rmdir_utf8
  //
  int
  rmdir_utf8(const char * dir_utf8)
  {
#ifdef _WIN32
    std::wstring dir_utf16 = utf8_to_utf16(std::string(dir_utf8));
    int ret = _wrmdir(dir_utf16.c_str());
#else

    int ret = remove(dir_utf8);
#endif

    return ret;
  }

  //----------------------------------------------------------------
  // mkdir_utf8
  //
  int
  mkdir_utf8(const char * path_utf8)
  {
#ifdef _WIN32
    std::wstring path_utf16 = utf8_to_utf16(std::string(path_utf8));
    int ret = _wmkdir(path_utf16.c_str());
#else

    int ret = mkdir(path_utf8, S_IRWXU);
#endif

    return ret;
  }

#ifdef _WIN32
  //----------------------------------------------------------------
  // rmdir_recursively_utf16
  //
  static bool
  rmdir_recursively_utf16(const wchar_t * dir_utf16)
  {
    if (!dir_utf16)
    {
      assert(false);
      return false;
    }

    if (!*dir_utf16)
    {
      assert(false);
      return true;
    }

    wchar_t searchPath[_MAX_PATH] = { 0 };
    _wmakepath_s(searchPath, _MAX_PATH, NULL, dir_utf16, L"*", L"");

    WIN32_FIND_DATAW file_data;
    while (true)
    {
      HANDLE handle = FindFirstFileW(searchPath, &file_data);
      if (handle == INVALID_HANDLE_VALUE)
      {
        return false;
      }

      wchar_t nextPath[_MAX_PATH] = { 0 };
      bool found = false;
      bool isDir = false;
      do
      {
        isDir = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (isDir)
        {
          if (wcscmp(file_data.cFileName, L".") == 0 ||
              wcscmp(file_data.cFileName, L"..") == 0)
          {
            continue;
          }
        }

        _wmakepath_s(nextPath,
                     _MAX_PATH,
                     NULL,
                     dir_utf16,
                     file_data.cFileName,
                     NULL);
        found = true;
      }
      while (!found && FindNextFileW(handle, &file_data));

      // close it, will reopen on next pass:
      FindClose(handle);

      if (!found)
      {
        break;
      }

      if (isDir)
      {
        if (!rmdir_recursively_utf16(nextPath))
        {
          return false;
        }
      }
      else
      {
        DWORD attrs = GetFileAttributesW(nextPath);

        attrs = FILE_ATTRIBUTE_NORMAL;
        SetFileAttributesW(nextPath, attrs);

        if (!DeleteFileW(nextPath))
        {
          return false;
        }
      }
    }

    if (_wrmdir(dir_utf16) != 0)
    {
      return false;
    }

    return true;
  }
#endif

  //----------------------------------------------------------------
  // rmdir_recursively_utf8
  //
  bool
  rmdir_recursively_utf8(const std::string & dir_to_remove)
  {
    if (dir_to_remove.empty())
    {
      assert(false);
      return true;
    }

#ifdef _WIN32
    // on windows utf-8 has to be converted to utf-16
    std::wstring dir_utf16 = utf8_to_utf16(dir_to_remove);
    return rmdir_recursively_utf16(dir_utf16.c_str());
#else

    // Some systems don't define the d_name element sufficiently long.
    // In this case the user has to provide additional space.
    // There must be room for at least NAME_MAX + 1 characters
    // in the d_name array
    union
    {
      struct dirent d;
      char b[offsetof(struct dirent, d_name) + NAME_MAX + 1];
    } u;

    memset(&u, 0, sizeof(u));

    bool ok = true;
    while (true)
    {
      DIR * dir = opendir(dir_to_remove.c_str());
      if (!dir)
      {
        return false;
      }

      struct dirent * found = NULL;
      while (true)
      {
        int err = readdir_r(dir, &u.d, &found);
        ok = (err == 0);

        if (!ok || !found)
        {
          break;
        }

        if (u.d.d_type == DT_DIR)
        {
          if (strcmp(u.d.d_name, ".") == 0 ||
              strcmp(u.d.d_name, "..") == 0)
          {
            continue;
          }
        }

        break;
      }

      // close it, will reopen on next pass:
      closedir(dir);

      if (!ok || !found)
      {
        break;
      }

      std::string nextPath(dir_to_remove);
      if (nextPath[nextPath.size() - 1] != '/')
      {
        nextPath += '/';
      }
      nextPath += std::string(u.d.d_name);

      if (u.d.d_type == DT_DIR)
      {
        if (!rmdir_recursively_utf8(nextPath))
        {
          return false;
        }
      }
      else
      {
        if (remove(nextPath.c_str()) != 0)
        {
          return false;
        }
      }
    }

    if (!ok || remove(dir_to_remove.c_str()) != 0)
    {
      return false;
    }

    return true;
#endif
  }

  //----------------------------------------------------------------
  // rmdir_recursively_utf8
  //
  bool
  rmdir_recursively_utf8(const char * dir_utf8)
  {
    if (!dir_utf8)
    {
      assert(false);
      return false;
    }

    if (!*dir_utf8)
    {
      assert(false);
      return true;
    }

    std::string dir_to_remove(dir_utf8);
    return rmdir_recursively_utf8(dir_to_remove);
  }

  //----------------------------------------------------------------
  // find_matching_files
  //
  // assemble a list of filenames in a given folder that match
  // a specified filename prefix and extension suffix
  //
  // NOTE: all strings are UTF-8 encoded
  //
  bool
  find_matching_files(std::list<std::string> & found,
                      const char * folder,
                      const char * prefix,
                      const char * suffix)
  {
    open_folder_t open_folder(folder);
    do
    {
      if (open_folder.item_is_folder())
      {
        continue;
      }

      std::string name = open_folder.item_name();
      if (string_match_head(name.c_str(), prefix) &&
          string_match_tail(name.c_str(), suffix))
      {
        found.push_back(name);
      }

    } while (open_folder.parse_next_item());

    return !found.empty();
  }

  //----------------------------------------------------------------
  // remove_files
  //
  // remove files found with find_matching_files(..)
  //
  // NOTE: all strings are UTF-8 encoded
  //
  bool
  remove_files(const char * folder,
               const std::list<std::string> & filenames)
  {
    std::string path(folder);
    if (!path.empty() && path[path.size() - 1] != *THE_PATH_SEPARATOR)
    {
      path += THE_PATH_SEPARATOR;
    }

    for (std::list<std::string>::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
    {
      const std::string & fn = *i;
      std::string full_path(path);
      full_path += fn;

      if (remove_utf8(full_path.c_str()) != 0)
      {
        return false;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // get_current_executable_path
  //
  bool
  get_current_executable_path(std::string & exe_path_utf8)
  {
    bool ok = false;

#ifdef _WIN32
    wchar_t path[_MAX_PATH] = { 0 };
    unsigned long pathLen = sizeof(path) / sizeof(wchar_t);
    if (GetModuleFileNameW(0, path, pathLen) != 0)
    {
      exe_path_utf8 = utf16_to_utf8(std::wstring(path));
      ok = true;
    }

#elif defined(__APPLE__)
    CFBundleRef bundle_ref = CFBundleGetMainBundle();
    if (bundle_ref)
    {
      CFURLRef url_ref =
        CFBundleCopyExecutableURL(bundle_ref);

      if (url_ref)
      {
        CFStringRef string_ref =
          CFURLCopyFileSystemPath(url_ref, kCFURLPOSIXPathStyle);

        char txt[1024] = { 0 };
        if (string_ref)
        {
          if (CFStringGetCString(string_ref,
                                 txt,
                                 sizeof(txt),
                                 kCFStringEncodingUTF8))
          {
            exe_path_utf8.assign(txt);
            ok = true;
          }

          CFRelease(string_ref);
        }

        CFRelease(url_ref);
      }
    }
#else
    char path[PATH_MAX + 1] = { 0 };
    if (readlink("/proc/self/exe", path, sizeof(path)) > 0)
    {
      exe_path_utf8.assign(path);
      ok = true;
    }
    /*
    else
    {
      // The function dladdr() takes a function pointer
      // and tries to resolve name and file where it is located.
      Dl_info info;

      // if (dladdr((const void *)&get_current_executable_path, &info))
      if (dladdr((const void *)&main, &info))
      {
        exe_path_utf8.assign(info.dli_fname);
        ok = true;
      }
    }
    */

    /*
    // Linux exe:

    // Linux lib:
    Dl_info info;
    dladdr(&symbol,&info);
    String fullpath = info.dli_filename;

    // BSD exe:
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;
    char buf[1024];
    size_t cb = sizeof(buf);
    sysctl(mib, 4, buf, &cb, NULL, 0);

    // solaris:
    getexename();
    */

#endif

    assert(!exe_path_utf8.empty());
    return ok;
  }

  //----------------------------------------------------------------
  // get_latest_err_str
  //
  std::string
  get_latest_err_str()
  {
    std::string err_str;

#ifdef _WIN32
    DWORD err = GetLastError();
    if (!err)
    {
      return std::string();
    }

    LPVOID msg = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,

                   NULL,
                   err,

                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&msg,
                   0,
                   NULL);

    err_str = utf16_to_utf8(std::wstring((LPWSTR)msg));
    LocalFree(msg);

#else
    err_str.assign(strerror(errno));
#endif

    return err_str;
  }

  //----------------------------------------------------------------
  // dump_latest_err
  //
  static void
  dump_latest_err()
  {
    std::cerr << "ERROR: " << get_latest_err_str().c_str() << std::endl;
  }

  //----------------------------------------------------------------
  // launch_app
  //
  bool
  launch_app(const std::string & exe_path_utf8,
             const std::list<std::string> & args_utf8,
             const std::string & work_dir_utf8,
             bool wait_to_finish)
  {
#ifdef _WIN32
    std::wstring exe_path_utf16 = utf8_to_utf16(exe_path_utf8);
    std::wstring work_dir_utf16 = utf8_to_utf16(work_dir_utf8);

    std::wstring params_utf16;
    for (std::list<std::string>::const_iterator i = args_utf8.begin();
         i != args_utf8.end(); ++i)
    {
      const std::string & arg = *i;
      std::wstring arg_utf16 = utf8_to_utf16(arg);
      params_utf16 += L"\"";
      params_utf16 += arg_utf16;
      params_utf16 += L"\" ";
    }

    SHELLEXECUTEINFOW shell_exe_info;
    memset(&shell_exe_info, 0, sizeof(SHELLEXECUTEINFOW));
    shell_exe_info.cbSize = sizeof(SHELLEXECUTEINFOW);
    shell_exe_info.fMask = SEE_MASK_NOCLOSEPROCESS;
    shell_exe_info.hwnd = NULL;
    shell_exe_info.lpVerb = NULL;
    shell_exe_info.lpFile = exe_path_utf16.c_str();
    shell_exe_info.lpParameters = params_utf16.c_str();
    shell_exe_info.lpDirectory = (work_dir_utf16.empty() ?
                                  NULL :
                                  work_dir_utf16.c_str());
    shell_exe_info.nShow = SW_HIDE;

    if (!ShellExecuteExW(&shell_exe_info))
    {
      dump_latest_err();
      return false;
    }

    if (wait_to_finish)
    {
      int r = WaitForSingleObject(shell_exe_info.hProcess, INFINITE);
      if (r != WAIT_OBJECT_0)
      {
        dump_latest_err();
        return false;
      }

      DWORD exit_code = -1;
      if (!GetExitCodeProcess(shell_exe_info.hProcess, &exit_code))
      {
        dump_latest_err();
        return false;
      }

      if (exit_code != 0)
      {
        return false;
      }
    }

#else
    std::vector<char *> args;
    args.push_back(const_cast<char *>(&exe_path_utf8[0]));

    for (std::list<std::string>::const_iterator i = args_utf8.begin();
         i != args_utf8.end(); ++i)
    {
      const std::string & arg = *i;
      args.push_back(const_cast<char *>(&arg[0]));
    }
    args.push_back(NULL);

    int pid = fork();
    if (pid == -1)
    {
      dump_latest_err();
      return false;
    }

    if (pid == 0)
    {
      if (!work_dir_utf8.empty())
      {
        int err = chdir(work_dir_utf8.c_str());
        if (err)
        {
          // terminate the fork:
          ::exit(errno);
        }
      }

      // exec(...) functions do not return, unless there is an error:
      char * const * argv = &args[0];
      int err = execvp(exe_path_utf8.c_str(), argv);
      if (err)
      {
        // terminate the fork:
        ::exit(errno);
      }
    }

    if (wait_to_finish)
    {
      int status = 0;
      waitpid(pid, &status, 0);

      if (!WIFEXITED(status))
      {
        dump_latest_err();
        return false;
      }

      int exit_code = WEXITSTATUS(status);
      if (exit_code != 0)
      {
        return false;
      }
    }
#endif

    return true;
  }

  //----------------------------------------------------------------
  // is_absolute_path
  //
  bool
  is_absolute_path(const std::string & path_utf8)
  {
    if (path_utf8.empty())
    {
      return false;
    }

#ifdef _WIN32
    // check for hostname or drive letter followed by ":\"
    std::size_t found = path_utf8.find(":\\");
    if (found != std::string::npos)
    {
      return true;
    }
#endif

    return (path_utf8[0] == *THE_PATH_SEPARATOR);
  }

  //----------------------------------------------------------------
  // simplify_path
  //
  bool
  simplify_path(const std::string & path_utf8,
                std::string & full_path_utf8)
  {
    if (path_utf8.empty())
    {
      return false;
    }

    std::string curdir;
    std::list<const char *> path;

    if (!is_absolute_path(path_utf8))
    {
      // get current working directory:
#ifdef _WIN32
      wchar_t wcurdir[4096] = { 0 };
      if (_wgetcwd(wcurdir, sizeof(wcurdir) / sizeof(wcurdir[0])) == NULL)
      {
        dump_latest_err();
        return false;
      }

      curdir = utf16_to_utf8(std::wstring(wcurdir));
#else
      char buffer[MAXPATHLEN] = { 0 };
      if (getcwd(buffer, sizeof(buffer)) == NULL)
      {
        dump_latest_err();
        return false;
      }

      curdir.assign(buffer);
#endif

      // split current working directory path into tokens:
      if (curdir[0] != *THE_PATH_SEPARATOR)
      {
        path.push_back(&curdir[0]);
      }

      for (char * i = &curdir[0]; *i; ++i)
      {
        char & c = *i;
        if (c == *THE_PATH_SEPARATOR)
        {
          c = 0;
          if (*(i + 1))
          {
            path.push_back(++i);
          }
        }
      }
    }

    // split given path into tokens:
    std::string relative(path_utf8);
    char * siter = &relative[0];

    if (*siter != *THE_PATH_SEPARATOR)
    {
      path.push_back(siter);
      ++siter;
    }

    for (; *siter; ++siter)
    {
      char & c = *siter;
      if (c == *THE_PATH_SEPARATOR)
      {
        c = 0;
        if (*(siter + 1))
        {
          path.push_back(++siter);
        }
      }
    }

    // remove redundant path elements:
    std::list<const char *>::iterator i = path.begin();
    while (i != path.end())
    {
      const char * a = *i;
      if (strcmp(a, ".") == 0)
      {
        // drop .
        i = path.erase(i);
      }
      else if (strcmp(a, "..") == 0)
      {
        if (i == path.begin())
        {
          return false;
        }

        // drop a/..
        --i;
        i = path.erase(i);
        i = path.erase(i);
      }
      else
      {
        ++i;
      }
    }

    if (path.empty())
    {
      full_path_utf8 = THE_PATH_SEPARATOR;
    }
    else
    {
      full_path_utf8.clear();

      std::list<const char *>::const_iterator i = path.begin();

#ifdef _WIN32
      {
        const char * a = *i;
        std::size_t na = string_length<char>(a);
        if (na > 0 && a[na - 1] == ':')
        {
          full_path_utf8 += a;
          ++i;
        }
      }
#endif

      for (; i != path.end(); ++i)
      {
        full_path_utf8 += THE_PATH_SEPARATOR;

        const char * a = *i;
        full_path_utf8 += a;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // parse_file_path
  //
  bool
  parse_file_path(const std::string & file_path,
                  std::string & folder,
                  std::string & name)
  {
    std::size_t	index_name = file_path.rfind(*THE_PATH_SEPARATOR);

    std::size_t	index_name_unix =
      (*THE_PATH_SEPARATOR != *THE_UNIX_PATH_SEPARATOR) ?
      file_path.rfind(*THE_UNIX_PATH_SEPARATOR) :
      index_name;

    if (index_name_unix != std::string::npos &&
        (index_name == std::string::npos ||
         index_name < index_name_unix))
    {
      // Unix directory separator used before file name
      index_name = index_name_unix;
    }

    if (index_name != std::string::npos)
    {
      folder = file_path.substr(0, index_name);
      name = file_path.substr(index_name + 1, file_path.size());
      return true;
    }

    folder = std::string();
    name = file_path;
    return false;
  }

#ifdef _WIN32
  //----------------------------------------------------------------
  // open_folder_t::private_t
  //
  class open_folder_t::private_t
  {
  public:
    private_t(const std::string & path):
      dir_(INVALID_HANDLE_VALUE)
    {
      // clean up the folder path:
      simplify_path(path, folder_path_);

      std::wstring wpath =
        utf8_to_utf16(std::string(folder_path_.c_str()));

      wchar_t searchPath[_MAX_PATH] = { 0 };
      _wmakepath_s(searchPath, _MAX_PATH, NULL, wpath.c_str(), L"*", L"");

      dir_ = FindFirstFileW(searchPath, &dirent_);
      if (dir_ == INVALID_HANDLE_VALUE)
      {
        std::string err =
          std::string("FindFirstFileW failed for \"") +
          folder_path_ + std::string("\"");
        throw std::runtime_error(err);
      }
    }

    ~private_t()
    {
      if (dir_ != INVALID_HANDLE_VALUE)
      {
        FindClose(dir_);
        dir_ = INVALID_HANDLE_VALUE;
      }
    }

    const std::string & folder_path() const
    {
      return folder_path_;
    }

    bool parse_next_item()
    {
      return
        (dir_ != INVALID_HANDLE_VALUE) ?
        FindNextFileW(dir_, &dirent_) != FALSE :
        false;
    }

    bool item_is_folder() const
    {
      return
        (dir_ != INVALID_HANDLE_VALUE) &&
        ((dirent_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    std::string item_name() const
    {
      std::string name = utf16_to_utf8(std::wstring(dirent_.cFileName));
      return name.c_str();
    }

    std::string item_path() const
    {
      std::string name = item_name();
      std::string path = folder_path_ + THE_PATH_SEPARATOR + name;
      return path;
    }

  protected:
    HANDLE dir_;
    WIN32_FIND_DATAW dirent_;
    std::string folder_path_;
  };

#else

  //----------------------------------------------------------------
  // open_folder_t::private_t
  //
  class open_folder_t::private_t
  {
  public:
    private_t(const std::string & path):
      dir_(NULL)
    {
      // clean up the folder path:
      simplify_path(path, folder_path_);

      memset(&dirent_, 0, sizeof(dirent_));

      dir_ = opendir(path.c_str());
      if (!dir_)
      {
        std::string err =
          std::string("opendir failed for \"") +
          path + std::string("\"");
        throw std::runtime_error(err);
      }

      struct dirent * found = NULL;
      int err = readdir_r(dir_, &dirent_.d, &found);
      if (err)
      {
        std::string err =
          std::string("readdir_r failed for \"") +
          path + std::string("\"");
        throw std::runtime_error(err);
      }
    }

    ~private_t()
    {
      if (dir_ != NULL)
      {
        closedir(dir_);
        dir_ = NULL;
      }
    }

    const std::string & folder_path() const
    {
      return folder_path_;
    }

    bool parse_next_item()
    {
      if (dir_ == NULL)
      {
        return false;
      }

      struct dirent * found = NULL;
      int error = readdir_r(dir_, &dirent_.d, &found);

      bool ok = (error == 0) && (found != NULL);
      return ok;
    }

    bool item_is_folder() const
    {
      return
        (dir_ != NULL) &&
        (dirent_.d.d_type == DT_DIR);
    }

    std::string item_name() const
    {
      std::string name(dirent_.d.d_name);
      return name;
    }

    std::string item_path() const
    {
      std::string name = item_name();
      std::string path = folder_path_ + THE_PATH_SEPARATOR + name;
      return path;
    }

  protected:
    //----------------------------------------------------------------
    // dirent_t
    //
    typedef union
    {
      struct dirent d;
      char b[offsetof(struct dirent, d_name) + NAME_MAX + 1];
    } dirent_t;

    std::string folder_path_;
    dirent_t dirent_;
    DIR * dir_;
  };
#endif

  //----------------------------------------------------------------
  // open_folder_t::open_folder_t
  //
  // NOTE: the constructor may throw exceptions if the folder
  //       doesn't exist, or is inaccessible.
  // NOTE: the constructor parses the first item in the folder,
  //       it may throw an exception if it can parse the item,
  open_folder_t::open_folder_t(const std::string & folder_path):
    private_(new open_folder_t::private_t(folder_path))
  {}

  //----------------------------------------------------------------
  // open_folder_t::~open_folder_t
  //
  open_folder_t::~open_folder_t()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // open_folder_t::folder_path
  //
  // accessor to the cleaned up folder path:
  //
  const std::string &
  open_folder_t::folder_path() const
  {
    return private_->folder_path();
  }

  //----------------------------------------------------------------
  // open_folder_t::parse_next_item
  //
  // parse the next item in the folder:
  //
  bool
  open_folder_t::parse_next_item()
  {
    return private_->parse_next_item();
  }

  //----------------------------------------------------------------
  // open_folder_t::item_is_folder
  //
  bool
  open_folder_t::item_is_folder() const
  {
    return private_->item_is_folder();
  }

  //----------------------------------------------------------------
  // open_folder_t::item_name
  //
  std::string
  open_folder_t::item_name() const
  {
    return private_->item_name();
  }

  //----------------------------------------------------------------
  // open_folder_t::item_path
  //
  std::string
  open_folder_t::item_path() const
  {
    return private_->item_path();
  }


  //----------------------------------------------------------------
  // open_file_t::open_file_t
  //
  // NOTE: the constructor may throw exceptions if the file
  //       doesn't exist, or is inaccessible.
  //
  open_file_t::open_file_t(const char * filename, const char * mode):
    file_(NULL)
  {
    file_ = fopen_utf8(filename, mode);
    if (!file_)
    {
      std::string err =
        std::string("failed to open \"") + filename +
        std::string("\", using \"") + mode +
        std::string("\" mode");
      throw std::runtime_error(err);
    }
  }

  //----------------------------------------------------------------
  // open_file_t::~open_file_t
  //
  // NOTE: the destructor will close the file:
  //
  open_file_t::~open_file_t()
  {
    if (file_)
    {
      fclose(file_);
      file_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // is_path_to_folder
  //
  bool
  is_path_to_folder(const std::string & path_utf8)
  {
    try
    {
      open_folder_t open_folder(path_utf8);
      return true;
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // path_exists
  //
  bool
  path_exists(const std::string & path_utf8)
  {
    if (is_path_to_folder(path_utf8))
    {
      return true;
    }

    try
    {
      open_file_t open_file(path_utf8.c_str(), "rb");
      return true;
    }
    catch (...)
    {}

    return false;
  }


  //----------------------------------------------------------------
  // count_files
  //
  // recursively count files stored at the specified path,
  // may throw std::exception
  //
  unsigned int
  count_files(const std::string & path_utf8)
  {
    unsigned int num_files = 0;

    if (is_path_to_folder(path_utf8))
    {
      open_folder_t open_folder(path_utf8);
      do
      {
        std::string item_name = open_folder.item_name();
        std::string item_path = open_folder.item_path();

        if (open_folder.item_is_folder())
        {
          if (strcmp(item_name.c_str(), ".") == 0 ||
              strcmp(item_name.c_str(), "..") == 0)
          {
            continue;
          }

          num_files += count_files(item_path);
        }
        else
        {
          open_file_t(item_path.c_str(), "rb");
          num_files++;
        }
      } while (open_folder.parse_next_item());
    }
    else
    {
      open_file_t(path_utf8.c_str(), "rb");
      num_files++;
    }

    return num_files;
  }


  //----------------------------------------------------------------
  // progress_reporter_base_t::progress_reporter_base_t
  //
  progress_reporter_base_t::progress_reporter_base_t(unsigned int items_total,
                                                     double precision)
  {
    reset(items_total);
    set_precision(precision);
  }

  //----------------------------------------------------------------
  // progress_reporter_base_t::~progress_reporter_base_t
  //
  progress_reporter_base_t::~progress_reporter_base_t()
  {}

  //----------------------------------------------------------------
  // progress_reporter_base_t::set_precision
  //
  // specify the smallest change in total progress that should be reported:
  //
  void
  progress_reporter_base_t::set_precision(double precision)
  {
    assert(precision > 0.0 && precision <= 1.0);
    precision_ = std::max<double>(1e-10, precision);
  }

  //----------------------------------------------------------------
  // progress_reporter_base_t::reset
  //
  // specify how many items are included in the total progress calculation:
  //
  void
  progress_reporter_base_t::reset(unsigned int items_total)
  {
    items_total_ = items_total;
    items_completed_ = 0;
    item_progress_ = 0.0;
    reported_progress_ = -1;
  }

  //----------------------------------------------------------------
  // progress_reporter_base_t::update_progress
  //
  // update progress for 1 item
  //
  bool
  progress_reporter_base_t::update_progress(double item_progress)
  {
    item_progress_ = item_progress;

    // report progress in increments specified by precision:
    double items_completed =
      std::min<double>(double(items_completed_) + item_progress_,
                       double(items_total_));

    double total_progress = items_completed / double(items_total_);

    int reportable_progress = int(total_progress / precision_);
    if (reportable_progress > reported_progress_)
    {
      reported_progress_ = reportable_progress;
      return report_progress(total_progress);
    }

    return true;
  }

  //----------------------------------------------------------------
  // progress_reporter_base_t::item_finished
  //
  // indicate that 1 item has completed
  //
  void
  progress_reporter_base_t::item_finished()
  {
    items_completed_++;
    update_progress(0.0);
  }


  //----------------------------------------------------------------
  // copy_file
  //
  // copy a file from a given source file
  // to the specified destination file,
  // may throw std::exception
  //
  // NOTE: all strings are UTF-8 encoded
  //
  bool
  copy_file(const std::string & src,
            const std::string & dst,
            progress_reporter_base_t * progress_reporter)
  {
    open_file_t open_src(src.c_str(), "rb");
    open_file_t open_dst(dst.c_str(), "wb");

    // seek to the end of the file so we can get its size:
    int error = fseek64(open_src.file_, 0, SEEK_END);
    if (error != 0)
    {
      std::string err =
        std::string("failed to seek \"") + src + std::string("\"");
      throw std::runtime_error(err);
    }

    // store file size:
    const off_t src_size = ftell64(open_src.file_);

    // seek back to the beginning of the file so we can read it:
    error = fseek64(open_src.file_, 0, SEEK_SET);
    if (error != 0)
    {
      std::string err =
        std::string("failed to seek \"") + src + std::string("\"");
      throw std::runtime_error(err);
    }

    std::size_t buffer_size = 8192;
    std::vector<unsigned char> buffer(buffer_size);

    off_t dst_size = 0;
    while (dst_size < src_size)
    {
      std::size_t nr = fread(&buffer[0], 1, buffer_size, open_src.file_);
      if (!nr)
      {
        break;
      }

      std::size_t nw = fwrite(&buffer[0], 1, nr, open_dst.file_);
      if (nw != nr)
      {
        std::string err =
          std::string("failed to write to \"") + dst + std::string("\"");
        throw std::runtime_error(err);
      }

      dst_size += nw;

      if (progress_reporter)
      {
        double file_progress = double(dst_size) / double(src_size);
        bool carry_on = progress_reporter->update_progress(file_progress);
        if (!carry_on)
        {
          break;
        }
      }
    }

    bool done = (src_size == dst_size);

    if (done && progress_reporter)
    {
      progress_reporter->item_finished();
    }

    return done;
  }

  //----------------------------------------------------------------
  // copy_folder_contents
  //
  // recursively copy folder contents of a given source folder
  // to the specified destination folder,
  // may throw std::exception
  //
  // NOTE: all strings are UTF-8 encoded
  //
  bool
  copy_folder_contents(const std::string & src,
                       const std::string & dst,
                       progress_reporter_base_t * progress_reporter)
  {
    // this may throw exceptions if either src/dst folder doesn't exist:
    open_folder_t src_folder(src);
    open_folder_t dst_folder(dst);

    bool ok = true;
    do
    {
      std::string src_name = src_folder.item_name();
      std::string src_path = src_folder.item_path();

      if (src_folder.item_is_folder())
      {
        if (strcmp(src_name.c_str(), ".") == 0 ||
            strcmp(src_name.c_str(), "..") == 0)
        {
          continue;
        }
      }

      std::string dst_path = dst_folder.folder_path();
      dst_path = dst_path + THE_PATH_SEPARATOR + src_name;

      if (src_folder.item_is_folder())
      {
        mkdir_utf8(dst_path.c_str());
        ok = copy_folder_contents(src_path, dst_path, progress_reporter);
      }
      else
      {
        ok = copy_file(src_path, dst_path, progress_reporter);
      }
    } while (ok && src_folder.parse_next_item());

    return ok;
  }

  //----------------------------------------------------------------
  // copy_files
  //
  // carry out the recursive file/folder copy operation;
  // returns true/false to indicate success/failure,
  // may throw a std::exception
  //
  // NOTE: all strings are UTF-8 encoded
  //
  bool
  copy_files(const std::string & from,
             const std::string & to,
             progress_reporter_base_t * progress_reporter)
  {
    std::string src;
    simplify_path(from, src);

    if (src.empty())
    {
      std::string err("must specify the source file/folder to copy from");
      throw std::runtime_error(err);
    }

    std::string dst;
    simplify_path(to, dst);

    if (dst.empty())
    {
      std::string err("must specify the destination file/folder to copy to");
      throw std::runtime_error(err);
    }

    if (!path_exists(src))
    {
      std::string err = src + " doesn't exist";
      throw std::runtime_error(err);
    }

    if (is_path_to_folder(src))
    {
      // copy a folder:

      if (path_exists(dst))
      {
        // copy A/B/C/* to X/Y/Z/C/*, Z folder exists:

        // 1. extract C folder name
        std::string folder;
        std::string name;
        parse_file_path(src, folder, name);

        // 2. create X/Y/Z/C/ folder
        std::string dst_path = dst + std::string(THE_PATH_SEPARATOR) + name;
        mkdir_utf8(dst_path.c_str());

        // 3. copy A/B/C/* to X/Y/Z/C/*
        return copy_folder_contents(src, dst, progress_reporter);
      }
      else
      {
        // copy A/B/C/* to X/Y/Z/*, Z folder doesn't exist:

        // 1. create X/Y/Z/ folder
        mkdir_utf8(dst.c_str());

        // 2. copy A/B/C/* to X/Y/Z/*
        return copy_folder_contents(src, dst, progress_reporter);
      }
    }
    else
    {
      // copy a file:

      if (is_path_to_folder(dst))
      {
        // copy A/B/C to X/Y/Z/C, Z is a folder:
        std::string folder;
        std::string name;
        parse_file_path(src, folder, name);

        std::string dst_path = dst + std::string(THE_PATH_SEPARATOR) + name;
        return copy_file(src, dst_path, progress_reporter);
      }
      else
      {
        // copy A/B/C to X/Y/Z, Z is not a folder:
        return copy_file(src, dst, progress_reporter);
      }
    }

    return true;
  }
}

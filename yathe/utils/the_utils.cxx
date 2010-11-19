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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <string.h>

// local includes:
#include "the_utils.hxx"

// forward declarations:
#ifndef _WIN32
extern "C" int main(int argc, char ** argv);
#endif

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
      
      CFRelease(bundle_ref);
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
}

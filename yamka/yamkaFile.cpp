// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 11:45:25 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// windows includes:
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#endif

// yamka includes:
#include <yamkaFile.h>
#include <yamkaSharedPtr.h>

// turn on 64-bit file offsets:
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

// system includes:
#include <cstdio>
#include <limits>


#ifdef _WIN32

#define off_t __int64
#define fseeko _fseeki64
#define ftello _ftelli64
#define ftruncate _chsize_s
#define fileno _fileno

//----------------------------------------------------------------
// __wgetmainargs
// 
extern "C" void __wgetmainargs(int * argc,
                               wchar_t *** argv,
                               wchar_t *** env,
                               int doWildCard,
                               int * startInfo);

#endif // _WIN32

namespace Yamka
{

#ifdef _WIN32
  //----------------------------------------------------------------
  // utf8_to_utf16
  // 
  wchar_t *
  utf8_to_utf16(const char * str_utf8)
  {
    int nchars = MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, NULL, 0);
    wchar_t * str_utf16 = (wchar_t *)malloc(nchars * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, str_utf16, nchars);
    return str_utf16;
  }
  
  //----------------------------------------------------------------
  // utf16_to_utf8
  // 
  char *
  utf16_to_utf8(const wchar_t * str_utf16)
  {
    int nchars = WideCharToMultiByte(CP_UTF8, 0, str_utf16, -1, NULL, 0, 0, 0);
    char * str_utf8 = (char *)malloc(nchars * sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, str_utf16, -1, str_utf8, nchars, 0, 0);
    return str_utf8;
  }

  //----------------------------------------------------------------
  // get_main_args_utf8
  // 
  void
  get_main_args_utf8(int & argc, char **& argv)
  {
    argc = 0;
    wchar_t ** wenpv = NULL;
    wchar_t ** wargv = NULL;
    int startupInfo = 0;

    __wgetmainargs(&argc, &wargv, &wenpv, 1, &startupInfo);
    
    argv = (char **)malloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++)
    {
        argv[i] = utf16_to_utf8(wargv[i]);
    }
  }
#endif
  
  //----------------------------------------------------------------
  // fopen_utf8
  // 
  std::FILE *
  fopen_utf8(const char * filename_utf8, const char * mode)
  {
    std::FILE * file = NULL;
    
#ifdef _WIN32
    wchar_t * wname = utf8_to_utf16(filename_utf8);
    wchar_t * wmode = utf8_to_utf16(mode);
    
    _wfopen_s(&file, wname, wmode);
    
    free(wname);
    free(wmode);
#else
    file = fopen(filename_utf8, mode);
#endif
    
    return file;
  }
  
  
  //----------------------------------------------------------------
  // SharedFile
  // 
  class SharedFile
  {
    // intentionally disabled:
    SharedFile();
    SharedFile(const SharedFile &);
    SharedFile & operator = (const SharedFile &);
    
  public:
    SharedFile(const std::string & path, File::AccessMode fileMode):
      file_(NULL)
    {
      this->open(path, fileMode);
    }
    
    ~SharedFile()
    {
      this->close();
    }

    void close()
    {
      if (file_)
      {
        fclose(file_);
        file_ = NULL;
      }
    }

    bool open(const std::string & path, File::AccessMode fileMode)
    {
      close();
      
      const char * mode =
        (fileMode == File::kReadWrite) ?
        "rb+" :
        "rb";

      path_ = path;
      file_ = fopen_utf8(path_.c_str(), mode);
      if (!file_ && fileMode == File::kReadWrite)
      {
        // try creating the file:
        file_ = fopen_utf8(path_.c_str(), "wb+");
      }

      bool ok = (file_ != NULL);
      return ok;
    }
    
    // UTF-8 file path:
    std::string path_;
    
    // file handle:
    std::FILE * file_;
  };
  
  //----------------------------------------------------------------
  // TSharedFilePtr
  // 
  typedef TSharedPtr<SharedFile> TSharedFilePtr;
  
  //----------------------------------------------------------------
  // File::Private
  // 
  class File::Private
  {
  public:
    Private(const std::string & path, File::AccessMode fileMode):
      shared_(new SharedFile(path, fileMode))
    {}
    
    TSharedFilePtr shared_;
  };
  
  
  //----------------------------------------------------------------
  // File::Seek::Seek
  // 
  File::Seek::Seek(File & file):
    file_(file),
    prev_(file.absolutePosition()),
    restoreOnExit_(true)
  {}
  
  //----------------------------------------------------------------
  // File::Seek::Seek
  // 
  File::Seek::Seek(File & file, TOff offset, PositionReference relativeTo):
    file_(file),
    prev_(file.absolutePosition()),
    restoreOnExit_(true)
  {
    seek(offset, relativeTo);
  }
  
  //----------------------------------------------------------------
  // File::Seek::~Seek
  // 
  File::Seek::~Seek()
  {
    if (restoreOnExit_)
    {
      seek(prev_, kAbsolutePosition);
    }
  }
  
  //----------------------------------------------------------------
  // File::Seek::doNotRestore
  // 
  void
  File::Seek::doNotRestore()
  {
    restoreOnExit_ = false;
  }
  
  //----------------------------------------------------------------
  // File::Seek::doRestore
  // 
  void
  File::Seek::doRestore()
  {
    restoreOnExit_ = true;
  }
  
  //----------------------------------------------------------------
  // File::Seek::absolutePosition
  // 
  File::TOff
  File::Seek::absolutePosition() const
  {
    return prev_;
  }
  
  //----------------------------------------------------------------
  // File::Seek::seek
  // 
  void
  File::Seek::seek(TOff offset, PositionReference relativeTo)
  {
    if (!file_.seek(offset, relativeTo))
    {
      std::runtime_error e(std::string("failed to seek"));
      throw e;
    }
  }
  
    
  //----------------------------------------------------------------
  // File::~File
  // 
  File::~File()
  {
    delete private_;
  }
  
  //----------------------------------------------------------------
  // File::File
  // 
  File::File(const std::string & path, File::AccessMode fileMode):
    private_(new File::Private(path, fileMode))
  {}

  //----------------------------------------------------------------
  // File::File
  // 
  File::File(const File & f):
    private_(new File::Private(*(f.private_)))
  {}

  //----------------------------------------------------------------
  // File::operator =
  // 
  File &
  File::operator = (const File & f)
  {
    if (&f != this)
    {
      *private_ = *(f.private_);
    }
    
    return *this;
  }

  //----------------------------------------------------------------
  // File::isOpen
  // 
  bool
  File::isOpen() const
  {
    return private_->shared_->file_ != NULL;
  }
  
  //----------------------------------------------------------------
  // File::close
  // 
  void
  File::close()
  {
    private_->shared_->close();
  }

  //----------------------------------------------------------------
  // File::open
  // 
  bool
  File::open(const std::string & pathUTF8, AccessMode fileMode)
  {
    return private_->shared_->open(pathUTF8, fileMode);
  }
  
  //----------------------------------------------------------------
  // File::seek
  // 
  bool
  File::seek(File::TOff offset, File::PositionReference relativeTo)
  {
    int err = fseeko(private_->shared_->file_, offset, relativeTo);
    return !err;
  }
  
  //----------------------------------------------------------------
  // File::absolutePosition
  // 
  File::TOff
  File::absolutePosition() const
  {
    File::TOff off = ftello(private_->shared_->file_);
    return off;
  }

  //----------------------------------------------------------------
  // File::size
  // 
  File::TOff
  File::size()
  {
    try
    {
      Seek tmp(*this, 0, kOffsetFromEnd);
      return absolutePosition();
    }
    catch (...)
    {}
    
    return std::numeric_limits<File::TOff>::max();
  }
  
  //----------------------------------------------------------------
  // File::setSize
  // 
  bool
  File::setSize(File::TOff size)
  {
    TOff pos = absolutePosition();
    int fd = fileno(private_->shared_->file_);
    int error = ftruncate(fd, size);
    
    if (error)
    {
      return false;
    }
    
    if (pos > size)
    {
      seek(size);
    }
    
    return true;
  }
    
  
  //----------------------------------------------------------------
  // File::write
  // 
  bool
  File::write(const void * source, std::size_t numBytes)
  {
    size_t bytesOut = fwrite(source, 1, numBytes, private_->shared_->file_);
    return bytesOut == numBytes;
  }
  
  //----------------------------------------------------------------
  // File::read
  // 
  bool
  File::read(void * destination, std::size_t numBytes)
  {
    size_t bytesRead = fread(destination,
                             1,
                             numBytes,
                             private_->shared_->file_);
    return bytesRead == numBytes;
  }
  
  //----------------------------------------------------------------
  // File::filename
  // 
  const std::string &
  File::filename() const
  {
    return private_->shared_->path_;
  }
  
  //----------------------------------------------------------------
  // File::save
  // 
  bool
  File::save(const Bytes & data)
  {
    const TByteVecDec & deq = *(data.bytes_);
    for (TByteVecDec::const_iterator i = deq.begin(); i != deq.end(); ++i)
    {
      const TByteVec & vec = *i;
      if (!vec.empty() && !write(&vec[0], vec.size()))
      {
        return false;
      }
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // File::load
  // 
  bool
  File::load(Bytes & data)
  {
    TByteVecDec & deq = *(data.bytes_);
    for (TByteVecDec::iterator i = deq.begin(); i != deq.end(); ++i)
    {
      TByteVec & vec = *i;
      if (!vec.empty() && !read(&vec[0], vec.size()))
      {
        return false;
      }
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // File::calcCrc32
  // 
  bool
  File::calcCrc32(File::TOff seekToPosition,
                  File::TOff totalBytesToRead,
                  Crc32 & computeCrc32)
  {
    if (!totalBytesToRead)
    {
      return true;
    }
    
    if (!seek(seekToPosition, File::kAbsolutePosition))
    {
      return false;
    }

    std::size_t bytesPerPass =
      (std::size_t)(std::min<File::TOff>(4096, totalBytesToRead));
    
    std::vector<unsigned char> data(bytesPerPass);
    unsigned char * dataPtr = &data[0];
    File::TOff bytesToRead = totalBytesToRead;
    
    while (bytesToRead)
    {
      std::size_t bytesToReadNow =
        (File::TOff(bytesPerPass) < bytesToRead) ?
        bytesPerPass :
        (std::size_t)bytesToRead;
      
      if (!read(dataPtr, bytesToReadNow))
      {
        return false;
      }
      
      computeCrc32.compute(dataPtr, bytesToReadNow);
      bytesToRead -= bytesToReadNow;
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // File::remove
  // 
  bool
  File::remove(const char * filename_utf8)
  {
#ifdef _WIN32
    wchar_t * wname = utf8_to_utf16(filename_utf8);
    int err = _wremove(wname);
    free(wname);
#else
    
    int err = ::remove(filename_utf8);
#endif
    
    return err == 0;
  }
  
}

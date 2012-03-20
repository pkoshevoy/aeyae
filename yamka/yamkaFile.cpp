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
#include <yamkaCache.h>
#include <yamkaFile.h>
#include <yamkaSharedPtr.h>
#include <yamkaStdInt.h>

// turn on 64-bit file offsets:
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

// system includes:
#include <algorithm>
#include <limits>
#include <vector>
#include <cstdio>

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
  class SharedFile : ICacheDataProvider
  {
    friend class TCache;
    
    // intentionally disabled:
    SharedFile();
    SharedFile(const SharedFile &);
    SharedFile & operator = (const SharedFile &);
    
  public:

    SharedFile(const std::string & path, File::AccessMode fileMode):
      mode_(fileMode),
      file_(NULL),
      pos_(0),
      end_(0),
      cache_(NULL)
    {
      this->open(path, fileMode);
    }
    
    ~SharedFile()
    {
      this->close();
    }

    //----------------------------------------------------------------
    // close
    // 
    void close()
    {
      if (cache_)
      {
#if 0
        std::cerr << path_ << " - close" << std::endl;
#endif
        
        delete cache_;
        cache_ = NULL;
      }
      
      if (file_)
      {
        fclose(file_);
        file_ = NULL;
      }
      
      pos_ = 0;
      end_ = 0;
    }
    
    //----------------------------------------------------------------
    // open
    // 
    bool open(const std::string & path, File::AccessMode fileMode)
    {
      close();
      
      const char * mode =
        (fileMode == File::kReadWrite) ?
        "rb+" :
        "rb";
      
      path_ = path;
      mode_ = fileMode;
      file_ = fopen_utf8(path_.c_str(), mode);
      
      if (!file_ && mode_ == File::kReadWrite)
      {
        // try creating the file:
        mode = "wb+";
        file_ = fopen_utf8(path_.c_str(), mode);
      }
      
      bool ok = (file_ != NULL);
      if (ok)
      {
        pos_ = ftello(file_);
        end_ = this->size();
        
        assert(!cache_);
        cache_ = new TCache(this, 1, 4096);

#if 0
        std::cerr << path_ << " - open " << mode
                  << ", size: " << size()
                  << ", pos: " << pos_
                  << ", end: " << end_
                  << std::endl;
#endif
      }
      
      return ok;
    }
    
    //----------------------------------------------------------------
    // cache
    // 
    inline TCache * cache() const
    { return cache_; }
    
    //----------------------------------------------------------------
    // isOpen
    // 
    inline bool isOpen() const
    { return file_ != NULL; }
    
    //----------------------------------------------------------------
    // getPath
    // 
    inline const std::string & getPath() const
    { return path_; }
    
    //----------------------------------------------------------------
    // seek
    // 
    bool seek(File::TOff offset, File::PositionReference relativeTo)
    {
      File::TOff pos = offset;
      
      if (relativeTo == File::kRelativeToCurrent)
      {
        relativeTo = File::kAbsolutePosition;
        pos = pos_ + offset;
      }
      
      assert(relativeTo != File::kRelativeToCurrent);
      
#if 0 // !defined(NDEBUG) && (defined(_DEBUG) || defined(DEBUG))
      File::TOff dst =
        relativeTo == File::kOffsetFromEnd ?
        size() + offset :
        pos;
#endif

#if 0
      int err = fseeko(file_, pos, relativeTo);
      if (err)
      {
        assert(false);
        return false;
      }
      
      pos_ = ftello(file_);
#else      
      pos_ = (relativeTo == File::kOffsetFromEnd) ? end_ : pos;
#endif
      
#if 0 // !defined(NDEBUG) && (defined(_DEBUG) || defined(DEBUG))
      assert(pos_ == dst);
#endif
      
      end_ = std::max<uint64>(end_, pos_);
      return true;
    }

    //----------------------------------------------------------------
    // absolutePosition
    // 
    File::TOff absolutePosition() const
    {
      return pos_;
    }
    
    //----------------------------------------------------------------
    // size
    // 
    File::TOff size()
    {
      if (cache_)
      {
        return end_;
      }
      
      File::TOff prev = ftello(file_);
      int err = fseeko(file_, 0, SEEK_END);
      
      if (!err)
      {
        File::TOff end = ftello(file_);
        err = fseeko(file_, prev, SEEK_SET);
        
        if (!err)
        {
          return end;
        }
      }
      
      assert(false);
      return std::numeric_limits<File::TOff>::max();
    }

    //----------------------------------------------------------------
    // setSize
    // 
    bool setSize(File::TOff size)
    {
      int fd = fileno(file_);
      int error = ftruncate(fd, size);
      
      if (error)
      {
        assert(false);
        return false;
      }
      
      if (pos_ > size)
      {
        pos_ = size;
      }
      
      cache_->truncate(size);
      end_ = size;
      return true;
    }
    
    //----------------------------------------------------------------
    // write
    // 
    bool write(const unsigned char * src, std::size_t numBytes)
    {
      size_t bytesOut = cache_->save(pos_, numBytes, src);
      pos_ += bytesOut;
      end_ = std::max<uint64>(end_, pos_);
      
      return bytesOut == numBytes;
    }
    
    //----------------------------------------------------------------
    // read
    // 
    bool read(unsigned char * dst, std::size_t numBytes)
    {
      size_t bytesRead = cache_->load(pos_, numBytes, dst);
      pos_ += bytesRead;
      assert(pos_ <= end_);
      
      return bytesRead == numBytes;
    }
    
    //----------------------------------------------------------------
    // peek
    //
    inline std::size_t
    peek(unsigned char * dst, std::size_t numBytes)
    {
      return cache_->load(pos_, numBytes, dst);
    }
    
  protected:
    
    // virtual:
    bool load(uint64 addr, std::size_t * size, unsigned char * dst)
    {
#if 0
      std::cerr << path_ << " - load " << addr
                << ", " << *size << std::endl;
#endif
      
      int err = fseeko(file_, addr, SEEK_SET);
      if (err)
      {
        assert(false);
        return false;
      }
      
      size_t bytesRead = fread(dst, 1, *size, file_);
      *size = bytesRead;
      return true;
    }
    
    // virtual:
    bool save(uint64 addr, std::size_t size, const unsigned char * src)
    {
#if 0
      std::cerr << path_ << " - save " << addr
                << ", " << size << std::endl;
#endif
      
      int err = fseeko(file_, addr, SEEK_SET);
      if (err)
      {
        assert(false);
        return false;
      }
      
      size_t bytesOut = fwrite(src, 1, size, file_);
      if (bytesOut != size)
      {
        assert(false);
        return false;
      }
      
      return true;
    }
    
    // UTF-8 file path:
    std::string path_;
    
    // file mode:
    File::AccessMode mode_;
    
    // file handle:
    std::FILE * file_;
    
    // file position:
    File::TOff pos_;
    
    // file end position:
    File::TOff end_;
    
    // I/O cache:
    TCache * cache_;    
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
  // File::cache
  // 
  TCache *
  File::cache() const
  {
    return private_->shared_->cache();
  }
  
  //----------------------------------------------------------------
  // File::isOpen
  // 
  bool
  File::isOpen() const
  {
    return private_->shared_->isOpen();
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
    return private_->shared_->seek(offset, relativeTo);
  }
  
  //----------------------------------------------------------------
  // File::absolutePosition
  // 
  File::TOff
  File::absolutePosition() const
  {
    return private_->shared_->absolutePosition();
  }

  //----------------------------------------------------------------
  // File::size
  // 
  File::TOff
  File::size()
  {
    return private_->shared_->size();
  }
  
  //----------------------------------------------------------------
  // File::setSize
  // 
  bool
  File::setSize(File::TOff size)
  {
    return private_->shared_->setSize(size);
  }
  
  //----------------------------------------------------------------
  // File::filename
  // 
  const std::string &
  File::filename() const
  {
    return private_->shared_->getPath();
  }
  
  //----------------------------------------------------------------
  // File::save
  // 
  bool
  File::save(const void * data, std::size_t size)
  {
    return private_->shared_->write((const unsigned char *)data, size);
  }
  
  //----------------------------------------------------------------
  // File::load
  // 
  bool
  File::load(void * data, std::size_t size)
  {
    return private_->shared_->read((unsigned char *)data, size);
  }
  
  //----------------------------------------------------------------
  // File::peek
  // 
  std::size_t
  File::peek(void * data, std::size_t size)
  {
    return private_->shared_->peek((unsigned char *)data, size);
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
      
      if (!this->load(dataPtr, bytesToReadNow))
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

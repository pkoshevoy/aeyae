// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 11:43:09 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_FILE_H_
#define YAMKA_FILE_H_

// yamka includes:
#include <yamkaBytes.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <string>
#include <stdexcept>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // File
  // 
  class File
  {
  public:
    virtual ~File();
    
    //----------------------------------------------------------------
    // AccessMode
    // 
    enum AccessMode
    {
      READ_ONLY,
      READ_WRITE
    };
    
    File(const std::string & pathUTF8 = std::string(),
         AccessMode fileMode = READ_ONLY);
    
    // shallow copy (file handle is shared):
    File(const File & f);
    File & operator = (const File & f);
    
    // check whether the file handle is open:
    virtual bool isOpen() const;
    
    //----------------------------------------------------------------
    // TOff
    // 
    typedef boost::int64_t TOff;
    
    //----------------------------------------------------------------
    // PositionReference
    // 
    enum PositionReference
    {
      ABSOLUTE = SEEK_SET,
      RELATIVE = SEEK_CUR,
      FROM_END = SEEK_END
    };

    // seek to a specified file position:
    virtual bool seek(TOff offset,
                      PositionReference relativeTo = ABSOLUTE);
    
    // return current absolute file position:
    virtual TOff absolutePosition() const;
    
    //----------------------------------------------------------------
    // Seek
    // 
    // A helper class used to seek (temporarily) to a given offset:
    // 
    struct Seek
    {
      // save current seek position,
      // then seek to a given offset,
      // throw exception if seek fails:
      Seek(File & file,
           TOff offset,
           PositionReference relativeTo = ABSOLUTE):
        file_(file),
        prev_(file.absolutePosition())
      {
        if (!file_.seek(offset, relativeTo))
        {
          std::runtime_error e(std::string("failed to seek"));
          throw e;
        }
      }
      
      // restore saved seek position:
      ~Seek()
      {
        if (!file_.seek(prev_, ABSOLUTE))
        {
          std::runtime_error e(std::string("failed to seek back"));
          throw e;
        }
      }
      
    private:
      Seek(const Seek &);
      Seek & operator = (const Seek &);
      
      File & file_;
      TOff prev_;
    };
    
    // helper to get current file size:
    virtual TOff size();
    
    // write out at current file position a specified number of bytes
    // from the source buffer:
    virtual bool write(const void * source, std::size_t numBytes);
    
    // read at current file position a specified number of bytes
    // into the destination buffer:
    virtual bool read(void * destination, std::size_t numBytes);
    
    // accessor to the filename (UTF-8):
    virtual const std::string & filename() const;
    
    // helpers for storing non-contiguous byte sequences:
    virtual bool save(const Bytes & data);
    virtual bool load(Bytes & data);
    
  private:
    // private implementation details:
    class Private;
    Private * private_;
  };
}


#endif // YAMKA_FILE_H_

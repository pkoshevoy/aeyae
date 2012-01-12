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
#include <yamkaCrc32.h>

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
      kReadOnly,
      kReadWrite
    };
    
    File(const std::string & pathUTF8 = std::string(),
         AccessMode fileMode = kReadOnly);
    
    // shallow copy (file handle is shared):
    File(const File & f);
    File & operator = (const File & f);
    
    // check whether the file handle is open:
    virtual bool isOpen() const;

    // close current file handle.
    // NOTE: this applies to all copies of this File object
    //       due to implicit sharing
    virtual void close();

    // close current file handle and open another file.
    // NOTE: this applies to all copies of this File object
    //       due to implicit sharing
    virtual bool open(const std::string & pathUTF8,
                      AccessMode fileMode = kReadOnly);
    
    //----------------------------------------------------------------
    // TOff
    // 
    typedef boost::int64_t TOff;
    
    //----------------------------------------------------------------
    // PositionReference
    // 
    enum PositionReference
    {
      kAbsolutePosition = SEEK_SET,
      kRelativeToCurrent = SEEK_CUR,
      kOffsetFromEnd = SEEK_END
    };

    // seek to a specified file position:
    virtual bool seek(TOff offset,
                      PositionReference relativeTo = kAbsolutePosition);
    
    // return current absolute file position:
    virtual TOff absolutePosition() const;
    
    //----------------------------------------------------------------
    // Seek
    // 
    // A helper class used to seek (temporarily) to a given offset:
    // 
    struct Seek
    {
      // save current seek position:
      Seek(File & file);
      
      // save current seek position,
      // then seek to a given offset,
      // throw exception if seek fails:
      Seek(File & file, TOff offset,
           PositionReference relativeTo = kAbsolutePosition);
      
      // if required, then restore saved seek position:
      ~Seek();
      
      // call this to disable restoring the previos file position:
      void doNotRestore();
      
      // call this to restore the previous file position:
      void doRestore();
      
      // accessor to the absolute position of the file
      // at the moment a Seek instance was created:
      TOff absolutePosition() const;
      
    private:
      Seek(const Seek &);
      Seek & operator = (const Seek &);

      // helper
      void seek(TOff offset, PositionReference relativeTo);
      
      File & file_;
      TOff prev_;
      bool restoreOnExit_;
    };
    
    // helper to get current file size:
    virtual TOff size();
    
    // truncate or extend file to a given size:
    virtual bool setSize(TOff size);
    
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
    
    // calculate CRC-32 checksum over a region of this file:
    virtual bool calcCrc32(TOff seekToPosition,
                           TOff numBytesToRead,
                           Crc32 & computeCrc32);
    
    // remove a given file:
    static bool remove(const char * filenameUTF8);
    
  private:
    // private implementation details:
    class Private;
    Private * const private_;
  };
}


#endif // YAMKA_FILE_H_

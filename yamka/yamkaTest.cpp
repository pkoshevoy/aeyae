// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 08:59:02 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaStdInt.h>
#include <yamkaFileStorage.h>
#include <yamkaEBML.h>
#include <yamkaMatroska.h>

// system includes:
#include <iostream>

// namespace access:
using namespace Yamka;


//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  std::cout << "0x" << uintEncode(0x1A45DFA3) << std::endl
            << "0x" << uintEncode(0xEC) << std::endl
            << "0x" << intEncode(5) << std::endl
            << "0x" << intEncode(-2) << std::endl
            << "0x" << intEncode(5, 3) << std::endl
            << "0x" << intEncode(-2, 3) << std::endl
            << "0x" << vsizeEncode(0x8000) << std::endl
            << "0x" << vsizeEncode(1) << std::endl
            << "0x" << vsizeEncode(0) << std::endl
            << "0x" << floatEncode(-11.1f)
            << " = " << floatDecode(floatEncode(-11.1f)) << std::endl
            << "0x" << doubleEncode(-11.1)
            << " = " << doubleDecode(doubleEncode(-11.1)) << std::endl
            << "0x" << intEncode(VDate().data_, 8) << std::endl
            << std::endl;
  
  FileStorage fs(std::string("testYamka.bin"), File::kReadWrite);
  if (!fs.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << fs.file_.filename()
              << " to read/write"
              << std::endl;
    ::exit(1);
  }
  else
  {
    std::cout << "opened (rw) " << fs.file_.filename()
              << ", current file size: " << fs.file_.size()
              << std::endl;
    
    Bytes bytes;
    bytes << uintEncode(0x1A45DFA3)
          << uintEncode(0x4286)
          << vsizeEncode(1)
          << uintEncode(1)
          << uintEncode(0x42f7)
          << vsizeEncode(1)
          << uintEncode(1);
    
    Crc32 crc32;
    FileStorage::IReceiptPtr receipt = fs.saveAndCalcCrc32(bytes, &crc32);
    if (receipt)
    {
      std::cout << "stored " << bytes.size()
                << " bytes, checksum: "
                << std::hex
                << std::uppercase
                << crc32.checksum()
                << std::dec
                << std::endl;
    }
  }
  
  FileStorage fs2(std::string("testYamka.ebml"), File::kReadWrite);
  if (!fs2.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << fs2.file_.filename()
              << " to read/write"
              << std::endl;
    ::exit(1);
  }
  else
  {
    std::cout << "opened (rw) " << fs2.file_.filename()
              << ", current file size: " << fs2.file_.size()
              << std::endl;
    
    Crc32 crc32;
    EbmlDoc doc;
    doc.head_.payload_.docType_.payload_.set(std::string("yamka"));
    doc.head_.payload_.docTypeVersion_.payload_.set(1);
    doc.head_.payload_.docTypeReadVersion_.payload_.set(1);
    FileStorage::IReceiptPtr receipt = doc.save(fs2, &crc32);
    
    if (receipt)
    {
      std::cout << "stored " << doc.calcSize()
                << " bytes, checksum: "
                << std::hex
                << std::uppercase
                << crc32.checksum()
                << std::dec
                << std::endl;
    }
  }
  
  FileStorage mkvSrc(std::string("testYamka.mkv"), File::kReadOnly);
  if (!mkvSrc.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << mkvSrc.file_.filename()
              << " to read"
              << std::endl;
    ::exit(1);
  }
  else
  {
    uint64 mkvSrcSize = mkvSrc.file_.size();
    std::cout << "opened (ro) " << mkvSrc.file_.filename()
              << ", file size: " << mkvSrcSize
              << std::endl;
    
    Crc32 crc32;
    MatroskaDoc doc;
    uint64 bytesRead = doc.load(mkvSrc, mkvSrcSize, &crc32);
    
    std::cout << "read " << bytesRead << " bytes, "
              << "doc size is " << doc.calcSize() << " bytes, "
              << "checksum: "
              << std::hex
              << std::uppercase
              << crc32.checksum()
              << std::dec
              << std::endl;
  }
  
  return 0;
}

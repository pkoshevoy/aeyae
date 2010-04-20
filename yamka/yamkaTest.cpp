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
// testSimpleBlockSerialization
// 
static void
testSimpleBlockSerialization(const Bytes & blockData)
{
  SimpleBlock sb;
  assert(sb.importData(blockData));
  
  Bytes exportedData;
  sb.exportData(exportedData);
  
  Crc32 srcCrc;
  srcCrc.compute(blockData);
  unsigned int srcChecksum = srcCrc.checksum();
  
  Crc32 outCrc;
  outCrc.compute(exportedData);
  unsigned int outChecksum = outCrc.checksum();
  
  assert(srcChecksum == outChecksum);
}


//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  uint64 vsizeSize = 0;
  std::cout << "0x" << uintEncode(0x1A45DFA3) << std::endl
            << "0x" << uintEncode(0xEC) << std::endl
            << "0x" << intEncode(5) << std::endl
            << "0x" << intEncode(-2) << std::endl
            << "0x" << intEncode(5, 3) << std::endl
            << "0x" << intEncode(-2, 3) << std::endl
            << "0x1456abcf = 0x"
            << std::hex
            << vsizeDecode(vsizeEncode(0x1456abcf), vsizeSize)
            << std::dec << std::endl
            << "-64 = "
            << vsizeSignedDecode(vsizeSignedEncode(-64), vsizeSize)
            << std::endl
            << "0x" << vsizeEncode(0x8000) << std::endl
            << "0x" << vsizeEncode(1) << std::endl
            << "0x" << vsizeEncode(0) << std::endl
            << "0x"
            << vsizeEncode(123, 4)
            << " = 0x"
            << vsizeEncode(vsizeDecode(vsizeEncode(123, 4), vsizeSize))
            << std::endl
            << "0x" << floatEncode(-11.1f)
            << " = " << floatDecode(floatEncode(-11.1f)) << std::endl
            << "0x" << doubleEncode(-11.1)
            << " = " << doubleDecode(doubleEncode(-11.1)) << std::endl
            << "0x" << intEncode(VDate().get(), 8) << std::endl
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
    
    // dump all simple blocks in the first cluster:
    typedef std::deque<Cluster::TSimpleBlock>::const_iterator TSimpleBlockIter;
    for (TSimpleBlockIter i = doc.
           segments_.front().payload_.
           clusters_.front().payload_.
           simpleBlocks_.begin();
         i != doc.
           segments_.front().payload_.
           clusters_.front().payload_.
           simpleBlocks_.end();
         ++i)
    {
      const Cluster::TSimpleBlock & sbElt = *i;
      
      Bytes importData;
      if (sbElt.payload_.get(importData))
      {
        SimpleBlock sb;
        if (sb.importData(importData))
        {
          std::cout << sb << std::endl;

          // test packing:
          testSimpleBlockSerialization(importData);
        }
      }
    }
    
    // dump all block groups in the first cluster:
    typedef std::deque<Cluster::TBlockGroup>::const_iterator TBlockGroupIter;
    for (TBlockGroupIter i = doc.
           segments_.front().payload_.
           clusters_.front().payload_.
           blockGroups_.begin();
         i != doc.
           segments_.front().payload_.
           clusters_.front().payload_.
           blockGroups_.end();
         ++i)
    {
      const Cluster::TBlockGroup & bgElt = *i;
      
      Bytes blockData;
      if (bgElt.payload_.block_.payload_.get(blockData))
      {
        SimpleBlock sb;
        if (sb.importData(blockData))
        {
          std::cout << sb << std::endl;
          
          // test packing:
          testSimpleBlockSerialization(blockData);
        }
      }
    }
    
    FileStorage mkvSrcOut(std::string("testYamkaOut.mkv"), File::kReadWrite);
    if (!mkvSrcOut.file_.isOpen())
    {
      std::cerr << "ERROR: failed to open " << mkvSrcOut.file_.filename()
                << " to read/write"
                << std::endl;
      ::exit(1);
    }
    else
    {
      uint64 mkvSrcOutSize = mkvSrcOut.file_.size();
      std::cout << "opened (rw) " << mkvSrcOut.file_.filename()
              << ", file size: " << mkvSrcOutSize
                << std::endl;
      
      Crc32 crc32out;
      FileStorage::IReceiptPtr receipt = doc.save(mkvSrcOut, &crc32out);
      
      if (receipt)
      {
        std::cout << "stored " << doc.calcSize()
                  << " bytes, checksum: "
                  << std::hex
                  << std::uppercase
                  << crc32out.checksum()
                  << std::dec
                  << std::endl;
      }
    }
  }
  
  FileStorage mkvSrcOut(std::string("testYamkaOut.mkv"), File::kReadOnly);
  if (!mkvSrcOut.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << mkvSrcOut.file_.filename()
              << " to read"
              << std::endl;
    ::exit(1);
  }
  else
  {
    uint64 mkvSrcOutSize = mkvSrcOut.file_.size();
    std::cout << "opened (ro) " << mkvSrcOut.file_.filename()
              << ", file size: " << mkvSrcOutSize
              << std::endl;
    
    Crc32 crc32;
    MatroskaDoc doc;
    uint64 bytesRead = doc.load(mkvSrcOut, mkvSrcOutSize, &crc32);
    
    std::cout << "read " << bytesRead << " bytes, "
              << "doc size is " << doc.calcSize() << " bytes, "
              << "checksum: "
              << std::hex
              << std::uppercase
              << crc32.checksum()
              << std::dec
              << std::endl;
    
    FileStorage mkvSrcOutOut(std::string("testYamkaOutOut.mkv"),
                             File::kReadWrite);
    if (!mkvSrcOutOut.file_.isOpen())
    {
      std::cerr << "ERROR: failed to open " << mkvSrcOutOut.file_.filename()
                << " to read/write"
                << std::endl;
      ::exit(1);
    }
    else
    {
      uint64 mkvSrcOutOutSize = mkvSrcOutOut.file_.size();
      std::cout << "opened (rw) " << mkvSrcOutOut.file_.filename()
              << ", file size: " << mkvSrcOutOutSize
                << std::endl;
      
      Crc32 crc32out;
      FileStorage::IReceiptPtr receipt = doc.save(mkvSrcOutOut, &crc32out);
      
      if (receipt)
      {
        std::cout << "stored " << doc.calcSize()
                  << " bytes, checksum: "
                  << std::hex
                  << std::uppercase
                  << crc32out.checksum()
                  << std::dec
                  << std::endl;
      }
    }
  }
  
  return 0;
}

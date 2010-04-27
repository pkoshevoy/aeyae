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
static bool
testSimpleBlockSerialization(const Bytes & blockData)
{
  SimpleBlock sb;
  if (!sb.importData(blockData))
  {
    std::cerr << "ERROR: bad block: \n"
              << blockData << std::endl;
    return false;
  }
  
  Bytes exportedData;
  sb.exportData(exportedData);
  
  Crc32 srcCrc;
  srcCrc.compute(blockData);
  unsigned int srcChecksum = srcCrc.checksum();
  
  Crc32 outCrc;
  outCrc.compute(exportedData);
  unsigned int outChecksum = outCrc.checksum();
  
  bool sameChecksum = srcChecksum == outChecksum;
  if (!sameChecksum)
  {
    std::cerr << "ERROR: couldn't export imported block!" << std::endl;
  }
  
  return sameChecksum;
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
    
    fs.file_.setSize(0);
    IStorage::IReceiptPtr receipt = fs.save(bytes);
    if (receipt)
    {
      std::cout << "stored " << bytes.size() << " bytes" << std::endl;
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
    
    EbmlDoc doc;
    doc.head_.payload_.docType_.payload_.set(std::string("yamka"));
    doc.head_.payload_.docTypeVersion_.payload_.set(1);
    doc.head_.payload_.docTypeReadVersion_.payload_.set(1);
    
    fs2.file_.setSize(0);
    IStorage::IReceiptPtr receipt = doc.save(fs2);
    
    if (receipt)
    {
      std::cout << "stored " << doc.calcSize() << " bytes" << std::endl;
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
    
    MatroskaDoc doc;
    uint64 bytesRead = doc.load(mkvSrc, mkvSrcSize);
    
    std::cout << "read " << bytesRead << " bytes, "
              << "doc size is " << doc.calcSize() << " bytes"
              << std::endl;
#if 0
    // for each segment:
    typedef std::list<MatroskaDoc::TSegment>::const_iterator TSegIter;
    for (TSegIter i = doc.segments_.begin(); i != doc.segments_.end(); ++i)
    {
      const MatroskaDoc::TSegment & segment = *i;
      std::cout << std::endl;
      
      // for each cluster:
      typedef std::list<Segment::TCluster>::const_iterator TClusterIter;
      for (TClusterIter j = segment.payload_.clusters_.begin();
           j != segment.payload_.clusters_.end(); ++j)
      {
        const Segment::TCluster & cluster = *j;
        std::cout << std::endl;
        
        // for each simple block:
        typedef std::list<Cluster::TSimpleBlock>::const_iterator TSimpleBlockIter;
        for (TSimpleBlockIter k = cluster.payload_.simpleBlocks_.begin();
             k != cluster.payload_.simpleBlocks_.end(); ++k)
        {
          const Cluster::TSimpleBlock & elt = *k;
          
          Bytes bin;
          if (elt.payload_.get(bin))
          {
            SimpleBlock sb;
            if (sb.importData(bin))
            {
              std::cout << sb << std::endl;
              
              // test packing:
              testSimpleBlockSerialization(bin);
            }
          }
        }
        
        // for each block group:
        typedef std::list<Cluster::TBlockGroup>::const_iterator TBlockGroupIter;
        for (TBlockGroupIter k = cluster.payload_.blockGroups_.begin();
             k != cluster.payload_.blockGroups_.end(); ++k)
        {
          const Cluster::TBlockGroup & elt = *k;
          
          Bytes bin;
          if (elt.payload_.block_.payload_.get(bin))
          {
            SimpleBlock sb;
            if (sb.importData(bin))
            {
              std::cout << sb << std::endl;
              
              // test packing:
              testSimpleBlockSerialization(bin);
            }
          }
        }
      }
    }
#endif

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
      
      mkvSrcOut.file_.setSize(0);
      doc.setCrc32(true);
      IStorage::IReceiptPtr receipt = doc.save(mkvSrcOut);
      
      if (receipt)
      {
        std::cout << "stored " << doc.calcSize() << " bytes" << std::endl;
      }
    }
  }

#if 1
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
    
    MatroskaDoc doc;
    uint64 bytesRead = doc.load(mkvSrcOut, mkvSrcOutSize);
    
    std::cout << "read " << bytesRead << " bytes, "
              << "doc size is " << doc.calcSize() << " bytes"
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
      
      mkvSrcOutOut.file_.setSize(0);
      doc.optimize();
      IStorage::IReceiptPtr receipt = doc.save(mkvSrcOutOut);
      
      if (receipt)
      {
        std::cout << "stored " << doc.calcSize()
                  << " bytes" << std::endl;
      }
    }
  }
#endif
  
  return 0;
}

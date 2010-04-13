// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:56:33 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaMatroska.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // SegInfo::SegInfo
  // 
  SegInfo::SegInfo()
  {
    timecodeScale_.alwaysSave().payload_.setDefault(1000000);
    duration_.alwaysSave().payload_.setDefault(0.0);
  }

  //----------------------------------------------------------------
  // Video::Video
  // 
  Video::Video()
  {
    aspectRatioType_.payload_.setDefault(0);
  }

  //----------------------------------------------------------------
  // Audio::Audio
  // 
  Audio::Audio()
  {
    sampFreq_.payload_.setDefault(8000.0);
    channels_.payload_.setDefault(1);
  }

  //----------------------------------------------------------------
  // Segment::calcSize
  // 
  uint64
  Segment::calcSize() const
  {
    uint64 size =
      info_.calcSize() +
      tracks_.calcSize() +
      chapters_.calcSize() +
      
      eltsCalcSize(seekHeads_) +
      eltsCalcSize(cues_) +
      eltsCalcSize(attachments_) +
      eltsCalcSize(tags_) +
      eltsCalcSize(clusters_);
    
    return size;
  }
  
  
  //----------------------------------------------------------------
  // MatroskaDoc::MatroskaDoc
  // 
  MatroskaDoc::MatroskaDoc():
    EbmlDoc("matroska", 1, 1)
  {}
  
  //----------------------------------------------------------------
  // MatroskaDoc::isDefault
  // 
  bool
  MatroskaDoc::isDefault() const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::calcSize
  // 
  // calculate payload size:
  uint64
  MatroskaDoc::calcSize() const
  {
    uint64 size =
      EbmlDoc::head_.calcSize() +
      eltsCalcSize(segments_);
    
    return size;  
  }

  //----------------------------------------------------------------
  // MatroskaDoc::save
  // 
  IStorage::IReceiptPtr
  MatroskaDoc::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = EbmlDoc::head_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::load
  // 
  uint64
  MatroskaDoc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    Bytes oneByte(1);
    uint64 bytesRead = 0;
    
    // skip forward until we load EBML head element:
    while (storageSize)
    {
      uint64 headSize = EbmlDoc::head_.load(storage, storageSize, crc);
      if (headSize)
      {
        storageSize -= headSize;
        bytesRead += headSize;
        break;
      }
      
      storage.load(oneByte);
      storageSize--;
    }
    
    // read Segments:
    uint64 segmentsSize = eltsLoad(segments_, storage, storageSize, crc);
    storageSize -= segmentsSize;
    bytesRead += segmentsSize;
    
    // read whatever is left:
    uint64 voidSize = EbmlPayload::load(storage, storageSize, crc);
    storageSize -= voidSize;
    bytesRead += voidSize;
    
    return bytesRead;
  }
  
}

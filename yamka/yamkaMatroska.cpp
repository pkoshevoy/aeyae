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
  // ChapTranslate::isDefault
  // 
  bool
  ChapTranslate::isDefault() const
  {
    bool allDefault =
      !editionUID_.mustSave() &&
      !chapTransCodec_.mustSave() &&
      !chapTransID_.mustSave();
    
    return allDefault;
  }

  //----------------------------------------------------------------
  // ChapTranslate::calcSize
  // 
  uint64
  ChapTranslate::calcSize() const
  {
    uint64 size =
      editionUID_.calcSize() +
      chapTransCodec_.calcSize() +
      chapTransID_.calcSize();
    
    return size;
  }

  //----------------------------------------------------------------
  // ChapTranslate::save
  // 
  IStorage::IReceiptPtr
  ChapTranslate::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = editionUID_.save(storage, crc);
    chapTransCodec_.save(storage, crc);
    chapTransID_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapTranslate::load
  // 
  uint64
  ChapTranslate::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= editionUID_.load(storage, storageSize, crc);
      storageSize -= chapTransCodec_.load(storage, storageSize, crc);
      storageSize -= chapTransID_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }

  
  //----------------------------------------------------------------
  // SegInfo::SegInfo
  // 
  SegInfo::SegInfo()
  {
    timecodeScale_.alwaysSave().payload_.setDefault(1000000);
    duration_.alwaysSave().payload_.setDefault(0.0);
  }
  
  //----------------------------------------------------------------
  // SegInfo::isDefault
  // 
  bool
  SegInfo::isDefault() const
  {
    bool allDefault =
      !segUID_.mustSave() &&
      !segFilename_.mustSave() &&
      !prevUID_.mustSave() &&
      !prevFilename_.mustSave() &&
      !nextUID_.mustSave() &&
      !nextFilename_.mustSave() &&
      !familyUID_.mustSave() &&
      !chapTranslate_.mustSave() &&
      !timecodeScale_.mustSave() &&
      !duration_.mustSave() &&
      !date_.mustSave() &&
      !title_.mustSave() &&
      !muxingApp_.mustSave() &&
      !writingApp_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SegInfo::calcSize
  // 
  uint64
  SegInfo::calcSize() const
  {
    uint64 size =
      segUID_.calcSize() +
      segFilename_.calcSize() +
      prevUID_.calcSize() +
      prevFilename_.calcSize() +
      nextUID_.calcSize() +
      nextFilename_.calcSize() +
      familyUID_.calcSize() +
      chapTranslate_.calcSize() +
      timecodeScale_.calcSize() +
      duration_.calcSize() +
      date_.calcSize() +
      title_.calcSize() +
      muxingApp_.calcSize() +
      writingApp_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SegInfo::save
  // 
  IStorage::IReceiptPtr
  SegInfo::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = segUID_.save(storage, crc);
    segFilename_.save(storage, crc);
    prevUID_.save(storage, crc);
    prevFilename_.save(storage, crc);
    nextUID_.save(storage, crc);
    nextFilename_.save(storage, crc);
    familyUID_.save(storage, crc);
    chapTranslate_.save(storage, crc);
    timecodeScale_.save(storage, crc);
    duration_.save(storage, crc);
    date_.save(storage, crc);
    title_.save(storage, crc);
    muxingApp_.save(storage, crc);
    writingApp_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SegInfo::load
  // 
  uint64
  SegInfo::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= segUID_.load(storage, storageSize, crc);
      storageSize -= segFilename_.load(storage, storageSize, crc);
      storageSize -= prevUID_.load(storage, storageSize, crc);
      storageSize -= prevFilename_.load(storage, storageSize, crc);
      storageSize -= nextUID_.load(storage, storageSize, crc);
      storageSize -= nextFilename_.load(storage, storageSize, crc);
      storageSize -= familyUID_.load(storage, storageSize, crc);
      storageSize -= chapTranslate_.load(storage, storageSize, crc);
      storageSize -= timecodeScale_.load(storage, storageSize, crc);
      storageSize -= duration_.load(storage, storageSize, crc);
      storageSize -= date_.load(storage, storageSize, crc);
      storageSize -= title_.load(storage, storageSize, crc);
      storageSize -= muxingApp_.load(storage, storageSize, crc);
      storageSize -= writingApp_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // TrackTranslate::isDefault
  // 
  bool
  TrackTranslate::isDefault() const
  {
    bool allDefault =
      !editionUID_.mustSave() &&
      !trackTransCodec_.mustSave() &&
      !trackTransID_.mustSave();
    
    return allDefault;
  }

  //----------------------------------------------------------------
  // TrackTranslate::calcSize
  // 
  uint64
  TrackTranslate::calcSize() const
  {
    uint64 size =
      editionUID_.calcSize() +
      trackTransCodec_.calcSize() +
      trackTransID_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // TrackTranslate::save
  // 
  IStorage::IReceiptPtr
  TrackTranslate::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = editionUID_.save(storage, crc);
    trackTransCodec_.save(storage, crc);
    trackTransID_.save(storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // TrackTranslate::load
  // 
  uint64
  TrackTranslate::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= editionUID_.load(storage, storageSize, crc);
      storageSize -= trackTransCodec_.load(storage, storageSize, crc);
      storageSize -= trackTransID_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }

  
  //----------------------------------------------------------------
  // Video::Video
  // 
  Video::Video()
  {
    aspectRatioType_.payload_.setDefault(0);
  }
  
  //----------------------------------------------------------------
  // Video::isDefault
  // 
  bool
  Video::isDefault() const
  {
    bool allDefault =
      !flagInterlaced_.mustSave() &&
      !stereoMode_.mustSave() &&
      !pixelWidth_.mustSave() &&
      !pixelHeight_.mustSave() &&
      !pixelCropBottom_.mustSave() &&
      !pixelCropTop_.mustSave() &&
      !pixelCropLeft_.mustSave() &&
      !pixelCropRight_.mustSave() &&
      !displayWidth_.mustSave() &&
      !displayHeight_.mustSave() &&
      !displayUnits_.mustSave() &&
      !aspectRatioType_.mustSave() &&
      !colorSpace_.mustSave() &&
      !gammaValue_.mustSave() &&
      !frameRate_.mustSave();
    
    return allDefault;
  }

  //----------------------------------------------------------------
  // Video::calcSize
  // 
  uint64
  Video::calcSize() const
  {
    uint64 size =
      flagInterlaced_.calcSize() +
      stereoMode_.calcSize() +
      pixelWidth_.calcSize() +
      pixelHeight_.calcSize() +
      pixelCropBottom_.calcSize() +
      pixelCropTop_.calcSize() +
      pixelCropLeft_.calcSize() +
      pixelCropRight_.calcSize() +
      displayWidth_.calcSize() +
      displayHeight_.calcSize() +
      displayUnits_.calcSize() +
      aspectRatioType_.calcSize() +
      colorSpace_.calcSize() +
      gammaValue_.calcSize() +
      frameRate_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Video::save
  // 
  IStorage::IReceiptPtr
  Video::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = flagInterlaced_.save(storage, crc);
    stereoMode_.save(storage, crc);
    pixelWidth_.save(storage, crc);
    pixelHeight_.save(storage, crc);
    pixelCropBottom_.save(storage, crc);
    pixelCropTop_.save(storage, crc);
    pixelCropLeft_.save(storage, crc);
    pixelCropRight_.save(storage, crc);
    displayWidth_.save(storage, crc);
    displayHeight_.save(storage, crc);
    displayUnits_.save(storage, crc);
    aspectRatioType_.save(storage, crc);
    colorSpace_.save(storage, crc);
    gammaValue_.save(storage, crc);
    frameRate_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Video::load
  // 
  uint64
  Video::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= flagInterlaced_.load(storage, storageSize, crc);
      storageSize -= stereoMode_.load(storage, storageSize, crc);
      storageSize -= pixelWidth_.load(storage, storageSize, crc);
      storageSize -= pixelHeight_.load(storage, storageSize, crc);
      storageSize -= pixelCropBottom_.load(storage, storageSize, crc);
      storageSize -= pixelCropTop_.load(storage, storageSize, crc);
      storageSize -= pixelCropLeft_.load(storage, storageSize, crc);
      storageSize -= pixelCropRight_.load(storage, storageSize, crc);
      storageSize -= displayWidth_.load(storage, storageSize, crc);
      storageSize -= displayHeight_.load(storage, storageSize, crc);
      storageSize -= displayUnits_.load(storage, storageSize, crc);
      storageSize -= aspectRatioType_.load(storage, storageSize, crc);
      storageSize -= colorSpace_.load(storage, storageSize, crc);
      storageSize -= gammaValue_.load(storage, storageSize, crc);
      storageSize -= frameRate_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
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
  // Audio::isDefault
  // 
  bool
  Audio::isDefault() const
  {
    bool allDefault =
      !sampFreq_.mustSave() &&
      !sampFreqOut_.mustSave() &&
      !channels_.mustSave() &&
      !channelPositions_.mustSave() &&
      !bitDepth_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Audio::calcSize
  // 
  uint64
  Audio::calcSize() const
  {
    uint64 size =
      sampFreq_.calcSize() +
      sampFreqOut_.calcSize() +
      channels_.calcSize() +
      channelPositions_.calcSize() +
      bitDepth_.calcSize();
    
    return size;
  }

  //----------------------------------------------------------------
  // Audio::save
  // 
  IStorage::IReceiptPtr
  Audio::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = sampFreq_.save(storage, crc);
    sampFreqOut_.save(storage, crc);
    channels_.save(storage, crc);
    channelPositions_.save(storage, crc);
    bitDepth_.save(storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Audio::load
  // 
  uint64
  Audio::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= sampFreq_.load(storage, storageSize, crc);
      storageSize -= sampFreqOut_.load(storage, storageSize, crc);
      storageSize -= channels_.load(storage, storageSize, crc);
      storageSize -= channelPositions_.load(storage, storageSize, crc);
      storageSize -= bitDepth_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ContentCompr::isDefault
  // 
  bool
  ContentCompr::isDefault() const
  {
    bool allDefault =
      !algo_.mustSave() &&
      !settings_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::calcSize
  // 
  uint64
  ContentCompr::calcSize() const
  {
    uint64 size =
      algo_.calcSize() +
      settings_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::save
  // 
  IStorage::IReceiptPtr
  ContentCompr::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = algo_.save(storage, crc);
    settings_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::load
  // 
  uint64
  ContentCompr::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= algo_.load(storage, storageSize, crc);
      storageSize -= settings_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ContentEncrypt::isDefault
  // 
  bool
  ContentEncrypt::isDefault() const
  {
    bool allDefault =
      !encAlgo_.mustSave() &&
      !encKeyID_.mustSave() &&
      !signature_.mustSave() &&
      !sigKeyID_.mustSave() &&
      !sigAlgo_.mustSave() &&
      !sigHashAlgo_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::calcSize
  // 
  uint64
  ContentEncrypt::calcSize() const
  {
    uint64 size =
      encAlgo_.calcSize() +
      encKeyID_.calcSize() +
      signature_.calcSize() +
      sigKeyID_.calcSize() +
      sigAlgo_.calcSize() +
      sigHashAlgo_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::save
  // 
  IStorage::IReceiptPtr
  ContentEncrypt::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = encAlgo_.save(storage, crc);
    encKeyID_.save(storage, crc);
    signature_.save(storage, crc);
    sigKeyID_.save(storage, crc);
    sigAlgo_.save(storage, crc);
    sigHashAlgo_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::load
  // 
  uint64
  ContentEncrypt::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= encAlgo_.load(storage, storageSize, crc);
      storageSize -= encKeyID_.load(storage, storageSize, crc);
      storageSize -= signature_.load(storage, storageSize, crc);
      storageSize -= sigKeyID_.load(storage, storageSize, crc);
      storageSize -= sigAlgo_.load(storage, storageSize, crc);
      storageSize -= sigHashAlgo_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ContentEnc::isDefault
  // 
  bool
  ContentEnc::isDefault() const
  {
    bool allDefault =
      !order_.mustSave() &&
      !scope_.mustSave() &&
      !type_.mustSave() &&
      !compression_.mustSave() &&
      !encryption_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::calcSize
  // 
  uint64
  ContentEnc::calcSize() const
  {
    uint64 size =
      order_.calcSize() +
      scope_.calcSize() +
      type_.calcSize() +
      compression_.calcSize() +
      encryption_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::save
  // 
  IStorage::IReceiptPtr
  ContentEnc::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = order_.save(storage, crc);
    scope_.save(storage, crc);
    type_.save(storage, crc);
    compression_.save(storage, crc);
    encryption_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::load
  // 
  uint64
  ContentEnc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= order_.load(storage, storageSize, crc);
      storageSize -= scope_.load(storage, storageSize, crc);
      storageSize -= type_.load(storage, storageSize, crc);
      storageSize -= compression_.load(storage, storageSize, crc);
      storageSize -= encryption_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ContentEncodings::isDefault
  // 
  bool
  ContentEncodings::isDefault() const
  {
    return encodings_.empty();
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::calcSize
  // 
  uint64
  ContentEncodings::calcSize() const
  {
    return eltsCalcSize(encodings_);
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::save
  // 
  IStorage::IReceiptPtr
  ContentEncodings::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(encodings_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::load
  // 
  uint64
  ContentEncodings::load(FileStorage & storage, uint64 storageSz, Crc32 * crc)
  {
    return eltsLoad(encodings_, storage, storageSz, crc);
  }
  
  
  //----------------------------------------------------------------
  // Track::isDefault
  // 
  bool
  Track::isDefault() const
  {
    bool allDefault =
      !trackNumber_.mustSave() &&
      !trackUID_.mustSave() &&
      !trackType_.mustSave() &&
      !flagEnabled_.mustSave() &&
      !flagDefault_.mustSave() &&
      !flagForced_.mustSave() &&
      !flagLacing_.mustSave() &&
      !minCache_.mustSave() &&
      !maxCache_.mustSave() &&
      !frameDuration_.mustSave() &&
      !timecodeScale_.mustSave() &&
      !trackOffset_.mustSave() &&
      !maxBlockAddID_.mustSave() &&
      !name_.mustSave() &&
      !language_.mustSave() &&
      !codecID_.mustSave() &&
      !codecPrivate_.mustSave() &&
      !codecName_.mustSave() &&
      !attachmentLink_.mustSave() &&
      !codecSettings_.mustSave() &&
      !codecInfoURL_.mustSave() &&
      !codecDownloadURL_.mustSave() &&
      !codecDecodeAll_.mustSave() &&
      !trackOverlay_.mustSave() &&
      !trackTranslate_.mustSave() &&
      !video_.mustSave() &&
      !audio_.mustSave() &&
      !contentEncs_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Track::calcSize
  // 
  uint64
  Track::calcSize() const
  {
    uint64 size =
      trackNumber_.calcSize() +
      trackUID_.calcSize() +
      trackType_.calcSize() +
      flagEnabled_.calcSize() +
      flagDefault_.calcSize() +
      flagForced_.calcSize() +
      flagLacing_.calcSize() +
      minCache_.calcSize() +
      maxCache_.calcSize() +
      frameDuration_.calcSize() +
      timecodeScale_.calcSize() +
      trackOffset_.calcSize() +
      maxBlockAddID_.calcSize() +
      name_.calcSize() +
      language_.calcSize() +
      codecID_.calcSize() +
      codecPrivate_.calcSize() +
      codecName_.calcSize() +
      attachmentLink_.calcSize() +
      codecSettings_.calcSize() +
      codecInfoURL_.calcSize() +
      codecDownloadURL_.calcSize() +
      codecDecodeAll_.calcSize() +
      trackOverlay_.calcSize() +
      trackTranslate_.calcSize() +
      video_.calcSize() +
      audio_.calcSize() +
      contentEncs_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Track::save
  // 
  IStorage::IReceiptPtr
  Track::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = trackNumber_.save(storage, crc);
    trackUID_.save(storage, crc);
    trackType_.save(storage, crc);
    flagEnabled_.save(storage, crc);
    flagDefault_.save(storage, crc);
    flagForced_.save(storage, crc);
    flagLacing_.save(storage, crc);
    minCache_.save(storage, crc);
    maxCache_.save(storage, crc);
    frameDuration_.save(storage, crc);
    timecodeScale_.save(storage, crc);
    trackOffset_.save(storage, crc);
    maxBlockAddID_.save(storage, crc);
    name_.save(storage, crc);
    language_.save(storage, crc);
    codecID_.save(storage, crc);
    codecPrivate_.save(storage, crc);
    codecName_.save(storage, crc);
    attachmentLink_.save(storage, crc);
    codecSettings_.save(storage, crc);
    codecInfoURL_.save(storage, crc);
    codecDownloadURL_.save(storage, crc);
    codecDecodeAll_.save(storage, crc);
    trackOverlay_.save(storage, crc);
    trackTranslate_.save(storage, crc);
    video_.save(storage, crc);
    audio_.save(storage, crc);
    contentEncs_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Track::load
  // 
  uint64
  Track::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= trackNumber_.load(storage, storageSize, crc);
      storageSize -= trackUID_.load(storage, storageSize, crc);
      storageSize -= trackType_.load(storage, storageSize, crc);
      storageSize -= flagEnabled_.load(storage, storageSize, crc);
      storageSize -= flagDefault_.load(storage, storageSize, crc);
      storageSize -= flagForced_.load(storage, storageSize, crc);
      storageSize -= flagLacing_.load(storage, storageSize, crc);
      storageSize -= minCache_.load(storage, storageSize, crc);
      storageSize -= maxCache_.load(storage, storageSize, crc);
      storageSize -= frameDuration_.load(storage, storageSize, crc);
      storageSize -= timecodeScale_.load(storage, storageSize, crc);
      storageSize -= trackOffset_.load(storage, storageSize, crc);
      storageSize -= maxBlockAddID_.load(storage, storageSize, crc);
      storageSize -= name_.load(storage, storageSize, crc);
      storageSize -= language_.load(storage, storageSize, crc);
      storageSize -= codecID_.load(storage, storageSize, crc);
      storageSize -= codecPrivate_.load(storage, storageSize, crc);
      storageSize -= codecName_.load(storage, storageSize, crc);
      storageSize -= attachmentLink_.load(storage, storageSize, crc);
      storageSize -= codecSettings_.load(storage, storageSize, crc);
      storageSize -= codecInfoURL_.load(storage, storageSize, crc);
      storageSize -= codecDownloadURL_.load(storage, storageSize, crc);
      storageSize -= codecDecodeAll_.load(storage, storageSize, crc);
      storageSize -= trackOverlay_.load(storage, storageSize, crc);
      storageSize -= trackTranslate_.load(storage, storageSize, crc);
      storageSize -= video_.load(storage, storageSize, crc);
      storageSize -= audio_.load(storage, storageSize, crc);
      storageSize -= contentEncs_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Tracks::isDefault
  // 
  bool
  Tracks::isDefault() const
  {
    return tracks_.empty();
  }
  
  //----------------------------------------------------------------
  // Tracks::calcSize
  // 
  uint64
  Tracks::calcSize() const
  {
    return eltsCalcSize(tracks_);
  }
  
  //----------------------------------------------------------------
  // Tracks::save
  // 
  IStorage::IReceiptPtr
  Tracks::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(tracks_, storage, crc);
  }

  //----------------------------------------------------------------
  // Tracks::load
  // 
  uint64
  Tracks::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(tracks_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // CueRef::isDefault
  // 
  bool
  CueRef::isDefault() const
  {
    bool allDefault =
      !time_.mustSave() &&
      !cluster_.mustSave() &&
      !block_.mustSave() &&
      !codecState_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // CueRef::calcSize
  // 
  uint64
  CueRef::calcSize() const
  {
    uint64 size =
      time_.calcSize() +
      cluster_.calcSize() +
      block_.calcSize() +
      codecState_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CueRef::save
  // 
  IStorage::IReceiptPtr
  CueRef::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = time_.save(storage, crc);
    cluster_.save(storage, crc);
    block_.save(storage, crc);
    codecState_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueRef::load
  // 
  uint64
  CueRef::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= time_.load(storage, storageSize, crc);
      storageSize -= cluster_.load(storage, storageSize, crc);
      storageSize -= block_.load(storage, storageSize, crc);
      storageSize -= codecState_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // CueTrkPos::isDefault
  // 
  bool
  CueTrkPos::isDefault() const
  {
    bool allDefault =
      !track_.mustSave() &&
      !cluster_.mustSave() &&
      !block_.mustSave() &&
      !codecState_.mustSave() &&
      !ref_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::calcSize
  // 
  uint64
  CueTrkPos::calcSize() const
  {
    uint64 size =
      track_.calcSize() +
      cluster_.calcSize() +
      block_.calcSize() +
      codecState_.calcSize() +
      ref_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::save
  // 
  IStorage::IReceiptPtr
  CueTrkPos::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = track_.save(storage, crc);
    cluster_.save(storage, crc);
    block_.save(storage, crc);
    codecState_.save(storage, crc);
    ref_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::load
  // 
  uint64
  CueTrkPos::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= track_.load(storage, storageSize, crc);
      storageSize -= cluster_.load(storage, storageSize, crc);
      storageSize -= block_.load(storage, storageSize, crc);
      storageSize -= codecState_.load(storage, storageSize, crc);
      storageSize -= ref_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // CuePoint::isDefault
  // 
  bool
  CuePoint::isDefault() const
  {
    bool allDefault =
      !time_.mustSave() &&
      trkPosns_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // CuePoint::calcSize
  // 
  uint64
  CuePoint::calcSize() const
  {
    uint64 size =
      time_.calcSize() +
      eltsCalcSize(trkPosns_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CuePoint::save
  // 
  IStorage::IReceiptPtr
  CuePoint::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = time_.save(storage, crc);
    eltsSave(trkPosns_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CuePoint::load
  // 
  uint64
  CuePoint::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= time_.load(storage, storageSize, crc);
      storageSize -= eltsLoad(trkPosns_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Cues::isDefault
  // 
  bool
  Cues::isDefault() const
  {
    return points_.empty();
  }
  
  //----------------------------------------------------------------
  // Cues::calcSize
  // 
  uint64
  Cues::calcSize() const
  {
    return eltsCalcSize(points_);
  }

  //----------------------------------------------------------------
  // Cues::save
  // 
  IStorage::IReceiptPtr
  Cues::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(points_, storage, crc);
  }

  //----------------------------------------------------------------
  // Cues::load
  // 
  uint64
  Cues::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(points_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // SeekEntry::isDefault
  // 
  bool
  SeekEntry::isDefault() const
  {
    bool allDefault =
      !id_.mustSave() &&
      !position_.mustSave();
    
    return allDefault;
  }

  //----------------------------------------------------------------
  // SeekEntry::calcSize
  // 
  uint64
  SeekEntry::calcSize() const
  {
    uint64 size =
      id_.calcSize() +
      position_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SeekEntry::save
  // 
  IStorage::IReceiptPtr
  SeekEntry::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = id_.save(storage, crc);
    position_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SeekEntry::load
  // 
  uint64
  SeekEntry::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= id_.load(storage, storageSize, crc);
      storageSize -= position_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // SeekHead::isDefault
  // 
  bool
  SeekHead::isDefault() const
  {
    return seek_.empty();
  }
  
  //----------------------------------------------------------------
  // SeekHead::calcSize
  // 
  uint64
  SeekHead::calcSize() const
  {
    return eltsCalcSize(seek_);
  }
  
  //----------------------------------------------------------------
  // SeekHead::save
  // 
  IStorage::IReceiptPtr
  SeekHead::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(seek_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // SeekHead::load
  // 
  uint64
  SeekHead::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(seek_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // AttdFile::isDefault
  // 
  bool
  AttdFile::isDefault() const
  {
    bool allDefault =
      !description_.mustSave() &&
      !filename_.mustSave() &&
      !mimeType_.mustSave() &&
      !data_.mustSave() &&
      !fileUID_.mustSave() &&
      !referral_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // AttdFile::calcSize
  // 
  uint64
  AttdFile::calcSize() const
  {
    uint64 size =
      description_.calcSize() +
      filename_.calcSize() +
      mimeType_.calcSize() +
      data_.calcSize() +
      fileUID_.calcSize() +
      referral_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // AttdFile::save
  // 
  IStorage::IReceiptPtr
  AttdFile::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = description_.save(storage, crc);
    filename_.save(storage, crc);
    mimeType_.save(storage, crc);
    data_.save(storage, crc);
    fileUID_.save(storage, crc);
    referral_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // AttdFile::load
  // 
  uint64
  AttdFile::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= description_.load(storage, storageSize, crc);
      storageSize -= filename_.load(storage, storageSize, crc);
      storageSize -= mimeType_.load(storage, storageSize, crc);
      storageSize -= data_.load(storage, storageSize, crc);
      storageSize -= fileUID_.load(storage, storageSize, crc);
      storageSize -= referral_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Attachments::isDefault
  // 
  bool
  Attachments::isDefault() const
  {
    return files_.empty();
  }
  
  //----------------------------------------------------------------
  // Attachments::calcSize
  // 
  uint64
  Attachments::calcSize() const
  {
    return eltsCalcSize(files_);
  }
  
  //----------------------------------------------------------------
  // Attachments::save
  // 
  IStorage::IReceiptPtr
  Attachments::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(files_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // Attachments::load
  // 
  uint64
  Attachments::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(files_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // ChapTrk::isDefault
  // 
  bool
  ChapTrk::isDefault() const
  {
    return tracks_.empty();
  }
  
  //----------------------------------------------------------------
  // ChapTrk::calcSize
  // 
  uint64
  ChapTrk::calcSize() const
  {
    return eltsCalcSize(tracks_);
  }

  //----------------------------------------------------------------
  // ChapTrk::save
  // 
  IStorage::IReceiptPtr
  ChapTrk::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(tracks_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // ChapTrk::load
  // 
  uint64
  ChapTrk::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(tracks_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // ChapDisp::isDefault
  // 
  bool
  ChapDisp::isDefault() const
  {
    bool allDefault =
      !string_.mustSave() &&
      !language_.mustSave() &&
      !country_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::calcSize
  // 
  uint64
  ChapDisp::calcSize() const
  {
    uint64 size =
      string_.calcSize() +
      language_.calcSize() +
      country_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::save
  // 
  IStorage::IReceiptPtr
  ChapDisp::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = string_.save(storage, crc);
    language_.save(storage, crc);
    country_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::load
  // 
  uint64
  ChapDisp::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= string_.load(storage, storageSize, crc);
      storageSize -= language_.load(storage, storageSize, crc);
      storageSize -= country_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ChapProcCmd::isDefault
  // 
  bool
  ChapProcCmd::isDefault() const
  {
    bool allDefault =
      !time_.mustSave() &&
      !data_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapProcCmd::calcSize
  // 
  uint64
  ChapProcCmd::calcSize() const
  {
    uint64 size =
      time_.calcSize() +
      data_.calcSize();
    
    return size;
  }

  //----------------------------------------------------------------
  // ChapProcCmd::save
  // 
  IStorage::IReceiptPtr
  ChapProcCmd::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = time_.save(storage, crc);
    data_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProcCmd::load
  // 
  uint64
  ChapProcCmd::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= time_.load(storage, storageSize, crc);
      storageSize -= data_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }

  
  //----------------------------------------------------------------
  // ChapProc::isDefault
  // 
  bool
  ChapProc::isDefault() const
  {
    bool allDefault =
      !codecID_.mustSave() &&
      !procPrivate_.mustSave() &&
      cmds_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapProc::calcSize
  // 
  uint64
  ChapProc::calcSize() const
  {
    uint64 size =
      codecID_.calcSize() +
      procPrivate_.calcSize() +
      eltsCalcSize(cmds_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapProc::save
  // 
  IStorage::IReceiptPtr
  ChapProc::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = codecID_.save(storage, crc);
    procPrivate_.save(storage, crc);
    
    eltsSave(cmds_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProc::load
  // 
  uint64
  ChapProc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= codecID_.load(storage, storageSize, crc);
      storageSize -= procPrivate_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(cmds_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ChapAtom::isDefault
  // 
  bool
  ChapAtom::isDefault() const
  {
    bool allDefault =
      !UID_.mustSave() &&
      !timeStart_.mustSave() &&
      !timeEnd_.mustSave() &&
      !hidden_.mustSave() &&
      !enabled_.mustSave() &&
      !segUID_.mustSave() &&
      !segEditionUID_.mustSave() &&
      !physEquiv_.mustSave() &&
      !tracks_.mustSave() &&
      display_.empty() &&
      process_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::calcSize
  // 
  uint64
  ChapAtom::calcSize() const
  {
    uint64 size =
      UID_.calcSize() +
      timeStart_.calcSize() +
      timeEnd_.calcSize() +
      hidden_.calcSize() +
      enabled_.calcSize() +
      segUID_.calcSize() +
      segEditionUID_.calcSize() +
      physEquiv_.calcSize() +
      tracks_.calcSize() +
      eltsCalcSize(display_) +
      eltsCalcSize(process_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::save
  // 
  IStorage::IReceiptPtr
  ChapAtom::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = UID_.save(storage, crc);
    timeStart_.save(storage, crc);
    timeEnd_.save(storage, crc);
    hidden_.save(storage, crc);
    enabled_.save(storage, crc);
    segUID_.save(storage, crc);
    segEditionUID_.save(storage, crc);
    physEquiv_.save(storage, crc);
    tracks_.save(storage, crc);
    
    eltsSave(display_, storage, crc);
    eltsSave(process_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::load
  // 
  uint64
  ChapAtom::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= UID_.load(storage, storageSize, crc);
      storageSize -= timeStart_.load(storage, storageSize, crc);
      storageSize -= timeEnd_.load(storage, storageSize, crc);
      storageSize -= hidden_.load(storage, storageSize, crc);
      storageSize -= enabled_.load(storage, storageSize, crc);
      storageSize -= segUID_.load(storage, storageSize, crc);
      storageSize -= segEditionUID_.load(storage, storageSize, crc);
      storageSize -= physEquiv_.load(storage, storageSize, crc);
      storageSize -= tracks_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(display_, storage, storageSize, crc);
      storageSize -= eltsLoad(process_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Edition::isDefault
  // 
  bool
  Edition::isDefault() const
  {
    bool allDefault =
      !UID_.mustSave() &&
      !flagHidden_.mustSave() &&
      !flagDefault_.mustSave() &&
      !flagOrdered_.mustSave() &&
      chapAtoms_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Edition::calcSize
  // 
  uint64
  Edition::calcSize() const
  {
    uint64 size =
      UID_.calcSize() +
      flagHidden_.calcSize() +
      flagDefault_.calcSize() +
      flagOrdered_.calcSize() +
      eltsCalcSize(chapAtoms_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Edition::save
  // 
  IStorage::IReceiptPtr
  Edition::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = UID_.save(storage, crc);
    flagHidden_.save(storage, crc);
    flagDefault_.save(storage, crc);
    flagOrdered_.save(storage, crc);
    
    eltsSave(chapAtoms_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Edition::load
  // 
  uint64
  Edition::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= UID_.load(storage, storageSize, crc);
      storageSize -= flagHidden_.load(storage, storageSize, crc);
      storageSize -= flagDefault_.load(storage, storageSize, crc);
      storageSize -= flagOrdered_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(chapAtoms_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Chapters::isDefault
  // 
  bool
  Chapters::isDefault() const
  {
    return editions_.empty();
  }
  
  //----------------------------------------------------------------
  // Chapters::calcSize
  // 
  uint64
  Chapters::calcSize() const
  {
    return eltsCalcSize(editions_);
  }
  
  //----------------------------------------------------------------
  // Chapters::save
  // 
  IStorage::IReceiptPtr
  Chapters::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(editions_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // Chapters::load
  // 
  uint64
  Chapters::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(editions_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // TagTargets::isDefault
  // 
  bool
  TagTargets::isDefault() const
  {
    bool allDefault =
      !typeValue_.mustSave() &&
      !type_.mustSave() &&
      trackUIDs_.empty() &&
      editionUIDs_.empty() &&
      chapterUIDs_.empty() &&
      attachmentUIDs_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // TagTargets::calcSize
  // 
  uint64
  TagTargets::calcSize() const
  {
    uint64 size =
      typeValue_.calcSize() +
      type_.calcSize() +
      eltsCalcSize(trackUIDs_) +
      eltsCalcSize(editionUIDs_) +
      eltsCalcSize(chapterUIDs_) +
      eltsCalcSize(attachmentUIDs_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // TagTargets::save
  // 
  IStorage::IReceiptPtr
  TagTargets::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = typeValue_.save(storage, crc);
    type_.save(storage, crc);
    
    eltsSave(trackUIDs_, storage, crc);
    eltsSave(editionUIDs_, storage, crc);
    eltsSave(chapterUIDs_, storage, crc);
    eltsSave(attachmentUIDs_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // TagTargets::load
  // 
  uint64
  TagTargets::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= typeValue_.load(storage, storageSize, crc);
      storageSize -= type_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(trackUIDs_, storage, storageSize, crc);
      storageSize -= eltsLoad(editionUIDs_, storage, storageSize, crc);
      storageSize -= eltsLoad(chapterUIDs_, storage, storageSize, crc);
      storageSize -= eltsLoad(attachmentUIDs_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // SimpleTag::isDefault
  // 
  bool
  SimpleTag::isDefault() const
  {
    bool allDefault =
      !name_.mustSave() &&
      !lang_.mustSave() &&
      !default_.mustSave() &&
      !string_.mustSave() &&
      !binary_.mustSave();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::calcSize
  // 
  uint64
  SimpleTag::calcSize() const
  {
    uint64 size =
      name_.calcSize() +
      lang_.calcSize() +
      default_.calcSize() +
      string_.calcSize() +
      binary_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::save
  // 
  IStorage::IReceiptPtr
  SimpleTag::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = name_.save(storage, crc);
    lang_.save(storage, crc);
    default_.save(storage, crc);
    string_.save(storage, crc);
    binary_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::load
  // 
  uint64
  SimpleTag::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= name_.load(storage, storageSize, crc);
      storageSize -= lang_.load(storage, storageSize, crc);
      storageSize -= default_.load(storage, storageSize, crc);
      storageSize -= string_.load(storage, storageSize, crc);
      storageSize -= binary_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Tag::isDefault
  // 
  bool
  Tag::isDefault() const
  {
    bool allDefault =
      !targets_.mustSave() &&
      simpleTags_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Tag::calcSize
  // 
  uint64
  Tag::calcSize() const
  {
    uint64 size =
      targets_.calcSize() +
      eltsCalcSize(simpleTags_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Tag::save
  // 
  IStorage::IReceiptPtr
  Tag::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = targets_.save(storage, crc);
    eltsSave(simpleTags_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Tag::load
  // 
  uint64
  Tag::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= targets_.load(storage, storageSize, crc);
      storageSize -= eltsLoad(simpleTags_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Tags::isDefault
  // 
  bool
  Tags::isDefault() const
  {
    return tags_.empty();
  }
  
  //----------------------------------------------------------------
  // Tags::calcSize
  // 
  uint64
  Tags::calcSize() const
  {
    return eltsCalcSize(tags_);
  }
  
  //----------------------------------------------------------------
  // Tags::save
  // 
  IStorage::IReceiptPtr
  Tags::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(tags_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // Tags::load
  // 
  uint64
  Tags::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(tags_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // SilentTracks::isDefault
  // 
  bool
  SilentTracks::isDefault() const
  {
    return tracks_.empty();
  }
  
  //----------------------------------------------------------------
  // SilentTracks::calcSize
  // 
  uint64
  SilentTracks::calcSize() const
  {
    return eltsCalcSize(tracks_);
  }
  
  //----------------------------------------------------------------
  // SilentTracks::save
  // 
  IStorage::IReceiptPtr
  SilentTracks::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(tracks_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // SilentTracks::load
  // 
  uint64
  SilentTracks::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(tracks_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // BlockMore::isDefault
  // 
  bool
  BlockMore::isDefault() const
  {
    bool allDefault =
      !blockAddID_.mustSave() &&
      !blockAdditional_.mustSave();
    
    return allDefault;
  }

  //----------------------------------------------------------------
  // BlockMore::calcSize
  // 
  uint64
  BlockMore::calcSize() const
  {
    uint64 size =
      blockAddID_.calcSize() +
      blockAdditional_.calcSize();
    
    return size;
  }
  
  //----------------------------------------------------------------
  // BlockMore::save
  // 
  IStorage::IReceiptPtr
  BlockMore::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = blockAddID_.save(storage, crc);
    blockAdditional_.save(storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockMore::load
  // 
  uint64
  BlockMore::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= blockAddID_.load(storage, storageSize, crc);
      storageSize -= blockAdditional_.load(storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // BlockAdditions::isDefault
  // 
  bool
  BlockAdditions::isDefault() const
  {
    return more_.empty();
  }
  
  //----------------------------------------------------------------
  // BlockAdditions::calcSize
  // 
  uint64
  BlockAdditions::calcSize() const
  {
    return eltsCalcSize(more_);
  }

  //----------------------------------------------------------------
  // BlockAdditions::save
  // 
  IStorage::IReceiptPtr
  BlockAdditions::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(more_, storage, crc);
  }

  //----------------------------------------------------------------
  // BlockAdditions::load
  // 
  uint64
  BlockAdditions::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    return eltsLoad(more_, storage, storageSize, crc);
  }
  
  
  //----------------------------------------------------------------
  // BlockGroup::isDefault
  // 
  bool
  BlockGroup::isDefault() const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::calcSize
  // 
  uint64
  BlockGroup::calcSize() const
  {
    uint64 size =
      duration_.calcSize() +
      block_.calcSize() +
      eltsCalcSize(blockVirtual_) +
      additions_.calcSize() +
      refPriority_.calcSize() +
      eltsCalcSize(refBlock_) +
      refVirtual_.calcSize() +
      codecState_.calcSize() +
      eltsCalcSize(slices_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::save
  // 
  IStorage::IReceiptPtr
  BlockGroup::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = duration_.save(storage, crc);
    block_.save(storage, crc);
    eltsSave(blockVirtual_, storage, crc);
    additions_.save(storage, crc);
    refPriority_.save(storage, crc);
    eltsSave(refBlock_, storage, crc);
    refVirtual_.save(storage, crc);
    codecState_.save(storage, crc);
    eltsSave(slices_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::load
  // 
  uint64
  BlockGroup::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= duration_.load(storage, storageSize, crc);
      storageSize -= block_.load(storage, storageSize, crc);
      storageSize -= eltsLoad(blockVirtual_, storage, storageSize, crc);
      storageSize -= additions_.load(storage, storageSize, crc);
      storageSize -= refPriority_.load(storage, storageSize, crc);
      storageSize -= eltsLoad(refBlock_, storage, storageSize, crc);
      storageSize -= refVirtual_.load(storage, storageSize, crc);
      storageSize -= codecState_.load(storage, storageSize, crc);
      storageSize -= eltsLoad(slices_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Cluster::isDefault
  // 
  bool
  Cluster::isDefault() const
  {
    return false;
  }

  //----------------------------------------------------------------
  // Cluster::calcSize
  // 
  uint64
  Cluster::calcSize() const
  {
    uint64 size =
      timecode_.calcSize() +
      silent_.calcSize() +
      position_.calcSize() +
      prevSize_.calcSize() +
      eltsCalcSize(blockGroups_) +
      eltsCalcSize(simpleBlocks_) +
      eltsCalcSize(encryptedBlocks_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Cluster::save
  // 
  IStorage::IReceiptPtr
  Cluster::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = timecode_.save(storage, crc);
    silent_.save(storage, crc);
    position_.save(storage, crc);
    prevSize_.save(storage, crc);
    
    eltsSave(blockGroups_, storage, crc);
    eltsSave(simpleBlocks_, storage, crc);
    eltsSave(encryptedBlocks_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Cluster::load
  // 
  uint64
  Cluster::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;
      
      storageSize -= timecode_.load(storage, storageSize, crc);
      storageSize -= silent_.load(storage, storageSize, crc);
      storageSize -= position_.load(storage, storageSize, crc);
      storageSize -= prevSize_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(blockGroups_, storage, storageSize, crc);
      storageSize -= eltsLoad(simpleBlocks_, storage, storageSize, crc);
      storageSize -= eltsLoad(encryptedBlocks_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Segment::isDefault
  // 
  bool
  Segment::isDefault() const
  {
    bool allDefault =
      !info_.mustSave() &&
      !tracks_.mustSave() &&
      !chapters_.mustSave() &&
      
      seekHeads_.empty() &&
      cues_.empty() &&
      attachments_.empty() &&
      tags_.empty() &&
      clusters_.empty();
    
    return allDefault;
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
  // Segment::save
  // 
  IStorage::IReceiptPtr
  Segment::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = info_.save(storage, crc);
    tracks_.save(storage, crc);
    chapters_.save(storage, crc);
    
    eltsSave(seekHeads_, storage, crc);
    eltsSave(cues_, storage, crc);
    eltsSave(attachments_, storage, crc);
    eltsSave(tags_, storage, crc);
    eltsSave(clusters_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Segment::load
  // 
  uint64
  Segment::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = storageSize;

      storageSize -= info_.load(storage, storageSize, crc);
      storageSize -= tracks_.load(storage, storageSize, crc);
      storageSize -= chapters_.load(storage, storageSize, crc);
      
      storageSize -= eltsLoad(seekHeads_, storage, storageSize, crc);
      storageSize -= eltsLoad(cues_, storage, storageSize, crc);
      storageSize -= eltsLoad(attachments_, storage, storageSize, crc);
      storageSize -= eltsLoad(tags_, storage, storageSize, crc);
      storageSize -= eltsLoad(clusters_, storage, storageSize, crc);
      
      uint64 bytesRead = prevStorageSize - storageSize;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    return bytesReadTotal;
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
    eltsSave(segments_, storage, crc);
    
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

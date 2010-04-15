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
      !chapTransID_.mustSave() &&
      voids_.empty();
    
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
      chapTransID_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // ChapTranslate::save
  // 
  IStorage::IReceiptPtr
  ChapTranslate::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    editionUID_.save(storage, crc);
    chapTransCodec_.save(storage, crc);
    chapTransID_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapTranslate::load
  // 
  uint64
  ChapTranslate::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= editionUID_.load(storage, bytesToRead, crc);
      bytesToRead -= chapTransCodec_.load(storage, bytesToRead, crc);
      bytesToRead -= chapTransID_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
    muxingApp_.alwaysSave().payload_.setDefault(std::string("yamka"));
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
      !writingApp_.mustSave() &&
      voids_.empty();
    
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
      writingApp_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SegInfo::save
  // 
  IStorage::IReceiptPtr
  SegInfo::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    segUID_.save(storage, crc);
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
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SegInfo::load
  // 
  uint64
  SegInfo::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= segUID_.load(storage, bytesToRead, crc);
      bytesToRead -= segFilename_.load(storage, bytesToRead, crc);
      bytesToRead -= prevUID_.load(storage, bytesToRead, crc);
      bytesToRead -= prevFilename_.load(storage, bytesToRead, crc);
      bytesToRead -= nextUID_.load(storage, bytesToRead, crc);
      bytesToRead -= nextFilename_.load(storage, bytesToRead, crc);
      bytesToRead -= familyUID_.load(storage, bytesToRead, crc);
      bytesToRead -= chapTranslate_.load(storage, bytesToRead, crc);
      bytesToRead -= timecodeScale_.load(storage, bytesToRead, crc);
      bytesToRead -= duration_.load(storage, bytesToRead, crc);
      bytesToRead -= date_.load(storage, bytesToRead, crc);
      bytesToRead -= title_.load(storage, bytesToRead, crc);
      bytesToRead -= muxingApp_.load(storage, bytesToRead, crc);
      bytesToRead -= writingApp_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !trackTransID_.mustSave() &&
      voids_.empty();
    
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
      trackTransID_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // TrackTranslate::save
  // 
  IStorage::IReceiptPtr
  TrackTranslate::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    editionUID_.save(storage, crc);
    trackTransCodec_.save(storage, crc);
    trackTransID_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // TrackTranslate::load
  // 
  uint64
  TrackTranslate::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= editionUID_.load(storage, bytesToRead, crc);
      bytesToRead -= trackTransCodec_.load(storage, bytesToRead, crc);
      bytesToRead -= trackTransID_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !frameRate_.mustSave() &&
      voids_.empty();
    
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
      frameRate_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Video::save
  // 
  IStorage::IReceiptPtr
  Video::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    flagInterlaced_.save(storage, crc);
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
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Video::load
  // 
  uint64
  Video::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= flagInterlaced_.load(storage, bytesToRead, crc);
      bytesToRead -= stereoMode_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelWidth_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelHeight_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelCropBottom_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelCropTop_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelCropLeft_.load(storage, bytesToRead, crc);
      bytesToRead -= pixelCropRight_.load(storage, bytesToRead, crc);
      bytesToRead -= displayWidth_.load(storage, bytesToRead, crc);
      bytesToRead -= displayHeight_.load(storage, bytesToRead, crc);
      bytesToRead -= displayUnits_.load(storage, bytesToRead, crc);
      bytesToRead -= aspectRatioType_.load(storage, bytesToRead, crc);
      bytesToRead -= colorSpace_.load(storage, bytesToRead, crc);
      bytesToRead -= gammaValue_.load(storage, bytesToRead, crc);
      bytesToRead -= frameRate_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !bitDepth_.mustSave() &&
      voids_.empty();
    
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
      bitDepth_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // Audio::save
  // 
  IStorage::IReceiptPtr
  Audio::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    sampFreq_.save(storage, crc);
    sampFreqOut_.save(storage, crc);
    channels_.save(storage, crc);
    channelPositions_.save(storage, crc);
    bitDepth_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Audio::load
  // 
  uint64
  Audio::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= sampFreq_.load(storage, bytesToRead, crc);
      bytesToRead -= sampFreqOut_.load(storage, bytesToRead, crc);
      bytesToRead -= channels_.load(storage, bytesToRead, crc);
      bytesToRead -= channelPositions_.load(storage, bytesToRead, crc);
      bytesToRead -= bitDepth_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !settings_.mustSave() &&
      voids_.empty();
    
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
      settings_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::save
  // 
  IStorage::IReceiptPtr
  ContentCompr::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    algo_.save(storage, crc);
    settings_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::load
  // 
  uint64
  ContentCompr::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= algo_.load(storage, bytesToRead, crc);
      bytesToRead -= settings_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !sigHashAlgo_.mustSave() &&
      voids_.empty();
    
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
      sigHashAlgo_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::save
  // 
  IStorage::IReceiptPtr
  ContentEncrypt::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    encAlgo_.save(storage, crc);
    encKeyID_.save(storage, crc);
    signature_.save(storage, crc);
    sigKeyID_.save(storage, crc);
    sigAlgo_.save(storage, crc);
    sigHashAlgo_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::load
  // 
  uint64
  ContentEncrypt::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= encAlgo_.load(storage, bytesToRead, crc);
      bytesToRead -= encKeyID_.load(storage, bytesToRead, crc);
      bytesToRead -= signature_.load(storage, bytesToRead, crc);
      bytesToRead -= sigKeyID_.load(storage, bytesToRead, crc);
      bytesToRead -= sigAlgo_.load(storage, bytesToRead, crc);
      bytesToRead -= sigHashAlgo_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !encryption_.mustSave() &&
      voids_.empty();
    
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
      encryption_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::save
  // 
  IStorage::IReceiptPtr
  ContentEnc::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    order_.save(storage, crc);
    scope_.save(storage, crc);
    type_.save(storage, crc);
    compression_.save(storage, crc);
    encryption_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::load
  // 
  uint64
  ContentEnc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= order_.load(storage, bytesToRead, crc);
      bytesToRead -= scope_.load(storage, bytesToRead, crc);
      bytesToRead -= type_.load(storage, bytesToRead, crc);
      bytesToRead -= compression_.load(storage, bytesToRead, crc);
      bytesToRead -= encryption_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ContentEncodings::isDefault
  // 
  bool
  ContentEncodings::isDefault() const
  {
    bool allDefault =
      encodings_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::calcSize
  // 
  uint64
  ContentEncodings::calcSize() const
  {
    uint64 size =
      eltsCalcSize(encodings_) +
      eltsCalcSize(voids_);
    
    return size;
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
  ContentEncodings::
  load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(encodings_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      !contentEncs_.mustSave() &&
      voids_.empty();
    
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
      contentEncs_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Track::save
  // 
  IStorage::IReceiptPtr
  Track::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    trackNumber_.save(storage, crc);
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
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Track::load
  // 
  uint64
  Track::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= trackNumber_.load(storage, bytesToRead, crc);
      bytesToRead -= trackUID_.load(storage, bytesToRead, crc);
      bytesToRead -= trackType_.load(storage, bytesToRead, crc);
      bytesToRead -= flagEnabled_.load(storage, bytesToRead, crc);
      bytesToRead -= flagDefault_.load(storage, bytesToRead, crc);
      bytesToRead -= flagForced_.load(storage, bytesToRead, crc);
      bytesToRead -= flagLacing_.load(storage, bytesToRead, crc);
      bytesToRead -= minCache_.load(storage, bytesToRead, crc);
      bytesToRead -= maxCache_.load(storage, bytesToRead, crc);
      bytesToRead -= frameDuration_.load(storage, bytesToRead, crc);
      bytesToRead -= timecodeScale_.load(storage, bytesToRead, crc);
      bytesToRead -= trackOffset_.load(storage, bytesToRead, crc);
      bytesToRead -= maxBlockAddID_.load(storage, bytesToRead, crc);
      bytesToRead -= name_.load(storage, bytesToRead, crc);
      bytesToRead -= language_.load(storage, bytesToRead, crc);
      bytesToRead -= codecID_.load(storage, bytesToRead, crc);
      bytesToRead -= codecPrivate_.load(storage, bytesToRead, crc);
      bytesToRead -= codecName_.load(storage, bytesToRead, crc);
      bytesToRead -= attachmentLink_.load(storage, bytesToRead, crc);
      bytesToRead -= codecSettings_.load(storage, bytesToRead, crc);
      bytesToRead -= codecInfoURL_.load(storage, bytesToRead, crc);
      bytesToRead -= codecDownloadURL_.load(storage, bytesToRead, crc);
      bytesToRead -= codecDecodeAll_.load(storage, bytesToRead, crc);
      bytesToRead -= trackOverlay_.load(storage, bytesToRead, crc);
      bytesToRead -= trackTranslate_.load(storage, bytesToRead, crc);
      bytesToRead -= video_.load(storage, bytesToRead, crc);
      bytesToRead -= audio_.load(storage, bytesToRead, crc);
      bytesToRead -= contentEncs_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Tracks::isDefault
  // 
  bool
  Tracks::isDefault() const
  {
    bool allDefault =
      tracks_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Tracks::calcSize
  // 
  uint64
  Tracks::calcSize() const
  {
    uint64 size =
      eltsCalcSize(tracks_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Tracks::save
  // 
  IStorage::IReceiptPtr
  Tracks::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(tracks_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Tracks::load
  // 
  uint64
  Tracks::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(tracks_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      !codecState_.mustSave() &&
      voids_.empty();
    
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
      codecState_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CueRef::save
  // 
  IStorage::IReceiptPtr
  CueRef::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    time_.save(storage, crc);
    cluster_.save(storage, crc);
    block_.save(storage, crc);
    codecState_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueRef::load
  // 
  uint64
  CueRef::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= time_.load(storage, bytesToRead, crc);
      bytesToRead -= cluster_.load(storage, bytesToRead, crc);
      bytesToRead -= block_.load(storage, bytesToRead, crc);
      bytesToRead -= codecState_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !ref_.mustSave() &&
      voids_.empty();
    
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
      ref_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::save
  // 
  IStorage::IReceiptPtr
  CueTrkPos::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    track_.save(storage, crc);
    cluster_.save(storage, crc);
    block_.save(storage, crc);
    codecState_.save(storage, crc);
    ref_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::load
  // 
  uint64
  CueTrkPos::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= track_.load(storage, bytesToRead, crc);
      bytesToRead -= cluster_.load(storage, bytesToRead, crc);
      bytesToRead -= block_.load(storage, bytesToRead, crc);
      bytesToRead -= codecState_.load(storage, bytesToRead, crc);
      bytesToRead -= ref_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      trkPosns_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(trkPosns_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // CuePoint::save
  // 
  IStorage::IReceiptPtr
  CuePoint::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    time_.save(storage, crc);
    eltsSave(trkPosns_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CuePoint::load
  // 
  uint64
  CuePoint::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= time_.load(storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(trkPosns_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Cues::isDefault
  // 
  bool
  Cues::isDefault() const
  {
    bool allDefault =
      points_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Cues::calcSize
  // 
  uint64
  Cues::calcSize() const
  {
    uint64 size =
      eltsCalcSize(points_) +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // Cues::save
  // 
  IStorage::IReceiptPtr
  Cues::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(points_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Cues::load
  // 
  uint64
  Cues::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(points_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // SeekEntry::isDefault
  // 
  bool
  SeekEntry::isDefault() const
  {
    bool allDefault =
      !id_.mustSave() &&
      !position_.mustSave() &&
      voids_.empty();
    
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
      position_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SeekEntry::save
  // 
  IStorage::IReceiptPtr
  SeekEntry::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    id_.save(storage, crc);
    position_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SeekEntry::load
  // 
  uint64
  SeekEntry::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= id_.load(storage, bytesToRead, crc);
      bytesToRead -= position_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // SeekHead::isDefault
  // 
  bool
  SeekHead::isDefault() const
  {
    bool allDefault =
      seek_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SeekHead::calcSize
  // 
  uint64
  SeekHead::calcSize() const
  {
    uint64 size =
      eltsCalcSize(seek_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SeekHead::save
  // 
  IStorage::IReceiptPtr
  SeekHead::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(seek_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SeekHead::load
  // 
  uint64
  SeekHead::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(seek_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      !referral_.mustSave() &&
      voids_.empty();
    
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
      referral_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // AttdFile::save
  // 
  IStorage::IReceiptPtr
  AttdFile::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    description_.save(storage, crc);
    filename_.save(storage, crc);
    mimeType_.save(storage, crc);
    data_.save(storage, crc);
    fileUID_.save(storage, crc);
    referral_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // AttdFile::load
  // 
  uint64
  AttdFile::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= description_.load(storage, bytesToRead, crc);
      bytesToRead -= filename_.load(storage, bytesToRead, crc);
      bytesToRead -= mimeType_.load(storage, bytesToRead, crc);
      bytesToRead -= data_.load(storage, bytesToRead, crc);
      bytesToRead -= fileUID_.load(storage, bytesToRead, crc);
      bytesToRead -= referral_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Attachments::isDefault
  // 
  bool
  Attachments::isDefault() const
  {
    bool allDefault =
      files_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Attachments::calcSize
  // 
  uint64
  Attachments::calcSize() const
  {
    uint64 size =
      eltsCalcSize(files_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Attachments::save
  // 
  IStorage::IReceiptPtr
  Attachments::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(files_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Attachments::load
  // 
  uint64
  Attachments::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(files_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // ChapTrk::isDefault
  // 
  bool
  ChapTrk::isDefault() const
  {
    bool allDefault =
      tracks_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapTrk::calcSize
  // 
  uint64
  ChapTrk::calcSize() const
  {
    uint64 size =
      eltsCalcSize(tracks_) +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // ChapTrk::save
  // 
  IStorage::IReceiptPtr
  ChapTrk::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(tracks_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapTrk::load
  // 
  uint64
  ChapTrk::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(tracks_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      !country_.mustSave() &&
      voids_.empty();
    
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
      country_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::save
  // 
  IStorage::IReceiptPtr
  ChapDisp::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    string_.save(storage, crc);
    language_.save(storage, crc);
    country_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::load
  // 
  uint64
  ChapDisp::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= string_.load(storage, bytesToRead, crc);
      bytesToRead -= language_.load(storage, bytesToRead, crc);
      bytesToRead -= country_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !data_.mustSave() &&
      voids_.empty();
    
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
      data_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // ChapProcCmd::save
  // 
  IStorage::IReceiptPtr
  ChapProcCmd::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    time_.save(storage, crc);
    data_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProcCmd::load
  // 
  uint64
  ChapProcCmd::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= time_.load(storage, bytesToRead, crc);
      bytesToRead -= data_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      cmds_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(cmds_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapProc::save
  // 
  IStorage::IReceiptPtr
  ChapProc::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    codecID_.save(storage, crc);
    procPrivate_.save(storage, crc);
    
    eltsSave(cmds_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProc::load
  // 
  uint64
  ChapProc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= codecID_.load(storage, bytesToRead, crc);
      bytesToRead -= procPrivate_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(cmds_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      process_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(process_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::save
  // 
  IStorage::IReceiptPtr
  ChapAtom::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    UID_.save(storage, crc);
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
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::load
  // 
  uint64
  ChapAtom::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= UID_.load(storage, bytesToRead, crc);
      bytesToRead -= timeStart_.load(storage, bytesToRead, crc);
      bytesToRead -= timeEnd_.load(storage, bytesToRead, crc);
      bytesToRead -= hidden_.load(storage, bytesToRead, crc);
      bytesToRead -= enabled_.load(storage, bytesToRead, crc);
      bytesToRead -= segUID_.load(storage, bytesToRead, crc);
      bytesToRead -= segEditionUID_.load(storage, bytesToRead, crc);
      bytesToRead -= physEquiv_.load(storage, bytesToRead, crc);
      bytesToRead -= tracks_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(display_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(process_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      chapAtoms_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(chapAtoms_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Edition::save
  // 
  IStorage::IReceiptPtr
  Edition::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    UID_.save(storage, crc);
    flagHidden_.save(storage, crc);
    flagDefault_.save(storage, crc);
    flagOrdered_.save(storage, crc);
    
    eltsSave(chapAtoms_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Edition::load
  // 
  uint64
  Edition::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= UID_.load(storage, bytesToRead, crc);
      bytesToRead -= flagHidden_.load(storage, bytesToRead, crc);
      bytesToRead -= flagDefault_.load(storage, bytesToRead, crc);
      bytesToRead -= flagOrdered_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(chapAtoms_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Chapters::isDefault
  // 
  bool
  Chapters::isDefault() const
  {
    bool allDefault =
      editions_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Chapters::calcSize
  // 
  uint64
  Chapters::calcSize() const
  {
    uint64 size =
      eltsCalcSize(editions_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Chapters::save
  // 
  IStorage::IReceiptPtr
  Chapters::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(editions_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Chapters::load
  // 
  uint64
  Chapters::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(editions_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      attachmentUIDs_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(attachmentUIDs_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // TagTargets::save
  // 
  IStorage::IReceiptPtr
  TagTargets::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    typeValue_.save(storage, crc);
    type_.save(storage, crc);
    
    eltsSave(trackUIDs_, storage, crc);
    eltsSave(editionUIDs_, storage, crc);
    eltsSave(chapterUIDs_, storage, crc);
    eltsSave(attachmentUIDs_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // TagTargets::load
  // 
  uint64
  TagTargets::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= typeValue_.load(storage, bytesToRead, crc);
      bytesToRead -= type_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(trackUIDs_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(editionUIDs_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(chapterUIDs_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(attachmentUIDs_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      !binary_.mustSave() &&
      voids_.empty();
    
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
      binary_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::save
  // 
  IStorage::IReceiptPtr
  SimpleTag::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    name_.save(storage, crc);
    lang_.save(storage, crc);
    default_.save(storage, crc);
    string_.save(storage, crc);
    binary_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::load
  // 
  uint64
  SimpleTag::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= name_.load(storage, bytesToRead, crc);
      bytesToRead -= lang_.load(storage, bytesToRead, crc);
      bytesToRead -= default_.load(storage, bytesToRead, crc);
      bytesToRead -= string_.load(storage, bytesToRead, crc);
      bytesToRead -= binary_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      simpleTags_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(simpleTags_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Tag::save
  // 
  IStorage::IReceiptPtr
  Tag::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    targets_.save(storage, crc);
    eltsSave(simpleTags_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Tag::load
  // 
  uint64
  Tag::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= targets_.load(storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(simpleTags_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // Tags::isDefault
  // 
  bool
  Tags::isDefault() const
  {
    bool allDefault =
      tags_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Tags::calcSize
  // 
  uint64
  Tags::calcSize() const
  {
    uint64 size =
      eltsCalcSize(tags_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Tags::save
  // 
  IStorage::IReceiptPtr
  Tags::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(tags_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Tags::load
  // 
  uint64
  Tags::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(tags_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // SilentTracks::isDefault
  // 
  bool
  SilentTracks::isDefault() const
  {
    bool allDefault =
      tracks_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::calcSize
  // 
  uint64
  SilentTracks::calcSize() const
  {
    uint64 size =
      eltsCalcSize(tracks_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::save
  // 
  IStorage::IReceiptPtr
  SilentTracks::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(tracks_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::load
  // 
  uint64
  SilentTracks::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(tracks_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // BlockMore::isDefault
  // 
  bool
  BlockMore::isDefault() const
  {
    bool allDefault =
      !blockAddID_.mustSave() &&
      !blockAdditional_.mustSave() &&
      voids_.empty();
    
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
      blockAdditional_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // BlockMore::save
  // 
  IStorage::IReceiptPtr
  BlockMore::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    blockAddID_.save(storage, crc);
    blockAdditional_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockMore::load
  // 
  uint64
  BlockMore::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= blockAddID_.load(storage, bytesToRead, crc);
      bytesToRead -= blockAdditional_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // BlockAdditions::isDefault
  // 
  bool
  BlockAdditions::isDefault() const
  {
    bool allDefault =
      more_.empty() &&
      voids_.empty();
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // BlockAdditions::calcSize
  // 
  uint64
  BlockAdditions::calcSize() const
  {
    uint64 size =
      eltsCalcSize(more_) +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // BlockAdditions::save
  // 
  IStorage::IReceiptPtr
  BlockAdditions::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    eltsSave(more_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // BlockAdditions::load
  // 
  uint64
  BlockAdditions::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(more_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
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
      eltsCalcSize(slices_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::save
  // 
  IStorage::IReceiptPtr
  BlockGroup::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    duration_.save(storage, crc);
    block_.save(storage, crc);
    eltsSave(blockVirtual_, storage, crc);
    additions_.save(storage, crc);
    refPriority_.save(storage, crc);
    eltsSave(refBlock_, storage, crc);
    refVirtual_.save(storage, crc);
    codecState_.save(storage, crc);
    eltsSave(slices_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::load
  // 
  uint64
  BlockGroup::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= duration_.load(storage, bytesToRead, crc);
      bytesToRead -= block_.load(storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(blockVirtual_, storage, bytesToRead, crc);
      bytesToRead -= additions_.load(storage, bytesToRead, crc);
      bytesToRead -= refPriority_.load(storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(refBlock_, storage, bytesToRead, crc);
      bytesToRead -= refVirtual_.load(storage, bytesToRead, crc);
      bytesToRead -= codecState_.load(storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(slices_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      eltsCalcSize(encryptedBlocks_) +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // Cluster::save
  // 
  IStorage::IReceiptPtr
  Cluster::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    timecode_.save(storage, crc);
    silent_.save(storage, crc);
    position_.save(storage, crc);
    prevSize_.save(storage, crc);
    
    eltsSave(blockGroups_, storage, crc);
    eltsSave(simpleBlocks_, storage, crc);
    eltsSave(encryptedBlocks_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Cluster::load
  // 
  uint64
  Cluster::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= timecode_.load(storage, bytesToRead, crc);
      bytesToRead -= silent_.load(storage, bytesToRead, crc);
      bytesToRead -= position_.load(storage, bytesToRead, crc);
      bytesToRead -= prevSize_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(blockGroups_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(simpleBlocks_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(encryptedBlocks_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      clusters_.empty() &&
      voids_.empty();
    
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
      eltsCalcSize(clusters_) +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // Segment::save
  // 
  IStorage::IReceiptPtr
  Segment::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 totalBytesToSave = calcSize();
    Bytes vsizeBytes = Bytes(vsizeEncode(totalBytesToSave));
    IStorage::IReceiptPtr receipt = storage.saveAndCalcCrc32(vsizeBytes);
    
    info_.save(storage, crc);
    tracks_.save(storage, crc);
    chapters_.save(storage, crc);
    
    eltsSave(seekHeads_, storage, crc);
    eltsSave(cues_, storage, crc);
    eltsSave(attachments_, storage, crc);
    eltsSave(tags_, storage, crc);
    eltsSave(clusters_, storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Segment::load
  // 
  uint64
  Segment::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;

      bytesToRead -= info_.load(storage, bytesToRead, crc);
      bytesToRead -= tracks_.load(storage, bytesToRead, crc);
      bytesToRead -= chapters_.load(storage, bytesToRead, crc);
      
      bytesToRead -= eltsLoad(seekHeads_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(cues_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(attachments_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(tags_, storage, bytesToRead, crc);
      bytesToRead -= eltsLoad(clusters_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;

      if (!bytesRead)
      {
        break;
      }
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
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
      eltsCalcSize(segments_) +
      eltsCalcSize(voids_);
    
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
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::load
  // 
  uint64
  MatroskaDoc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 bytesToRead = storageSize;
    
    // let the base class load the EBML header:
    uint64 bytesReadTotal = EbmlDoc::load(storage, storageSize, crc);
    bytesToRead -= bytesReadTotal;
    
    // read Segments:
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(segments_, storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    return bytesReadTotal;
  }
  
}

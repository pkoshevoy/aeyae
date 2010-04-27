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
  // ChapTranslate::eval
  // 
  bool
  ChapTranslate::eval(IElementCrawler & crawler)
  {
    return 
      editionUID_.eval(crawler) ||
      chapTransCodec_.eval(crawler) ||
      chapTransID_.eval(crawler);
  }
  
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
  ChapTranslate::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += editionUID_.save(storage);
    *receipt += chapTransCodec_.save(storage);
    *receipt += chapTransID_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapTranslate::load
  // 
  uint64
  ChapTranslate::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= editionUID_.load(storage, bytesToRead);
    bytesToRead -= chapTransCodec_.load(storage, bytesToRead);
    bytesToRead -= chapTransID_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
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
  // SegInfo::eval
  // 
  bool
  SegInfo::eval(IElementCrawler & crawler)
  {
    return
      segUID_.eval(crawler) ||
      segFilename_.eval(crawler) ||
      prevUID_.eval(crawler) ||
      prevFilename_.eval(crawler) ||
      nextUID_.eval(crawler) ||
      nextFilename_.eval(crawler) ||
      familyUID_.eval(crawler) ||
      chapTranslate_.eval(crawler) ||
      timecodeScale_.eval(crawler) ||
      duration_.eval(crawler) ||
      date_.eval(crawler) ||
      title_.eval(crawler) ||
      muxingApp_.eval(crawler) ||
      writingApp_.eval(crawler);
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
  SegInfo::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += segUID_.save(storage);
    *receipt += segFilename_.save(storage);
    *receipt += prevUID_.save(storage);
    *receipt += prevFilename_.save(storage);
    *receipt += nextUID_.save(storage);
    *receipt += nextFilename_.save(storage);
    *receipt += familyUID_.save(storage);
    *receipt += chapTranslate_.save(storage);
    *receipt += timecodeScale_.save(storage);
    *receipt += duration_.save(storage);
    *receipt += date_.save(storage);
    *receipt += title_.save(storage);
    *receipt += muxingApp_.save(storage);
    *receipt += writingApp_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SegInfo::load
  // 
  uint64
  SegInfo::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= segUID_.load(storage, bytesToRead);
    bytesToRead -= segFilename_.load(storage, bytesToRead);
    bytesToRead -= prevUID_.load(storage, bytesToRead);
    bytesToRead -= prevFilename_.load(storage, bytesToRead);
    bytesToRead -= nextUID_.load(storage, bytesToRead);
    bytesToRead -= nextFilename_.load(storage, bytesToRead);
    bytesToRead -= familyUID_.load(storage, bytesToRead);
    bytesToRead -= chapTranslate_.load(storage, bytesToRead);
    bytesToRead -= timecodeScale_.load(storage, bytesToRead);
    bytesToRead -= duration_.load(storage, bytesToRead);
    bytesToRead -= date_.load(storage, bytesToRead);
    bytesToRead -= title_.load(storage, bytesToRead);
    bytesToRead -= muxingApp_.load(storage, bytesToRead);
    bytesToRead -= writingApp_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // TrackTranslate::eval
  // 
  bool
  TrackTranslate::eval(IElementCrawler & crawler)
  {
    return 
      editionUID_.eval(crawler) ||
      trackTransCodec_.eval(crawler) ||
      trackTransID_.eval(crawler);
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
  TrackTranslate::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += editionUID_.save(storage);
    *receipt += trackTransCodec_.save(storage);
    *receipt += trackTransID_.save(storage);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // TrackTranslate::load
  // 
  uint64
  TrackTranslate::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= editionUID_.load(storage, bytesToRead);
    bytesToRead -= trackTransCodec_.load(storage, bytesToRead);
    bytesToRead -= trackTransID_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }

  
  //----------------------------------------------------------------
  // Video::Video
  // 
  Video::Video()
  {
    aspectRatioType_.payload_.setDefault(0);
  }
  
  //----------------------------------------------------------------
  // Video::eval
  // 
  bool
  Video::eval(IElementCrawler & crawler)
  {
    return
      flagInterlaced_.eval(crawler) ||
      stereoMode_.eval(crawler) ||
      pixelWidth_.eval(crawler) ||
      pixelHeight_.eval(crawler) ||
      pixelCropBottom_.eval(crawler) ||
      pixelCropTop_.eval(crawler) ||
      pixelCropLeft_.eval(crawler) ||
      pixelCropRight_.eval(crawler) ||
      displayWidth_.eval(crawler) ||
      displayHeight_.eval(crawler) ||
      displayUnits_.eval(crawler) ||
      aspectRatioType_.eval(crawler) ||
      colorSpace_.eval(crawler) ||
      gammaValue_.eval(crawler) ||
      frameRate_.eval(crawler);
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
  Video::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += flagInterlaced_.save(storage);
    *receipt += stereoMode_.save(storage);
    *receipt += pixelWidth_.save(storage);
    *receipt += pixelHeight_.save(storage);
    *receipt += pixelCropBottom_.save(storage);
    *receipt += pixelCropTop_.save(storage);
    *receipt += pixelCropLeft_.save(storage);
    *receipt += pixelCropRight_.save(storage);
    *receipt += displayWidth_.save(storage);
    *receipt += displayHeight_.save(storage);
    *receipt += displayUnits_.save(storage);
    *receipt += aspectRatioType_.save(storage);
    *receipt += colorSpace_.save(storage);
    *receipt += gammaValue_.save(storage);
    *receipt += frameRate_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Video::load
  // 
  uint64
  Video::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= flagInterlaced_.load(storage, bytesToRead);
    bytesToRead -= stereoMode_.load(storage, bytesToRead);
    bytesToRead -= pixelWidth_.load(storage, bytesToRead);
    bytesToRead -= pixelHeight_.load(storage, bytesToRead);
    bytesToRead -= pixelCropBottom_.load(storage, bytesToRead);
    bytesToRead -= pixelCropTop_.load(storage, bytesToRead);
    bytesToRead -= pixelCropLeft_.load(storage, bytesToRead);
    bytesToRead -= pixelCropRight_.load(storage, bytesToRead);
    bytesToRead -= displayWidth_.load(storage, bytesToRead);
    bytesToRead -= displayHeight_.load(storage, bytesToRead);
    bytesToRead -= displayUnits_.load(storage, bytesToRead);
    bytesToRead -= aspectRatioType_.load(storage, bytesToRead);
    bytesToRead -= colorSpace_.load(storage, bytesToRead);
    bytesToRead -= gammaValue_.load(storage, bytesToRead);
    bytesToRead -= frameRate_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
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
  // Audio::eval
  // 
  bool
  Audio::eval(IElementCrawler & crawler)
  {
    return
      sampFreq_.eval(crawler) ||
      sampFreqOut_.eval(crawler) ||
      channels_.eval(crawler) ||
      channelPositions_.eval(crawler) ||
      bitDepth_.eval(crawler);
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
  Audio::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += sampFreq_.save(storage);
    *receipt += sampFreqOut_.save(storage);
    *receipt += channels_.save(storage);
    *receipt += channelPositions_.save(storage);
    *receipt += bitDepth_.save(storage);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Audio::load
  // 
  uint64
  Audio::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= sampFreq_.load(storage, bytesToRead);
    bytesToRead -= sampFreqOut_.load(storage, bytesToRead);
    bytesToRead -= channels_.load(storage, bytesToRead);
    bytesToRead -= channelPositions_.load(storage, bytesToRead);
    bytesToRead -= bitDepth_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  

  //----------------------------------------------------------------
  // ContentCompr::eval
  // 
  bool
  ContentCompr::eval(IElementCrawler & crawler)
  {
    return
      algo_.eval(crawler) ||
      settings_.eval(crawler);
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
  ContentCompr::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += algo_.save(storage);
    *receipt += settings_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentCompr::load
  // 
  uint64
  ContentCompr::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= algo_.load(storage, bytesToRead);
    bytesToRead -= settings_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ContentEncrypt::eval
  // 
  bool
  ContentEncrypt::eval(IElementCrawler & crawler)
  {
    return
      encAlgo_.eval(crawler) ||
      encKeyID_.eval(crawler) ||
      signature_.eval(crawler) ||
      sigKeyID_.eval(crawler) ||
      sigAlgo_.eval(crawler) ||
      sigHashAlgo_.eval(crawler);
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
  ContentEncrypt::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += encAlgo_.save(storage);
    *receipt += encKeyID_.save(storage);
    *receipt += signature_.save(storage);
    *receipt += sigKeyID_.save(storage);
    *receipt += sigAlgo_.save(storage);
    *receipt += sigHashAlgo_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEncrypt::load
  // 
  uint64
  ContentEncrypt::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= encAlgo_.load(storage, bytesToRead);
    bytesToRead -= encKeyID_.load(storage, bytesToRead);
    bytesToRead -= signature_.load(storage, bytesToRead);
    bytesToRead -= sigKeyID_.load(storage, bytesToRead);
    bytesToRead -= sigAlgo_.load(storage, bytesToRead);
    bytesToRead -= sigHashAlgo_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ContentEnc::eval
  // 
  bool
  ContentEnc::eval(IElementCrawler & crawler)
  {
    return
      order_.eval(crawler) ||
      scope_.eval(crawler) ||
      type_.eval(crawler) ||
      compression_.eval(crawler) ||
      encryption_.eval(crawler);
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
  ContentEnc::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += order_.save(storage);
    *receipt += scope_.save(storage);
    *receipt += type_.save(storage);
    *receipt += compression_.save(storage);
    *receipt += encryption_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEnc::load
  // 
  uint64
  ContentEnc::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= order_.load(storage, bytesToRead);
    bytesToRead -= scope_.load(storage, bytesToRead);
    bytesToRead -= type_.load(storage, bytesToRead);
    bytesToRead -= compression_.load(storage, bytesToRead);
    bytesToRead -= encryption_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ContentEncodings::eval
  // 
  bool
  ContentEncodings::eval(IElementCrawler & crawler)
  {
    return eltsEval(encodings_, crawler);
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::isDefault
  // 
  bool
  ContentEncodings::isDefault() const
  {
    bool allDefault = encodings_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::calcSize
  // 
  uint64
  ContentEncodings::calcSize() const
  {
    uint64 size = eltsCalcSize(encodings_);
    return size;
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::save
  // 
  IStorage::IReceiptPtr
  ContentEncodings::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(encodings_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ContentEncodings::load
  // 
  uint64
  ContentEncodings::
  load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(encodings_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Track::eval
  // 
  bool
  Track::eval(IElementCrawler & crawler)
  {
    return
      trackNumber_.eval(crawler) ||
      trackUID_.eval(crawler) ||
      trackType_.eval(crawler) ||
      flagEnabled_.eval(crawler) ||
      flagDefault_.eval(crawler) ||
      flagForced_.eval(crawler) ||
      flagLacing_.eval(crawler) ||
      minCache_.eval(crawler) ||
      maxCache_.eval(crawler) ||
      frameDuration_.eval(crawler) ||
      timecodeScale_.eval(crawler) ||
      trackOffset_.eval(crawler) ||
      maxBlockAddID_.eval(crawler) ||
      name_.eval(crawler) ||
      language_.eval(crawler) ||
      codecID_.eval(crawler) ||
      codecPrivate_.eval(crawler) ||
      codecName_.eval(crawler) ||
      attachmentLink_.eval(crawler) ||
      codecSettings_.eval(crawler) ||
      codecInfoURL_.eval(crawler) ||
      codecDownloadURL_.eval(crawler) ||
      codecDecodeAll_.eval(crawler) ||
      trackOverlay_.eval(crawler) ||
      trackTranslate_.eval(crawler) ||
      video_.eval(crawler) ||
      audio_.eval(crawler) ||
      contentEncs_.eval(crawler);
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
  Track::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += trackNumber_.save(storage);
    *receipt += trackUID_.save(storage);
    *receipt += trackType_.save(storage);
    *receipt += flagEnabled_.save(storage);
    *receipt += flagDefault_.save(storage);
    *receipt += flagForced_.save(storage);
    *receipt += flagLacing_.save(storage);
    *receipt += minCache_.save(storage);
    *receipt += maxCache_.save(storage);
    *receipt += frameDuration_.save(storage);
    *receipt += timecodeScale_.save(storage);
    *receipt += trackOffset_.save(storage);
    *receipt += maxBlockAddID_.save(storage);
    *receipt += name_.save(storage);
    *receipt += language_.save(storage);
    *receipt += codecID_.save(storage);
    *receipt += codecPrivate_.save(storage);
    *receipt += codecName_.save(storage);
    *receipt += attachmentLink_.save(storage);
    *receipt += codecSettings_.save(storage);
    *receipt += codecInfoURL_.save(storage);
    *receipt += codecDownloadURL_.save(storage);
    *receipt += codecDecodeAll_.save(storage);
    *receipt += trackOverlay_.save(storage);
    *receipt += trackTranslate_.save(storage);
    *receipt += video_.save(storage);
    *receipt += audio_.save(storage);
    *receipt += contentEncs_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Track::load
  // 
  uint64
  Track::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= trackNumber_.load(storage, bytesToRead);
    bytesToRead -= trackUID_.load(storage, bytesToRead);
    bytesToRead -= trackType_.load(storage, bytesToRead);
    bytesToRead -= flagEnabled_.load(storage, bytesToRead);
    bytesToRead -= flagDefault_.load(storage, bytesToRead);
    bytesToRead -= flagForced_.load(storage, bytesToRead);
    bytesToRead -= flagLacing_.load(storage, bytesToRead);
    bytesToRead -= minCache_.load(storage, bytesToRead);
    bytesToRead -= maxCache_.load(storage, bytesToRead);
    bytesToRead -= frameDuration_.load(storage, bytesToRead);
    bytesToRead -= timecodeScale_.load(storage, bytesToRead);
    bytesToRead -= trackOffset_.load(storage, bytesToRead);
    bytesToRead -= maxBlockAddID_.load(storage, bytesToRead);
    bytesToRead -= name_.load(storage, bytesToRead);
    bytesToRead -= language_.load(storage, bytesToRead);
    bytesToRead -= codecID_.load(storage, bytesToRead);
    bytesToRead -= codecPrivate_.load(storage, bytesToRead);
    bytesToRead -= codecName_.load(storage, bytesToRead);
    bytesToRead -= attachmentLink_.load(storage, bytesToRead);
    bytesToRead -= codecSettings_.load(storage, bytesToRead);
    bytesToRead -= codecInfoURL_.load(storage, bytesToRead);
    bytesToRead -= codecDownloadURL_.load(storage, bytesToRead);
    bytesToRead -= codecDecodeAll_.load(storage, bytesToRead);
    bytesToRead -= trackOverlay_.load(storage, bytesToRead);
    bytesToRead -= trackTranslate_.load(storage, bytesToRead);
    bytesToRead -= video_.load(storage, bytesToRead);
    bytesToRead -= audio_.load(storage, bytesToRead);
    bytesToRead -= contentEncs_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Tracks::eval
  // 
  bool
  Tracks::eval(IElementCrawler & crawler)
  {
    return eltsEval(tracks_, crawler);
  }
  
  //----------------------------------------------------------------
  // Tracks::isDefault
  // 
  bool
  Tracks::isDefault() const
  {
    bool allDefault = tracks_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Tracks::calcSize
  // 
  uint64
  Tracks::calcSize() const
  {
    uint64 size = eltsCalcSize(tracks_);
    return size;
  }
  
  //----------------------------------------------------------------
  // Tracks::save
  // 
  IStorage::IReceiptPtr
  Tracks::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(tracks_, storage);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Tracks::load
  // 
  uint64
  Tracks::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(tracks_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // CueRef::eval
  // 
  bool
  CueRef::eval(IElementCrawler & crawler)
  {
    return
      time_.eval(crawler) ||
      cluster_.eval(crawler) ||
      block_.eval(crawler) ||
      codecState_.eval(crawler);
  }
  
  //----------------------------------------------------------------
  // CueRef::isDefault
  // 
  bool
  CueRef::isDefault() const
  {
    bool allDefault =
      !cluster_.mustSave() &&
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
  CueRef::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += time_.save(storage);
    *receipt += cluster_.save(storage);
    *receipt += block_.save(storage);
    *receipt += codecState_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueRef::load
  // 
  uint64
  CueRef::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= time_.load(storage, bytesToRead);
    bytesToRead -= cluster_.load(storage, bytesToRead);
    bytesToRead -= block_.load(storage, bytesToRead);
    bytesToRead -= codecState_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // CueTrkPos::CueTrkPos
  // 
  CueTrkPos::CueTrkPos()
  {
    track_.alwaysSave();
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::eval
  // 
  bool
  CueTrkPos::eval(IElementCrawler & crawler)
  {
    return
      track_.eval(crawler) ||
      cluster_.eval(crawler) ||
      block_.eval(crawler) ||
      codecState_.eval(crawler) ||
      ref_.eval(crawler);
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::isDefault
  // 
  bool
  CueTrkPos::isDefault() const
  {
    bool allDefault =
      !cluster_.mustSave() &&
      !codecState_.mustSave();
    
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
  CueTrkPos::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += track_.save(storage);
    *receipt += cluster_.save(storage);
    *receipt += block_.save(storage);
    *receipt += codecState_.save(storage);
    *receipt += ref_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CueTrkPos::load
  // 
  uint64
  CueTrkPos::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= track_.load(storage, bytesToRead);
    bytesToRead -= cluster_.load(storage, bytesToRead);
    bytesToRead -= block_.load(storage, bytesToRead);
    bytesToRead -= codecState_.load(storage, bytesToRead);
    bytesToRead -= ref_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // CuePoint::CuePoint
  // 
  CuePoint::CuePoint()
  {
    time_.alwaysSave();
  }
  
  //----------------------------------------------------------------
  // CuePoint::eval
  // 
  bool
  CuePoint::eval(IElementCrawler & crawler)
  {
    return
      time_.eval(crawler) ||
      eltsEval(trkPosns_, crawler);
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
  CuePoint::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += time_.save(storage);
    *receipt += eltsSave(trkPosns_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // CuePoint::load
  // 
  uint64
  CuePoint::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= time_.load(storage, bytesToRead);
    bytesToRead -= eltsLoad(trkPosns_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Cues::eval
  // 
  bool
  Cues::eval(IElementCrawler & crawler)
  {
    return eltsEval(points_, crawler);
  }
  
  //----------------------------------------------------------------
  // Cues::isDefault
  // 
  bool
  Cues::isDefault() const
  {
    bool allDefault = points_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Cues::calcSize
  // 
  uint64
  Cues::calcSize() const
  {
    uint64 size = eltsCalcSize(points_);
    return size;
  }

  //----------------------------------------------------------------
  // Cues::save
  // 
  IStorage::IReceiptPtr
  Cues::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(points_, storage);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // Cues::load
  // 
  uint64
  Cues::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(points_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // SeekEntry::eval
  // 
  bool
  SeekEntry::eval(IElementCrawler & crawler)
  {
    return
      id_.eval(crawler) ||
      position_.eval(crawler);
  }
  
  //----------------------------------------------------------------
  // SeekEntry::isDefault
  // 
  bool
  SeekEntry::isDefault() const
  {
    bool allDefault =
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
  SeekEntry::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += id_.save(storage);
    *receipt += position_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SeekEntry::load
  // 
  uint64
  SeekEntry::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= id_.load(storage, bytesToRead);
    bytesToRead -= position_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // SeekHead::eval
  // 
  bool
  SeekHead::eval(IElementCrawler & crawler)
  {
    return eltsEval(seek_, crawler);
  }
  
  //----------------------------------------------------------------
  // SeekHead::isDefault
  // 
  bool
  SeekHead::isDefault() const
  {
    bool allDefault = seek_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SeekHead::calcSize
  // 
  uint64
  SeekHead::calcSize() const
  {
    uint64 size = eltsCalcSize(seek_);
    return size;
  }
  
  //----------------------------------------------------------------
  // SeekHead::save
  // 
  IStorage::IReceiptPtr
  SeekHead::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(seek_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SeekHead::load
  // 
  uint64
  SeekHead::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(seek_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // SeekHead::indexThis
  // 
  void
  SeekHead::indexThis(const IElement * segment,
                      const IElement * element,
                      IStorage & binaryStorage)
  {
    if (!element)
    {
      return;
    }
    
    seek_.push_back(TSeekEntry());
    TSeekEntry & index = seek_.back();
    
    Bytes eltId = Bytes(uintEncode(element->getId()));
    index.payload_.id_.payload_.set(eltId, binaryStorage);
    index.payload_.position_.payload_.setOrigin(segment);
    index.payload_.position_.payload_.setElt(element);
  }
  
  
  //----------------------------------------------------------------
  // AttdFile::eval
  // 
  bool
  AttdFile::eval(IElementCrawler & crawler)
  {
    return
      description_.eval(crawler) ||
      filename_.eval(crawler) ||
      mimeType_.eval(crawler) ||
      data_.eval(crawler) ||
      fileUID_.eval(crawler) ||
      referral_.eval(crawler);
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
  AttdFile::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += description_.save(storage);
    *receipt += filename_.save(storage);
    *receipt += mimeType_.save(storage);
    *receipt += data_.save(storage);
    *receipt += fileUID_.save(storage);
    *receipt += referral_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // AttdFile::load
  // 
  uint64
  AttdFile::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= description_.load(storage, bytesToRead);
    bytesToRead -= filename_.load(storage, bytesToRead);
    bytesToRead -= mimeType_.load(storage, bytesToRead);
    bytesToRead -= data_.load(storage, bytesToRead);
    bytesToRead -= fileUID_.load(storage, bytesToRead);
    bytesToRead -= referral_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Attachments::eval
  // 
  bool
  Attachments::eval(IElementCrawler & crawler)
  {
    return eltsEval(files_, crawler);
  }
    
  //----------------------------------------------------------------
  // Attachments::isDefault
  // 
  bool
  Attachments::isDefault() const
  {
    bool allDefault = files_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Attachments::calcSize
  // 
  uint64
  Attachments::calcSize() const
  {
    uint64 size = eltsCalcSize(files_);
    return size;
  }
  
  //----------------------------------------------------------------
  // Attachments::save
  // 
  IStorage::IReceiptPtr
  Attachments::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(files_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Attachments::load
  // 
  uint64
  Attachments::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(files_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ChapTrk::eval
  // 
  bool
  ChapTrk::eval(IElementCrawler & crawler)
  {
    return eltsEval(tracks_, crawler);
  }
  
  //----------------------------------------------------------------
  // ChapTrk::isDefault
  // 
  bool
  ChapTrk::isDefault() const
  {
    bool allDefault = tracks_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // ChapTrk::calcSize
  // 
  uint64
  ChapTrk::calcSize() const
  {
    uint64 size = eltsCalcSize(tracks_);
    return size;
  }

  //----------------------------------------------------------------
  // ChapTrk::save
  // 
  IStorage::IReceiptPtr
  ChapTrk::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(tracks_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapTrk::load
  // 
  uint64
  ChapTrk::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(tracks_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ChapDisp::eval
  // 
  bool
  ChapDisp::eval(IElementCrawler & crawler)
  {
    return
      string_.eval(crawler) ||
      language_.eval(crawler) ||
      country_.eval(crawler);
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
  ChapDisp::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += string_.save(storage);
    *receipt += language_.save(storage);
    *receipt += country_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapDisp::load
  // 
  uint64
  ChapDisp::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= string_.load(storage, bytesToRead);
    bytesToRead -= language_.load(storage, bytesToRead);
    bytesToRead -= country_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ChapProcCmd::eval
  // 
  bool
  ChapProcCmd::eval(IElementCrawler & crawler)
  {
    return
      time_.eval(crawler) ||
      data_.eval(crawler);
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
  ChapProcCmd::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += time_.save(storage);
    *receipt += data_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProcCmd::load
  // 
  uint64
  ChapProcCmd::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= time_.load(storage, bytesToRead);
    bytesToRead -= data_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ChapProc::eval
  // 
  bool
  ChapProc::eval(IElementCrawler & crawler)
  {
    return
      codecID_.eval(crawler) ||
      procPrivate_.eval(crawler) ||
      eltsEval(cmds_, crawler);
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
  ChapProc::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += codecID_.save(storage);
    *receipt += procPrivate_.save(storage);
    
    *receipt += eltsSave(cmds_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapProc::load
  // 
  uint64
  ChapProc::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= codecID_.load(storage, bytesToRead);
    bytesToRead -= procPrivate_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(cmds_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // ChapAtom::eval
  // 
  bool
  ChapAtom::eval(IElementCrawler & crawler)
  {
    return
      UID_.eval(crawler) ||
      timeStart_.eval(crawler) ||
      timeEnd_.eval(crawler) ||
      hidden_.eval(crawler) ||
      enabled_.eval(crawler) ||
      segUID_.eval(crawler) ||
      segEditionUID_.eval(crawler) ||
      physEquiv_.eval(crawler) ||
      tracks_.eval(crawler) ||
      eltsEval(display_, crawler) ||
      eltsEval(process_, crawler);
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
  ChapAtom::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += UID_.save(storage);
    *receipt += timeStart_.save(storage);
    *receipt += timeEnd_.save(storage);
    *receipt += hidden_.save(storage);
    *receipt += enabled_.save(storage);
    *receipt += segUID_.save(storage);
    *receipt += segEditionUID_.save(storage);
    *receipt += physEquiv_.save(storage);
    *receipt += tracks_.save(storage);
    
    *receipt += eltsSave(display_, storage);
    *receipt += eltsSave(process_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // ChapAtom::load
  // 
  uint64
  ChapAtom::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= UID_.load(storage, bytesToRead);
    bytesToRead -= timeStart_.load(storage, bytesToRead);
    bytesToRead -= timeEnd_.load(storage, bytesToRead);
    bytesToRead -= hidden_.load(storage, bytesToRead);
    bytesToRead -= enabled_.load(storage, bytesToRead);
    bytesToRead -= segUID_.load(storage, bytesToRead);
    bytesToRead -= segEditionUID_.load(storage, bytesToRead);
    bytesToRead -= physEquiv_.load(storage, bytesToRead);
    bytesToRead -= tracks_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(display_, storage, bytesToRead);
    bytesToRead -= eltsLoad(process_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Edition::eval
  // 
  bool
  Edition::eval(IElementCrawler & crawler)
  {
    return
      UID_.eval(crawler) ||
      flagHidden_.eval(crawler) ||
      flagDefault_.eval(crawler) ||
      flagOrdered_.eval(crawler) ||
      eltsEval(chapAtoms_, crawler);
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
  Edition::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += UID_.save(storage);
    *receipt += flagHidden_.save(storage);
    *receipt += flagDefault_.save(storage);
    *receipt += flagOrdered_.save(storage);
    
    *receipt += eltsSave(chapAtoms_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Edition::load
  // 
  uint64
  Edition::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= UID_.load(storage, bytesToRead);
    bytesToRead -= flagHidden_.load(storage, bytesToRead);
    bytesToRead -= flagDefault_.load(storage, bytesToRead);
    bytesToRead -= flagOrdered_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(chapAtoms_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Chapters::eval
  // 
  bool
  Chapters::eval(IElementCrawler & crawler)
  {
    return eltsEval(editions_, crawler);
  }
  
  //----------------------------------------------------------------
  // Chapters::isDefault
  // 
  bool
  Chapters::isDefault() const
  {
    bool allDefault = editions_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Chapters::calcSize
  // 
  uint64
  Chapters::calcSize() const
  {
    uint64 size = eltsCalcSize(editions_);
    return size;
  }
  
  //----------------------------------------------------------------
  // Chapters::save
  // 
  IStorage::IReceiptPtr
  Chapters::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(editions_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Chapters::load
  // 
  uint64
  Chapters::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(editions_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  

  //----------------------------------------------------------------
  // TagTargets::eval
  // 
  bool
  TagTargets::eval(IElementCrawler & crawler)
  {
    return
      typeValue_.eval(crawler) ||
      type_.eval(crawler) ||
      eltsEval(trackUIDs_, crawler) ||
      eltsEval(editionUIDs_, crawler) ||
      eltsEval(chapterUIDs_, crawler) ||
      eltsEval(attachmentUIDs_, crawler);
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
  TagTargets::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += typeValue_.save(storage);
    *receipt += type_.save(storage);
    
    *receipt += eltsSave(trackUIDs_, storage);
    *receipt += eltsSave(editionUIDs_, storage);
    *receipt += eltsSave(chapterUIDs_, storage);
    *receipt += eltsSave(attachmentUIDs_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // TagTargets::load
  // 
  uint64
  TagTargets::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= typeValue_.load(storage, bytesToRead);
    bytesToRead -= type_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(trackUIDs_, storage, bytesToRead);
    bytesToRead -= eltsLoad(editionUIDs_, storage, bytesToRead);
    bytesToRead -= eltsLoad(chapterUIDs_, storage, bytesToRead);
    bytesToRead -= eltsLoad(attachmentUIDs_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // SimpleTag::eval
  // 
  bool
  SimpleTag::eval(IElementCrawler & crawler)
  {
    return
      name_.eval(crawler) ||
      lang_.eval(crawler) ||
      default_.eval(crawler) ||
      string_.eval(crawler) ||
      binary_.eval(crawler);
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
  SimpleTag::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += name_.save(storage);
    *receipt += lang_.save(storage);
    *receipt += default_.save(storage);
    *receipt += string_.save(storage);
    *receipt += binary_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SimpleTag::load
  // 
  uint64
  SimpleTag::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= name_.load(storage, bytesToRead);
    bytesToRead -= lang_.load(storage, bytesToRead);
    bytesToRead -= default_.load(storage, bytesToRead);
    bytesToRead -= string_.load(storage, bytesToRead);
    bytesToRead -= binary_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Tag::eval
  // 
  bool
  Tag::eval(IElementCrawler & crawler)
  {
    return
      targets_.eval(crawler) ||
      eltsEval(simpleTags_, crawler);
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
  Tag::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += targets_.save(storage);
    *receipt += eltsSave(simpleTags_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Tag::load
  // 
  uint64
  Tag::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= targets_.load(storage, bytesToRead);
    bytesToRead -= eltsLoad(simpleTags_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Tags::eval
  // 
  bool
  Tags::eval(IElementCrawler & crawler)
  {
    return eltsEval(tags_, crawler);
  }
    
  //----------------------------------------------------------------
  // Tags::isDefault
  // 
  bool
  Tags::isDefault() const
  {
    bool allDefault = tags_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // Tags::calcSize
  // 
  uint64
  Tags::calcSize() const
  {
    uint64 size = eltsCalcSize(tags_);
    return size;
  }
  
  //----------------------------------------------------------------
  // Tags::save
  // 
  IStorage::IReceiptPtr
  Tags::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(tags_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Tags::load
  // 
  uint64
  Tags::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(tags_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // SilentTracks::eval
  // 
  bool
  SilentTracks::eval(IElementCrawler & crawler)
  {
    return eltsEval(tracks_, crawler);
  }
  
  //----------------------------------------------------------------
  // SilentTracks::isDefault
  // 
  bool
  SilentTracks::isDefault() const
  {
    bool allDefault = tracks_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::calcSize
  // 
  uint64
  SilentTracks::calcSize() const
  {
    uint64 size = eltsCalcSize(tracks_);
    return size;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::save
  // 
  IStorage::IReceiptPtr
  SilentTracks::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(tracks_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // SilentTracks::load
  // 
  uint64
  SilentTracks::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(tracks_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // BlockMore::eval
  // 
  bool
  BlockMore::eval(IElementCrawler & crawler)
  {
    return
      blockAddID_.eval(crawler) ||
      blockAdditional_.eval(crawler);
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
  BlockMore::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += blockAddID_.save(storage);
    *receipt += blockAdditional_.save(storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockMore::load
  // 
  uint64
  BlockMore::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= blockAddID_.load(storage, bytesToRead);
    bytesToRead -= blockAdditional_.load(storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // BlockAdditions::eval
  // 
  bool
  BlockAdditions::eval(IElementCrawler & crawler)
  {
    return eltsEval(more_, crawler);
  }
  
  //----------------------------------------------------------------
  // BlockAdditions::isDefault
  // 
  bool
  BlockAdditions::isDefault() const
  {
    bool allDefault = more_.empty();
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // BlockAdditions::calcSize
  // 
  uint64
  BlockAdditions::calcSize() const
  {
    uint64 size = eltsCalcSize(more_);
    return size;
  }

  //----------------------------------------------------------------
  // BlockAdditions::save
  // 
  IStorage::IReceiptPtr
  BlockAdditions::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += eltsSave(more_, storage);
    
    return receipt;
  }

  //----------------------------------------------------------------
  // BlockAdditions::load
  // 
  uint64
  BlockAdditions::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= eltsLoad(more_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // BlockGroup::eval
  // 
  bool
  BlockGroup::eval(IElementCrawler & crawler)
  {
    return
      duration_.eval(crawler) ||
      block_.eval(crawler) ||
      eltsEval(blockVirtual_, crawler) ||
      additions_.eval(crawler) ||
      refPriority_.eval(crawler) ||
      eltsEval(refBlock_, crawler) ||
      refVirtual_.eval(crawler) ||
      codecState_.eval(crawler) ||
      eltsEval(slices_, crawler);
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
  BlockGroup::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += duration_.save(storage);
    *receipt += block_.save(storage);
    *receipt += eltsSave(blockVirtual_, storage);
    *receipt += additions_.save(storage);
    *receipt += refPriority_.save(storage);
    *receipt += eltsSave(refBlock_, storage);
    *receipt += refVirtual_.save(storage);
    *receipt += codecState_.save(storage);
    *receipt += eltsSave(slices_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // BlockGroup::load
  // 
  uint64
  BlockGroup::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= duration_.load(storage, bytesToRead);
    bytesToRead -= block_.load(storage, bytesToRead);
    bytesToRead -= eltsLoad(blockVirtual_, storage, bytesToRead);
    bytesToRead -= additions_.load(storage, bytesToRead);
    bytesToRead -= refPriority_.load(storage, bytesToRead);
    bytesToRead -= eltsLoad(refBlock_, storage, bytesToRead);
    bytesToRead -= refVirtual_.load(storage, bytesToRead);
    bytesToRead -= codecState_.load(storage, bytesToRead);
    bytesToRead -= eltsLoad(slices_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  

  //----------------------------------------------------------------
  // SimpleBlock::SimpleBlock
  // 
  SimpleBlock::SimpleBlock():
    autoLacing_(false),
    trackNumber_(0),
    timeCode_(0),
    flags_(0)
  {}
  
  //----------------------------------------------------------------
  // SimpleBlock::getTrackNumber
  // 
  uint64
  SimpleBlock::getTrackNumber() const
  {
    return trackNumber_;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setTrackNumber
  // 
  void
  SimpleBlock::setTrackNumber(uint64 trackNumber)
  {
    trackNumber_ = trackNumber;
  }

  //----------------------------------------------------------------
  // SimpleBlock::getRelativeTimecode
  // 
  short int
  SimpleBlock::getRelativeTimecode() const
  {
    return timeCode_;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setRelativeTimecode
  // 
  void
  SimpleBlock::setRelativeTimecode(short int timeCode)
  {
    timeCode_ = timeCode;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::isKeyframe
  // 
  bool
  SimpleBlock::isKeyframe() const
  {
    bool f = (flags_ & kFlagKeyframe) != 0;
    return f;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setKeyframe
  // 
  void
  SimpleBlock::setKeyframe(bool keyframe)
  {
    if (keyframe)
    {
      flags_ |= kFlagKeyframe;
    }
    else
    {
      flags_ &= (0xFF & ~kFlagKeyframe);
    }
    
    // sanity check:
    assert(keyframe == isKeyframe());
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::isInvisible
  // 
  bool
  SimpleBlock::isInvisible() const
  {
    bool f = (flags_ & kFlagFrameInvisible) != 0;
    return f;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setInvisible
  // 
  void
  SimpleBlock::setInvisible(bool invisible)
  {
    if (invisible)
    {
      flags_ |= kFlagFrameInvisible;
    }
    else
    {
      flags_ &= (0xFF & ~kFlagFrameInvisible);
    }
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::isDiscardable
  // 
  bool
  SimpleBlock::isDiscardable() const
  {
    bool f = flags_ & kFlagFrameDiscardable;
    return f;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setDiscardable
  // 
  void
  SimpleBlock::setDiscardable(bool discardable)
  {
    if (discardable)
    {
      flags_ |= kFlagFrameDiscardable;
    }
    else
    {
      flags_ &= (0xFF & ~kFlagFrameDiscardable);
    }
    
    // sanity check:
    assert(discardable == isDiscardable());
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::getLacing
  // 
  SimpleBlock::Lacing
  SimpleBlock::getLacing() const
  {
    // extract lacing flags:
    Lacing lacing = (Lacing)((flags_ & kFlagLacingEBML) >> 1);
    return lacing;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setLacing
  // 
  void
  SimpleBlock::setLacing(Lacing lacing)
  {
    // clear previous lacing flags:
    autoLacing_ = false;
    flags_ = setLacingBits(flags_, lacing);
    
    // sanity check:
    assert(lacing == getLacing());
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setAutoLacing
  // 
  void
  SimpleBlock::setAutoLacing()
  {
    autoLacing_ = true;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::getNumberOfFrames
  // 
  std::size_t
  SimpleBlock::getNumberOfFrames() const
  {
    return frames_.size();
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::getFrame
  // 
  const Bytes &
  SimpleBlock::getFrame(std::size_t frameNumber) const
  {
    return frames_[frameNumber];
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::addFrame
  // 
  void
  SimpleBlock::addFrame(const Bytes & frame)
  {
    frames_.push_back(frame);
  }
  
  //----------------------------------------------------------------
  // sameSize
  // 
  static bool
  sameSize(const std::deque<Bytes> & frames)
  {
    if (frames.empty())
    {
      return true;
    }
    
    const std::size_t frameSize = frames[0].size();
    const std::size_t numFrames = frames.size();
    for (std::size_t i = 1; i < numFrames; i++)
    {
      if (frames[i].size() != frameSize)
      {
        return false;
      }
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::exportData
  // 
  void
  SimpleBlock::exportData(Bytes & simpleBlock) const
  {
    simpleBlock = Bytes();
    simpleBlock << vsizeEncode(trackNumber_)
                << intEncode(timeCode_, 2);
    
    std::size_t lastFrameIndex = getNumberOfFrames() - 1;
    Lacing lacing = getLacing();

    Bytes laceXiph;
    Bytes laceEBML;
    
    // choose the best lacing method:
    if (autoLacing_)
    {
      if (lastFrameIndex == 0)
      {
        // 1 frame -- no lacing:
        lacing = kLacingNone;
      }
      else if (sameSize(frames_))
      {
        // all frames have the same size, use fixed size lacing:
        lacing = kLacingFixedSize;
      }
      else
      {
        // try Xiph lacing
        {
          for (std::size_t i = 0; i < lastFrameIndex; i++)
          {
            uint64 frameSize = frames_[i].size();
            while (true)
            {
              TByte sz = frameSize < 0xFF ? TByte(frameSize) : 0xFF;
              frameSize -= sz;
              
              laceXiph << sz;
              if (sz < 0xFF)
              {
                break;
              }
            }
          }
        }
        
        // try EBML lacing
        {
          uint64 frameSize = frames_[0].size();
          laceEBML << vsizeEncode(frameSize);
          
          for (std::size_t i = 1; i < lastFrameIndex; i++)
          {
            int64 frameSizeDiff = (int64(frames_[i].size()) - 
                                   int64(frames_[i - 1].size()));
            laceEBML << vsizeSignedEncode(frameSizeDiff);
          }
        }
        
        // choose the one with lowest overhead:
        if (laceXiph.size() < laceEBML.size())
        {
          lacing = kLacingXiph;
        }
        else
        {
          lacing = kLacingEBML;
        }
      }
    }
    
    // save the flag with correct lacing bits set:
    unsigned char flags = setLacingBits(flags_, lacing);
    simpleBlock << flags;
    
    if (lacing != kLacingNone)
    {
      simpleBlock << TByte(lastFrameIndex);
    }
    
    if (lacing == kLacingXiph)
    {
      simpleBlock << laceXiph;
    }
    else if (lacing == kLacingEBML)
    {
      simpleBlock << laceEBML;
    }
    
    for (std::size_t i = 0; i <= lastFrameIndex; i++)
    {
      simpleBlock << Bytes(frames_[i]);
    }
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::importData
  // 
  bool
  SimpleBlock::importData(const Bytes & simpleBlock)
  {
    const uint64 blockSize = simpleBlock.size();
    
    uint64 bytesRead = 0;
    trackNumber_ = vsizeDecode(simpleBlock, bytesRead);
    if (bytesRead == 0)
    {
      return false;
    }
    
    if (blockSize - bytesRead < 3)
    {
      // need 2 more bytes for timecode, and 1 more for flags:
      return false;
    }
    
    // convert to contiguous memory to simplify parsing:
    TByteVec blockByteVec(simpleBlock);
    const TByte * blockData = &(blockByteVec[0]);
    
    // decode the timecode:
    TByteVec timecode(2);
    timecode[0] = blockData[bytesRead++];
    timecode[1] = blockData[bytesRead++];
    timeCode_ = (short int)(intDecode(timecode, 2));
    
    // get the flags:
    autoLacing_ = false;
    flags_ = blockData[bytesRead++];
    
    // get the number of frames:
    std::size_t lastFrameIndex = 0;
    
    Lacing lacing = getLacing();
    if (lacing != kLacingNone)
    {
      if (blockSize - bytesRead < 1)
      {
        return false;
      }
      
      lastFrameIndex = blockData[bytesRead++];
    }
    
    // unpack the frame(s):
    frames_.resize(lastFrameIndex + 1);
    uint64 leadingFramesSize = 0;
    
    if (lacing == kLacingXiph)
    {
      for (std::size_t i = 0; i < lastFrameIndex; i++)
      {
        uint64 frameSize = 0;
        while (true)
        {
          if (bytesRead > blockSize)
          {
            return false;
          }
          
          uint64 n = blockData[bytesRead++];
          frameSize += n;
          
          if (n < 0xFF)
          {
            break;
          }
        }
        
        frames_[i] = Bytes((std::size_t)frameSize);
        leadingFramesSize += frameSize;
      }
    }
    else if (lacing == kLacingEBML)
    {
      uint64 vsizeSize = 0;
      uint64 frameSize = vsizeDecode(&blockData[bytesRead],
                                      vsizeSize);
      bytesRead += vsizeSize;
      frames_[0] = Bytes((std::size_t)frameSize);
      leadingFramesSize += frameSize;
      
      for (std::size_t i = 1; i < lastFrameIndex; i++)
      {
        int64 frameSizeDiff = vsizeSignedDecode(&blockData[bytesRead],
                                                vsizeSize);
        bytesRead += vsizeSize;
        frameSize += frameSizeDiff;
        frames_[i] = Bytes((std::size_t)frameSize);
        leadingFramesSize += frameSize;
      }
    }
    else if (lacing == kLacingFixedSize)
    {
      uint64 numFrames = lastFrameIndex + 1;
      uint64 frameSize = (blockSize - bytesRead) / numFrames;
      
      for (std::size_t i = 0; i < lastFrameIndex; i++)
      {
        frames_[i] = Bytes((std::size_t)frameSize);
        leadingFramesSize += frameSize;
      }
    }
    
    // last frame:
    uint64 lastFrameSize = (blockSize -
                            bytesRead -
                            leadingFramesSize);
    frames_[lastFrameIndex] = Bytes((std::size_t)lastFrameSize);
    
    // load the frames:
    for (std::size_t i = 0; i <= lastFrameIndex; i++)
    {
      std::size_t numBytes = frames_[i].size();
      frames_[i].deepCopy(&blockData[bytesRead], numBytes);
      bytesRead += numBytes;
    }
    
    // sanity check:
    assert(bytesRead == blockSize);
    
    return true;
  }
  
  //----------------------------------------------------------------
  // SimpleBlock::setLacingBits
  // 
  unsigned char
  SimpleBlock::setLacingBits(unsigned char flags,
                             SimpleBlock::Lacing lacing)
  {
    flags &= (0xFF & ~kFlagLacingEBML);
    flags |= lacing << 1;
    return flags;
  }
  
  //----------------------------------------------------------------
  // operator <<
  // 
  std::ostream &
  operator << (std::ostream & os, const SimpleBlock & sb)
  {
    os << "track " << sb.getTrackNumber()
       << ", ltc " << sb.getRelativeTimecode();
    
    if (sb.isKeyframe())
    {
      os << ", keyframe";
    }
    
    if (sb.isInvisible())
    {
      os << ", invisible";
    }
    
    if (sb.isDiscardable())
    {
      os << ", discardable";
    }
    
    std::size_t numFrames = sb.getNumberOfFrames();
    os << ", " << numFrames << " frame(s)";
    
    SimpleBlock::Lacing lacing = sb.getLacing();
    if (lacing == SimpleBlock::kLacingXiph)
    {
      os << ", Xiph lacing";
    }
    else if (lacing == SimpleBlock::kLacingFixedSize)
    {
      os << ", fixed size lacing";
    }
    else if (lacing == SimpleBlock::kLacingEBML)
    {
      os << ", EBML lacing";
    }
    
    for (std::size_t j = 0; j < numFrames; j++)
    {
      const Bytes & frame = sb.getFrame(j);
      os << ", f[" << j << "] "
         << frame.size() << " bytes";
    }
    
    return os;
  }
  
  
  //----------------------------------------------------------------
  // Cluster::Cluster
  // 
  Cluster::Cluster()
  {
    timecode_.alwaysSave();
  }
  
  //----------------------------------------------------------------
  // Cluster::eval
  // 
  bool
  Cluster::eval(IElementCrawler & crawler)
  {
    return
      timecode_.eval(crawler) ||
      silent_.eval(crawler) ||
      position_.eval(crawler) ||
      prevSize_.eval(crawler) ||
      eltsEval(blockGroups_, crawler) ||
      eltsEval(simpleBlocks_, crawler) ||
      eltsEval(encryptedBlocks_, crawler);
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
  Cluster::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    *receipt += timecode_.save(storage);
    *receipt += silent_.save(storage);
    *receipt += position_.save(storage);
    *receipt += prevSize_.save(storage);
    
    *receipt += eltsSave(blockGroups_, storage);
    *receipt += eltsSave(simpleBlocks_, storage);
    *receipt += eltsSave(encryptedBlocks_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Cluster::load
  // 
  uint64
  Cluster::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= timecode_.load(storage, bytesToRead);
    bytesToRead -= silent_.load(storage, bytesToRead);
    bytesToRead -= position_.load(storage, bytesToRead);
    bytesToRead -= prevSize_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(blockGroups_, storage, bytesToRead);
    bytesToRead -= eltsLoad(simpleBlocks_, storage, bytesToRead);
    bytesToRead -= eltsLoad(encryptedBlocks_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // Segment::eval
  // 
  bool
  Segment::eval(IElementCrawler & crawler)
  {
    return
      info_.eval(crawler) ||
      tracks_.eval(crawler) ||
      chapters_.eval(crawler) ||
      cues_.eval(crawler) ||
      
      eltsEval(seekHeads_, crawler) ||
      eltsEval(attachments_, crawler) ||
      eltsEval(tags_, crawler) ||
      eltsEval(clusters_, crawler);
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
      !cues_.mustSave() &&
      
      seekHeads_.empty() &&
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
      cues_.calcSize() +
      
      eltsCalcSize(seekHeads_) +
      eltsCalcSize(attachments_) +
      eltsCalcSize(tags_) +
      eltsCalcSize(clusters_);
    
    return size;
  }

  //----------------------------------------------------------------
  // Segment::save
  // 
  // Save using conventional matroska segment layout
  // 
  IStorage::IReceiptPtr
  Segment::save(IStorage & storage) const
  {
    if (delegateSave_)
    {
      // let the delegate handle saving this segment:
      return delegateSave_->save(*this, storage);
    }
    
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    typedef std::list<TSeekHead>::const_iterator TSeekHeadIter;
    TSeekHeadIter seekHeadIter = seekHeads_.begin();
    
    // save the first seekhead:
    if (seekHeadIter != seekHeads_.end())
    {
      const TSeekHead & seekHead = *seekHeadIter;
      *receipt += seekHead.save(storage);
      ++seekHeadIter;
    }

    *receipt += info_.save(storage);
    *receipt += tracks_.save(storage);
    *receipt += chapters_.save(storage);
    
    *receipt += eltsSave(clusters_, storage);
    
    // save any remaining seekheads:
    for (; seekHeadIter != seekHeads_.end(); ++seekHeadIter)
    {
      const TSeekHead & seekHead = *seekHeadIter;
      *receipt += seekHead.save(storage);
    }
    
    *receipt += cues_.save(storage);
    
    *receipt += eltsSave(attachments_, storage);
    *receipt += eltsSave(tags_, storage);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // Segment::load
  // 
  uint64
  Segment::load(FileStorage & storage, uint64 bytesToRead)
  {
    uint64 prevBytesToRead = bytesToRead;
    
    bytesToRead -= info_.load(storage, bytesToRead);
    bytesToRead -= tracks_.load(storage, bytesToRead);
    bytesToRead -= chapters_.load(storage, bytesToRead);
    bytesToRead -= cues_.load(storage, bytesToRead);
      
    bytesToRead -= eltsLoad(seekHeads_, storage, bytesToRead);
    bytesToRead -= eltsLoad(attachments_, storage, bytesToRead);
    bytesToRead -= eltsLoad(tags_, storage, bytesToRead);
    bytesToRead -= eltsLoad(clusters_, storage, bytesToRead);
    
    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // Segment::resolveReferences
  // 
  void
  Segment::resolveReferences(const IElement * origin)
  {
    // shortcuts:
    typedef std::list<TSeekHead>::iterator TSeekHeadIter;
    typedef std::list<TCues>::iterator TCuesIter;
    typedef std::list<TCluster>::iterator TClusterIter;
    
    typedef SeekHead::TSeekEntry TSeekEntry;
    typedef std::list<TSeekEntry>::iterator TSeekEntryIter;
    
    typedef Cues::TCuePoint TCuePoint;
    typedef std::list<TCuePoint>::iterator TCuePointIter;
    
    typedef CuePoint::TCueTrkPos TCueTrkPos;
    typedef std::list<TCueTrkPos>::iterator TCueTrkPosIter;
    
    if (!origin)
    {
      return;
    }
    
    IStorage::IReceiptPtr originReceipt = origin->payloadReceipt();
    if (!originReceipt)
    {
      return;
    }
    
    // get the payload position:
    uint64 originPosition = originReceipt->position();
    
    // resolve seek position references:
    for (TSeekHeadIter i = seekHeads_.begin(); i != seekHeads_.end(); ++i)
    {
      TSeekHead & seekHead = *i;
      std::list<TSeekEntry> & seeks = seekHead.payload_.seek_;
      
      for (TSeekEntryIter j = seeks.begin(); j != seeks.end(); ++j)
      {
        TSeekEntry & seek = *j;
        
        VEltPosition & eltReference = seek.payload_.position_.payload_;
        eltReference.setOrigin(origin);
        
        Bytes eltIdBytes;
        if (!seek.payload_.id_.payload_.get(eltIdBytes))
        {
          continue;
        }
        
        uint64 eltId = uintDecode(eltIdBytes, eltIdBytes.size());
        uint64 relativePosition = eltReference.position();
        uint64 absolutePosition = originPosition + relativePosition;
        
        if (eltId == TInfo::kId)
        {
          eltReference.setElt(&info_);
        }
        else if (eltId == TTracks::kId)
        {
          eltReference.setElt(&tracks_);
        }
        else if (eltId == TSeekHead::kId)
        {
          const TSeekHead * ref = eltsFind(seekHeads_, absolutePosition);
          eltReference.setElt(ref);
        }
        else if (eltId == TCues::kId)
        {
          eltReference.setElt(&cues_);
        }
        else if (eltId == TAttachment::kId)
        {
          const TAttachment * ref = eltsFind(attachments_, absolutePosition);
          eltReference.setElt(ref);
        }
        else if (eltId == TChapters::kId)
        {
          eltReference.setElt(&chapters_);
        }
        else if (eltId == TTag::kId)
        {
          const TTag * ref = eltsFind(tags_, absolutePosition);
          eltReference.setElt(ref);
        }
        else if (eltId == TCluster::kId)
        {
          const TCluster * ref = eltsFind(clusters_, absolutePosition);
          eltReference.setElt(ref);
        }
      }
    }
    
    // resolve cue track position references:
    std::list<TCuePoint> & cuePoints = cues_.payload_.points_;
    for (TCuePointIter i = cuePoints.begin(); i != cuePoints.end(); ++i)
    {
      TCuePoint & cuePoint = *i;
      std::list<TCueTrkPos> & cueTrkPns = cuePoint.payload_.trkPosns_;
      
      for (TCueTrkPosIter j = cueTrkPns.begin(); j != cueTrkPns.end(); ++j)
      {
        TCueTrkPos & cueTrkPos = *j;
        VEltPosition & clusterRef = cueTrkPos.payload_.cluster_.payload_;
        
        uint64 relativePosition = clusterRef.position();
        uint64 absolutePosition = originPosition + relativePosition;
        
        const TCluster * cluster = eltsFind(clusters_, absolutePosition);
        clusterRef.setElt(cluster);
        clusterRef.setOrigin(origin);
      }
    }
    
    // resolve cluster position references:
    for (TClusterIter i = clusters_.begin(); i != clusters_.end(); ++i)
    {
      TCluster & cluster = *i;
      
      VEltPosition & clusterRef = cluster.payload_.position_.payload_;
      clusterRef.setElt(&cluster);
      clusterRef.setOrigin(origin);
    }
  }
  
  //----------------------------------------------------------------
  // Segment::setCrc32
  // 
  void
  Segment::setCrc32(bool enableCrc32)
  {
    info_.setCrc32(enableCrc32);
    tracks_.setCrc32(enableCrc32);
    chapters_.setCrc32(enableCrc32);
    cues_.setCrc32(enableCrc32);
    
    eltsSetCrc32(seekHeads_, enableCrc32);
    eltsSetCrc32(clusters_, enableCrc32);
    eltsSetCrc32(attachments_, enableCrc32);
    eltsSetCrc32(tags_, enableCrc32);
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::MatroskaDoc
  // 
  MatroskaDoc::MatroskaDoc():
    EbmlDoc("matroska", 1, 1)
  {}
  
  //----------------------------------------------------------------
  // MatroskaDoc::eval
  // 
  bool
  MatroskaDoc::eval(IElementCrawler & crawler)
  {
    return
      EbmlDoc::head_.eval(crawler) ||
      eltsEval(segments_, crawler);
  }
  
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
  // RewriteReferences
  // 
  struct RewriteReferences : public IElementCrawler
  {
    // virtual:
    bool evalElement(IElement & elt)
    { return false; }
    
    // virtual:
    bool evalPayload(IPayload & payload)
    {
      VEltPosition * eltRef = dynamic_cast<VEltPosition *>(&payload);
      if (eltRef)
      {
        eltRef->rewrite();
      }
      
      return payload.eval(*this);
    }
  };
  
  //----------------------------------------------------------------
  // ReplaceCrc32Placeholders
  // 
  struct ReplaceCrc32Placeholders : public IElementCrawler
  {
    // virtual:
    bool evalElement(IElement & elt)
    {
      IStorage::IReceiptPtr receiptCrc32 = elt.crc32Receipt();
      if (receiptCrc32)
      {
        IStorage::IReceiptPtr receiptPayload = elt.payloadReceipt();
        
        // calculate and save CRC-32 checksum:
        Crc32 crc32;
        receiptPayload->calcCrc32(crc32, receiptCrc32);
        unsigned int checksumCrc32 = crc32.checksum();
        
        Bytes bytesCrc32;
        bytesCrc32 << uintEncode(kIdCrc32)
                   << vsizeEncode(4)
                   << uintEncode(checksumCrc32, 4);
        
        receiptCrc32->save(bytesCrc32);
      }
      
      return false;
    }
    
    // virtual:
    bool evalPayload(IPayload & payload)
    {
      return payload.eval(*this);
    }
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc::save
  // 
  IStorage::IReceiptPtr
  MatroskaDoc::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = EbmlDoc::head_.save(storage);
    
    *receipt += eltsSave(segments_, storage);
    
    // rewrite element position references (second pass):
    {
      RewriteReferences crawler;
      const_cast<MatroskaDoc *>(this)->eval(crawler);
    }
    
    // replace CRC-32 placeholders (final pass):
    {
      ReplaceCrc32Placeholders crawler;
      const_cast<MatroskaDoc *>(this)->eval(crawler);
    }
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // DiscardReceipts
  // 
  struct DiscardReceipts : public IElementCrawler
  {
    // virtual:
    bool evalElement(IElement & elt)
    {
      elt.discardReceipts();
      return false;
    }
    
    // virtual:
    bool evalPayload(IPayload & payload)
    { return payload.eval(*this); }
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc::load
  // 
  uint64
  MatroskaDoc::load(FileStorage & storage, uint64 bytesToRead)
  {
    // let the base class load the EBML header:
    uint64 bytesReadTotal = EbmlDoc::load(storage, bytesToRead);
    bytesToRead -= bytesReadTotal;
    
    // read Segments:
    while (true)
    {
      uint64 prevBytesToRead = bytesToRead;
      
      bytesToRead -= eltsLoad(segments_, storage, bytesToRead);
      
      uint64 bytesRead = prevBytesToRead - bytesToRead;
      bytesReadTotal += bytesRead;
      
      if (!bytesRead)
      {
        break;
      }
    }
    
    // resolve positional references (seeks, cues, clusters, etc...):
    resolveReferences();
    
    // discard loaded storage receipts so they wouldn't screw up saving later:
    DiscardReceipts crawler;
    eval(crawler);
    
    return bytesReadTotal;
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::resolveReferences
  // 
  void
  MatroskaDoc::resolveReferences()
  {
    // shortcut:
    typedef std::list<TSegment>::iterator TSegmentIter;
    
    for (TSegmentIter i = segments_.begin(); i != segments_.end(); ++i)
    {
      TSegment & segment = *i;
      segment.payload_.resolveReferences(&segment);
    }
  }
  
  //----------------------------------------------------------------
  // MatroskaDoc::setCrc32
  // 
  void
  MatroskaDoc::setCrc32(bool enableCrc32)
  {
    eltsSetCrc32(segments_, enableCrc32);
  }
  
  //----------------------------------------------------------------
  // Optimizer
  // 
  struct Optimizer : public IElementCrawler
  {
    // helper:
    template <typename block_t>
    void
    optimizeBlock(block_t & block)
    {
      Bytes blockBytes;
      block.payload_.get(blockBytes);
      const uint64 sizeBefore = blockBytes.size();
      
      SimpleBlock simpleBlock;
      simpleBlock.importData(blockBytes);
      simpleBlock.setAutoLacing();
      simpleBlock.exportData(blockBytes);
      const uint64 sizeAfter = blockBytes.size();
      
      if (sizeAfter < sizeBefore)
      {
        block.payload_.receipt_->save(blockBytes);
        block.payload_.receipt_->setNumBytes(blockBytes.size());
      }
      else if (sizeAfter > sizeBefore)
      {
        // auto-lacing is only supposed to make things better, not worse:
        assert(false);
      }
    }
    
    // virtual:
    bool evalElement(IElement & elt)
    {
      // remove CRC-32 element:
      elt.setCrc32(false);
      
      if (elt.getId() == Segment::TCluster::kId)
      {
        Segment::TCluster & cluster = dynamic_cast<Segment::TCluster &>(elt);
        
        // remove cluster position element:
        cluster.payload_.position_.payload_.setElt(NULL);
        cluster.payload_.position_.payload_.setOrigin(NULL);
      }
      else if (elt.getId() == Cluster::TSimpleBlock::kId)
      {
        Cluster::TSimpleBlock & block =
          dynamic_cast<Cluster::TSimpleBlock &>(elt);
        optimizeBlock(block);
      }
      else if (elt.getId() == BlockGroup::TBlock::kId)
      {
        BlockGroup::TBlock & block =
          dynamic_cast<BlockGroup::TBlock &>(elt);
        optimizeBlock(block);
      }
      
      return false;
    }
    
    // virtual:
    bool evalPayload(IPayload & payload)
    {
      EbmlMaster * container = dynamic_cast<EbmlMaster *>(&payload);
      if (container)
      {
        // remove all void elements:
        container->voids_.clear();
      }
      
      return payload.eval(*this);
    }
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc::optimize
  // 
  void
  MatroskaDoc::optimize()
  {
    // doc.setCrc32(false);
    Optimizer optimizer;
    eval(optimizer);
  }
  
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:56:33 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_MATROSKA_H_
#define YAMKA_MATROSKA_H_

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaEBML.h>

// system includes:
#include <deque>


namespace Yamka
{

  //----------------------------------------------------------------
  // ChapTranslate
  // 
  struct ChapTranslate : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x69FC, "EditionUID") TEditionUID;
    TEditionUID editionUID_;
    
    TypedefYamkaElt(VUInt, 0x69BF, "ChapTransCodec") TChapTransCodec;
    TChapTransCodec chapTransCodec_;
    
    TypedefYamkaElt(VBinary, 0x69A5, "ChapTransID") TChapTransID;
    TChapTransID chapTransID_;
  };
  
  //----------------------------------------------------------------
  // SegInfo
  // 
  struct SegInfo : public EbmlMaster
  {
    SegInfo();
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VBytes<16>, 0x73A4, "SegmentUID") TSegUID;
    TSegUID segUID_;
    
    TypedefYamkaElt(VString, 0x7384, "SegmentFilename") TSegFilename;
    TSegFilename segFilename_;
    
    TypedefYamkaElt(VBytes<16>, 0x3CB923, "PrevUID") TPrevUID;
    TPrevUID prevUID_;
    
    TypedefYamkaElt(VString, 0x3C83AB, "PrevFilename") TPrevFilename;
    TPrevFilename prevFilename_;
    
    TypedefYamkaElt(VBytes<16>, 0x3EB923, "NextUID") TNextUID;
    TNextUID nextUID_;
    
    TypedefYamkaElt(VString, 0x3E83BB, "NextFilename") TNextFilename;
    TNextFilename nextFilename_;
    
    TypedefYamkaElt(VBytes<16>, 0x4444, "FamilyUID") TFamilyUID;
    TFamilyUID familyUID_;
    
    TypedefYamkaElt(ChapTranslate, 0x6924, "ChapTranslate") TChapTranslate;
    TChapTranslate chapTranslate_;
    
    TypedefYamkaElt(VUInt, 0x2AD7B1, "TimecodeScale") TTimecodeScale;
    TTimecodeScale timecodeScale_;
    
    TypedefYamkaElt(VFloat, 0x4489, "Duration") TDuration;
    TDuration duration_;
    
    TypedefYamkaElt(VDate, 0x4461, "DateUTC") TDate;
    TDate date_;
    
    TypedefYamkaElt(VString, 0x7BA9, "Title") TTitle;
    TTitle title_;
    
    TypedefYamkaElt(VString, 0x4D80, "MuxingApp") TMuxingApp;
    TMuxingApp muxingApp_;
    
    TypedefYamkaElt(VString, 0x5741, "WritingApp") TWritingApp;
    TWritingApp writingApp_;
  };
  
  //----------------------------------------------------------------
  // TrackTranslate
  // 
  struct TrackTranslate : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x66FC, "EditionUID") TEditionUID;
    TEditionUID editionUID_;
    
    TypedefYamkaElt(VUInt, 0x66BF, "TrackTransCodec") TTrackTransCodec;
    TTrackTransCodec trackTransCodec_;
    
    TypedefYamkaElt(VBinary, 0x66A5, "TrackTransID") TTrackTransID;
    TTrackTransID trackTransID_;
  };
  
  //----------------------------------------------------------------
  // Video
  // 
  struct Video : public EbmlMaster
  {
    Video();
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x9A, "FlagInterlaced") TFlagInterlaced;
    TFlagInterlaced flagInterlaced_;
    
    TypedefYamkaElt(VUInt, 0x53B8, "StereoMode") TStereoMode;
    TStereoMode stereoMode_;
    
    TypedefYamkaElt(VUInt, 0xB0, "PixelWidth") TPixelWidth;
    TPixelWidth pixelWidth_;
    
    TypedefYamkaElt(VUInt, 0xBA, "PixelHeight") TPixelHeight;
    TPixelHeight pixelHeight_;
    
    TypedefYamkaElt(VUInt, 0x54AA, "PixelCropBottom") TPixelCropBottom;
    TPixelCropBottom pixelCropBottom_;
    
    TypedefYamkaElt(VUInt, 0x54BB, "PixelCropTop") TPixelCropTop;
    TPixelCropTop pixelCropTop_;
    
    TypedefYamkaElt(VUInt, 0x54CC, "PixelCropLeft") TPixelCropLeft;
    TPixelCropLeft pixelCropLeft_;
    
    TypedefYamkaElt(VUInt, 0x54DD, "PixelCropRight") TPixelCropRight;
    TPixelCropRight pixelCropRight_;
    
    TypedefYamkaElt(VUInt, 0x54B0, "DisplayWidth") TDisplayWidth;
    TDisplayWidth displayWidth_;
    
    TypedefYamkaElt(VUInt, 0x54BA, "DisplayHeight") TDisplayHeight;
    TDisplayHeight displayHeight_;
    
    TypedefYamkaElt(VUInt, 0x54B2, "DisplayUnits") TDisplayUnits;
    TDisplayUnits displayUnits_;
    
    TypedefYamkaElt(VUInt, 0x54B3, "AspectRatioType") TAspectRatioType;
    TAspectRatioType aspectRatioType_;
    
    TypedefYamkaElt(VBytes<4>, 0x2EB524, "ColorSpace") TColorSpace;
    TColorSpace colorSpace_;
    
    TypedefYamkaElt(VFloat, 0x2FB523, "GammaValue") TGammaValue;
    TGammaValue gammaValue_;
    
    TypedefYamkaElt(VFloat, 0x2383E3, "FrameRate") TFrameRate;
    TFrameRate frameRate_;
  };
  
  //----------------------------------------------------------------
  // Audio
  // 
  struct Audio : public EbmlMaster
  {
    Audio();
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VFloat, 0xB5, "SamplingFrequency") TSampFreq;
    TSampFreq sampFreq_;
    
    TypedefYamkaElt(VFloat, 0x78B5, "OutputSamplingFrequency") TSampFreqOut;
    TSampFreqOut sampFreqOut_;
    
    TypedefYamkaElt(VUInt, 0x9F, "Channels") TChannels;
    TChannels channels_;
    
    TypedefYamkaElt(VBinary, 0x7D7B, "ChannelPositions") TChannelPositions;
    TChannelPositions channelPositions_;
    
    TypedefYamkaElt(VUInt, 0x6264, "BitDepth") TBitDepth;
    TBitDepth bitDepth_;
  };
  
  //----------------------------------------------------------------
  // ContentCompr
  // 
  struct ContentCompr : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x4254, "ContentCompAlgo") TAlgo;
    TAlgo algo_;
    
    TypedefYamkaElt(VBinary, 0x4255, "ContentCompSettings") TSettings;
    TSettings settings_;
  };
  
  //----------------------------------------------------------------
  // ContentEncrypt
  // 
  struct ContentEncrypt : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x47E1, "ContentEncAlgo") TEncAlgo;
    TEncAlgo encAlgo_;
    
    TypedefYamkaElt(VBinary, 0x47E2, "ContentEncKeyID") TEncKeyID;
    TEncKeyID encKeyID_;
    
    TypedefYamkaElt(VBinary, 0x47E3, "ContentSignature") TSignature;
    TSignature signature_;
    
    TypedefYamkaElt(VBinary, 0x47E4, "ContentSigKeyID") TSigKeyID;
    TSigKeyID sigKeyID_;
    
    TypedefYamkaElt(VUInt, 0x47E5, "ContentSigAlgo") TSigAlgo;
    TSigAlgo sigAlgo_;
    
    TypedefYamkaElt(VUInt, 0x47E6, "ContentSigHashAlgo") TSigHashAlgo;
    TSigHashAlgo sigHashAlgo_;
  };
  
  //----------------------------------------------------------------
  // ContentEnc
  // 
  struct ContentEnc : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x5031, "ContentEncodingOrder") TOrder;
    TOrder order_;
    
    TypedefYamkaElt(VUInt, 0x5032, "ContentEncodingScope") TScope;
    TScope scope_;
    
    TypedefYamkaElt(VUInt, 0x5033, "ContentEncodingType") TType;
    TType type_;
    
    TypedefYamkaElt(ContentCompr, 0x5034, "ContentCompression") TCompression;
    TCompression compression_;
    
    TypedefYamkaElt(ContentEncrypt, 0x5035, "ContentEncryption") TEncryption;
    TEncryption encryption_;
  };
  
  //----------------------------------------------------------------
  // ContentEncodings
  // 
  struct ContentEncodings : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(ContentEnc, 0x6240, "ContentEnc") TEncoding;
    std::deque<TEncoding> encodings_;
  };
  
  //----------------------------------------------------------------
  // Track
  // 
  struct Track : public EbmlMaster
  {
    enum MatroskaTrackType
    {
      kTrackTypeVideo = 1,
      kTrackTypeAudio = 2,
      kTrackTypeComplex = 3,
      kTrackTypeLogo = 0x10,
      kTrackTypeSubtitle = 0x11,
      kTrackTypeButtons = 0x12,
      kTrackTypeControl = 0x20
    };
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0xD7, "TrackNumber") TTrackNumber;
    TTrackNumber trackNumber_;
    
    TypedefYamkaElt(VUInt, 0x73C5, "TrackUID") TTrackUID;
    TTrackUID trackUID_;
    
    TypedefYamkaElt(VUInt, 0x83, "TrackType") TTrackType;
    TTrackType trackType_;
    
    TypedefYamkaElt(VUInt, 0xB9, "FlagEnabled") TFlagEnabled;
    TFlagEnabled flagEnabled_;
    
    TypedefYamkaElt(VUInt, 0x88, "FlagDefault") TFlagDefault;
    TFlagDefault flagDefault_;
    
    TypedefYamkaElt(VUInt, 0x55AA, "FlagForces") TFlagForced;
    TFlagForced flagForced_;
    
    TypedefYamkaElt(VUInt, 0x9C, "FlagLacing") TFlagLacing;
    TFlagLacing flagLacing_;
    
    TypedefYamkaElt(VUInt, 0x6DE7, "MinCache") TMinCache;
    TMinCache minCache_;
    
    TypedefYamkaElt(VUInt, 0x6DF8, "MaxCache") TMaxCache;
    TMaxCache maxCache_;
    
    TypedefYamkaElt(VUInt, 0x23E383, "DefaultDuration") TFrameDuration;
    TFrameDuration frameDuration_;
    
    TypedefYamkaElt(VFloat, 0x23314F, "TrackTimecodeScale") TTimecodeScale;
    TTimecodeScale timecodeScale_;
    
    TypedefYamkaElt(VInt, 0x537F, "TrackOffset") TTrackOffset;
    TTrackOffset trackOffset_;
    
    TypedefYamkaElt(VUInt, 0x55EE, "MaxBlockAddID") TMaxBlockAddID;
    TMaxBlockAddID maxBlockAddID_;
    
    TypedefYamkaElt(VString, 0x536E, "Name") TName;
    TName name_;
    
    TypedefYamkaElt(VString, 0x22B59C, "Language") TLanguage;
    TLanguage language_;
    
    TypedefYamkaElt(VString, 0x86, "CodecID") TCodecID;
    TCodecID codecID_;
    
    TypedefYamkaElt(VBinary, 0x63A2, "CodecPrivate") TCodecPrivate;
    TCodecPrivate codecPrivate_;
    
    TypedefYamkaElt(VString, 0x258688, "CodecName") TCodecName;
    TCodecName codecName_;
    
    TypedefYamkaElt(VUInt, 0x7446, "AttachmentLink") TAttachmentLink;
    TAttachmentLink attachmentLink_;
    
    TypedefYamkaElt(VString, 0x3A9697, "CodecSettings") TCodecSettings;
    TCodecSettings codecSettings_;
    
    TypedefYamkaElt(VString, 0x3B4040, "CodecInfoURL") TCodecInfoURL;
    TCodecInfoURL codecInfoURL_;
    
    TypedefYamkaElt(VString, 0x26B240, "CodecDownloadURL") TCodecDownloadURL;
    TCodecDownloadURL codecDownloadURL_;
    
    TypedefYamkaElt(VUInt, 0xAA, "CodecDecodeAll") TCodecDecodeAll;
    TCodecDecodeAll codecDecodeAll_;
    
    TypedefYamkaElt(VUInt, 0x6FAB, "TrackOverlay") TTrackOverlay;
    TTrackOverlay trackOverlay_;
    
    TypedefYamkaElt(TrackTranslate, 0x6624, "TrackTranslate") TTrackTranslate;
    TTrackTranslate trackTranslate_;
    
    TypedefYamkaElt(Video, 0xE0, "Video") TVideo;
    TVideo video_;
    
    TypedefYamkaElt(Audio, 0xE1, "Audio") TAudio;
    TAudio audio_;
    
    TypedefYamkaElt(ContentEncodings, 0x6D80, "ContentEncodings") TContentEncs;
    TContentEncs contentEncs_;
  };
  
  //----------------------------------------------------------------
  // Tracks
  // 
  struct Tracks : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(Track, 0xAE, "TrackEntry") TTrack;
    std::deque<TTrack> tracks_;
  };
  
  //----------------------------------------------------------------
  // CueRef
  // 
  struct CueRef : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x96, "CueRefTime") TTime;
    TTime time_;
    
    TypedefYamkaElt(VEltPosition, 0x97, "CueRefCluster") TCluster;
    TCluster cluster_;
    
    TypedefYamkaElt(VUInt, 0x535F, "CueRefNumber") TBlock;
    TBlock block_;
    
    TypedefYamkaElt(VEltPosition, 0xEB, "CueRefCodecState") TCodecState;
    TCodecState codecState_;
  };
  
  //----------------------------------------------------------------
  // CueTrkPos
  // 
  struct CueTrkPos : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0xF7, "Track") TTrack;
    TTrack track_;
    
    TypedefYamkaElt(VEltPosition, 0xF1, "ClusterPosition") TCluster;
    TCluster cluster_;
    
    TypedefYamkaElt(VUInt, 0x5378, "CueBlockNumber") TBlock;
    TBlock block_;
    
    TypedefYamkaElt(VEltPosition, 0xEA, "CueCodecState") TCodecState;
    TCodecState codecState_;
    
    TypedefYamkaElt(CueRef, 0xDB, "CueReference") TRef;
    TRef ref_;
  };
  
  //----------------------------------------------------------------
  // CuePoint
  // 
  struct CuePoint : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0xB3, "CueTime") TTime;
    TTime time_;
    
    TypedefYamkaElt(CueTrkPos, 0xB7, "CueTrackPosition") TCueTrkPos;
    std::deque<TCueTrkPos> trkPosns_;
  };
  
  //----------------------------------------------------------------
  // Cues
  // 
  struct Cues : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(CuePoint, 0xBB, "CuePoint") TCuePoint;
    std::deque<TCuePoint> points_;
  };
  
  //----------------------------------------------------------------
  // SeekEntry
  // 
  struct SeekEntry : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VBinary, 0x53AB, "SeekID") TId;
    TId id_;
    
    TypedefYamkaElt(VEltPosition, 0x53AC, "SeekPosition") TPosition;
    TPosition position_;
  };
  
  //----------------------------------------------------------------
  // SeekHead
  // 
  struct SeekHead : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    void indexThis(const IElement * segment,
                   const IElement * element,
                   IStorage & binaryStorage);
    
    TypedefYamkaElt(SeekEntry, 0x4DBB, "Seek") TSeekEntry;
    std::deque<TSeekEntry> seek_;
  };
  
  //----------------------------------------------------------------
  // AttdFile
  // 
  struct AttdFile : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VString, 0x467E, "FileDescription") TDescription;
    TDescription description_;
    
    TypedefYamkaElt(VString, 0x466E, "FileName") TFilename;
    TFilename filename_;
    
    TypedefYamkaElt(VString, 0x4660, "FileMimeType") TMimeType;
    TMimeType mimeType_;
    
    TypedefYamkaElt(VBinary, 0x465C, "FileData") TData;
    TData data_;
    
    TypedefYamkaElt(VUInt, 0x46AE, "FileUID") TFileUID;
    TFileUID fileUID_;
    
    TypedefYamkaElt(VBinary, 0x4675, "FileReferral") TReferral;
    TReferral referral_;
  };
  
  //----------------------------------------------------------------
  // Attachments
  // 
  struct Attachments : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(AttdFile, 0x61A7, "AttachedFile") TFile;
    std::deque<TFile> files_;
  };
  
  //----------------------------------------------------------------
  // ChapTrk
  // 
  struct ChapTrk : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x89, "ChapterTrackNumber") TTrkNum;
    std::deque<TTrkNum> tracks_;
  };
  
  //----------------------------------------------------------------
  // ChapDisp
  // 
  struct ChapDisp : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VString, 0x85, "ChapString") TString;
    TString string_;
    
    TypedefYamkaElt(VString, 0x437C, "ChapLanguage") TLanguage;
    TLanguage language_;
    
    TypedefYamkaElt(VString, 0x437E, "ChapCountry") TCountry;
    TCountry country_;
  };
  
  //----------------------------------------------------------------
  // ChapProcCmd
  // 
  struct ChapProcCmd : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x6922, "ChapProcessTime") TTime;
    TTime time_;
    
    TypedefYamkaElt(VBinary, 0x6933, "ChapProcessData") TData;
    TData data_;
  };
  
  //----------------------------------------------------------------
  // ChapProc
  // 
  struct ChapProc : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x6955, "ChapProcessCodecID") TCodecID;
    TCodecID codecID_;
    
    TypedefYamkaElt(VBinary, 0x450D, "ChapProcessPrivate") TProcPrivate;
    TProcPrivate procPrivate_;
    
    TypedefYamkaElt(ChapProcCmd, 0x6911, "ChapProcCommands") TCmd;
    std::deque<TCmd> cmds_;
  };
  
  //----------------------------------------------------------------
  // ChapAtom
  // 
  struct ChapAtom : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x73C4, "ChapterUID") TUID;
    TUID UID_;
    
    TypedefYamkaElt(VUInt, 0x91, "ChapterTimeStart") TTimeStart;
    TTimeStart timeStart_;
    
    TypedefYamkaElt(VUInt, 0x92, "ChapterTimeEnd") TTimeEnd;
    TTimeEnd timeEnd_;
    
    TypedefYamkaElt(VUInt, 0x98, "ChapterFlagHidden") THidden;
    THidden hidden_;
    
    TypedefYamkaElt(VUInt, 0x4598, "ChapterFlagEnabled") TEnabled;
    TEnabled enabled_;
    
    TypedefYamkaElt(VBinary, 0x6E67, "ChapterSegmentUID") TSegUID;
    TSegUID segUID_;
    
    TypedefYamkaElt(VBinary, 0x6EBC, "ChapSegmentEditionUID") TSegEditionUID;
    TSegEditionUID segEditionUID_;
    
    TypedefYamkaElt(VUInt, 0x63C3, "ChapterPhysicalEquiv") TPhysEquiv;
    TPhysEquiv physEquiv_;
    
    TypedefYamkaElt(ChapTrk, 0x8F, "ChapterTracks") TTracks;
    TTracks tracks_;
    
    TypedefYamkaElt(ChapDisp, 0x80, "ChapterDisplay") TDisplay;
    std::deque<TDisplay> display_;
    
    TypedefYamkaElt(ChapProc, 0x6944, "ChapProcess") TProcess;
    std::deque<TProcess> process_;
  };
  
  //----------------------------------------------------------------
  // Edition
  // 
  struct Edition : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x45BC, "EditionUID") TUID;
    TUID UID_;
    
    TypedefYamkaElt(VUInt, 0x45BD, "EditionFlagHidden") TFlagHidden;
    TFlagHidden flagHidden_;
    
    TypedefYamkaElt(VUInt, 0x45DB, "EditionFlagDefault") TFlagDefault;
    TFlagDefault flagDefault_;
    
    TypedefYamkaElt(VUInt, 0x45DD, "EditionFlagOrdered") TFlagOrdered;
    TFlagOrdered flagOrdered_;
    
    TypedefYamkaElt(ChapAtom, 0xB6, "ChapterAtom") TChapAtom;
    std::deque<TChapAtom> chapAtoms_;
  };
  
  //----------------------------------------------------------------
  // Chapters
  // 
  struct Chapters : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(Edition, 0x45B9, "EditionEntry") TEdition;
    std::deque<TEdition> editions_;
  };
  
  //----------------------------------------------------------------
  // TagTargets
  // 
  struct TagTargets : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x68CA, "TargetTypeValue") TTypeValue;
    TTypeValue typeValue_;
    
    TypedefYamkaElt(VString, 0x63CA, "TargetType") TType;
    TType type_;
    
    TypedefYamkaElt(VUInt, 0x63C5, "TrackUID") TTrackUID;
    std::deque<TTrackUID> trackUIDs_;
    
    TypedefYamkaElt(VUInt, 0x63C9, "EditionUID") TEditionUID;
    std::deque<TEditionUID> editionUIDs_;
    
    TypedefYamkaElt(VUInt, 0x63C4, "ChapterUID") TChapterUID;
    std::deque<TChapterUID> chapterUIDs_;
    
    TypedefYamkaElt(VUInt, 0x63C6, "AttachmentUID") TAttachmentUID;
    std::deque<TAttachmentUID> attachmentUIDs_;
  };
  
  //----------------------------------------------------------------
  // SimpleTag
  // 
  struct SimpleTag : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VString, 0x45A3, "TagName") TName;
    TName name_;
    
    TypedefYamkaElt(VString, 0x447A, "TagLanguage") TLang;
    TLang lang_;
    
    TypedefYamkaElt(VUInt, 0x4484, "TagDefault") TDefault;
    TDefault default_;
    
    TypedefYamkaElt(VString, 0x4487, "TagString") TString;
    TString string_;
    
    TypedefYamkaElt(VBinary, 0x4485, "TagBinary") TBinary;
    TBinary binary_;
  };
  
  //----------------------------------------------------------------
  // Tag
  // 
  struct Tag : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(TagTargets, 0x63C0, "Targets") TTargets;
    TTargets targets_;
    
    TypedefYamkaElt(SimpleTag, 0x67C8, "SimpleTags") TSimpleTag;
    std::deque<TSimpleTag> simpleTags_;
  };
  
  //----------------------------------------------------------------
  // Tags
  // 
  struct Tags : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(Tag, 0x7373, "Tag") TTag;
    std::deque<TTag> tags_;
  };
  
  //----------------------------------------------------------------
  // SilentTracks
  // 
  struct SilentTracks : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x58D7, "SilentTrackNumber") TTrack;
    std::deque<TTrack> tracks_;
  };
  
  //----------------------------------------------------------------
  // Block
  // 
  struct Block : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // BlockVirtual
  // 
  struct BlockVirtual : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // BlockMore
  // 
  struct BlockMore : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0xEE, "BlockAddID") TBlockAddID;
    TBlockAddID blockAddID_;
    
    TypedefYamkaElt(VBinary, 0xA5, "BlockAdditional") TBlockAdditional;
    TBlockAdditional blockAdditional_;
  };
  
  //----------------------------------------------------------------
  // BlockAdditions
  // 
  struct BlockAdditions : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(BlockMore, 0xA6, "BlockMore") TMore;
    std::deque<TMore> more_;
  };
  
  //----------------------------------------------------------------
  // BlockGroup
  // 
  struct BlockGroup : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x9B, "BlockDuration") TDuration;
    TDuration duration_;
    
    TypedefYamkaElt(Block, 0xA1, "Block") TBlock;
    TBlock block_;
    
    TypedefYamkaElt(BlockVirtual, 0xA2, "BlockVirtual") TBlockVirtual;
    std::deque<TBlockVirtual> blockVirtual_;
    
    TypedefYamkaElt(BlockAdditions, 0x75A1, "BlockAdditions") TAdditions;
    TAdditions additions_;
    
    TypedefYamkaElt(VUInt, 0xFA, "ReferencePriority") TRefPriority;
    TRefPriority refPriority_;
    
    TypedefYamkaElt(VInt, 0xFB, "ReferenceBlock") TRefBlock;
    std::deque<TRefBlock> refBlock_;
    
    TypedefYamkaElt(VInt, 0xFD, "ReferenceVirtual") TRefVirtual;
    TRefVirtual refVirtual_;
    
    TypedefYamkaElt(VBinary, 0xA4, "CodecState") TCodecState;
    TCodecState codecState_;
    
    TypedefYamkaElt(VBinary, 0x8E, "Slice") TSlice;
    std::deque<TSlice> slices_;
  };
  
  //----------------------------------------------------------------
  // SimpleBlock
  // 
  struct SimpleBlock : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // Cluster
  // 
  struct Cluster : public EbmlMaster
  {
    Cluster();
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0xE7, "Timecode") TTimecode;
    TTimecode timecode_;
    
    TypedefYamkaElt(SilentTracks, 0x5854, "SilentTracks") TSilent;
    TSilent silent_;
    
    TypedefYamkaElt(VEltPosition, 0xA7, "Position") TPosition;
    TPosition position_;
    
    TypedefYamkaElt(VUInt, 0xAB, "PrevSize") TPrevSize;
    TPrevSize prevSize_;
    
    TypedefYamkaElt(BlockGroup, 0xA0, "BlockGroup") TBlockGroup;
    std::deque<TBlockGroup> blockGroups_;
    
    TypedefYamkaElt(SimpleBlock, 0xA3, "SimpleBlock") TSimpleBlock;
    std::deque<TSimpleBlock> simpleBlocks_;
    
    TypedefYamkaElt(VBinary, 0xAF, "EncryptedBlock") TEncryptedBlock;
    std::deque<TEncryptedBlock> encryptedBlocks_;
  };
  
  //----------------------------------------------------------------
  // Segment
  // 
  struct Segment : public EbmlMaster
  {
    ImplementsYamkaPayloadAPI();
    
    // resolve positional references (seeks, cues, etc...)
    void resolveReferences(const IElement * origin);
    
    TypedefYamkaElt(SegInfo, 0x1549A966, "SegInfo") TInfo;
    TInfo info_;
    
    TypedefYamkaElt(Tracks, 0x1654AE6B, "Tracks") TTracks;
    TTracks tracks_;
    
    TypedefYamkaElt(SeekHead, 0x114D9B74, "SeekHead") TSeekHead;
    std::deque<TSeekHead> seekHeads_;
    
    TypedefYamkaElt(Cues, 0x1C53BB6B, "Cues") TCue;
    std::deque<TCue> cues_;
    
    TypedefYamkaElt(Attachments, 0x1941A469, "Attachments") TAttachment;
    std::deque<TAttachment> attachments_;
    
    TypedefYamkaElt(Chapters, 0x1043A770, "Chapters") TChapters;
    TChapters chapters_;
    
    TypedefYamkaElt(Tags, 0x1254C367, "Tags") TTag;
    std::deque<TTag> tags_;
    
    TypedefYamkaElt(Cluster, 0x1F43B675, "Cluster") TCluster;
    std::deque<TCluster> clusters_;
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc
  // 
  struct MatroskaDoc : public EbmlDoc
  {
    MatroskaDoc();
    
    ImplementsYamkaPayloadAPI();
    
    // resolve positional references (seeks, cues, etc...)
    // for each segment:
    void resolveReferences();
    
    TypedefYamkaElt(Segment, 0x18538067, "Segment") TSegment;
    std::deque<TSegment> segments_;
  };
  
}


#endif // YAMKA_MATROSKA_H_

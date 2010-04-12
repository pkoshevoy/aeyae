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
  struct ChapTranslate
  {
    ChapTranslate()
    {
      editionUID_.setParent(this);
      chapTransCodec_.setParent(this);
      chapTransID_.setParent(this);
    }
    
    Elt<ChapTranslate, VUInt, 0x69fc, "EditionUID"> editionUID_;
    Elt<ChapTranslate, VUInt, 0x69bf, "ChapTransCodec"> chapTransCodec_;
    Elt<ChapTranslate, VBinary, 0x69a5, "ChapTransID"> chapTransID_;
  };
  
  //----------------------------------------------------------------
  // SegInfo
  // 
  struct SegInfo
  {
    SegInfo()
    {
      segUID_.setParent(this);
      segFilename_.setParent(this);
      prevUID_.setParent(this);
      prevFilename_.setParent(this);
      nextUID_.setParent(this);
      nextFilename_.setParent(this);
      familyUID_.setParent(this);
      chapTranslate_.setParent(this);
      timecodeScale_.setParent(this).setDefault(1000000);
      duration_.setParent(this).setDefault(0.0);
      date_.setParent(this);
      title_.setParent(this);
      muxingApp_.setParent(this);
      writingApp_.setParent(this);
    }
    
    Elt<SegInfo, VBytes<16>, 0x73a4, "SegmentUID"> segUID_;
    Elt<SegInfo, VString, 0x7384, "SegmentFilename"> segFilename_;
    Elt<SegInfo, VBytes<16>, 0x3cb923, "PrevUID"> prevUID_;
    Elt<SegInfo, VString, 0x3c83ab, "PrevFilename"> prevFilename_;
    Elt<SegInfo, VBytes<16>, 0x3eb923, "NextUID"> nextUID_;
    Elt<SegInfo, VString, 0x3e83bb, "NextFilename"> nextFilename_;
    Elt<SegInfo, VBytes<16>, 0x4444, "FamilyUID"> familyUID_;
    Elt<SegInfo, ChapTranslate, 0x6924, "ChapTranslate"> chapTranslate_;
    Elt<SegInfo, VUInt, 0x2ad7b1, "TimecodeScale"> timecodeScale_;
    Elt<SegInfo, VFloat, 0x4489, "Duration"> duration_;
    Elt<SegInfo, Date, 0x4461, "DateUTC"> date_;
    Elt<SegInfo, VString, 0x7ba9, "Title"> title_;
    Elt<SegInfo, VString, 0x4d80, "MuxingApp"> muxingApp_;
    Elt<SegInfo, VString, 0x5741, "WritingApp"> writingApp_;
  };

  //----------------------------------------------------------------
  // TrackTranslate
  // 
  struct TrackTranslate
  {
    TrackTranslate()
    {
      editionUID_.setParent(this);
      trackTransCodec_.setParent(this);
      trackTransID_.setParent(this);
    }
    
    Elt<TrackTranslate, VUInt, 0x66fc, "EditionUID"> editionUID_;
    Elt<TrackTranslate, VUInt, 0x66bf, "TrackTransCodec"> trackTransCodec_;
    Elt<TrackTranslate, VBinary, 0x66a5, "TrackTransID"> trackTransID_;
  };

  //----------------------------------------------------------------
  // Video
  // 
  struct Video
  {
    Video()
    {
      aspectRatioType_.setParent(this).setDefault(0);
    }

    Elt<Video, VUInt, 0x9A, "FlagInterlaced"> flagInterlaced_;
    Elt<Video, VUInt, 0x53B8, "StereoMode"> stereoMode_;
    Elt<Video, VUInt, 0xB0, "PixelWidth"> pixelWidth_;
    Elt<Video, VUInt, 0xBA, "PixelHeight"> pixelHeight_;
    Elt<Video, VUInt, 0x54AA, "PixelCropBottom"> pixelCropBottom_;
    Elt<Video, VUInt, 0x54BB, "PixelCropTop"> pixelCropTop_;
    Elt<Video, VUInt, 0x54CC, "PixelCropLeft"> pixelCropLeft_;
    Elt<Video, VUInt, 0x54DD, "PixelCropRight"> pixelCropRight_;
    Elt<Video, VUInt, 0x54B0, "DisplayWidth"> displayWidth_;
    Elt<Video, VUInt, 0x54BA, "DisplayHeight"> displayHeight_;
    Elt<Video, VUInt, 0x54B2, "DisplayUnits"> displayUnits_;
    Elt<Video, VUInt, 0x54B3, "AspectRatioType"> aspectRatioType_;
    Elt<Video, VBytes<4>, 0x2EB524, "ColorSpace"> colorSpace_;
    Elt<Video, VFloat, 0x2FB523, "GammaValue"> gammaValue_;
    Elt<Video, VFloat, 0x2383E3, "FrameRate"> frameRate_;
  };

  //----------------------------------------------------------------
  // Audio
  // 
  struct Audio
  {
    Audio()
    {
      sampFreq_.setParent(this).setDefault(8000.0);
      channels_.setParent(this).setDefault(1);
    }
    
    Elt<Audio, VFloat, 0xB5, "SamplingFrequency"> sampFreq_;
    Elt<Audio, VFloat, 0x78B5, "OutputSamplingFrequency"> sampFreqOut_;
    Elt<Audio, VUInt, 0x9F, "Channels"> channels_;
    Elt<Audio, VBinary, 0x7D7B, "ChannelPositions"> channelPositions_;
    Elt<Audio, VUInt, 0x6264, "BitDepth"> bitDepth_;
  };

  //----------------------------------------------------------------
  // ContentCompr
  // 
  struct ContentCompr
  {
    Elt<ContentCompr, VUInt, 0x4254, "ContentCompAlgo"> algo_;
    Elt<ContentCompr, VBinary, 0x4255, "ContentCompSettings"> settings_;
  };

  //----------------------------------------------------------------
  // ContentEncrypt
  // 
  struct ContentEncrypt
  {
    Elt<ContentEncrypt, VUInt, 0x47E1, "ContentEncAlgo"> encAlgo_;
    Elt<ContentEncrypt, VBinary, 0x47E2, "ContentEncKeyID"> encKeyID_;
    Elt<ContentEncrypt, VBinary, 0x47E3, "ContentSignature"> signature_;
    Elt<ContentEncrypt, VBinary, 0x47E4, "ContentSigKeyID"> sigKeyID_;
    Elt<ContentEncrypt, VUInt, 0x47E5, "ContentSigAlgo"> sigAlgo_;
    Elt<ContentEncrypt, VUInt, 0x47e6, "ContentSigHashAlgo"> sigHashAlgo_;
  };
  
  //----------------------------------------------------------------
  // ContentEnc
  // 
  struct ContentEnc
  {
    Elt<ContentEnc, VUInt, 0x5031, "ContentEncodingOrder"> order_;
    Elt<ContentEnc, VUInt, 0x5032, "ContentEncodingScope"> scope_;
    Elt<ContentEnc, VUInt, 0x5033, "ContentEncodingType"> type_;
    Elt<ContentEnc, ContentCompr, 0x5034, "ContentCompression"> compression_;
    Elt<ContentEnc, ContentEncrypt, 0x5033, "ContentEncryption"> encryption_;
  };
  
  //----------------------------------------------------------------
  // ContentEncs
  // 
  struct ContentEncs
  {
    std::deque<Elt<ContentEncs, ContentEnc, 0x6240, "ContentEnc"> > encoding_;
  };
  
  //----------------------------------------------------------------
  // Track
  // 
  struct Track
  {
    Elt<Track, VUInt, 0xD7, "TrackNumber"> trackNumber_;
    Elt<Track, VUInt, 0x73C5, "TrackUID"> trackUID_;
    Elt<Track, VUInt, 0x83, "TrackType"> trackType_;
    Elt<Track, VUInt, 0xB9, "FlagEnabled"> flagEnabled_;
    Elt<Track, VUInt, 0x88, "FlagDefault"> flagDefault_;
    Elt<Track, VUInt, 0x55AA, "FlagForces"> flagForced_;
    Elt<Track, VUInt, 0x9C, "FlagLacing"> flagLacing_;
    Elt<Track, VUInt, 0x6DE7, "MinCache"> minCache_;
    Elt<Track, VUInt, 0x6DF8, "MaxCache"> maxCache_;
    Elt<Track, VUInt, 0x23E383, "DefaultDuration"> frameDuration_;
    Elt<Track, VFLoat, 0x23314F, "TrackTimecodeScale"> timecodeScale_;
    Elt<Track, VInt, 0x537F, "TrackOffset"> trackOffset_;
    Elt<Track, VUInt, 0x55EE, "MaxBlockAddID"> maxBlockAddID_;
    Elt<Track, VString, 0x536E, "Name"> name_;
    Elt<Track, VString, 0x22B59C, "Language"> language_;
    Elt<Track, VString, 0x86, "CodecID"> codecID_;
    Elt<Track, VBinary, 0x63A2, "CodecPrivate"> codecPrivate_;
    Elt<Track, VString, 0x258688, "CodecName"> codecName_;
    Elt<Track, VUInt, 0x7446, "AttachmentLink"> attachmentLink_;
    Elt<Track, VString, 0x3A9697, "CodecSettings"> codecSettings_;
    Elt<Track, VString, 0x3B4040, "CodecInfoURL"> codecInfoURL_;
    Elt<Track, VString, 0x26B240, "CodecDownloadURL"> codecDownloadURL_;
    Elt<Track, VUInt, 0xAA, "CodecDecodeAll"> codecDecodeAll_;
    Elt<Track, VUInt, 0x6FAB, "TrackOverlay"> trackOverlay_;
    Elt<Track, TrackTranslate, 0x6624, "TrackTranslate"> trackTranslate_;
    Elt<Track, Video, 0xE0, "Video"> video_;
    Elt<Track, Audio, 0xE1, "Audio"> audio_;
    Elt<Track, ContentEncodings, 0x6D80, "ContentEncodings"> contentEncs_;
  };
  
  //----------------------------------------------------------------
  // Tracks
  // 
  struct Tracks
  {
    std::deque<Elt<Tracks, Track, 0xae, "TrackEntry"> > tracks_;
  };

  //----------------------------------------------------------------
  // CueRef
  // 
  struct CueRef
  {
    Elt<CueRef, VUInt, 0x96, "CueRefTime"> time_;
    Elt<CueRef, VUInt, 0x97, "CueRefCluster"> cluster_;
    Elt<CueRef, VUInt, 0x535F, "CueRefNumber"> block_;
    Elt<CueRef, VUInt, 0xEB, "CueRefCodecState"> codecState_;
  };
  
  //----------------------------------------------------------------
  // CueTrkPos
  // 
  struct CueTrkPos
  {
    Elt<CueTrkPos, VUInt, 0xF7, "Track"> track_;
    Elt<CueTrkPos, VUInt, 0xF1, "ClusterPosition"> cluster_;
    Elt<CueTrkPos, VUInt, 0x5378, "CueBlockNumber"> block_;
    Elt<CueTrkPos, VUInt, 0xEA, "CueCodecState"> codecState_;
    Elt<CueTrkPos, CueRef, 0xDB, "CueReference"> ref_;
  };
  
  //----------------------------------------------------------------
  // CuePoint
  // 
  struct CuePoint
  {
    Elt<CuePoint, VUInt, 0xB3, "CueTime"> time_;
    std::deque<Elt<CuePoint, CueTrkPos, 0xB7, "CueTrackPosition"> > trkPosns_;
  };
  
  //----------------------------------------------------------------
  // Cues
  // 
  struct Cues
  {
    std::deque<Elt<Cues, CuePoint, 0xBB, "CuePoint"> > points_;
  };

  //----------------------------------------------------------------
  // Seek
  // 
  struct Seek
  {
    Elt<Seek, VBinary, 0x53AB, "SeekID"> id_;
    Elt<Seek, VUInt, 0x53AC, "SeekPosition"> position_;
  };
  
  //----------------------------------------------------------------
  // SeekHead
  // 
  struct SeekHead
  {
    Elt<SeekHead, Seek, 0x4DBB, "Seek"> seek_;
  };

  //----------------------------------------------------------------
  // AttdFile
  // 
  struct AttdFile
  {
    Elt<AttdFile, VString, 0x467E, "FileDescription"> description_;
    Elt<AttdFile, VString, 0x466E, "FileName"> filename_;
    Elt<AttdFile, VString, 0x4660, "FileMimeType"> mimeType_;
    Elt<AttdFile, VBinary, 0x465C, "FileData"> data_;
    Elt<AttdFile, VUInt, 0x46AE, "FileUID"> fileUID_;
    Elt<AttdFile, VBinary, 0x4675, "FileReferral"> referral_;
  };

  //----------------------------------------------------------------
  // Attachments
  // 
  struct Attachments
  {
    std::deque<Elt<Attachments, AttdFile, 0x61A7, "AttachedFile"> > files_;
  };

  //----------------------------------------------------------------
  // ChapTrk
  // 
  struct ChapTrk
  {
    std::deque<Elt<ChapTrk, VUInt, 0x89, "ChapterTrackNumber"> > tracks_;
  };

  //----------------------------------------------------------------
  // ChapDisp
  // 
  struct ChapDisp
  {
    Elt<ChapDisp, VString, 0x85, "ChapString"> string_;
    Elt<ChapDisp, VString, 0x437C, "ChapLanguage"> language_;
    Elt<ChapDisp, VString, 0x437E, "ChapCountry"> country_;
  };

  //----------------------------------------------------------------
  // ChapProcCmd
  // 
  struct ChapProcCmd
  {
    Elt<ChapProcCmd, VUInt, 0x6922, "ChapProcessTime"> time_;
    Elt<ChapProcCmd, VBinary, 0x6933, "ChapProcessData"> data_;
  };

  //----------------------------------------------------------------
  // ChapProc
  // 
  struct ChapProc
  {
    Elt<ChapDisp, VUInt, 0x6955, "ChapProcessCodecID"> codecID_;
    Elt<ChapDisp, VBinary, 0x450D, "ChapProcessPrivate"> procPivate_;
    std::deque<Elt<ChapDisp, ChapProcCmd, 0x6911, "ChapProcCommands"> > cmds_;
  };
  
  //----------------------------------------------------------------
  // ChapAtom
  // 
  struct ChapAtom
  {
    Elt<ChapAtom, VUInt, 0x73C4, "ChapterUID"> UID_;
    Elt<ChapAtom, VUInt, 0x91, "ChapterTimeStart"> timeStart_;
    Elt<ChapAtom, VUInt, 0x92, "ChapterTimeEnd"> timeEnd_;
    Elt<ChapAtom, VUInt, 0x98, "ChapterFlagHidden"> hidden_;
    Elt<ChapAtom, VUInt, 0x4598, "ChapterFlagEnabled"> enabled_;
    Elt<ChapAtom, VBinary, 0x6E67, "ChapterSegmentUID"> segUID_;
    Elt<ChapAtom, VBinary, 0x6EBC, "ChapterSegmentEditionUID"> segEditionUID_;
    Elt<ChapAtom, VUInt, 0x63C3, "ChapterPhysicalEquiv"> physEquiv_;
    Elt<ChapAtom, ChapTrk, 0x8F, "ChapterTracks"> tracks_;
    std::deque<Elt<ChapAtom, ChapDisp, 0x80, "ChapterDisplay"> > display_;
    std::deque<Elt<ChapAtom, ChapProc, 0x6944, "ChapProcess"> > process_;
  };
  
  //----------------------------------------------------------------
  // Edition
  // 
  struct Edition
  {
    Elt<Edition, VUInt, 0x45BC, "EditionUID"> UID_;
    Elt<Edition, VUInt, 0x45BD, "EditionFlagHidden"> flagHidden_;
    Elt<Edition, VUInt, 0x45DB, "EditionFlagDefault"> flagDefault_;
    Elt<Edition, VUInt, 0x45DD, "EditionFlagOrdered"> flagOrdered_;
    std::deque<Elt<Edition, ChapAtom, 0xB6, "ChapterAtom"> chapAtoms_;
  };
  
  //----------------------------------------------------------------
  // Chapters
  // 
  struct Chapters
  {
    std::deque<Elt<Chapters, Edition, 0x45B9, "EditionEntry"> > editions_;
  };

  //----------------------------------------------------------------
  // Target
  // 
  struct Target
  {
    Elt<Target, VUInt, 0x68CA, "TargetTypeValue"> typeValue_;
    Elt<Target, VString, 0x63CA, "TargetType"> type_;
    std::deque<Elt<Target, VUInt, 0x63C5, "TrackUID"> > trackUIDs_;
    std::deque<Elt<Target, VUInt, 0x63C9, "EditionUID"> > editionUIDs_;
    std::deque<Elt<Target, VUInt, 0x63C4, "ChapterUID"> > chapterUIDs_;
    std::deque<Elt<Target, VUInt, 0x63C6, "AttachmentUID"> > attachmentUIDs_;
  };

  //----------------------------------------------------------------
  // SimpleTag
  // 
  struct SimpleTag
  {
    Elt<SimpleTag, VString, 0x45A3, "TagName"> name_;
    Elt<SimpleTag, VString, 0x447A, "TagLanguage"> lang_;
    Elt<SimpleTag, VUInt, 0x4484, "TagDefault"> default_;
    Elt<SimpleTag, VString, 0x4487, "TagString"> string_;
    Elt<SimpleTag, VBinary, 0x4485, "TagBinary"> binary_;
  };

  //----------------------------------------------------------------
  // Tag
  // 
  struct Tag
  {
    Elt<Tag, Target, 0x63C0, "Target"> target_;
    std::deque<Elt<Tag, SimpleTag, 0x67C8, "SimpleTags"> simpleTag_;
  };
  
  //----------------------------------------------------------------
  // Tags
  // 
  struct Tags
  {
    std::deque<Elt<Tags, Tag, 0x7373, "Tag"> tags_;;
  };

  //----------------------------------------------------------------
  // SilentTracks
  // 
  struct SilentTracks
  {
    std::deque<Elt<SilentTracks, VUInt, 0x58D7, "SilentTrackNumber"> > tracks_;
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
  struct BlockMore
  {
    Elt<BlockMore, VUInt, 0xEE, "BlockAddID"> blockAddID_;
    Elt<BlockMore, VBinary, 0xA5, "BlockAdditional"> blockAdditional_;
  };
  
  //----------------------------------------------------------------
  // BlockAdditions
  // 
  struct BlockAdditions
  {
    std::deque<Elt<BlockAdditions, BlockMore, 0xA6, "BlockMore"> > more_;
  };
  
  //----------------------------------------------------------------
  // BlockGroup
  // 
  struct BlockGroup
  {
    Elt<BlockGroup, Block, 0xA1, "Block"> block_;
    std::deque<Elt<BlockGroup, BlockVirtual, 0xA2, "BlockVirtual"> > vblock_;
    Elt<BlockGroup, BlockAdditions, 0x75A1, "BlockAdditions"> additions_;
    Elt<BlockGroup, VUInt, 0x9B, "BlockDuration"> duration_;
    Elt<BlockGroup, VUInt, 0xFA, "ReferencePriority"> refPriority_;
    std::deque<Elt<BlockGroup, VInt, 0xFB, "ReferenceBlock"> > refBlock_;
    Elt<BlockGroup, VInt, 0xFD, "ReferenceVirtual"> refVirtual_;
    Elt<BlockGroup, VBinary, 0xA4, "CodecState"> codecState_;
    std::deque<Elt<BlockGroup, VBinary, 0x8E, "Slice"> > slices_;
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
  struct Cluster
  {
    Elt<Cluster, VUInt, 0xE7, "Timecode"> timecode_;
    Elt<Cluster, SilentTracks, 0x5854, "SilentTracks"> silent_;
    Elt<Cluster, VUInt, 0xA7, "Position"> position_;
    Elt<Cluster, VUInt, 0xAB, "PrevSize"> prevSize_;
    std::deque<Elt<Cluster, BlockGroup, 0xA0, "BlockGroup"> > blockGroups_;
    std::deque<Elt<Cluster, SimpleBlock, 0xA3, "SimpleBlock"> > simpleBlocks_;
    std::deque<Elt<Cluster, VBinary, 0xAF, "EncryptedBlock"> > encryptedBlocks_;
  };
  
  //----------------------------------------------------------------
  // Segment
  // 
  struct Segment
  {
    Segment()
    {
      info_.setParent(this);
      tracks_.setParent(this);
    }
    
    Elt<Segment, SegInfo, 0x1549A966, "SegInfo"> info_;
    Elt<Segment, Tracks, 0x1654AE6b, "Tracks"> tracks_;
    std::deque<Elt<Segment, SeekHead, 0x114D9B74, "SeekHead"> > seekHeads_;
    std::deque<Elt<Segment, Cues, 0x1C53BB6B, "Cues"> > cues_;
    std::deque<Elt<Segment, Attachments, 0x1941a469, "Attachments"> > attts_;
    Elt<Segment, Chapters, 0x1043a770, "Chapters"> chapters_;
    std::deque<Elt<Segment, Tags, 0x1254C367, "Tags"> > tags_;
    std::deque<Elt<Segment, Cluster, 0x1f43b675, "Cluster"> > clusters_;
    std::deque<Elt<Segment, VBinary, 0xEC, "Void"> > voids_;
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc
  // 
  struct MatroskaDoc : public EbmlDoc
  {
    MatroskaDoc():
      EbmlDoc()
    {
      head_.docType_.setDefault(std::string("matroska"));
      head_.docTypeVersion_.setDefault(1);
      head_.docTypeReadVersion_.setDefault(1);
    }
    
    std::deque<Elt<EbmlDoc, Segment, 0x18538067, "Segment"> > segments_;
  };
  
}


#endif // YAMKA_MATROSKA_H_

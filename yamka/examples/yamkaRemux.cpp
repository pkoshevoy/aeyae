// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Nov  6 22:05:29 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaStdInt.h>
#include <yamkaFileStorage.h>
#include <yamkaEBML.h>
#include <yamkaMatroska.h>
#include <yamkaVersion.h>

// system includes:
#include <limits>
#include <sstream>
#include <iostream>
#include <string.h>
#include <string>
#include <time.h>
#include <map>

// namespace access:
using namespace Yamka;

// shortcuts:
typedef MatroskaDoc::TSegment TSegment;
typedef Segment::TInfo TSegInfo;
typedef Segment::TTracks TTracks;
typedef Segment::TSeekHead TSeekHead;
typedef SeekHead::TSeekEntry TSeekEntry;
typedef Segment::TCluster TCluster;
typedef Segment::TCues TCues;
typedef Segment::TTags TTags;
typedef Segment::TChapters TChapters;
typedef Segment::TAttachment TAttachment;
typedef Cues::TCuePoint TCuePoint;
typedef CuePoint::TCueTrkPos TCueTrkPos;
typedef Tags::TTag TTag;
typedef Tag::TTargets TTagTargets;
typedef Tag::TSimpleTag TSimpleTag;
typedef TagTargets::TTrackUID TTagTrackUID;
typedef Tracks::TTrack TTrack;
typedef Cluster::TBlockGroup TBlockGroup;
typedef Cluster::TSimpleBlock TSimpleBlock;
typedef Cluster::TEncryptedBlock TEncryptedBlock;
typedef Cluster::TSilent TSilentTracks;
typedef Chapters::TEdition TEdition;
typedef Edition::TChapAtom TChapAtom;
typedef ChapAtom::TDisplay TChapDisplay;

typedef std::deque<TSeekHead>::iterator TSeekHeadIter;
typedef std::list<TCluster>::iterator TClusterIter;
typedef std::list<TSeekEntry>::iterator TSeekEntryIter;
typedef std::list<TCuePoint>::iterator TCuePointIter;
typedef std::list<TCueTrkPos>::iterator TCueTrkPosIter;

typedef std::deque<TSeekHead>::const_iterator TSeekHeadConstIter;
typedef std::list<TCluster>::const_iterator TClusterConstIter;
typedef std::list<TSeekEntry>::const_iterator TSeekEntryConstIter;
typedef std::list<TCuePoint>::const_iterator TCuePointConstIter;
typedef std::list<TCueTrkPos>::const_iterator TCueTrkPosConstIter;

//----------------------------------------------------------------
// kMinShort
//
static const short int kMinShort = std::numeric_limits<short int>::min();

//----------------------------------------------------------------
// kMaxShort
//
static const short int kMaxShort = std::numeric_limits<short int>::max();

//----------------------------------------------------------------
// BE_QUIET
//
#define BE_QUIET

//----------------------------------------------------------------
// NANOSEC_PER_SEC
//
// 1 second, in nanoseconds
//
static const Yamka::uint64 NANOSEC_PER_SEC = 1000000000;

//----------------------------------------------------------------
// has
//
template <typename TDataContainer, typename TData>
bool
has(const TDataContainer & container, const TData & value)
{
  typename TDataContainer::const_iterator found =
    std::find(container.begin(), container.end(), value);

  return found != container.end();
}

//----------------------------------------------------------------
// has
//
template <typename TKey, typename TValue>
bool
has(const std::map<TKey, TValue> & keyValueMap, const TValue & key)
{
  typename std::map<TKey, TValue>::const_iterator found =
    keyValueMap.find(key);

  return found != keyValueMap.end();
}

//----------------------------------------------------------------
// endsWith
//
static bool
endsWith(const std::string & str, const char * suffix)
{
  std::size_t strSize = str.size();
  std::string suffixStr(suffix);
  std::size_t suffixStrSize = suffixStr.size();
  std::size_t found = str.rfind(suffixStr);
  return (found == strSize - suffixStrSize);
}

//----------------------------------------------------------------
// toScalar
//
template <typename TScalar>
static TScalar
toScalar(const char * text)
{
  std::istringstream iss;
  iss.str(std::string(text));

  TScalar v = (TScalar)0;
  iss >> v;

  return v;
}

//----------------------------------------------------------------
// toText
//
template <typename TScalar>
static std::string
toText(TScalar v)
{
  std::ostringstream oss;
  oss << v;
  std::string text = oss.str().c_str();
  return text;
}

//----------------------------------------------------------------
// printCurrentTime
//
static void
printCurrentTime(const char * msg)
{
  time_t rawtime = 0;
  time(&rawtime);

  struct tm * timeinfo = localtime(&rawtime);
  std::string s(asctime(timeinfo));
  s[s.size() - 1] = '\0';
  std::cout << "\n\n" << s.c_str() << " -- " << msg << std::endl;
}

//----------------------------------------------------------------
// getTrack
//
static const Track *
getTrack(const std::deque<TTrack> & tracks, uint64 trackNo)
{
  for (std::deque<TTrack>::const_iterator i = tracks.begin();
       i != tracks.end(); ++i)
  {
    const Track & track = i->payload_;
    uint64 tn = track.trackNumber_.payload_.get();
    if (tn == trackNo)
    {
      return &track;
    }
  }

  return NULL;
}


//----------------------------------------------------------------
// LoaderSkipClusterPayload
//
// Skip cluster payload, let the generic mechanism load everything else
//
struct LoaderSkipClusterPayload : public LoadWithProgress
{
  bool skipCues_;

  LoaderSkipClusterPayload(uint64 srcSize, bool skipCues):
    LoadWithProgress(srcSize),
    skipCues_(skipCues)
  {}

  // virtual:
  uint64 load(FileStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    LoadWithProgress::load(storage, payloadBytesToRead, eltId, payload);

    const bool payloadIsCluster = (eltId == TCluster::kId);
    const bool payloadIsCues = (eltId == TCues::kId);
    if (payloadIsCluster || (payloadIsCues && skipCues_))
    {
      storage.file_.seek(payloadBytesToRead, File::kRelativeToCurrent);
      return payloadBytesToRead;
    }

    // let the generic load mechanism handle the actual loading:
    return 0;
  }
};

//----------------------------------------------------------------
// TTrackMap
//
typedef std::map<uint64, uint64> TTrackMap;

//----------------------------------------------------------------
// TFifo
//
template <typename TLink>
struct TFifo
{
  TFifo():
    head_(NULL),
    tail_(NULL),
    size_(0)
  {}

  ~TFifo()
  {
    assert(!head_);
  }

  void push(TLink * link)
  {
    assert(!link->next_);

    if (!head_)
    {
      assert(!tail_);
      assert(!size_);
      head_ = link;
      tail_ = link;
      size_ = 1;
    }
    else
    {
      assert(tail_);
      assert(size_);
      tail_->next_ = link;
      link->prev_ = tail_;
      tail_ = link;
      size_++;
    }
  }

  TLink * pop()
  {
    if (!head_)
    {
      return NULL;
    }

    TLink * link = head_;
    head_ = link->next_;
    link->prev_ = NULL;
    link->next_ = NULL;
    size_--;

    if (!head_)
    {
      tail_ = NULL;
    }
    else
    {
      head_->prev_ = NULL;
    }

    return link;
  }

  TLink * head_;
  TLink * tail_;
  std::size_t size_;
};

//----------------------------------------------------------------
// TBlockInfo
//
struct TBlockInfo
{
  TBlockInfo():
    dts_(0),
    pts_(0),
    duration_(0),
    trackNo_(0),
    keyframe_(false),
    next_(NULL),
    prev_(NULL)
  {}

  inline HodgePodge * getBlockData()
  {
    if (sblockElt_.mustSave())
    {
      return &(sblockElt_.payload_.data_);
    }
    else if (bgroupElt_.mustSave())
    {
      return &(bgroupElt_.payload_.block_.payload_.data_);
    }
    else if (eblockElt_.mustSave())
    {
      return &(eblockElt_.payload_.data_);
    }

    assert(false);
    return NULL;
  }

  IStorage::IReceiptPtr save(FileStorage & storage)
  {
    if (sblockElt_.mustSave())
    {
      return sblockElt_.save(storage);
    }
    else if (bgroupElt_.mustSave())
    {
      return bgroupElt_.save(storage);
    }
    else if (eblockElt_.mustSave())
    {
      return eblockElt_.save(storage);
    }

    assert(false);
    return IStorage::IReceiptPtr();
  }

  TSilentTracks silentElt_;
  TSimpleBlock sblockElt_;
  TBlockGroup bgroupElt_;
  TEncryptedBlock eblockElt_;

  SimpleBlock block_;
  IStorage::IReceiptPtr header_;
  IStorage::IReceiptPtr frames_;

  uint64 dts_;
  uint64 pts_;
  uint64 duration_;
  uint64 trackNo_;
  bool keyframe_;

  TBlockInfo * next_;
  TBlockInfo * prev_;
};

//----------------------------------------------------------------
// TBlockFifo
//
typedef TFifo<TBlockInfo> TBlockFifo;

//----------------------------------------------------------------
// TLace
//
struct TLace
{
  TLace():
    cuesTrackNo_(0),
    size_(0)
  {}

  void push(TBlockInfo * binfo)
  {
    TBlockFifo & track = track_[(std::size_t)(binfo->trackNo_)];
    track.push(binfo);
    size_++;

    uint64 dts = binfo->dts_;
    for (TBlockInfo * i = track.tail_->prev_; i != NULL; i = i->prev_)
    {
      if (i->dts_ <= dts)
      {
        break;
      }

      i->dts_ = dts;
    }
  }

  TBlockInfo * pop()
  {
    std::size_t numTracks = track_.size();

    bool cuesTrackHasData =
      cuesTrackNo_ < numTracks &&
      track_[(std::size_t)(cuesTrackNo_)].size_;

    uint64 bestTrackNo = cuesTrackHasData ? cuesTrackNo_ : numTracks;
    uint64 bestTrackTime = nextTime((std::size_t)(cuesTrackNo_));

    uint64 trackTime = (uint64)(~0);
    for (std::size_t i = 0; i < numTracks; i++)
    {
      if (i != cuesTrackNo_ && (trackTime = nextTime(i)) < bestTrackTime)
      {
        bestTrackNo = i;
        bestTrackTime = trackTime;
      }
    }

    if (bestTrackNo < numTracks)
    {
      TBlockInfo * binfo = track_[(std::size_t)(bestTrackNo)].pop();
      size_--;

      if (!binfo->duration_)
      {
        Track::MatroskaTrackType trackType =
          trackType_[(std::size_t)(bestTrackNo)];

        uint64 defaultDuration =
          defaultDuration_[(std::size_t)(bestTrackNo)];

        if (defaultDuration)
        {
          double frameDuration =
            double(defaultDuration) /
            double(timecodeScale_);

          std::size_t numFrames = binfo->block_.getNumberOfFrames();
          binfo->duration_ = (uint64)(0.5 + frameDuration * double(numFrames));
        }
        else if (binfo->next_ && trackType == Track::kTrackTypeAudio)
        {
          // calculate audio frame duration based on next frame duration:
          binfo->duration_ = binfo->next_->pts_ - binfo->pts_;
        }
      }

      return binfo;
    }

    return NULL;
  }

  uint64 nextTime(std::size_t trackNo)
  {
    if (trackNo >= track_.size() || !track_[trackNo].size_)
    {
      return (uint64)(~0);
    }

    TBlockFifo & track = track_[trackNo];
    uint64 startTime = track.head_->dts_;
    return startTime;
  }

  std::vector<Track::MatroskaTrackType> trackType_;
  std::vector<uint64> defaultDuration_;
  std::vector<TBlockFifo> track_;
  std::vector<std::string> codecID_;
  uint64 timecodeScale_;
  uint64 cuesTrackNo_;
  std::size_t size_;
};


//----------------------------------------------------------------
// TDataTable
//
template <typename TData, unsigned int PageSize = 65536>
struct TDataTable
{
  //----------------------------------------------------------------
  // TPage
  //
  struct TPage
  {
    enum { kSize = PageSize };
    TData data_[PageSize];
  };

  TDataTable():
    pages_(NULL),
    numPages_(0),
    size_(0)
  {}

  ~TDataTable()
  {
    clear();
    free(pages_);
  }

  inline bool empty() const
  { return !size_; }

  void clear()
  {
    for (unsigned int i = 0; i < numPages_; i++)
    {
      TPage * page = pages_[i];
      if (page)
      {
        free(page);
      }

      pages_[i] = NULL;
    }

    size_ = 0;
  }

  void grow()
  {
    unsigned int numPages = numPages_ ? numPages_ * 2 : 256;

    unsigned int oldSize = sizeof(TPage *) * numPages_;
    unsigned int newSize = sizeof(TPage *) * numPages;

    TPage ** pages = (TPage **)malloc(newSize);
    memcpy(pages, pages_, oldSize);
    memset(pages + oldSize, 0, newSize - oldSize);
    free(pages_);
    pages_ = pages;
    numPages_ = numPages;
  }

  void add(const TData & data)
  {
    unsigned int i = size_ / TPage::kSize;
    if (i >= numPages_)
    {
      grow();
    }

    TPage * page = pages_[i];
    if (!page)
    {
      unsigned int memSize = sizeof(TPage);
      page = (TPage *)malloc(memSize);
      memset(page, 0, memSize);
      pages_[i] = page;
    }

    unsigned int j = size_ % TPage::kSize;
    page->data_[j] = data;
    size_++;
  }

  TPage ** pages_;
  unsigned int numPages_;
  unsigned int size_;
};

//----------------------------------------------------------------
// TCue
//
struct TCue
{
  uint64 time_;
  uint64 track_;
  uint64 cluster_;
  uint64 block_;
};

//----------------------------------------------------------------
// TRemuxer
//
struct TRemuxer : public LoadWithProgress
{
  TRemuxer(const TTrackMap & trackSrcDst,
           const TTrackMap & trackDstSrc,
           const TSegment & srcSeg,
           TSegment & dstSeg,
           FileStorage & src,
           FileStorage & dst,
           IStorage & tmp);

  void remux(uint64 t0, uint64 t1, bool extractFromKeyframe, bool fixKeyFlag);
  void flush();

  // Returns true if the given Block/SimpleBlock/EncryptedBlock
  // should be kept, returns false if the block should be discarded:
  bool isRelevant(uint64 clusterTime, TBlockInfo & binfo);

  // if necessary remaps the track number, adjusts blockTime,
  // keyframe flag, and stores the updated block header without
  // altering the (possibly) laced (or encrypted) block data:
  bool updateHeader(uint64 clusterTime, TBlockInfo & binfo);

  // lace together BlockGroups, SimpleBlocks, EncryptedBlocks, SilentTracks:
  TBlockInfo * push(uint64 clusterTime, const IElement * elt);

  void mux(std::size_t minLaceSize = 150);
  void startNextCluster(TBlockInfo * binfo);
  void finishCurrentCluster();
  void addCuePoint(TBlockInfo * binfo);

  const TTrackMap & trackSrcDst_;
  const TTrackMap & trackDstSrc_;
  const TSegment & srcSeg_;
  TSegment & dstSeg_;
  FileStorage & src_;
  FileStorage & dst_;
  IStorage & tmp_;
  uint64 clusterBlocks_;
  uint64 cuesTrackHasKeyframes_;
  std::vector<bool> needCuePointForTrack_;

  const uint64 segmentPayloadPosition_;
  uint64 clusterRelativePosition_;

  TLace lace_;
  TDataTable<uint64> seekTable_;
  TDataTable<TCue> cueTable_;
  TCluster clusterElt_;
  uint64 t0_;
  uint64 t1_;
  bool extractTimeSegment_;
  bool extractFromKeyframe_;
  bool foundFirstKeyframe_;
};

//----------------------------------------------------------------
// TRemuxer::TRemuxer
//
TRemuxer::TRemuxer(const TTrackMap & trackSrcDst,
                   const TTrackMap & trackDstSrc,
                   const TSegment & srcSeg,
                   TSegment & dstSeg,
                   FileStorage & src,
                   FileStorage & dst,
                   IStorage & tmp):
  LoadWithProgress(src.file_.size()),
  trackSrcDst_(trackSrcDst),
  trackDstSrc_(trackDstSrc),
  srcSeg_(srcSeg),
  dstSeg_(dstSeg),
  src_(src),
  dst_(dst),
  tmp_(tmp),
  clusterBlocks_(0),
  cuesTrackHasKeyframes_(0),
  segmentPayloadPosition_(dstSeg.payloadReceipt()->position()),
  clusterRelativePosition_(0),
  t0_(0),
  t1_(0),
  extractTimeSegment_(false),
  extractFromKeyframe_(false),
  foundFirstKeyframe_(false)
{
  TTracks & tracksElt = dstSeg_.payload_.tracks_;
  std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
  std::size_t numTracks = tracks.size();

  // create a barrier to avoid making too many CuePoints:
  needCuePointForTrack_.assign(numTracks + 1, true);

  lace_.timecodeScale_ =
    dstSeg_.payload_.info_.payload_.timecodeScale_.payload_.get();

  // store track types using base-1 array index for quicker lookup,
  // because track numbers are also base-1:
  lace_.trackType_.resize(numTracks + 1);
  lace_.defaultDuration_.resize(numTracks + 1);
  lace_.codecID_.resize(numTracks + 1);
  lace_.track_.resize(numTracks + 1);

  lace_.trackType_[0] = Track::kTrackTypeUndefined;
  lace_.defaultDuration_[0] = 0;

  for (std::size_t i = 0; i < numTracks; i++)
  {
    Track & track = tracks[i].payload_;
    uint64 trackType = track.trackType_.payload_.get();
    lace_.trackType_[i + 1] = (Track::MatroskaTrackType)trackType;

    uint64 defaultDuration = track.frameDuration_.payload_.get();
    lace_.defaultDuration_[i + 1] = defaultDuration;

    lace_.codecID_[i + 1] = track.codecID_.payload_.get();
  }

  // determine which track is the "main" track for starting Clusters:
  for (std::size_t i = 0; i < numTracks; i++)
  {
    Track & track = tracks[i].payload_;
    uint64 trackNo = track.trackNumber_.payload_.get();

    Track::MatroskaTrackType trackType = lace_.trackType_[i + 1];
    if (trackType == Track::kTrackTypeVideo)
    {
      lace_.cuesTrackNo_ = trackNo;
      break;
    }
    else if (trackType == Track::kTrackTypeAudio && !lace_.cuesTrackNo_)
    {
      lace_.cuesTrackNo_ = trackNo;
    }
  }

  if (!lace_.cuesTrackNo_ && numTracks)
  {
    lace_.cuesTrackNo_ = tracks[0].payload_.trackNumber_.payload_.get();
  }

  // add 2nd SeekHead:
  dstSeg_.payload_.seekHeads_.push_back(TSeekHead());
  TSeekHead & seekHeadElt = dstSeg_.payload_.seekHeads_.back();

  // index the 2nd SeekHead and Cues:
  SeekHead & seekHead1 = dstSeg_.payload_.seekHeads_.front().payload_;
  seekHead1.indexThis(&dstSeg_, &seekHeadElt);
  seekHead1.indexThis(&dstSeg_, &(dstSeg_.payload_.cues_));
}

//----------------------------------------------------------------
// overwriteUnknownPayloadSize
//
static bool
overwriteUnknownPayloadSize(IElement & elt, IStorage & storage)
{
  IStorage::IReceiptPtr receipt = elt.storageReceipt();
  if (!receipt)
  {
    return false;
  }

  IStorage::IReceiptPtr payload = elt.payloadReceipt();
  IStorage::IReceiptPtr payloadEnd = storage.receipt();

  uint64 payloadSize = payloadEnd->position() - payload->position();
  uint64 elementIdSize = uintNumBytes(TSegment::kId);

  IStorage::IReceiptPtr payloadSizeReceipt =
    receipt->receipt(elementIdSize, 8);

  unsigned char v[8];
  vsizeEncode(payloadSize, v, 8);
  return payloadSizeReceipt->save(v, 8);
}


//----------------------------------------------------------------
// TNal
//
struct TNal
{
  enum TUnitType
  {
    kUnspecified0 = 0,
    kCodedSliceNonIDR = 1,
    kCodedSliceDataA = 2,
    kCodedSliceDataB = 3,
    kCodedSliceDataC = 4,
    kCodedSliceIDR = 5,
    kSEI = 6,
    kSPS = 7,
    kPPS = 8,
    kAUD = 9,
    kEndOfSequence = 10,
    kEndOfStream = 11,
    kFillerData = 12,
    kReserved13 = 13,
    kReserved14 = 14,
    kReserved15 = 15,
    kReserved16 = 16,
    kReserved17 = 17,
    kReserved18 = 18,
    kReserved19 = 19,
    kReserved20 = 20,
    kReserved21 = 21,
    kReserved22 = 22,
    kReserved23 = 23,
    kUnspecified24 = 24,
    kUnspecified25 = 25,
    kUnspecified26 = 26,
    kUnspecified27 = 27,
    kUnspecified28 = 28,
    kUnspecified29 = 29,
    kUnspecified30 = 30,
    kUnspecified31 = 31
  };

  TNal(unsigned char head, uint64 offset, std::size_t size = 0):
    offset_(offset),
    size_(size)
  {
    forbidden_zero_bit_ = (head >> 7) & 0x01;
    nal_ref_idc_        = (head >> 5) & 0x03;
    nal_unit_type_      =  head       & 0x1F;
  }

  inline TUnitType unitType() const
  { return TUnitType(nal_unit_type_); }

  unsigned char forbidden_zero_bit_ : 1;
  unsigned char nal_ref_idc_        : 2;
  unsigned char nal_unit_type_      : 5;

  uint64 offset_;
  std::size_t size_;
};

//----------------------------------------------------------------
// TSei
//
struct TSei
{
  enum TMessageType
  {
    kBufferingPeriod = 0,
    kPicTiming = 1,
    kPanScanRect = 2,
    kFillerPayload = 3,
    kUserDataRegistered_ITU_T_T35 = 4,
    kUserDataUnregistered = 5,
    kRecoveryPoint = 6,
    kDecRefPicMarkingRepetition = 7,
    kSparePic = 8,
    kSceneInfo = 9,
    kSubSeqInfo = 10,
    kSubSeqLayerCharacteristics = 11,
    kSubSeqCharacteristics = 12,
    kFullFrameFreeze = 13,
    kFullFrameFreezeRelease = 14,
    kFullFrameSnapshot = 15,
    kProgressiveRefinementSegmentStart = 16,
    kProgressiveRefinementSegmentEnd = 17,
    kMotionConstrainedSliceGroupSet = 18
  };

  TSei(uint64 payloadType, uint64 offset, uint64 size):
    offset_(offset),
    size_((std::size_t)size)
  {
    type_ = TMessageType(payloadType);
  }

  TMessageType type_;
  uint64 offset_;
  std::size_t size_;
};

//----------------------------------------------------------------
// toText
//
static std::string
toText(TNal::TUnitType nalUnitType)
{
  std::ostringstream os;

  if (nalUnitType == TNal::kCodedSliceNonIDR)
  {
    os << "CodedSliceNonIDR";
  }
  else if (nalUnitType >= TNal::kCodedSliceDataA &&
           nalUnitType <= TNal::kCodedSliceDataC)
  {
    os << "CodedSliceData" << 'A' + int(nalUnitType - TNal::kCodedSliceDataA);
  }
  else if (nalUnitType == TNal::kCodedSliceIDR)
  {
    os << "CodedSliceIDR";
  }
  else if (nalUnitType == TNal::kSEI)
  {
    os << "SEI";
  }
  else if (nalUnitType == TNal::kSPS)
  {
    os << "SPS";
  }
  else if (nalUnitType == TNal::kPPS)
  {
    os << "PPS";
  }
  else if (nalUnitType == TNal::kAUD)
  {
    os << "AUD";
  }
  else if (nalUnitType == TNal::kEndOfSequence)
  {
    os << "EndOfSequence";
  }
  else if (nalUnitType == TNal::kEndOfStream)
  {
    os << "EndOfStream";
  }
  else if (nalUnitType == TNal::kFillerData)
  {
    os << "FillerData";
  }
  else if (nalUnitType >= TNal::kReserved13 &&
           nalUnitType <= TNal::kReserved23)
  {
    os << "Reserved" << int(nalUnitType);
  }
  else
  {
    os << "Unspecified" << int(nalUnitType);
  }

  return std::string(os.str().c_str());
}

//----------------------------------------------------------------
// toText
//
static std::string
toText(TSei::TMessageType seiMsgType)
{
  std::ostringstream os;

  if (seiMsgType == TSei::kBufferingPeriod)
  {
    os << "buffering_period";
  }
  else if (seiMsgType == TSei::kPicTiming)
  {
    os << "pic_timing";
  }
  else if (seiMsgType == TSei::kPanScanRect)
  {
    os << "pan_scan_rect";
  }
  else if (seiMsgType == TSei::kFillerPayload)
  {
    os << "filler_payload";
  }
  else if (seiMsgType == TSei::kUserDataRegistered_ITU_T_T35)
  {
    os << "user_data_registered_itu_t_t35";
  }
  else if (seiMsgType == TSei::kUserDataUnregistered)
  {
    os << "user_data_unregistered";
  }
  else if (seiMsgType == TSei::kRecoveryPoint)
  {
    os << "recovery_point";
  }
  else if (seiMsgType == TSei::kDecRefPicMarkingRepetition)
  {
    os << "dec_ref_pic_marking_repetition";
  }
  else if (seiMsgType == TSei::kSparePic)
  {
    os << "spare_pic";
  }
  else if (seiMsgType == TSei::kSceneInfo)
  {
    os << "scene_info";
  }
  else if (seiMsgType == TSei::kSubSeqInfo)
  {
    os << "sub_seq_info";
  }
  else if (seiMsgType == TSei::kSubSeqLayerCharacteristics)
  {
    os << "sub_seq_layer_characteristics";
  }
  else if (seiMsgType == TSei::kSubSeqCharacteristics)
  {
    os << "sub_seq_characteristics";
  }
  else if (seiMsgType == TSei::kFullFrameFreeze)
  {
    os << "full_frame_freeze";
  }
  else if (seiMsgType == TSei::kFullFrameFreezeRelease)
  {
    os << "full_frame_freeze_release";
  }
  else if (seiMsgType == TSei::kFullFrameSnapshot)
  {
    os << "full_frame_snapshot";
  }
  else if (seiMsgType == TSei::kProgressiveRefinementSegmentStart)
  {
    os << "progressive_refinement_segment_start";
  }
  else if (seiMsgType == TSei::kProgressiveRefinementSegmentEnd)
  {
    os << "progressive_refinement_segment_end";
  }
  else if (seiMsgType == TSei::kMotionConstrainedSliceGroupSet)
  {
    os << "motion_constrained_slice_group_set";
  }
  else
  {
    os << "Reserved" << int(seiMsgType);
  }

  return std::string(os.str().c_str());
}

//----------------------------------------------------------------
// isH264Keyframe
//
static bool
isH264Keyframe(const HodgePodge * data)
{
  std::size_t dataSize = data ? (std::size_t)data->numBytes() : 0;
  if (dataSize < 5)
  {
    return false;
  }

  HodgePodgeConstIter begin(*data);
  HodgePodgeConstIter iter(*data);
  HodgePodgeConstIter end(*data, dataSize);

  int numIDR = 0;
  int numNonIDR = 0;
  int numRecoveryPoint = 0;

  while (iter < end)
  {
    // 4 bytes payload size, followed by the payload
    unsigned int nalSize = ((iter[0] << 24) |
                            (iter[1] << 16) |
                            (iter[2] << 8) |
                            iter[3]);

    TNal nal(iter[4], (iter - begin) + 4, nalSize);
    TNal::TUnitType nalUnitType = nal.unitType();

#if 0
    std::cout << "\nNAL: " << toText(nalUnitType)
              << ", size " << nalSize << std::endl;
#endif

    if (nalUnitType == TNal::kCodedSliceIDR)
    {
      // return true;
      numIDR++;
    }
    else if (nalUnitType == TNal::kCodedSliceNonIDR ||
             nalUnitType >= TNal::kCodedSliceDataA &&
             nalUnitType <= TNal::kCodedSliceDataC)
    {
      numNonIDR++;
    }
    else if (nalUnitType == TNal::kSEI)
    {
#if 0
      std::cout << "\nNAL: " << toText(nalUnitType)
                << ", size " << nalSize << std::endl;
#endif

      uint64 j = 1;
      while (j < nalSize && iter[4 + j] != 0x80)
      {
        // extract SEI message type:
        uint64 payloadType = 0;
        for (; j < nalSize && iter[4 + j] == 0xFF; j++)
        {
          payloadType += 0xFF;
        }
        payloadType += iter[4 + j];
        j++;

        // extract SET message size:
        uint64 payloadSize = 0;
        for (; j < nalSize && iter[4 + j] == 0xFF; j++)
        {
          payloadSize += 0xFF;
        }
        payloadSize += iter[4 + j];
        j++;

        TSei sei(payloadType, (iter - begin) + 4 + j, payloadSize);
        if (sei.type_ == TSei::kRecoveryPoint)
        {
          numRecoveryPoint++;
        }

#if 0
        std::cout << " " << toText(sei.type_)
                  << ", size: " << payloadSize
                  << std::endl;
#endif

        // skip to the next SEI message:
        j += payloadSize;
      }

      assert(j == nalSize || j + 1 == nalSize && iter[4 + j] == 0x80);
    }
#if 0
    else if (nalUnitType != TNal::kSPS &&
             nalUnitType != TNal::kPPS)
    {
      std::cout << "\nNAL: " << toText(nalUnitType)
                << ", size " << nalSize << std::endl;
    }
#endif

    iter += (4 + nalSize);
    assert(iter <= end);
  }

  if (numIDR)
  {
    assert(!numNonIDR);
    return !numNonIDR;
  }

  bool intraRefresh = (numRecoveryPoint > 0);
  return intraRefresh;
}


//----------------------------------------------------------------
// TRemuxer::remux
//
void
TRemuxer::remux(uint64 inPointInMsec,
                uint64 outPointInMsec,
                bool extractFromKeyframe,
                bool fixKeyFlag)
{
  const std::list<TCluster> & clusters = srcSeg_.payload_.clusters_;
  std::list<TCluster>::const_iterator clusterIter = clusters.begin();

  uint64 oneSecond = NANOSEC_PER_SEC / lace_.timecodeScale_;
  t0_ = (oneSecond * inPointInMsec) / 1000;
  t1_ = (oneSecond * outPointInMsec) / 1000;

  extractTimeSegment_ = (t0_ < t1_);
  extractFromKeyframe_ = (extractTimeSegment_ && extractFromKeyframe);
  foundFirstKeyframe_ = false;

  if (extractTimeSegment_)
  {
    // use CuePoints to find the closest Cluster:
    const std::list<Cues::TCuePoint> & cuePoints =
      srcSeg_.payload_.cues_.payload_.points_;

    // lookup the source segment track number we are interested in:
    uint64 srcCuesTrackNo = (uint64)(~0);
    {
      TTrackMap::const_iterator found =
        trackDstSrc_.find(lace_.cuesTrackNo_);
      if (found != trackDstSrc_.end())
      {
        srcCuesTrackNo = found->second;
      }
    }

    // keep track of adjacent CuePoint times for the track we care about:
    uint64 ta = (uint64)(~0);
    uint64 tb = (uint64)(~0);

    const TCluster * found = NULL;
    for (std::list<Cues::TCuePoint>::const_iterator i = cuePoints.begin();
         i != cuePoints.end(); ++i)
    {
      const CuePoint & cuePoint = i->payload_;
      if (cuePoint.trkPosns_.empty())
      {
        continue;
      }

      const CueTrkPos & cueTrkPos =
        cuePoint.trkPosns_.front().payload_;
      uint64 trackNo = cueTrkPos.track_.payload_.get();
      if (trackNo != srcCuesTrackNo)
      {
        continue;
      }

      ta = tb;
      tb = cuePoint.time_.payload_.get();
      if (tb > t0_)
      {
        break;
      }

      const IElement * elt = cueTrkPos.cluster_.payload_.getElt();
      found = static_cast<const TCluster *>(elt);
    }

    if (tb != (uint64)(~0))
    {
      // decide which is the closest CuePoint:
      uint64 t = tb;

      if (ta != (uint64)(~0))
      {
        uint64 da = t0_ - ta;
        uint64 db = tb - t0_;
        if (da <= db)
        {
          t = ta;
        }
        else
        {
          t = tb;
        }
      }

      uint64 temp = t;
      uint64 msec = ((temp * 1000) / oneSecond) % 1000;
      temp /= oneSecond;
      uint64 sec = temp % 60;
      temp /= 60;
      uint64 min = temp % 60;
      temp /= 60;

      std::cout << "closest cue point: "
                << temp << "h, "
                << min << "m, "
                << sec << "s, "
                << msec << "ms"
                << std::endl;

      if (extractFromKeyframe_)
      {
        // adjust the in-point to the closest CuePoint:
        t0_ = t;
      }
    }

    // find the cluster that contains the in-point:
    for (std::list<TCluster>::const_iterator i = clusters.begin();
         i != clusters.end() && found; ++i)
    {
      const TCluster & clusterElt = *i;
      if (found == &clusterElt)
      {
        clusterIter = i;
        break;
      }
    }
  }

  for (; clusterIter != clusters.end(); ++clusterIter)
  {
    TCluster clusterElt = *clusterIter;

    uint64 position = clusterElt.storageReceipt()->position();
    uint64 numBytes = clusterElt.storageReceipt()->numBytes();
    clusterElt.discardReceipts();

    src_.file_.seek(position);
    uint64 bytesRead = clusterElt.load(src_, numBytes, this);
    assert(bytesRead == numBytes);

    Cluster & cluster = clusterElt.payload_;
    uint64 clusterTime = cluster.timecode_.payload_.get();

    if (extractTimeSegment_)
    {
      if (clusterTime > t1_ + oneSecond)
      {
        // finish once the out-point is reached:
        break;
      }

      if (clusterTime + oneSecond * 32 < t0_)
      {
        continue;
      }
    }

    // use SilentTracks as a cluster delimiter:
    if (cluster.silent_.mustSave())
    {
      push(clusterTime, &cluster.silent_);
    }

    // iterate through simple blocks and push them into a lace:
    const std::list<IElement *> & blocks = cluster.blocks_.elts();
    for (std::list<IElement *>::const_iterator i = blocks.begin();
         i != blocks.end(); ++i)
    {
      TBlockInfo * binfo = push(clusterTime, *i);
      if (!binfo)
      {
        continue;
      }

      if (fixKeyFlag)
      {
        if (lace_.codecID_[(std::size_t)binfo->trackNo_] == "V_MPEG4/ISO/AVC")
        {
          HodgePodge blockFrames;
          blockFrames.set(binfo->frames_);
          binfo->keyframe_ = isH264Keyframe(&blockFrames);
        }
      }

      mux();
    }
  }

  flush();

  // save the seek point table, rewrite the SeekHead size
  {
    TSeekHead & seekHead2 = dstSeg_.payload_.seekHeads_.back();
    seekHead2.setFixedSize(uintMax[8]);
    seekHead2.save(dst_);

    unsigned int pageSize = TDataTable<uint64>::TPage::kSize;
    unsigned int numPages = seekTable_.size_ / pageSize;
    if (seekTable_.size_ % pageSize)
    {
      numPages++;
    }

    TEightByteBuffer bvCluster = uintEncode(TCluster::kId);
    TEightByteBuffer bvSeekEntry = uintEncode(TSeekEntry::kId);
    TEightByteBuffer bvSeekId = uintEncode(SeekEntry::TId::kId);
    TEightByteBuffer bvSeekPos = uintEncode(SeekEntry::TPosition::kId);

    for (unsigned int i = 0; i < numPages; i++)
    {
      const TDataTable<uint64>::TPage * page = seekTable_.pages_[i];

      unsigned int size = pageSize;
      if ((i + 1) == numPages)
      {
        size = seekTable_.size_ % pageSize;
      }

      for (unsigned int j = 0; j < size; j++)
      {
        TEightByteBuffer bvPosition = uintEncode(page->data_[j]);

        TByteVec seekPointPayload;
        seekPointPayload
          << bvSeekId
          << vsizeEncode(bvCluster.n_)
          << bvCluster
          << bvSeekPos
          << vsizeEncode(bvPosition.n_)
          << bvPosition;

        TByteVec seekPoint;
        seekPoint
          << bvSeekEntry
          << vsizeEncode(seekPointPayload.size())
          << seekPointPayload;

        Yamka::save(dst_, seekPoint);
      }
    }

    // rewrite SeekHead size:
    overwriteUnknownPayloadSize(seekHead2, dst_);
  }

  // save the cue point table, rewrite the Cues size
  {
    TCues & cuesElt = dstSeg_.payload_.cues_;
    cuesElt.setFixedSize(uintMax[8]);
    cuesElt.save(dst_);

    unsigned int pageSize = TDataTable<uint64>::TPage::kSize;
    unsigned int numPages = cueTable_.size_ / pageSize;
    if (cueTable_.size_ % pageSize)
    {
      numPages++;
    }

    TEightByteBuffer bvCuePoint = uintEncode(TCuePoint::kId);
    TEightByteBuffer bvCueTime = uintEncode(CuePoint::TTime::kId);
    TEightByteBuffer bvCueTrkPos = uintEncode(CuePoint::TCueTrkPos::kId);
    TEightByteBuffer bvCueTrack = uintEncode(CueTrkPos::TTrack::kId);
    TEightByteBuffer bvCueCluster = uintEncode(CueTrkPos::TCluster::kId);
    TEightByteBuffer bvCueBlock = uintEncode(CueTrkPos::TBlock::kId);

    for (unsigned int i = 0; i < numPages; i++)
    {
      const TDataTable<TCue>::TPage * page = cueTable_.pages_[i];

      unsigned int size = pageSize;
      if ((i + 1) == numPages)
      {
        size = cueTable_.size_ % pageSize;
      }

      for (unsigned int j = 0; j < size; j++)
      {
        // shortcut to Cue data:
        const TCue & cue = page->data_[j];

        TEightByteBuffer bvPosition = uintEncode(cue.cluster_);
        TEightByteBuffer bvBlock = uintEncode(cue.block_);
        TEightByteBuffer bvTrack = uintEncode(cue.track_);
        TEightByteBuffer bvTime = uintEncode(cue.time_);

        TByteVec cueTrkPosPayload;
        cueTrkPosPayload
          << bvCueTrack
          << vsizeEncode(bvTrack.n_)
          << bvTrack
          << bvCueCluster
          << vsizeEncode(bvPosition.n_)
          << bvPosition;

        if (cue.block_ > 1)
        {
          cueTrkPosPayload
            << bvCueBlock
            << vsizeEncode(bvBlock.n_)
            << bvBlock;
        }

        TByteVec cuePointPayload;
        cuePointPayload
          << bvCueTime
          << vsizeEncode(bvTime.n_)
          << bvTime
          << bvCueTrkPos
          << vsizeEncode(cueTrkPosPayload.size())
          << cueTrkPosPayload;

        TByteVec cuePoint;
        cuePoint
          << bvCuePoint
          << vsizeEncode(cuePointPayload.size())
          << cuePointPayload;

        Yamka::save(dst_, cuePoint);
      }
    }

    // rewrite Cues size:
    overwriteUnknownPayloadSize(cuesElt, dst_);
  }
}

//----------------------------------------------------------------
// TRemuxer::flush
//
void
TRemuxer::flush()
{
  while (lace_.size_)
  {
    mux(lace_.size_);
  }

  finishCurrentCluster();
}

//----------------------------------------------------------------
// TRemuxer::isRelevant
//
bool
TRemuxer::isRelevant(uint64 clusterTime, TBlockInfo & binfo)
{
  HodgePodge * blockData = binfo.getBlockData();
  if (!blockData)
  {
    return false;
  }

  // check whether the given block is for a track we are interested in:
  uint64 bytesRead = binfo.block_.importData(*blockData);
  if (!bytesRead)
  {
    assert(false);
    return false;
  }

  uint64 srcTrackNo = binfo.block_.getTrackNumber();
  TTrackMap::const_iterator found = trackSrcDst_.find(srcTrackNo);
  if (found == trackSrcDst_.end())
  {
    return false;
  }

  const uint64 blockSize = blockData->numBytes();
  HodgePodgeConstIter blockDataIter(*blockData);
  binfo.header_ = blockDataIter.receipt(0, bytesRead);
  binfo.frames_ = blockDataIter.receipt(bytesRead, blockSize - bytesRead);

  binfo.trackNo_ = found->second;
  binfo.pts_ = clusterTime + binfo.block_.getRelativeTimecode();
  binfo.dts_ = binfo.pts_;

  Track::MatroskaTrackType trackType =
    lace_.trackType_[(std::size_t)(binfo.trackNo_)];

  binfo.keyframe_ = binfo.block_.isKeyframe();

  if (!binfo.keyframe_)
  {
    if (trackType == Track::kTrackTypeSubtitle ||
        trackType == Track::kTrackTypeAudio)
    {
      binfo.keyframe_ = true;
    }
    else if (trackType == Track::kTrackTypeVideo &&
             binfo.bgroupElt_.mustSave())
    {
      const BlockGroup & bg = binfo.bgroupElt_.payload_;
      binfo.keyframe_ = bg.refBlock_.empty();
    }
  }

  if (extractFromKeyframe_ && !foundFirstKeyframe_)
  {
    if (!binfo.keyframe_ || binfo.trackNo_ != lace_.cuesTrackNo_)
    {
      return false;
    }

    // adjust the in-point:
    t0_ = binfo.pts_;
    foundFirstKeyframe_ = true;
  }

  return true;
}

//----------------------------------------------------------------
// TRemuxer::updateHeader
//
bool
TRemuxer::updateHeader(uint64 clusterTime, TBlockInfo & binfo)
{
  int64 dstBlockTime = binfo.pts_ - clusterTime;
  assert(dstBlockTime >= kMinShort &&
         dstBlockTime <= kMaxShort);

  bool dstIsKeyframe = binfo.keyframe_;
  if (binfo.bgroupElt_.mustSave())
  {
    // BlockGroup blocks do not have a keyframe flag:
    dstIsKeyframe = false;
  }

  uint64 srcTrackNo = binfo.block_.getTrackNumber();
  short int srcBlockTime = binfo.block_.getRelativeTimecode();
  bool srcIsKeyframe = binfo.block_.isKeyframe();

  if (srcTrackNo == binfo.trackNo_ &&
      srcBlockTime == dstBlockTime &&
      srcIsKeyframe == dstIsKeyframe)
  {
    // nothing changed:
    return true;
  }

  HodgePodge * blockData = binfo.getBlockData();
  if (!blockData)
  {
    return false;
  }

  binfo.block_.setTrackNumber(binfo.trackNo_);
  binfo.block_.setRelativeTimecode((short int)(dstBlockTime));
  binfo.block_.setKeyframe(dstIsKeyframe);

  binfo.header_ = binfo.block_.writeHeader(tmp_);

  blockData->set(binfo.header_);
  blockData->add(binfo.frames_);

  return true;
}

//----------------------------------------------------------------
// TRemuxer::push
//
TBlockInfo *
TRemuxer::push(uint64 clusterTime, const IElement * elt)
{
  TBlockInfo * info = new TBlockInfo();

  uint64 eltId = elt->getId();
  switch (eltId)
  {
    case TBlockGroup::kId:
      info->bgroupElt_ = *((const TBlockGroup *)elt);
      info->duration_ = info->bgroupElt_.payload_.duration_.payload_.get();
      break;

    case TSimpleBlock::kId:
      info->sblockElt_ = *((const TSimpleBlock *)elt);
      break;

    case TEncryptedBlock::kId:
      info->eblockElt_ = *((const TEncryptedBlock *)elt);
      break;

    case TSilentTracks::kId:
      info->silentElt_ = *((const TSilentTracks *)elt);
      info->pts_ = clusterTime;
      info->dts_ = clusterTime;
      break;

    default:
      assert(false);
      delete info;
      return NULL;
  }

  if (eltId != TSilentTracks::kId && !isRelevant(clusterTime, *info))
  {
    delete info;
    return NULL;
  }

  lace_.push(info);
  return info;
}

//----------------------------------------------------------------
// TRemuxer::mux
//
void
TRemuxer::mux(std::size_t minLaceSize)
{
  if (lace_.size_ < minLaceSize)
  {
    return;
  }

  TBlockInfo * binfo = lace_.pop();
  uint64 clusterTime = clusterElt_.payload_.timecode_.payload_.get();
  int64 blockTime = binfo->pts_ - clusterTime;

  if (extractTimeSegment_)
  {
    if ((binfo->pts_ > t1_) ||
        (binfo->pts_ + binfo->duration_ <= t0_))
    {
      delete binfo;
      return;
    }
  }

  if ((binfo->trackNo_ == 0) ||
      (blockTime > kMaxShort) ||
      (binfo->trackNo_ == lace_.cuesTrackNo_ &&
       binfo->keyframe_ &&
       cuesTrackHasKeyframes_ == 1) ||
      (!clusterElt_.storageReceipt()))
  {
    startNextCluster(binfo);

    clusterTime = clusterElt_.payload_.timecode_.payload_.get();
    blockTime = binfo->pts_ - clusterTime;
  }

  if (binfo->trackNo_ && updateHeader(clusterTime, *binfo))
  {
    Cluster & cluster = clusterElt_.payload_;

    if (binfo->sblockElt_.mustSave())
    {
      cluster.blocks_.push_back(binfo->sblockElt_);
      binfo->sblockElt_ = cluster.blocks_.back<TSimpleBlock>();
    }
    else if (binfo->bgroupElt_.mustSave())
    {
      cluster.blocks_.push_back(binfo->bgroupElt_);
      binfo->bgroupElt_ = cluster.blocks_.back<TBlockGroup>();
    }
    else if (binfo->eblockElt_.mustSave())
    {
      cluster.blocks_.push_back(binfo->eblockElt_);
      binfo->eblockElt_ = cluster.blocks_.back<TEncryptedBlock>();
    }

    binfo->save(dst_);

    clusterBlocks_++;
    addCuePoint(binfo);
  }

  delete binfo;
}

//----------------------------------------------------------------
// TRemuxer::startNextCluster
//
void
TRemuxer::startNextCluster(TBlockInfo * binfo)
{
  finishCurrentCluster();

  // start a new cluster:
  clusterBlocks_ = 0;
  cuesTrackHasKeyframes_ = 0;
  clusterElt_ = TCluster();

  // set the start time:
  clusterElt_.payload_.timecode_.payload_.set(binfo->pts_);

  if (!binfo->trackNo_ && binfo->silentElt_.mustSave())
  {
    // copy the SilentTracks, update track numbers to match:
    typedef SilentTracks::TTrack TTrkNumElt;
    typedef std::list<TTrkNumElt> TTrkNumElts;

    clusterElt_.payload_.silent_.alwaysSave();

    const TTrkNumElts & srcSilentTracks =
      binfo->silentElt_.payload_.tracks_;

    TTrkNumElts & dstSilentTracks =
      clusterElt_.payload_.silent_.payload_.tracks_;

    for (TTrkNumElts::const_iterator i = srcSilentTracks.begin();
         i != srcSilentTracks.end(); ++i)
    {
      const TTrkNumElt & srcTnElt = *i;
      uint64 srcTrackNo = srcTnElt.payload_.get();

      TTrackMap::const_iterator found = trackSrcDst_.find(srcTrackNo);
      if (found != trackSrcDst_.end())
      {
        dstSilentTracks.push_back(TTrkNumElt());
        TTrkNumElt & dstTnElt = dstSilentTracks.back();

        uint64 dstTrackNo = found->second;
        dstTnElt.payload_.set(dstTrackNo);
      }
    }
  }

  clusterElt_.savePaddedUpToSize(dst_, uintMax[8]);

  // save relative cluster position for 2nd SeekHead:
  uint64 absPos = clusterElt_.storageReceipt()->position();
  clusterRelativePosition_ = absPos - segmentPayloadPosition_;
  seekTable_.add(clusterRelativePosition_);
}

//----------------------------------------------------------------
// TRemuxed::finishCurrentCluster
//
void
TRemuxer::finishCurrentCluster()
{
  // fix the cluster size:
  overwriteUnknownPayloadSize(clusterElt_, dst_);
}

//----------------------------------------------------------------
// TRemuxer::addCuePoint
//
void
TRemuxer::addCuePoint(TBlockInfo * binfo)
{
  if (!binfo->keyframe_)
  {
    return;
  }

  if (binfo->trackNo_ == lace_.cuesTrackNo_)
  {
    cuesTrackHasKeyframes_++;

    // reset the CuePoint barrier:
    needCuePointForTrack_.assign(needCuePointForTrack_.size(), true);
  }

  if (!needCuePointForTrack_[(std::size_t)(binfo->trackNo_)])
  {
    // avoid making too many CuePoints:
    return;
  }

  needCuePointForTrack_[(std::size_t)(binfo->trackNo_)] = false;

  TCue cue;
  cue.time_ = binfo->pts_;
  cue.track_ = binfo->trackNo_;
  cue.cluster_ = clusterRelativePosition_;
  cue.block_ = clusterBlocks_;
  cueTable_.add(cue);
}

//----------------------------------------------------------------
// usage
//
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "NOTE: remuxing input files containing multiple segments "
            << "with mismatched tracks will not work correctly"
            << std::endl;

  std::cerr << "USAGE: " << argv[0]
            << " -i input.mkv -o output.mkv [-t trackNo | +t trackNo]* "
            << "[[-t0|-k0]  hh mm ss msec] [-t1 hh mm ss msec] "
            << "[-fixKeyFlag]"
            << std::endl;

  std::cerr << "EXAMPLE: " << argv[0]
            << " -i input.mkv -o output.mkv +t 1 +t 2"
            << " -k0 00 15 30 000 -t1 00 17 00 000"
            << std::endl;

  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }

  ::exit(1);
}

//----------------------------------------------------------------
// usage
//
inline static void
usage(char ** argv, const std::string & message)
{
  usage(argv, message.c_str());
}

//----------------------------------------------------------------
// main
//
int
main(int argc, char ** argv)
{
#ifdef _WIN32
  get_main_args_utf8(argc, argv);
#endif

  printCurrentTime("start");

  std::string srcPath;
  std::string dstPath;
  std::list<uint64> tracksToKeep;
  std::list<uint64> tracksDelete;

  // time segment extraction region,
  // t0 and t1 are expressed in milliseconds:
  bool extractFromKeyframe = false;
  uint64 t0 = 0;
  uint64 t1 = 0;

  // yamkaRemux r166, r167 wiped out the SimpleBlock keyframe flag
  // and created CuePoints for non-keyframes:
  bool fixKeyFlag = false;

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-i") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -i parameter");
      i++;
      srcPath.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-o") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -o parameter");
      i++;
      dstPath.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-t") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -t parameter");
      i++;

      if (!tracksToKeep.empty())
      {
        usage(argv, "-t and +t parameter can not be used together");
      }

      uint64 trackNo = toScalar<uint64>(argv[i]);
      if (trackNo == 0)
      {
        usage(argv, "could not parse -t parameter value");
      }
      else
      {
        tracksDelete.push_back(trackNo);
      }
    }
    else if (strcmp(argv[i], "+t") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse +t parameter");
      i++;

      if (!tracksDelete.empty())
      {
        usage(argv, "-t and +t parameter can not be used together");
      }

      uint64 trackNo = toScalar<uint64>(argv[i]);
      if (trackNo == 0)
      {
        usage(argv, "could not parse +t parameter value");
      }
      else
      {
        tracksToKeep.push_back(trackNo);
      }
    }
    else if (strcmp(argv[i], "-t0") == 0 ||
             strcmp(argv[i], "-k0") == 0)
    {
      if ((argc - i) <= 4)
      {
        usage(argv, (std::string("could not parse ") + argv[i] +
                     std::string(" parameter")));
      }

      extractFromKeyframe = (strcmp(argv[i], "-k0") == 0);

      i++;
      uint64 hh = toScalar<uint64>(argv[i]);

      i++;
      uint64 mm = toScalar<uint64>(argv[i]);

      i++;
      uint64 ss = toScalar<uint64>(argv[i]);

      i++;
      uint64 ms = toScalar<uint64>(argv[i]);

      t0 = ms + 1000 * (ss + 60 * (mm + 60 * hh));
    }
    else if (strcmp(argv[i], "-t1") == 0)
    {
      if ((argc - i) <= 4) usage(argv, "could not parse -t1 parameter");

      i++;
      uint64 hh = toScalar<uint64>(argv[i]);

      i++;
      uint64 mm = toScalar<uint64>(argv[i]);

      i++;
      uint64 ss = toScalar<uint64>(argv[i]);

      i++;
      uint64 ms = toScalar<uint64>(argv[i]);

      t1 = ms + 1000 * (ss + 60 * (mm + 60 * hh));
    }
    else if (strcmp(argv[i], "-fixKeyFlag") == 0)
    {
      fixKeyFlag = true;
    }
    else
    {
      usage(argv, (std::string("unknown option: ") +
                   std::string(argv[i])).c_str());
    }
  }

  if (t0 > t1)
  {
    usage(argv,
          "start time (-t0 hh mm ss ms) is greater than "
          "finish time (-t1 hh mm ss ms)");
  }

  bool keepAllTracks = tracksToKeep.empty() && tracksDelete.empty();

  FileStorage src(srcPath, File::kReadOnly);
  if (!src.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 srcPath +
                 std::string(" for reading")).c_str());
  }

  bool extractTimeSegment = (t0 < t1);
  bool skipCues = !extractTimeSegment;
  uint64 srcSize = src.file_.size();
  MatroskaDoc doc;
  LoaderSkipClusterPayload skipClusters(srcSize, skipCues);

  // attempt to load via SeekHead(s):
  printCurrentTime("doc.loadSeekHead");
  bool ok = doc.loadSeekHead(src, srcSize);

  if (ok)
  {
    printCurrentTime("doc.loadViaSeekHead");
    ok = doc.loadViaSeekHead(src, &skipClusters, true);
  }

  if (!ok ||
      !doc.segments_.empty() &&
      doc.segments_.front().payload_.clusters_.empty())
  {
    std::cout << "failed to find Clusters via SeekHead, "
              << "attempting brute force"
              << std::endl;

    doc = MatroskaDoc();
    src.file_.seek(0);

    printCurrentTime("doc.loadAndKeepReceipts");
    doc.loadAndKeepReceipts(src, srcSize, &skipClusters);
  }

  if (doc.segments_.empty())
  {
    usage(argv, (std::string("failed to load any matroska segments").c_str()));
  }

  printCurrentTime("doc.load... finished");

  std::size_t numSegments = doc.segments_.size();
  std::vector<std::map<uint64, uint64> > trackSrcDst(numSegments);
  std::vector<std::map<uint64, uint64> > trackDstSrc(numSegments);
  std::vector<std::vector<std::list<Frame> > > segmentTrackFrames(numSegments);

  // verify that the specified tracks exist:
  std::size_t segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    std::map<uint64, uint64> & trackInOut = trackSrcDst[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = trackDstSrc[segmentIndex];

    const Segment & segment = i->payload_;

    const std::deque<TTrack> & tracks = segment.tracks_.payload_.tracks_;
    for (std::deque<TTrack>::const_iterator j = tracks.begin();
         j != tracks.end(); ++j)
    {
      const Track & track = j->payload_;
      uint64 trackNo = track.trackNumber_.payload_.get();
      uint64 trackNoOut = trackInOut.size() + 1;

      if (!tracksToKeep.empty() && has(tracksToKeep, trackNo))
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
      else if (!tracksDelete.empty() && !has(tracksDelete, trackNo))
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
      else if (keepAllTracks)
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
    }

    if (trackOutIn.empty())
    {
      usage(argv,
            std::string("segment ") +
            toText(segmentIndex + 1) +
            std::string(", none of the specified input tracks exist"));
    }

    std::size_t numTracks = trackInOut.size();
    segmentTrackFrames[segmentIndex].resize(numTracks);
  }

  FileStorage dst(dstPath, File::kReadWrite);
  if (!dst.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 dstPath +
                 std::string(" for writing")).c_str());
  }

  MemoryStorage tmp;
  MatroskaDoc out;

  // keep the original DocType:
  out.head_.payload_.docTypeVersion_ =
    doc.head_.payload_.docTypeVersion_;
  out.head_.payload_.docTypeReadVersion_ =
    doc.head_.payload_.docTypeReadVersion_;

  dst.file_.setSize(0);
  out.save(dst);

  segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    printCurrentTime("parse next segment");

    TSegment segmentElt;
    segmentElt.setFixedSize(uintMax[8]);

    Segment & segment = segmentElt.payload_;
    const Segment & segmentIn = i->payload_;

    // add first SeekHead, written before clusters:
    segment.seekHeads_.push_back(TSeekHead());
    TSeekHead & seekHeadElt = segment.seekHeads_.back();
    seekHeadElt.setFixedSize(256);
    SeekHead & seekHead = seekHeadElt.payload_;

    // copy segment info:
    TSegInfo & segInfoElt = segment.info_;
    SegInfo & segInfo = segInfoElt.payload_;
    const SegInfo & segInfoIn = segmentIn.info_.payload_;

    segInfo = segInfoIn;
    segInfo.muxingApp_.payload_.set(segInfo.muxingApp_.payload_.getDefault());
    segInfo.writingApp_.payload_.set("yamkaRemux rev." YAMKA_REVISION);

    // segment timecode scale, such that
    // timeInNanosec := timecodeScale * (clusterTime + blockTime):
    uint64 timecodeScale = segInfo.timecodeScale_.payload_.get();

    // copy track info:
    TTracks & tracksElt = segment.tracks_;
    std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
    const std::deque<TTrack> & tracksIn = segmentIn.tracks_.payload_.tracks_;

    std::map<uint64, uint64> & trackInOut = trackSrcDst[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = trackDstSrc[segmentIndex];
    uint64 trackMapSize = trackOutIn.size();

    for (uint64 trackNo = 1; trackNo <= trackMapSize; trackNo++)
    {
      tracks.push_back(TTrack());
      Track & track = tracks.back().payload_;

      uint64 trackNoIn = trackOutIn[trackNo];
      const Track * trackIn = getTrack(tracksIn, trackNoIn);
      if (trackIn)
      {
        track = *trackIn;
      }
      else
      {
        assert(false);
      }

      track.trackNumber_.payload_.set(trackNo);
      track.flagLacing_.payload_.set(1);
    }

    // index the first set of top-level elements:
    seekHead.indexThis(&segmentElt, &segInfoElt);
    seekHead.indexThis(&segmentElt, &tracksElt);

    TAttachment & attachmentsElt = segmentElt.payload_.attachments_;
    attachmentsElt = segmentIn.attachments_;
    if (attachmentsElt.mustSave())
    {
      seekHead.indexThis(&segmentElt, &attachmentsElt);
    }

    TChapters & chaptersElt = segmentElt.payload_.chapters_;
    chaptersElt = segmentIn.chapters_;
    if (chaptersElt.mustSave())
    {
      seekHead.indexThis(&segmentElt, &chaptersElt);
    }

    // minor cleanup prior to saving:
    RemoveVoids().eval(segmentElt);

    // discard previous storage receipts:
    DiscardReceipts().eval(segmentElt);

    // save a placeholder:
    IStorage::IReceiptPtr segmentReceipt = segmentElt.save(dst);

    printCurrentTime("begin segment remux");

    // on-the-fly remux Clusters/BlockGroups/SimpleBlocks:
    TRemuxer remuxer(trackInOut, trackOutIn, *i, segmentElt, src, dst, tmp);
    remuxer.remux(t0, t1, extractFromKeyframe, fixKeyFlag);

    printCurrentTime("finished segment remux");

    // rewrite the SeekHead:
    {
      TSeekHead & seekHead = segmentElt.payload_.seekHeads_.front();
      IStorage::IReceiptPtr receipt = seekHead.storageReceipt();

      File::Seek autoRestorePosition(dst.file_);
      dst.file_.seek(receipt->position());

      seekHead.save(dst);
    }

    // rewrite element position references (second pass):
    RewriteReferences().eval(segmentElt);

    // rewrite the segment payload size:
    overwriteUnknownPayloadSize(segmentElt, dst);
  }

  // close open file handles:
  src.file_.close();
  dst.file_.close();

  printCurrentTime("done");

  // avoid waiting for all the destructors to be called:
  ::exit(0);

  doc = MatroskaDoc();
  out = WebmDoc(kFileFormatMatroska);

  return 0;
}

//  +t 1 +t 2 +t 7 -k0 00 17 00 000 -t1 00 19 00 000

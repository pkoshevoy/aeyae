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
#include <yamkaMixedElements.h>
#include <yamkaVersion.h>

// system includes:
#include <limits>
#include <sstream>
#include <iostream>
#include <string.h>
#include <string>
#include <time.h>
#include <map>

// forward declarations:
struct TTodo;

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
template <typename TScalar, typename TString>
static TScalar
toScalar(const TString & text)
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
// getOffset
//
static double
getOffset(const std::map<uint64, double> & tsOffset, uint64 trackNo)
{
  std::map<uint64, double>::const_iterator found = tsOffset.find(trackNo);
  return (found == tsOffset.end()) ? 0.0 : found->second;
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
  {
    content_.addType<TSilentTracks>();
    content_.addType<TBlockGroup>();
    content_.addType<TSimpleBlock>();
    content_.addType<TEncryptedBlock>();
  }

  inline HodgePodge * getBlockData()
  {
    if (content_.mustSave())
    {
      IElement * e = content_.elts().front();
      const uint64 id = e->getId();

      if (id == TSimpleBlock::kId)
      {
        TSimpleBlock * sblockElt = (TSimpleBlock *)e;
        return &(sblockElt->payload_.data_);
      }
      else if (id == TBlockGroup::kId)
      {
        TBlockGroup * bgroupElt = (TBlockGroup *)e;
        return &(bgroupElt->payload_.block_.payload_.data_);
      }
      else if (id == TEncryptedBlock::kId)
      {
        TEncryptedBlock * eblockElt = (TEncryptedBlock *)e;
        return &(eblockElt->payload_.data_);
      }
    }

    assert(false);
    return NULL;
  }

  inline bool contains(const uint64 id) const
  {
    if (!content_.elts().empty())
    {
      IElement * e = content_.elts().front();
      const uint64 eltId = e->getId();
      return eltId == id;
    }

    return false;
  }

  IStorage::IReceiptPtr save(FileStorage & storage)
  {
    if (content_.mustSave())
    {
      return content_.save(storage);
    }

    assert(false);
    return IStorage::IReceiptPtr();
  }

  // SilentTracks, BlockGroups, SimpleBlocks, EncryptedBlocks:
  MixedElements content_;

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
  uint64 relPos_;
};

//----------------------------------------------------------------
// TRemuxer
//
struct TRemuxer : public LoadWithProgress
{
  TRemuxer(const std::map<uint64, double> & tsOffset,
           const TTrackMap & trackSrcDst,
           const TTrackMap & trackDstSrc,
           const TSegment & srcSeg,
           TSegment & dstSeg,
           FileStorage & src,
           FileStorage & dst,
           IStorage & tmp);

  void remux(uint64 t0,
             uint64 t1,
             bool extractFromKeyframe,
             bool fixKeyFlag);
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
  void addCuePoint(TBlockInfo * binfo, const IStorage::IReceiptPtr & receipt);

  const std::map<uint64, double> & tsOffset_;
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
  uint64 clusterPayloadPosition_;

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
TRemuxer::TRemuxer(const std::map<uint64, double> & tsOffset,
                   const TTrackMap & trackSrcDst,
                   const TTrackMap & trackDstSrc,
                   const TSegment & srcSeg,
                   TSegment & dstSeg,
                   FileStorage & src,
                   FileStorage & dst,
                   IStorage & tmp):
  LoadWithProgress(src.file_.size()),
  tsOffset_(tsOffset),
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
  clusterPayloadPosition_(uintMax[8]),
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
             (nalUnitType >= TNal::kCodedSliceDataA &&
              nalUnitType <= TNal::kCodedSliceDataC))
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

      const CueTrackPositions & cueTrkPos =
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
    TEightByteBuffer bvCueTrack = uintEncode(CueTrackPositions::TTrack::kId);
    TEightByteBuffer bvCueClstr = uintEncode(CueTrackPositions::TCluster::kId);
    TEightByteBuffer bvCueBlock = uintEncode(CueTrackPositions::TBlock::kId);
    TEightByteBuffer bvCueRelPos =
      uintEncode(CueTrackPositions::TRelativePos::kId);

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
        TEightByteBuffer bvRelPos = uintEncode(cue.relPos_);
        TEightByteBuffer bvBlock = uintEncode(cue.block_);
        TEightByteBuffer bvTrack = uintEncode(cue.track_);
        TEightByteBuffer bvTime = uintEncode(cue.time_);

        TByteVec cueTrkPosPayload;
        cueTrkPosPayload
          << bvCueTrack
          << vsizeEncode(bvTrack.n_)
          << bvTrack
          << bvCueClstr
          << vsizeEncode(bvPosition.n_)
          << bvPosition
          << bvCueRelPos
          << vsizeEncode(bvRelPos.n_)
          << bvRelPos;

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

  // lookup timestamp offset:
  double dtSeconds = getOffset(tsOffset_, srcTrackNo);
  int64 dt = (int64)(double(NANOSEC_PER_SEC / lace_.timecodeScale_) *
                     dtSeconds);

  const uint64 blockSize = blockData->numBytes();
  HodgePodgeConstIter blockDataIter(*blockData);
  binfo.header_ = blockDataIter.receipt(0, bytesRead);
  binfo.frames_ = blockDataIter.receipt(bytesRead, blockSize - bytesRead);

  binfo.trackNo_ = found->second;
  binfo.pts_ = clusterTime + binfo.block_.getRelativeTimecode();
  binfo.dts_ = binfo.pts_;

  if (dt < 0 && (binfo.pts_ < -dt || binfo.dts_ < -dt))
  {
    return false;
  }

  binfo.pts_ += dt;
  binfo.dts_ += dt;

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
             binfo.contains(TBlockGroup::kId) &&
             binfo.content_.mustSave())
    {
      TBlockGroup * bgroupElt = binfo.content_.find<TBlockGroup>();
      const BlockGroup & bg = bgroupElt->payload_;
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
  if (binfo.contains(TBlockGroup::kId))
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

  if (!elt || !info->content_.push_back(*elt))
  {
    assert(false);
    delete info;
    return NULL;
  }

  const uint64 eltId = elt->getId();
  if (eltId == TBlockGroup::kId)
  {
    const TBlockGroup * bgroupElt = (const TBlockGroup *)elt;
    info->duration_ = bgroupElt->payload_.duration_.payload_.get();
  }
  else if (eltId == TSilentTracks::kId)
  {
    info->pts_ = clusterTime;
    info->dts_ = clusterTime;
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
    IElement * e = binfo->content_.elts().front();
    const uint64 id = e->getId();

    if (id == TSimpleBlock::kId ||
        id == TBlockGroup::kId ||
        id == TEncryptedBlock::kId)
    {
      Cluster & cluster = clusterElt_.payload_;
      cluster.blocks_.push_back(*e);
    }

    IStorage::IReceiptPtr receipt = binfo->save(dst_);

    clusterBlocks_++;
    addCuePoint(binfo, receipt);
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

  if (!binfo->trackNo_ && binfo->contains(TSilentTracks::kId))
  {
    // copy the SilentTracks, update track numbers to match:
    typedef SilentTracks::TTrack TTrkNumElt;
    typedef std::list<TTrkNumElt> TTrkNumElts;

    clusterElt_.payload_.silent_.alwaysSave();

    TSilentTracks * silentElt =
      (TSilentTracks *)binfo->content_.elts().front();

    const TTrkNumElts & srcSilentTracks =
      silentElt->payload_.tracks_;

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
  clusterPayloadPosition_ = clusterElt_.payloadReceipt()->position();

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
TRemuxer::addCuePoint(TBlockInfo * binfo,
                      const IStorage::IReceiptPtr & receipt)
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
  cue.relPos_ = receipt->position() - clusterPayloadPosition_;

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
            << " -i input.mkv -o output.mkv\n"
            << " [-t trackNo | +t trackNo]*\n"
            << " [-t0|-k0  hh mm ss msec]\n"
            << " [-t1 hh mm ss msec]\n"
            << " [--fix-keyframe-flag]\n"
            << " [--attach mime/type attachment.xyz]\n"
            << " [--copy-codec-private fromhere.mkv]\n"
            << " [--save-chapters output.txt]\n"
            << " [--load-chapters input.txt]\n"
            << " [--lang trackNo lang]\n"
            << " [+dt trackNo secondsToAdd]\n"
            << std::endl;

  std::cerr << "EXAMPLE: " << argv[0]
            << " -i input.mkv -o output.mkv +t 1 +t 2"
            << " -k0 00 15 30 000 -t1 00 17 00 000"
            << " --lang 1 eng --lang 2 jpn"
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
// TTodoFunc
//
typedef void(*TTodoFunc)(MatroskaDoc &, const TTodo &, char **);

//----------------------------------------------------------------
// TTodo
//
// something to do prior to remuxing
//
struct TTodo
{
  TTodo(TTodoFunc func):
    func_(func)
  {}

  void addParam(const char * name, const char * value)
  {
    params_[std::string(name)] = std::string(value);
  }

  std::string getParam(const char * name) const
  {
    std::map<std::string, std::string>::const_iterator found =
      params_.find(std::string(name));

    if (found != params_.end())
    {
      return found->second;
    }

    return std::string();
  }

  void execute(MatroskaDoc & doc, char ** argv) const
  {
    func_(doc, *this, argv);
  }

  TTodoFunc func_;
  std::map<std::string, std::string> params_;
};

//----------------------------------------------------------------
// addAttachment
//
static void
addAttachment(MatroskaDoc & doc, const TTodo & todo, char ** argv)
{
  std::string attType = todo.getParam("type");
  std::string attPath = todo.getParam("file");

  FileStorage att(attPath, File::kReadOnly);
  if (!att.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 attPath +
                 std::string(" for reading")));
  }

  uint64 attSize = att.file_.size();
  assert(attSize < uint64(std::numeric_limits<std::size_t>::max()));
  if (!attSize)
  {
    usage(argv, std::string("file is empty - ") + attPath);
  }

  // don't load anything into memory, use a storage receipt instead:
  att.file_.seek(0);
  IStorage::IReceiptPtr attReceipt = att.receipt();
  attReceipt->setNumBytes(attSize);

  // get the last segment:
  MatroskaDoc::TSegment & segment = doc.segments_.back();

  // shortcut to the attachments element:
  Segment::TAttachment & attachments = segment.payload_.attachments_;

  // create an attached file element:
  attachments.payload_.files_.push_back(Attachments::TFile());
  Attachments::TFile & attachment = attachments.payload_.files_.back();

  attachment.payload_.description_.payload_.
      set("put optional attachment description here");

  attachment.payload_.mimeType_.payload_.set(attType);
  attachment.payload_.filename_.payload_.set(attPath);
  attachment.payload_.data_.payload_.set(attReceipt);

  uint64 attUID = Yamka::createUID();
  attachment.payload_.fileUID_.payload_.set(attUID);

  // add attachment to the seekhead index:
  if (segment.payload_.seekHeads_.empty())
  {
      // this should't happen, but if the SeekHead element
      // is missing -- add it:
      segment.payload_.seekHeads_.push_back(Segment::TSeekHead());
  }

  Segment::TSeekHead & seekHead = segment.payload_.seekHeads_.front();
  if (!seekHead.payload_.findIndex(&attachments))
  {
    seekHead.payload_.indexThis(&segment, &attachments);
  }
}

//----------------------------------------------------------------
// copyCodecPrivate
//
static void
copyCodecPrivate(MatroskaDoc & doc, const TTodo & todo, char ** argv)
{
  std::string auxPath = todo.getParam("file");

  FileStorage aux(auxPath, File::kReadOnly);
  if (!aux.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 auxPath +
                 std::string(" for reading")));
  }

  uint64 auxSize = aux.file_.size();
  MatroskaDoc ref;

  bool skipCues = true;
  LoaderSkipClusterPayload skipClusters(auxSize, skipCues);

  // attempt to load via SeekHead(s):
  printCurrentTime("ref.loadSeekHead");
  bool ok = ref.loadSeekHead(aux, auxSize);

  if (ok)
  {
    printCurrentTime("ref.loadViaSeekHead");
    ok = ref.loadViaSeekHead(aux, &skipClusters, true);
  }

  if (!ok || (!ref.segments_.empty() &&
              ref.segments_.front().payload_.clusters_.empty()))
  {
    std::cout << "failed to find Clusters via SeekHead, "
              << "attempting brute force"
              << std::endl;

    ref = MatroskaDoc();
    aux.file_.seek(0);

    printCurrentTime("ref.loadAndKeepReceipts");
    ref.loadAndKeepReceipts(aux, auxSize, &skipClusters);
  }

  if (ref.segments_.empty())
  {
    usage(argv, "reference source file has no matroska segments");
  }

  MatroskaDoc::TSegment & refSegment = ref.segments_.back();
  MatroskaDoc::TSegment & srcSegment = doc.segments_.back();

  std::deque<Tracks::TTrack> & refTracks =
    refSegment.payload_.tracks_.payload_.tracks_;

  std::deque<Tracks::TTrack> & srcTracks =
    srcSegment.payload_.tracks_.payload_.tracks_;

  for (std::deque<Tracks::TTrack>::iterator
         i = refTracks.begin(), j = srcTracks.begin();
       i != refTracks.end() && j != srcTracks.end(); ++i, ++j)
  {
    Track & refTrack = i->payload_;
    Track & srcTrack = j->payload_;

    if (refTrack.trackNumber_.payload_.get() !=
        srcTrack.trackNumber_.payload_.get())
    {
      continue;
    }

    if (refTrack.codecID_.payload_.get() !=
        srcTrack.codecID_.payload_.get())
    {
      continue;
    }

    std::cout
      << "copying track info for track number "
      << refTrack.trackNumber_.payload_.get()
      << ", codec id: "
      << refTrack.codecID_.payload_.get()
      << std::endl
      << "name: " << refTrack.name_.payload_.get() << std::endl
      << "lang: " << refTrack.language_.payload_.get() << std::endl
      << "codec private data size: "
      << refTrack.codecPrivate_.payload_.calcSize() << std::endl
      << std::endl;

    srcTrack.name_ = refTrack.name_;
    srcTrack.language_ = refTrack.language_;
    srcTrack.codecPrivate_ = refTrack.codecPrivate_;
  }
}

//----------------------------------------------------------------
// nsecToText
//
static std::string
nsecToText(uint64 ns)
{
  // convert from nanoseconds to milliseconds
  uint64 t = ns / 1000000;

  uint64 ms = t % 1000;
  t /= 1000;
  uint64 ss = t % 60;
  t /= 60;
  uint64 mm = t % 60;
  t /= 60;
  uint64 hh = t;

  std::ostringstream os;
  os << std::setw(2) << std::setfill('0') << hh << ':'
     << std::setw(2) << std::setfill('0') << mm << ':'
     << std::setw(2) << std::setfill('0') << ss << '.'
     << std::setw(3) << std::setfill('0') << ms;

  return std::string(os.str().c_str());
}

//----------------------------------------------------------------
// exportText
//
static void
exportText(const char * tag, const std::string & text, std::ostream & os)
{
  os << tag << '\t' << text << std::endl;
}

//----------------------------------------------------------------
// exportChapter
//
static void
exportChapter(const ChapAtom & atom, std::ostream & os)
{
  os << "uid\t" << atom.UID_.payload_.get() << '\n'
     << "start\t" << nsecToText(atom.timeStart_.payload_.get()) << '\n'
     << "end\t" << nsecToText(atom.timeEnd_.payload_.get()) << '\n';

  const std::list<ChapAtom::TDisplay> & display = atom.display_;
  for (std::list<ChapAtom::TDisplay>::const_iterator
         i = display.begin(); i != display.end(); ++i)
  {
    const ChapDisp & title = i->payload_;
    exportText("lang", title.language_.payload_.get(), os);
    exportText("country", title.country_.payload_.get(), os);
    exportText("title", title.string_.payload_.get(), os);
  }
}

//----------------------------------------------------------------
// saveChapters
//
static void
saveChapters(MatroskaDoc & doc, const TTodo & todo, char ** argv)
{
  std::string auxPath = todo.getParam("file");

  FileStorage aux(auxPath, File::kReadWrite);
  if (!aux.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 auxPath +
                 std::string(" for writing")));
  }
  aux.file_.setSize(0);

  std::ostringstream os;
  for (std::list<MatroskaDoc::TSegment>::const_iterator
         i = doc.segments_.begin(); i != doc.segments_.end(); ++i)
  {
    const Segment & segment = i->payload_;

    const std::list<Chapters::TEdition> & editions =
      segment.chapters_.payload_.editions_;

    for (std::list<Chapters::TEdition>::const_iterator
           j = editions.begin(); j != editions.end(); ++j)
    {
      const Edition & edition = j->payload_;

      const std::list<Edition::TChapAtom> & chapters = edition.chapAtoms_;
      for (std::list<Edition::TChapAtom>::const_iterator
             k = chapters.begin(); k != chapters.end(); ++k)
      {
        os << "begin chapter" << std::endl;
        const ChapAtom & atom = k->payload_;
        exportChapter(atom, os);
        os << "chapter end\n" << std::endl;
      }

      os << "edition end\n" << std::endl;
    }
    os << "segment end\n" << std::endl;
  }

  aux.save((const unsigned char *)os.str().c_str(), os.str().size());
}

//----------------------------------------------------------------
// toNanoSec
//
static uint64
toNanoSec(const std::string & text)
{
  std::string tmp = text;
  tmp[2] = ' ';
  tmp[5] = ' ';
  tmp[8] = ' ';

  std::istringstream in(tmp);
  uint64 hh = 0;
  uint64 mm = 0;
  uint64 ss = 0;
  uint64 ms = 0;

  in >> hh;
  in >> mm;
  in >> ss;
  in >> ms;

  // convert to nanoseconds:
  uint64 t = 1000000 * (ms + 1000 * (ss + 60 * (mm + 60 * hh)));
  return t;
}

//----------------------------------------------------------------
// importText
//
static std::string
importText(std::istream & in)
{
  char value[1024] = { 0 };
  in.getline(value, sizeof(value));
  std::string text(value);
  return text;
}

//----------------------------------------------------------------
// importChapter
//
static void
importChapter(ChapAtom & atom, std::istream & in)
{
  char separator = 0;
  std::string token;

  while (!in.eof())
  {
    in >> token;
    in.get(separator);

    if (token == "chapter")
    {
      std::string value;
      in >> value;
      if (value == "end")
      {
        break;
      }

      assert(false);
      break;
    }
    else if (token == "uid")
    {
      uint64 value = 0;
      in >> value;
      atom.UID_.payload_.set(value);
    }
    else if (token == "start")
    {
      std::string value;
      in >> value;
      uint64 t = toNanoSec(value);
      atom.timeStart_.payload_.set(t);
    }
    else if (token == "end")
    {
      std::string value;
      in >> value;
      uint64 t = toNanoSec(value);
      atom.timeEnd_.payload_.set(t);
    }
    else if (token == "lang")
    {
      atom.display_.push_back(ChapAtom::TDisplay());

      ChapDisp & title = atom.display_.back().payload_;
      std::string value = importText(in);
      title.language_.payload_.set(value);

      in >> token;
      in.get(separator);
      value = importText(in);

      if (token != "country")
      {
        assert(false);
        break;
      }

      title.country_.payload_.set(value);

      in >> token;
      in.get(separator);
      value = importText(in);

      if (token != "title")
      {
        assert(false);
        break;
      }

      title.string_.payload_.set(value);

      bool isDefault = title.isDefault();
      assert(!isDefault);
    }
  }
}

//----------------------------------------------------------------
// loadChapters
//
static void
loadChapters(MatroskaDoc & doc, const TTodo & todo, char ** argv)
{
  std::string auxPath = todo.getParam("file");

  FileStorage aux(auxPath, File::kReadOnly);
  if (!aux.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 auxPath +
                 std::string(" for reading")));
  }

  std::string auxText(aux.file_.size() + 1, 0);
  aux.load((unsigned char *)(&auxText[0]), aux.file_.size());

  std::istringstream in(auxText);

  // setup segment iterator:
  std::list<MatroskaDoc::TSegment>::iterator iSeg = doc.segments_.begin();
  if (iSeg == doc.segments_.end())
  {
    std::cerr << "document has not segments, nowhere to put chapters"
              << std::endl;
    return;
  }

  // setup chapter editions iterator:
  std::list<Chapters::TEdition>::iterator iEdn;
  {
    Segment & segment = iSeg->payload_;
    std::list<Chapters::TEdition> & editions =
      segment.chapters_.payload_.editions_;

    iEdn = editions.begin();
    if (iEdn == editions.end())
    {
      iEdn = editions.insert(editions.end(), Chapters::TEdition());
    }
  }

  while (!in.eof())
  {
    char token[256] = { 0 };
    in.getline(token, sizeof(token));

    if (strcmp(token, "segment end") == 0)
    {
      ++iSeg;
      if (iSeg == doc.segments_.end())
      {
        break;
      }

      Segment & segment = iSeg->payload_;
      std::list<Chapters::TEdition> & editions =
        segment.chapters_.payload_.editions_;

      iEdn = editions.begin();
      if (iEdn == editions.end())
      {
        iEdn = editions.insert(editions.end(), Chapters::TEdition());
      }
    }
    else if (strcmp(token, "edition end") == 0)
    {
      Segment & segment = iSeg->payload_;
      std::list<Chapters::TEdition> & editions =
        segment.chapters_.payload_.editions_;

      ++iEdn;
      if (iEdn == editions.end())
      {
        iEdn = editions.insert(editions.end(), Chapters::TEdition());
      }
    }
    else if (strcmp(token, "begin chapter") == 0)
    {
      ChapAtom atom;
      importChapter(atom, in);

      // find a chapter with matching UID:
      Edition & edition = iEdn->payload_;
      std::list<Edition::TChapAtom> & chapters = edition.chapAtoms_;

      std::list<Edition::TChapAtom>::iterator found = chapters.begin();
      for (; found != chapters.end(); ++found)
      {
        ChapAtom & chap = found->payload_;
        if (chap.UID_.payload_.get() == atom.UID_.payload_.get())
        {
          // found it:
          break;
        }
      }

      if (found == chapters.end())
      {
        found = chapters.insert(chapters.end(), Edition::TChapAtom());
      }

      ChapAtom & chap = found->payload_;
      chap.UID_.payload_.set(atom.UID_.payload_.get());
      chap.timeStart_.payload_.set(atom.timeStart_.payload_.get());
      chap.timeEnd_.payload_.set(atom.timeEnd_.payload_.get());
      chap.display_ = atom.display_;
      assert(!chap.display_.empty());
    }
  }
}

//----------------------------------------------------------------
// setTrackLang
//
static void
setTrackLang(MatroskaDoc & doc, const TTodo & todo, char ** argv)
{
  uint64 trackNo = 0;
  {
    std::string t = todo.getParam("track");
    trackNo = toScalar<uint64>(t);
    if (trackNo == 0)
    {
      usage(argv, std::string("invalid track number: ") + t);
    }
  }

  std::string lang = todo.getParam("lang");

  std::size_t segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    Segment & segment = i->payload_;

    std::deque<TTrack> & tracks = segment.tracks_.payload_.tracks_;
    for (std::deque<TTrack>::iterator j = tracks.begin();
         j != tracks.end(); ++j)
    {
      Track & track = j->payload_;
      uint64 n = track.trackNumber_.payload_.get();
      if (n == trackNo)
      {
        track.language_.payload_.set(lang);
        track.language_.alwaysSave();
      }
    }
  }
}

//----------------------------------------------------------------
// addTodo
//
static TTodo &
addTodo(std::list<TTodo> & todoList, TTodoFunc todoFunc)
{
  todoList.push_back(TTodo(todoFunc));
  return todoList.back();
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
  std::list<TTodo> todoList;

  // time segment extraction region,
  // t0 and t1 are expressed in milliseconds:
  bool extractFromKeyframe = false;
  uint64 t0 = 0;
  uint64 t1 = 0;

  std::map<uint64, double> tsOffset;

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
    else if (strcmp(argv[i], "--attach") == 0)
    {
      if ((argc - i) <= 2)
      {
        usage(argv, "could not parse --attach parameter");
      }

      TTodo & todo = addTodo(todoList, &addAttachment);

      i++;
      todo.addParam("type", argv[i]);

      i++;
      todo.addParam("file", argv[i]);
    }
    else if (strcmp(argv[i], "--copy-codec-private") == 0)
    {
      if ((argc - i) <= 1)
      {
        usage(argv, "could not parse --copy-codec-private parameter");
      }

      TTodo & todo = addTodo(todoList, &copyCodecPrivate);

      i++;
      todo.addParam("file", argv[i]);
    }
    else if (strcmp(argv[i], "--save-chapters") == 0)
    {
      if ((argc - i) <= 1)
      {
        usage(argv, "could not parse --save-chapters parameter");
      }

      TTodo & todo = addTodo(todoList, &saveChapters);

      i++;
      todo.addParam("file", argv[i]);
    }
    else if (strcmp(argv[i], "--load-chapters") == 0)
    {
      if ((argc - i) <= 1)
      {
        usage(argv, "could not parse --load-chapters parameter");
      }

      TTodo & todo = addTodo(todoList, &loadChapters);

      i++;
      todo.addParam("file", argv[i]);
    }
    else if (strcmp(argv[i], "--lang") == 0)
    {
      if ((argc - i) <= 2) usage(argv, "could not parse --lang parameter");

      TTodo & todo = addTodo(todoList, &setTrackLang);

      i++;
      todo.addParam("track", argv[i]);

      i++;
      todo.addParam("lang", argv[i]);
    }
    else if (strcmp(argv[i], "+dt") == 0)
    {
      if ((argc - i) <= 2) usage(argv, "could not parse +dt parameters");

      i++;
      uint64 trackNo = toScalar<uint64>(argv[i]);

      i++;
      double dt = toScalar<double>(argv[i]);

      tsOffset[trackNo] = dt;
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
    else if (strcmp(argv[i], "--fix-keyframe-flag") == 0)
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

  if (!ok || (!doc.segments_.empty() &&
              doc.segments_.front().payload_.clusters_.empty()))
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

  // execute the todo list:
  for (std::list<TTodo>::const_iterator i = todoList.begin();
       i != todoList.end(); ++i)
  {
    const TTodo & todo = *i;
    todo.execute(doc, argv);
  }

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

  // set the DocType to 4, due to CueRelativePosition:
  out.head_.payload_.docTypeVersion_.payload_.set(4);
  out.head_.payload_.docTypeReadVersion_.payload_.set(2);

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
    TRemuxer remuxer(tsOffset,
                     trackInOut,
                     trackOutIn,
                     *i,
                     segmentElt,
                     src,
                     dst,
                     tmp);
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

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

// system includes:
#include <sstream>
#include <iostream>
#include <string.h>
#include <string>
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
  std::map<TKey, TValue>::const_iterator found =
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
// usage
// 
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "NOTE: remuxing input files containing multiple segments "
            << "with mismatched tracks will not work correctly"
            << std::endl;
  
  std::cerr << "USAGE: " << argv[0]
            << " -i input.mkv -o output.mkv [-t trackNo | +t trackNo]*"
            << std::endl;
  
  std::cerr << "EXAMPLE: " << argv[0]
            << " -i input.mkv -o output.mkv -t 1 -t 2"
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
// PartialClusterReader
// 
// skip loading any blocks:
// 
struct PartialClusterReader : public IDelegateLoad
{
  // virtual:
  uint64 load(FileStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    if (eltId == TBlockGroup::kId ||
        eltId == TSimpleBlock::kId ||
        eltId == TEncryptedBlock::kId)
    {
      storage.file_.seek(payloadBytesToRead, File::kRelativeToCurrent);
      return payloadBytesToRead;
    }
    
    // let the generic load mechanism handle id:
    return 0;
  }
};


//----------------------------------------------------------------
// OneClusterReader
// 
// Load the first cluster, skip everything else.
// 
struct OneClusterReader : public IDelegateLoad
{
  bool loadedOneCluster_;
  
  OneClusterReader():
    loadedOneCluster_(false)
  {}
  
  // virtual:
  uint64 load(FileStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    if (loadedOneCluster_)
    {
      return uintMax[8];
    }
    
    if (eltId == TSegment::kId)
    {
      // avoid skipping the entire segment:
      return 0;
    }
    
    const bool payloadIsCluster = (eltId == TCluster::kId);
    if (!payloadIsCluster)
    {
      storage.file_.seek(payloadBytesToRead, File::kRelativeToCurrent);
      return payloadBytesToRead;
    }
    
    // let the generic load mechanism handle the actual loading:
    loadedOneCluster_ = payloadIsCluster;
    return 0;
  }
};

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
// getFirstClusterPosition
// 
static uint64
getFirstClusterPosition(const TSegment & segmentElt,
                        FileStorage & storage)
{
  // shortcuts:
  const Segment & segment = segmentElt.payload_;
  
  IStorage::IReceiptPtr originReceipt = segmentElt.payloadReceipt();
  uint64 originPosition = originReceipt->position();
  
  IStorage::IReceiptPtr segmentReceipt = segmentElt.storageReceipt();
  uint64 segmentPosition = segmentReceipt->position();
  uint64 segmentSize = segmentReceipt->numBytes();
  
  // check the SeekHead(s):
  const std::deque<TSeekHead> & seekHeads = segment.seekHeads_;
  std::size_t numSeekHeads = seekHeads.size();
  for (std::size_t i = 0; i < numSeekHeads; i++)
  {
    const SeekHead & seekHead = seekHeads[i].payload_;
    const std::list<TSeekEntry> & seekList = seekHead.seek_;

    for (std::list<TSeekEntry>::const_iterator j = seekList.begin();
         j != seekList.end(); ++j)
    {
      const SeekEntry & seekEntry = j->payload_;
      
      Bytes eltIdBytes;
      if (!seekEntry.id_.payload_.get(eltIdBytes))
      {
        assert(false);
        continue;
      }
      
      uint64 eltId = uintDecode(eltIdBytes, eltIdBytes.size());
      if (eltId != TCluster::kId)
      {
        continue;
      }
        
      const VEltPosition & eltReference = seekEntry.position_.payload_;
      if (!eltReference.hasPosition())
      {
        assert(false);
        continue;
      }
      
      uint64 relativePosition = eltReference.position();
      uint64 absolutePosition = originPosition + relativePosition;
      return absolutePosition;
    }
  }

  // check the Cues:
  const std::list<TCuePoint> & cuePoints = segment.cues_.payload_.points_;
  for (TCuePointConstIter i = cuePoints.begin(); i != cuePoints.end(); ++i)
  {
    const CuePoint & cuePoint = i->payload_;
    const std::list<TCueTrkPos> & cueTrkPns = cuePoint.trkPosns_;
    
    for (TCueTrkPosConstIter j = cueTrkPns.begin(); j != cueTrkPns.end(); ++j)
    {
      const CueTrkPos & cueTrkPos = j->payload_;
      const VEltPosition & clusterRef = cueTrkPos.cluster_.payload_;
      if (!clusterRef.hasPosition())
      {
        continue;
      }
      
      uint64 relativePosition = clusterRef.position();
      uint64 absolutePosition = originPosition + relativePosition;
      return absolutePosition;
    }
  }
  
  // search the segment for the first cluster:
  storage.file_.seek(segmentPosition, File::kAbsolutePosition);
  
  OneClusterReader reader;
  TSegment segElt;
  segElt.load(storage, segmentSize, &reader);

  Segment & seg = segElt.payload_;
  if (seg.clusters_.empty())
  {
    // could not load any clusters:
    return uintMax[8];
  }
  
  assert(seg.clusters_.size() == 1);
  uint64 clusterPosition = seg.clusters_.front().storageReceipt()->position();
  return clusterPosition;
}

//----------------------------------------------------------------
// finishCurrentBlock
// 
static void
finishCurrentBlock(Cluster & cluster,
                   SimpleBlock & simpleBlock,
                   FileStorage & tmp)
{
    if (!simpleBlock.getNumberOfFrames())
    {
        return;
    }
    
    cluster.simpleBlocks_.push_back(TSimpleBlock());
    TSimpleBlock & block = cluster.simpleBlocks_.back();
    
    Bytes data;
    simpleBlock.setAutoLacing();
    simpleBlock.exportData(data);
    block.payload_.set(data, tmp);
    
    simpleBlock = SimpleBlock();
}

//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  std::string srcPath;
  std::string dstPath;
  std::string tmpPath;
  std::list<uint64> tracksToKeep;
  std::list<uint64> tracksDelete;
  
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
      tmpPath = dstPath + std::string(".yamka");
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
    else
    {
      usage(argv, (std::string("unknown option: ") +
                   std::string(argv[i])).c_str());
    }
  }
  
  bool keepAllTracks = tracksToKeep.empty() && tracksDelete.empty();
  
  FileStorage src(srcPath, File::kReadOnly);
  if (!src.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 srcPath +
                 std::string(" for reading")).c_str());
  }
  
  uint64 srcSize = src.file_.size();
  MatroskaDoc doc;
  if (!doc.loadSeekHead(src, srcSize) ||
      !doc.loadViaSeekHead(src, NULL, false) ||
      doc.segments_.empty())
  {
    usage(argv, (std::string("failed to load any matroska segments").c_str()));
  }

  std::size_t numSegments = doc.segments_.size();
  std::vector<std::map<uint64, uint64> > segmentTrackInOut(numSegments);
  std::vector<std::map<uint64, uint64> > segmentTrackOutIn(numSegments);
  std::vector<std::vector<std::list<Frame> > > segmentTrackFrames(numSegments);
  
  // verify that the specified tracks exist:
  std::size_t segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    std::map<uint64, uint64> & trackInOut = segmentTrackInOut[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = segmentTrackOutIn[segmentIndex];
    
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
  
  FileStorage tmp(tmpPath, File::kReadWrite);
  if (!tmp.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 tmpPath +
                 std::string(" for writing")).c_str());
  }
  else
  {
    tmp.file_.setSize(0);
  }

  bool isWebmOutput= endsWith(dstPath, ".webm") || endsWith(dstPath, ".weba");
  WebmDoc out(isWebmOutput ? kFileFormatWebm : kFileFormatMatroska);
  
  segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    out.segments_.push_back(TSegment());
    TSegment & segmentElt = out.segments_.back();
    
    Segment & segment = segmentElt.payload_;
    const Segment & segmentIn = i->payload_;
    
    // add first SeekHead, written before clusters:
    segment.seekHeads_.push_back(TSeekHead());
    TSeekHead & seekHeadElt = segment.seekHeads_.back();
    SeekHead & seekHead = seekHeadElt.payload_;
    
    // copy segment info:
    TSegInfo & segInfoElt = segment.info_;
    SegInfo & segInfo = segInfoElt.payload_;
    const SegInfo & segInfoIn = segmentIn.info_.payload_;
    
    segInfo = segInfoIn;
    segInfo.muxingApp_.payload_.set(segInfo.muxingApp_.payload_.getDefault());
    segInfo.writingApp_.payload_.set("yamkaRemux");

    // segment timecode scale, such that
    // timeInNanosec := timecodeScale * (clusterTime + blockTime):
    uint64 timecodeScale = segInfo.timecodeScale_.payload_.get();
    
    // copy track info:
    TTracks & tracksElt = segment.tracks_;
    std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
    const std::deque<TTrack> & tracksIn = segmentIn.tracks_.payload_.tracks_;

    std::map<uint64, uint64> & trackInOut = segmentTrackInOut[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = segmentTrackOutIn[segmentIndex];
    uint64 trackMapSize = trackOutIn.size();

    std::vector<Track::MatroskaTrackType> trackType((std::size_t)trackMapSize);
    uint64 videoTrackNo = 0;
    
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

      trackType[(std::size_t)(trackNo - 1)] =
        (Track::MatroskaTrackType)(track.trackType_.payload_.get());
      
      if (!videoTrackNo &&
          trackType[(std::size_t)(trackNo - 1)] == Track::kTrackTypeVideo)
      {
        videoTrackNo = trackNo;
      }
    }

    // index the first set of top-level elements:
    seekHead.indexThis(&segmentElt, &segInfoElt, tmp);
    seekHead.indexThis(&segmentElt, &tracksElt, tmp);

    // Find the first cluster in this segment, either via SeekHead(s),
    // a CuePoint, or by searching the level-1 elements in the file:
    uint64 clusterPosition = getFirstClusterPosition(*i, src);
    src.file_.seek(clusterPosition, File::kAbsolutePosition);
    
    // shortcut:
    std::vector<std::list<Frame> > & trackFrames =
      segmentTrackFrames[segmentIndex];
    
    // Read cluster(s) and extract track frames that we are interested in:
    while (true)
    {
      TCluster clusterElt;
      Cluster & cluster = clusterElt.payload_;
      
      PartialClusterReader clusterReader;
      uint64 bytesRead = clusterElt.load(src, uintMax[8], &clusterReader);
      if (!bytesRead)
      {
        break;
      }
      
      File::Seek autoRestore(src.file_);
      const uint64 clusterTime = cluster.timecode_.payload_.get();
      
      std::deque<TBlockGroup> & blockGroups = cluster.blockGroups_;
      std::size_t numBlockGroups = blockGroups.size();
      for (std::size_t j = 0; j < numBlockGroups; j++)
      {
        TBlockGroup & bg = blockGroups[j];
      }
      
      std::deque<TSimpleBlock> & simpleBlocks = cluster.simpleBlocks_;
      std::size_t numSimpleBlocks = simpleBlocks.size();
      for (std::size_t j = 0; j < numSimpleBlocks; j++)
      {
        TSimpleBlock & sb = simpleBlocks[j];

        IStorage::IReceiptPtr receipt = sb.payloadReceipt();
        uint64 position = receipt->position();
        uint64 numBytes = receipt->numBytes();
        src.file_.seek(position, File::kAbsolutePosition);
        
        // check whether this block is for a track we are interested in:
        uint64 trackNoIn = 0;
        {
          File::Seek storageStart(src.file_);
          
          // read track number:
          uint64 vsizeSize = 0;
          trackNoIn = vsizeDecode(src, vsizeSize);

          if (!has(trackInOut, trackNoIn))
          {
            // ignore this block:
            continue;
          }
        }

        Bytes blockData((std::size_t)numBytes);
        receipt = src.load(blockData);
        if (!receipt)
        {
          assert(false);
          continue;
        }
        
        SimpleBlock block;
        if (!block.importData(blockData))
        {
          assert(false);
          continue;
        }

        const short int blockTime = block.getRelativeTimecode();
        
        uint64 trackNo = trackInOut[trackNoIn];
        const Track & track = tracks[(std::size_t)(trackNo - 1)].payload_;

        // shortcut to the output frame list:
        std::list<Frame> & frames = trackFrames[(std::size_t)(trackNo - 1)];
        
        // default frame duration, expressed in nanoseconds:
        uint64 frameDuration = track.frameDuration_.payload_.get(); 
        
        std::size_t numFrames = block.getNumberOfFrames();
        for (std::size_t k = 0; k < numFrames; k++)
        {
          frames.push_back(Frame());
          Frame & frame = frames.back();
          
          frame.trackNumber_ = trackNo;
          frame.isKeyframe_ = (k == 0) && block.isKeyframe();
          frame.ts_.extent_ = frameDuration;
          frame.ts_.start_ = (clusterTime + blockTime) * timecodeScale;
          frame.ts_.base_ = NANOSEC_PER_SEC;
          
          const Bytes & frameData = block.getFrame(k);
          frame.data_.set(frameData, tmp);
        }
      }
      
      if (!isWebmOutput)
      {
        // webm doesn't support encrypted blocks:
        std::deque<TEncryptedBlock> & encBlocks = cluster.encryptedBlocks_;
        std::size_t numEncBlocks = encBlocks.size();
        for (std::size_t j = 0; j < numEncBlocks; j++)
        {
          TEncryptedBlock & eb = encBlocks[j];
        }
      }
    }
    
    // split frames into groups:
    const uint64 clusterTimeBase = NANOSEC_PER_SEC / timecodeScale;
    std::list<GroupOfFrames> gofs;
    gofs.push_back(GroupOfFrames(clusterTimeBase));

    if (videoTrackNo)
    {
      std::list<Frame> & videoFrames =
        trackFrames[(std::size_t)(videoTrackNo - 1)];
      
      while (!videoFrames.empty())
      {
        Frame videoFrame = videoFrames.front();
        videoFrames.pop_front();
        
        if (videoFrame.isKeyframe_ || !gofs.back().mayAdd(videoFrame))
        {
          // start a new group of frames:
          gofs.push_back(GroupOfFrames(clusterTimeBase));
        }
        
        GroupOfFrames & gof = gofs.back();
        gof.add(videoFrame);
        
        for (uint64 trackNo = 1; trackNo <= trackMapSize; trackNo++)
        {
          if (trackNo == videoTrackNo)
          {
            continue;
          }
          
          // const Track & track = tracks[trackNo - 1].payload_;
          bool isVideoTrack =
            trackType[(std::size_t)(trackNo - 1)] == Track::kTrackTypeVideo;
          
          std::list<Frame> & frames = trackFrames[(std::size_t)(trackNo - 1)];
          
          // add frames whose time span end point is contained
          // in the current group of frames:
          while (!frames.empty())
          {
            Frame frame = frames.front();
            if (!gof.ts_.contains(frame.ts_.getEnd(gof.ts_.base_),
                                  gof.ts_.base_))
            {
              break;
            }
            
            frames.pop_front();
            if (gof.mayAdd(frame))
            {
              gof.add(frame);
            }
            else
            {
              assert(false);
            }
          }
        }
      }
    }
    
    // split remaining frames into groups of frames:
    while (true)
    {
      bool addedFrames = false;
      
      for (uint64 trackNo = 1; trackNo <= trackMapSize; trackNo++)
      {
        std::list<Frame> & frames = trackFrames[(std::size_t)(trackNo - 1)];
        if (frames.empty())
        {
          continue;
        }
        
        // shortcut:
        bool isVideoTrack =
          trackType[(std::size_t)(trackNo - 1)] == Track::kTrackTypeVideo;
        
        // start a new group of frames:
        Frame frame = frames.front();
        
        if ((isVideoTrack && frame.isKeyframe_) ||
            !gofs.back().mayAdd(frame))
        {
          // start a new group of frames:
          gofs.push_back(GroupOfFrames(clusterTimeBase));
        }
        
        GroupOfFrames & gof = gofs.back();
        while (true)
        {
          if (gof.mayAdd(frame))
          {
            gof.add(frame);
            addedFrames = true;
            
            frames.pop_front();
            if (frames.empty())
            {
              break;
            }
          }
          else
          {
            // move on to the next track:
            break;
          }
          
          frame = frames.front();
        }
      }

      if (!addedFrames)
      {
        break;
      }
    }
    
    // assemble groups of frames into clusters:
    bool allowManyKeyframes = !isWebmOutput;
    std::list<MetaCluster> metaClusters;
    while (!gofs.empty())
    {
      GroupOfFrames gof = gofs.front();
      gofs.pop_front();
      
      if (metaClusters.empty() || !metaClusters.back().mayAdd(gof))
      {
        // start a new meta cluster:
        metaClusters.push_back(MetaCluster(allowManyKeyframes));
      }
      
      MetaCluster & metaCluster = metaClusters.back();
      metaCluster.add(gof);
    }

    // convert meta clusters into matroska Clusters and SimpleBlocks,
    // create Cues along the way:
    TCues & cuesElt = segment.cues_;
    Cues & cues = cuesElt.payload_;
    
    while (!metaClusters.empty())
    {
      MetaCluster metaCluster = metaClusters.front();
      metaClusters.pop_front();
      
      // sort frames in ascending timecode order, per WebM guideline,
      // while preserving the decode order of the frames:
      std::list<Frame> frames;
      metaCluster.getSortedFrames(frames);
      metaCluster.frames_.clear();
      
      segment.clusters_.push_back(TCluster());
      TCluster & clusterElt = segment.clusters_.back();
      Cluster & cluster = clusterElt.payload_;
      
      cluster.timecode_.payload_.set(metaCluster.ts_.start_);
      cluster.position_.payload_.setOrigin(&segmentElt);
      cluster.position_.payload_.setElt(&clusterElt);
      
      seekHead.indexThis(&segmentElt, &clusterElt, tmp);
      
      if (!videoTrackNo && !frames.empty())
      {
        // audio-only files should include a CuePoint
        // to the first audio frame of each cluster:
        Frame & frame = frames.front();

        Track::MatroskaTrackType tt =
          trackType[(std::size_t)(frame.trackNumber_ - 1)];
          
        if (tt == Track::kTrackTypeAudio)
        {
          frame.isKeyframe_ = true;
        }
      }
      
      SimpleBlock simpleBlock;
      while (!frames.empty())
      {
        Frame frame = frames.front();
        frames.pop_front();
        
        // NOTE: WebM doesn't support lacing, yet:
        if (isWebmOutput ||
            simpleBlock.getTrackNumber() != frame.trackNumber_ ||
            simpleBlock.getNumberOfFrames() > 7 ||
            frame.trackNumber_ == videoTrackNo ||
            frame.isKeyframe_)
        {
          finishCurrentBlock(cluster, simpleBlock, tmp);
        }
        
        uint64 absTimecode = frame.ts_.getStart(metaCluster.ts_.base_);
        int64 relTimecode = absTimecode - metaCluster.ts_.start_;
        assert(relTimecode < kShortDistLimit);
        if (simpleBlock.getNumberOfFrames() == 0)
        {
          simpleBlock.setRelativeTimecode((short int)relTimecode);
        }
        
        simpleBlock.setTrackNumber(frame.trackNumber_);
        if (frame.isKeyframe_)
        {
          simpleBlock.setKeyframe(true);
          
          // add block to cues:
          cues.points_.push_back(TCuePoint());
          TCuePoint & cuePoint = cues.points_.back();
          
          // set cue timepoint:
          cuePoint.payload_.time_.payload_.set(absTimecode);
          
          // add a track position for this timepoint:
          cuePoint.payload_.trkPosns_.resize(1);
          CueTrkPos & pos = cuePoint.payload_.trkPosns_.back().payload_;
          
          std::size_t blockIndex = cluster.simpleBlocks_.size() + 1;
          pos.block_.payload_.set(blockIndex);
          pos.track_.payload_.set(frame.trackNumber_);
          
          pos.cluster_.payload_.setOrigin(&segmentElt);
          pos.cluster_.payload_.setElt(&clusterElt);
        }
        
        Bytes frameData;
        frame.data_.get(frameData);
        simpleBlock.addFrame(frameData);
      }
      
      // finish the last block in this cluster:
      finishCurrentBlock(cluster, simpleBlock, tmp);
    }
    
    if (cuesElt.mustSave())
    {
      seekHead.indexThis(&segmentElt, &cuesElt, tmp);
    }
    /*
    if (!isWebmOutput)
    {
      // enable CRC-32 for Level-1 elements:
      out.setCrc32(true);
    }
    */
  }
  
  // save the file:
  dst.file_.setSize(0);
  IStorage::IReceiptPtr receipt = out.save(dst);
  
  // close open file handles:
  doc = MatroskaDoc();
  out = WebmDoc(kFileFormatMatroska);
  src = FileStorage();
  dst = FileStorage();
  tmp = FileStorage();

  // remove temp file:
  File::remove(tmpPath.c_str());
  
  return 0;
}

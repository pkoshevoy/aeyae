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
// TTrackMap
// 
typedef std::map<uint64, uint64> TTrackMap;

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

#if 0
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
  binfo.keyframe_ = binfo.block_.isKeyframe();
  
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
  
  uint64 srcTrackNo = binfo.block_.getTrackNumber();
  short int srcBlockTime = binfo.block_.getRelativeTimecode();
  bool srcIsKeyframe = binfo.block_.isKeyframe();
  
  if (srcTrackNo == binfo.trackNo_ &&
      srcBlockTime == dstBlockTime &&
      srcIsKeyframe == binfo.keyframe_)
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
  binfo.block_.setKeyframe(binfo.keyframe_);
  
  binfo.header_ = binfo.block_.writeHeader(tmp_);
  
  blockData->set(binfo.header_);
  blockData->add(binfo.frames_);
  
  return true;
}
#endif

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
            << "[-t0 hh mm ss] [-t1 hh mm ss]"
            << std::endl;
  
  std::cerr << "EXAMPLE: " << argv[0]
            << " -i input.mkv -o output.mkv -t 1 -t 2"
            << " -t0 00 04 00 -t1 00 08 00"
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
  std::string tmpPath;
  std::list<uint64> tracksToKeep;
  std::list<uint64> tracksDelete;

  uint64 t0 = 0;
  uint64 t1 = (uint64)(~0);
  
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
    else if (strcmp(argv[i], "-t0") == 0)
    {
      if ((argc - i) <= 3) usage(argv, "could not parse -t0 parameter");
      
      i++;
      uint64 hh = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 mm = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 ss = toScalar<uint64>(argv[i]);
      
      t0 = ss + 60 * (mm + 60 * hh);
    }
    else if (strcmp(argv[i], "-t1") == 0)
    {
      if ((argc - i) <= 3) usage(argv, "could not parse -t1 parameter");
      
      i++;
      uint64 hh = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 mm = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 ss = toScalar<uint64>(argv[i]);
      
      t1 = ss + 60 * (mm + 60 * hh);
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
          "start time (-t0 hh mm ss) is greater than "
          "finish time (-t1 hh mm ss)");
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

  // attempt to load via SeekHead(s):
  bool ok = doc.loadSeekHead(src, srcSize);
  printCurrentTime("doc.loadSeekHead finished");
  
  LoadWithProgress showProgress(srcSize);
  if (ok)
  {
    ok = doc.loadViaSeekHead(src, &showProgress, true);
    printCurrentTime("doc.loadViaSeekHead finished");
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
    
    doc.loadAndKeepReceipts(src, srcSize, &showProgress);
    printCurrentTime("doc.loadAndKeepReceipts finished");
  }

  if (doc.segments_.empty())
  {
    usage(argv, (std::string("failed to load any matroska segments").c_str()));
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
  
  FileStorage tmp(tmpPath, File::kReadWrite);
  if (!tmp.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 tmpPath +
                 std::string(" for writing")).c_str());
  }
  
  tmp.file_.setSize(0);

  printCurrentTime("simplifying the SeekHead");
  for (std::list<MatroskaDoc::TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i)
  {
    MatroskaDoc::TSegment & segment = *i;
    
    // shortcuts:
    Segment::TInfo & segInfo = segment.payload_.info_;
    Segment::TTracks & tracks = segment.payload_.tracks_;
    Segment::TCues & cues = segment.payload_.cues_;
    Segment::TAttachment & attachments = segment.payload_.attachments_;
    Segment::TChapters & chapters = segment.payload_.chapters_;
    std::list<Segment::TTags> & tagsList = segment.payload_.tags_;
    std::list<Segment::TCluster> & clusters = segment.payload_.clusters_;
    
    segment.payload_.seekHeads_.clear();
    segment.payload_.seekHeads_.push_back(Segment::TSeekHead());
    
    Segment::TSeekHead & seekHead = segment.payload_.seekHeads_.front();
    seekHead.payload_.seek_.clear();

    if (segInfo.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &segInfo);
    }

    if (tracks.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &tracks);
    }

    if (cues.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &cues);
    }

    if (attachments.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &attachments);
    }

    if (chapters.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &chapters);
    }
    
    for (std::list<Segment::TTags>::iterator j = tagsList.begin();
         j != tagsList.end(); ++j)
    {
      Segment::TTags & tags = *j;
      if (tags.mustSave())
      {
        seekHead.payload_.indexThis(&segment, &tags);
      }
    }
    
    for (std::list<Segment::TCluster>::iterator j = clusters.begin();
         j != clusters.end(); ++j)
    {
      Segment::TCluster & cluster = *j;
      if (cluster.mustSave())
      {
        seekHead.payload_.indexThis(&segment, &cluster);
      }
    }
    
    // update muxer credits if necessary:
    SegInfo::TMuxingApp & muxingApp = segInfo.payload_.muxingApp_;
    std::string credits = muxingApp.payload_.get();
    std::string creditsYamka = muxingApp.payload_.getDefault();
    if (credits.find(creditsYamka) == std::string::npos)
    {
      if (!credits.empty())
      {
        credits += ", ";
      }
      
      credits += "remuxed with ";
      credits += creditsYamka;
      muxingApp.payload_.set(credits);
    }
  }
  
  // optimize the document:
  printCurrentTime("optimizing");
  doc.optimize(tmp);

#if 0
  // enable CRC-32 for Level-1 elements:
  printCurrentTime("enabling CRC-32");
  doc.setCrc32(true);
#endif
  
  // save the document:
  printCurrentTime("saving");
  dst.file_.setSize(0);
  IStorage::IReceiptPtr receipt = doc.save(dst);
  if (receipt)
  {
    std::cout << "stored " << doc.calcSize() << " bytes" << std::endl;
  }
  
  // close open file handles:
  printCurrentTime("cleaning up");
  src.file_.close();
  dst.file_.close();
  tmp.file_.close();
  
  // remove temp file:
  File::remove(tmpPath.c_str());

  printCurrentTime("done");

  // avoid waiting for all the destructors to be called:
  ::exit(0);
  return 0;
}

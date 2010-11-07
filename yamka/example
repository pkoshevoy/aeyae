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
#include <iostream>
#include <string.h>
#include <string>

// namespace access:
using namespace Yamka;

//----------------------------------------------------------------
// usage
// 
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "USAGE: " << argv[0]
            << " -i source.mkv -o output.mkv"
            << std::endl;
  
  std::cerr << "EXAMPLE: " << argv[0]
            << " -i source.mkv -o output.mkv"
            << std::endl;
  
  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }
  
  ::exit(1);
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
    else
    {
      usage(argv, (std::string("unknown option: ") +
                   std::string(argv[i])).c_str());
    }
  }
  
  FileStorage src(srcPath, File::kReadOnly);
  if (!src.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 srcPath +
                 std::string(" for reading")).c_str());
  }
  
  uint64 srcSize = src.file_.size();
  MatroskaDoc doc;
  uint64 bytesRead = doc.load(src, srcSize);
  if (!bytesRead || doc.segments_.empty())
  {
    usage(argv, (std::string("source file has no matroska segments").c_str()));
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
    
    // add attachment to the seekhead index:
    if (segment.payload_.seekHeads_.empty())
    {
      // this should't happen, but if the SeekHead element
      // is missing -- add it:
      segment.payload_.seekHeads_.push_back(Segment::TSeekHead());
    }
    
    Segment::TSeekHead & seekHead = segment.payload_.seekHeads_.front();
    seekHead.payload_.seek_.clear();

    if (segInfo.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &segInfo, tmp);
    }

    if (tracks.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &tracks, tmp);
    }

    if (cues.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &cues, tmp);
    }

    if (attachments.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &attachments, tmp);
    }

    if (chapters.mustSave())
    {
      seekHead.payload_.indexThis(&segment, &chapters, tmp);
    }
    
    for (std::list<Segment::TTags>::iterator j = tagsList.begin();
	 j != tagsList.end(); ++j)
    {
      Segment::TTags & tags = *j;
      if (tags.mustSave())
      {
	seekHead.payload_.indexThis(&segment, &tags, tmp);
      }
    }
    
    for (std::list<Segment::TCluster>::iterator j = clusters.begin();
	 j != clusters.end(); ++j)
    {
      Segment::TCluster & cluster = *j;
      if (cluster.mustSave())
      {
	seekHead.payload_.indexThis(&segment, &cluster, tmp);
      }
    }
  }
  
  // save the document:
  dst.file_.setSize(0);
  IStorage::IReceiptPtr receipt = doc.save(dst);
  if (receipt)
  {
    std::cout << "stored " << doc.calcSize() << " bytes" << std::endl;
  }
  
  // close open file handles:
  doc = MatroskaDoc();
  src = FileStorage();
  dst = FileStorage();
  tmp = FileStorage();

  // remove temp file:
  File::remove(tmpPath.c_str());
  
  return 0;
}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Sep 14 10:28:10 MDT 2010
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
#include <limits>

// namespace access:
using namespace Yamka;

//----------------------------------------------------------------
// usage
// 
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "USAGE: " << argv[0]
            << " -i source.mkv -o output.mkv -a mimetype attachment.xyz"
            << std::endl;
  
  std::cerr << "EXAMPLE: " << argv[0]
            << " -i source.mkv -o output.mkv -a text/plain notes.txt"
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
  std::string attType;
  std::string attPath;
  
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-a") == 0)
    {
      if ((argc - i) <= 2) usage(argv, "could not parse -a parameters");
      i++;
      attType.assign(argv[i]);
      i++;
      attPath.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-i") == 0)
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
  
  FileStorage att(attPath, File::kReadOnly);
  if (!att.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 attPath +
                 std::string(" for reading")).c_str());
  }
  
  uint64 attSize = att.file_.size();
  assert(attSize < uint64(std::numeric_limits<std::size_t>::max()));
  Bytes attData((std::size_t)attSize);
  IStorage::IReceiptPtr attReceipt = att.load(attData);
  
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
  attachment.payload_.data_.payload_.set(attData, tmp);
  
  TByteVec attUID = Yamka::createUID(16);
  attachment.payload_.fileUID_.payload_.set(Yamka::uintDecode(attUID, 16));
  
  // add attachment to the seekhead index:
  if (segment.payload_.seekHeads_.empty())
  {
      // this should't happen, but if the SeekHead element
      // is missing -- add it:
      segment.payload_.seekHeads_.push_back(Segment::TSeekHead());
  }
  
  Segment::TSeekHead & seekHead = segment.payload_.seekHeads_.front();
  seekHead.payload_.indexThis(&segment, &attachments, tmp);
  
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

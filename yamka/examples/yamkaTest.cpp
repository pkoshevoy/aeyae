// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 08:59:02 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaStdInt.h>
#include <yamkaFileStorage.h>
#include <yamkaEBML.h>
#include <yamkaMatroska.h>
#include <yamkaHodgePodge.h>
#include <yamkaConstMemoryStorage.h>

// system includes:
#include <iostream>
#include <typeinfo>
#include <vector>

// namespace access:
using namespace Yamka;

//----------------------------------------------------------------
// sanity_check
//
template <typename TPayload>
bool
sanity_check()
{
  TPayload p;
  bool ok = p.isDefault();
  if (!ok)
  {
    std::cerr << typeid(TPayload).name()
              << " default constructor did not create a default payload"
              << std::endl;
    assert(false);
  }

  return ok;
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

  sanity_check<ChapTranslate>();
  sanity_check<SegInfo>();
  sanity_check<TrackTranslate>();
  sanity_check<Video>();
  sanity_check<Audio>();
  sanity_check<ContentCompr>();
  sanity_check<ContentEncrypt>();
  sanity_check<ContentEnc>();
  sanity_check<ContentEncodings>();
  sanity_check<Track>();
  sanity_check<TrackPlane>();
  sanity_check<TrackCombinePlanes>();
  sanity_check<TrackJoinBlocks>();
  sanity_check<TrackOperation>();
  sanity_check<Tracks>();
  sanity_check<CueRef>();
  sanity_check<CueTrackPositions>();
  sanity_check<CuePoint>();
  sanity_check<Cues>();
  sanity_check<SeekEntry>();
  sanity_check<SeekHead>();
  sanity_check<AttdFile>();
  sanity_check<Attachments>();
  sanity_check<ChapTrk>();
  sanity_check<ChapDisp>();
  sanity_check<ChapProcCmd>();
  sanity_check<ChapProc>();
  sanity_check<ChapAtom>();
  sanity_check<Edition>();
  sanity_check<Chapters>();
  sanity_check<TagTargets>();
  sanity_check<SimpleTag>();
  sanity_check<Tag>();
  sanity_check<Tags>();
  sanity_check<SilentTracks>();
  sanity_check<BlockMore>();
  sanity_check<BlockAdditions>();
  sanity_check<TimeSlice>();
  sanity_check<Slices>();
  sanity_check<BlockGroup>();
  sanity_check<Cluster>();
  sanity_check<Segment>();

  uint64 vsizeSize = 0;
  std::cout << "0x" << uintEncode(0x1A45DFA3) << std::endl
            << "0x" << uintEncode(0xEC) << std::endl
            << "0x" << intEncode(5) << std::endl
            << "0x" << intEncode(-2) << std::endl
            << "0x" << intEncode(5, 3) << std::endl
            << "0x" << intEncode(-2, 3) << std::endl
            << "0x1456abcf = 0x"
            << std::hex
            << vsizeDecode(vsizeEncode(0x1456abcf), vsizeSize)
            << std::dec << std::endl
            << "-64 = "
            << vsizeSignedDecode(vsizeSignedEncode(-64), vsizeSize)
            << std::endl
            << "0x" << vsizeEncode(0x8000) << std::endl
            << "0x" << vsizeEncode(1) << std::endl
            << "0x" << vsizeEncode(0) << std::endl
            << "0x"
            << vsizeEncode(123, 4)
            << " = 0x"
            << vsizeEncode(vsizeDecode(vsizeEncode(123, 4), vsizeSize))
            << std::endl
            << "0x" << floatEncode(-11.1f)
            << " = " << floatDecode(floatEncode(-11.1f)) << std::endl
            << "0x" << doubleEncode(-11.1)
            << " = " << doubleDecode(doubleEncode(-11.1)) << std::endl
            << "0x" << intEncode(VDate().get(), 8) << std::endl
            << std::endl;

  FileStorage fs(std::string("testYamka.bin"), File::kReadWrite);
  if (!fs.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << fs.file_.filename()
              << " to read/write"
              << std::endl;
    ::exit(1);
  }
  else
  {
    std::cout << "opened (rw) " << fs.file_.filename()
              << ", current file size: " << fs.file_.size()
              << std::endl;

    TByteVec bytes;
    bytes << uintEncode(0x1A45DFA3)
          << uintEncode(0x4286)
          << vsizeEncode(1)
          << uintEncode(1)
          << uintEncode(0x42f7)
          << vsizeEncode(1)
          << uintEncode(1);

    fs.file_.setSize(0);
    IStorage::IReceiptPtr receipt = Yamka::save(fs, bytes);
    if (receipt)
    {
      std::cout << "stored " << bytes.size() << " bytes" << std::endl;
    }
  }

  FileStorage fs2(std::string("testYamka.ebml"), File::kReadWrite);
  if (!fs2.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << fs2.file_.filename()
              << " to read/write"
              << std::endl;
    ::exit(1);
  }
  else
  {
    std::cout << "opened (rw) " << fs2.file_.filename()
              << ", current file size: " << fs2.file_.size()
              << std::endl;

    EbmlDoc doc;
    doc.head_.payload_.docType_.payload_.set(std::string("yamka"));
    doc.head_.payload_.docTypeVersion_.payload_.set(1);
    doc.head_.payload_.docTypeReadVersion_.payload_.set(1);

    fs2.file_.setSize(0);
    IStorage::IReceiptPtr receipt = doc.save(fs2);

    if (receipt)
    {
      std::cout << "stored " << doc.calcSize() << " bytes" << std::endl;
    }
  }

  // close fs2:
  fs2 = FileStorage();

  FileStorage ebmlSrc(std::string("testYamka.ebml"), File::kReadOnly);
  if (!ebmlSrc.file_.isOpen())
  {
    std::cerr << "ERROR: failed to open " << ebmlSrc.file_.filename()
              << " to read"
              << std::endl;
    ::exit(1);
  }
  else
  {
    uint64 ebmlSrcSize = ebmlSrc.file_.size();
    std::cout << "opened (ro) " << ebmlSrc.file_.filename()
              << ", file size: " << ebmlSrcSize
              << std::endl;
  }

  // close the file:
  ebmlSrc = FileStorage();

  // test memory storage:
  {
    FileStorage d2f(std::string("yamka-direct-to-file.ebml"),
                    File::kReadWrite);
    assert(d2f.file_.isOpen());
    d2f.file_.setSize(0);

    FileStorage m2f(std::string("yamka-memory-to-file.ebml"),
                    File::kReadWrite);
    assert(m2f.file_.isOpen());
    m2f.file_.setSize(0);

    EbmlDoc doc;
    doc.head_.payload_.docType_.payload_.set(std::string("storage-test"));
    doc.head_.payload_.docTypeVersion_.payload_.set(16);
    doc.head_.payload_.docTypeReadVersion_.payload_.set(78);

    IStorage::IReceiptPtr rd2f = doc.save(d2f);

    IStorage::IReceiptPtr rmem = doc.save(MemoryStorage::Instance);
    IStorage::IReceiptPtr rm2f = rmem->saveTo(m2f);

    assert(rd2f->numBytes() == rm2f->numBytes());

    std::vector<unsigned char> mem((std::size_t)(rmem->numBytes()));
    rmem->load(&mem[0]);

    ConstMemoryStorage ro(&mem[0], mem.size());
    EbmlDoc doc2;
    Yamka::uint64 bytesConsumed = doc2.load(ro, mem.size());
    assert(bytesConsumed == doc.calcSize());
    assert(doc.head_.payload_.docType_.payload_.get() ==
           doc2.head_.payload_.docType_.payload_.get());
    assert(doc.head_.payload_.docTypeVersion_.payload_.get() ==
           doc2.head_.payload_.docTypeVersion_.payload_.get());
    assert(doc.head_.payload_.docTypeReadVersion_.payload_.get() ==
           doc2.head_.payload_.docTypeReadVersion_.payload_.get());
  }

  // make sure direct-to-file and memory-to-file produce the same results:
  {
    FileStorage d2f(std::string("yamka-direct-to-file.ebml"),
                    File::kReadOnly);
    assert(d2f.file_.isOpen());
    uint64 sz_d2f = d2f.file_.size();

    FileStorage m2f(std::string("yamka-memory-to-file.ebml"),
                    File::kReadOnly);
    assert(m2f.file_.isOpen());
    uint64 sz_m2f = m2f.file_.size();

    assert(sz_d2f == sz_m2f);

    std::vector<unsigned char> dat_d2f((std::size_t)sz_d2f);
    IStorage::IReceiptPtr rd2f = d2f.load(&dat_d2f[0], (std::size_t)sz_d2f);

    std::vector<unsigned char> dat_m2f((std::size_t)sz_m2f);
    IStorage::IReceiptPtr rmem = m2f.load(&dat_m2f[0], (std::size_t)sz_m2f);

    assert(dat_d2f == dat_m2f);
  }

  // check HodgePodgeConstIter
  {
    const std::string text[] = {
      std::string("a. Line One"),
      std::string("2) Second Line"),
      std::string("III, Line The Third")
    };

    HodgePodge hodgePodge;
    for (std::size_t i = 0; i < 3; i++)
    {
      hodgePodge.add(receiptForConstMemory(&(text[i][0]), text[i].size()));
    }

    HodgePodgeConstIter iter0(hodgePodge);
    HodgePodgeConstIter iter1(hodgePodge);
    std::size_t offset = 0;

    for (std::size_t i = 0; i < 3; i++)
    {
      const unsigned char * txt = (unsigned char *)&(text[i][0]);
      const std::size_t txtLen = text[i].size();
      const unsigned char * src = txt;
      const unsigned char * end = src + txtLen;

      for (; src < end; ++src, ++offset, ++iter1)
      {
        unsigned char c0 = iter0[offset];
        unsigned char c1 = *iter1;
        assert(c0 == c1);
        assert(*src == c0);
      }
    }
  }

  std::cerr << "done..." << std::endl;
  return 0;
}

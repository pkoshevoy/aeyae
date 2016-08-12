// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Nov  6 23:09:59 MDT 2010
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
#include <iomanip>
#include <string.h>
#include <string>
#include <time.h>

// namespace access:
using namespace Yamka;

//----------------------------------------------------------------
// usage
//
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "USAGE: " << argv[0]
            << " [-q] [--readEverything] [--skipClusters]"
            << " -i source.mkv"
            << std::endl;

  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }

  ::exit(1);
}


//----------------------------------------------------------------
// PartialReader
//
struct PartialReader : public IDelegateLoad
{
  // virtual:
  uint64 load(IStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    if (eltId != Segment::TCluster::kId)
    {
      // let the generic load mechanism handle it:
      return 0;
    }

    // skip/postpone reading the cluster (to shorten file load time):
    storage.skip(payloadBytesToRead);
    return payloadBytesToRead;
  }
};


//----------------------------------------------------------------
// Examiner
//
struct Examiner : public IElementCrawler
{
  enum Verbosity
  {
    kHideFileOffsets = 0,
    kShowFileOffsets = 1
  };

  Examiner(Verbosity verbosity):
    verbosity_(verbosity),
    indentation_(0),
    clusterTime_(0)
  {}

  // virtual:
  bool eval(IElement & elt)
  {
    IStorage::IReceiptPtr storageReceipt = elt.storageReceipt();
    IStorage::IReceiptPtr payloadReceipt = elt.payloadReceipt();
    IStorage::IReceiptPtr crc32Receipt = elt.crc32Receipt();

    uint64 eltId = elt.getId();
    IPayload & payload = elt.getPayload();

    if (storageReceipt)
    {
      std::cout
        << indent(indentation_)
        << std::setw(8) << uintEncode(eltId);

      if (verbosity_ == kShowFileOffsets)
      {
        std::cout
          << " @ "
          << std::hex << "0x"
          << storageReceipt->position()
          << std::dec
          << " ("
          << storageReceipt->position()
          << ")";
      }

      std::cout << " -- " << elt.getName();

      if (verbosity_ == kShowFileOffsets)
      {
        std::cout << ", " << storageReceipt->numBytes() << " bytes";
      }

      if (payloadReceipt && verbosity_ == kShowFileOffsets)
      {
        std::cout << " (" << payloadReceipt->numBytes() << " payload)";
      }

      std::cout << std::endl;

      if (crc32Receipt)
      {
        std::cout
          << indent(indentation_)
          << " CRC-32";

        if (verbosity_ == kShowFileOffsets)
        {
          std::cout
            << " @ "
            << std::hex << "0x"
            << crc32Receipt->position()
            << std::dec
            << " ("
            << crc32Receipt->position()
            << ")";
        }

        std::cout
          << std::hex << ", stored checksum 0x"
          << elt.storedCrc32_
          << std::dec;

        if (elt.computedCrc32_ && elt.computedCrc32_ != elt.storedCrc32_)
        {
          std::cout
            << ", does not match computed checksum "
            << std::hex << "0x"
            << elt.computedCrc32_
            << std::dec;
        }

        std::cout << std::endl;
      }

      if (eltId == Segment::TCluster::kId)
      {
        Cluster * cluster = dynamic_cast<Cluster *>(&payload);
        clusterTime_ = cluster->timecode_.payload_.get();
      }
    }

    Indent::More indentMore(indentation_);

    if (payload.isComposite())
    {
      EbmlMaster * ebmlMaster = dynamic_cast<EbmlMaster *>(&payload);
      for (std::list<IPayload::TVoid>::iterator i = ebmlMaster->voids_.begin();
           i != ebmlMaster->voids_.end(); ++i)
      {
        IPayload::TVoid & eltVoid = *i;
        eval(eltVoid);
      }

      payload.eval(*this);
    }
    else if (payloadReceipt)
    {
      const VEltPosition * vEltPos = dynamic_cast<VEltPosition *>(&payload);
      if (vEltPos && !vEltPos->hasPosition())
      {
        return false;
      }

      const VInt * vInt = dynamic_cast<VInt *>(&payload);
      const VUInt * vUInt = dynamic_cast<VUInt *>(&payload);
      const VFloat * vFloat = dynamic_cast<VFloat *>(&payload);
      const VDate * vDate = dynamic_cast<VDate *>(&payload);
      const VString * vString = dynamic_cast<VString *>(&payload);
      const VVoid * vVoid = dynamic_cast<VVoid *>(&payload);
      const VBinary * vBinary = dynamic_cast<VBinary *>(&payload);

      std::cout << indent(indentation_);

      if (vInt)
      {
        std::cout << "int: " << vInt->get();
      }
      else if (vUInt)
      {
        std::cout << "uint: " << vUInt->get();
      }
      else if (vFloat)
      {
        std::cout << "float: "
                  << std::setiosflags(std::ios_base::fixed)
                  << vFloat->get();
      }
      else if (vDate)
      {
        struct tm gmt;

#ifdef _WIN32
        __time64_t t = __time64_t(kDateMilleniumUTC +
                                  vDate->get() / 1000000000);
        _gmtime64_s(&gmt, &t);
#else
        time_t t = time_t(kDateMilleniumUTC +
                          vDate->get() / 1000000000);
        gmtime_r(&t, &gmt);
#endif

        std::cout
          << "date: " << vDate->get() << ", "
          << std::right
          << std::setfill('0') << std::setw(4) << gmt.tm_year + 1900 << '/'
          << std::setfill('0') << std::setw(2) << gmt.tm_mon + 1 << '/'
          << std::setfill('0') << std::setw(2) << gmt.tm_mday << ' '
          << std::setfill('0') << std::setw(2) << gmt.tm_hour << ':'
          << std::setfill('0') << std::setw(2) << gmt.tm_min << ':'
          << std::setfill('0') << std::setw(2) << gmt.tm_sec;
      }
      else if (vString)
      {
        std::cout << "string: " << vString->get();
      }
      else if (vVoid)
      {
        std::cout << "void, size " << vVoid->get();
      }
      else if (vBinary)
      {
        if (eltId == BlockGroup::TBlock::kId ||
            eltId == Cluster::TSimpleBlock::kId ||
            eltId == Cluster::TEncryptedBlock::kId)
        {
          SimpleBlock block;
          block.importData(vBinary->data_);

          short int offset = block.getRelativeTimecode();
          std::cout << "track "
                    << std::right << std::setfill(' ') << std::setw(3)
                    << block.getTrackNumber()
                    << ", abs time "
                    << std::right << std::setfill(' ') << std::setw(10)
                    << (clusterTime_ + offset);

          if (block.isKeyframe())
          {
            std::cout << ", key";
          }

          if (block.isDiscardable())
          {
            std::cout << ", discardable";
          }

          std::size_t n = block.getNumberOfFrames();
          std::cout << ", " << (n == 1 ? "frame" : "frames");
        }
        else
        {
          std::cout << "variable size binary data";
          uint64 binSize = vBinary->data_.numBytes();
          if (binSize)
          {
            std::cout << ", size " << binSize;
          }
        }

        static const char * hex = "0123456789abcdef";
        HodgePodgeConstIter blockDataIter(vBinary->data_);
        const uint64 nbytes = vBinary->data_.numBytes();
        uint64 n = std::min<uint64>(nbytes, 16);
        for (std::size_t i = 0; i < n; i++)
        {
          unsigned char b = blockDataIter[i];
          std::cout << ' ' << hex[b >> 4] << hex[b & 0xf];
        }

        if (n < nbytes)
        {
          std::cout << " ...";
        }
      }
      else if (vEltPos)
      {
        std::cout << "element position";

        if (verbosity_ == kShowFileOffsets)
        {
          std::cout
            << std::hex << " 0x"
            << vEltPos->position()
            << std::dec;
        }

        const IElement * elt = vEltPos->getElt();
        if (!elt)
        {
          std::cout << ", unresolved";
        }
        else if (elt->storageReceipt())
        {
          IStorage::IReceiptPtr storageReceipt = elt->storageReceipt();
          std::cout
            << ", resolved to "
            << elt->getName()
            << "(" << uintEncode(elt->getId()) << ")";

          if (verbosity_ == kShowFileOffsets)
          {
            std::cout
              << " @ "
              << std::hex << "0x"
              << storageReceipt->position()
              << std::dec;
          }
        }
      }
      else
      {
        std::cout << "binary data, size " << payload.calcSize();

        if (payloadReceipt)
        {
          TByteVec data;
          if (Yamka::load(payloadReceipt, data) && data.size())
          {
            std::cout << ", ";
            std::cout << data;
          }
        }
      }

      if (!elt.mustSave())
      {
        std::cout << ", default value";
      }

      std::cout << std::endl;
    }

    return false;
  }

  Verbosity verbosity_;
  unsigned int indentation_;
  uint64 clusterTime_;
};


//----------------------------------------------------------------
// main
//
int
main(int argc, char ** argv)
{
#ifdef _WIN32
  get_main_args_utf8(argc, argv);
#endif

  Examiner::Verbosity verbosity = Examiner::kShowFileOffsets;
  std::string srcPath;
  bool skipClusters = false;
  bool useSeekHead = true;

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-i") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -i parameter");
      i++;
      srcPath.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-q") == 0)
    {
      verbosity = Examiner::kHideFileOffsets;
    }
    else if (strcmp(argv[i], "--skipClusters") == 0)
    {
      skipClusters = true;
    }
    else if (strcmp(argv[i], "--readEverything") == 0)
    {
      useSeekHead = false;
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

  PartialReader fastLoader;
  IDelegateLoad * loader = skipClusters ? &fastLoader : NULL;

  uint64 bytesRead = 0;

  if (useSeekHead)
  {
    bool loadClusters = !skipClusters;

    if (doc.loadSeekHead(src, srcSize) &&
        doc.loadViaSeekHead(src, loader, loadClusters))
    {
      bytesRead = srcSize;
    }
  }
  else
  {
    bytesRead = doc.loadAndKeepReceipts(src, srcSize, loader);
  }

  if (!bytesRead || doc.segments_.empty())
  {
    usage(argv, (std::string("source file has no matroska segments").c_str()));
  }

  Examiner examiner(verbosity);
  doc.eval(examiner);

  // close open file handles:
  doc = MatroskaDoc();
  src = FileStorage();

  return 0;
}

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

// system includes:
#include <iostream>

// namespace access:
using namespace Yamka;

//----------------------------------------------------------------
// usage
// 
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "USAGE: " << argv[0]
            << " source.mkv"
            << std::endl;
  
  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }
  
  ::exit(1);
}

//----------------------------------------------------------------
// Examiner
// 
struct Examiner : public IElementCrawler
{
  Examiner():
    indentation_(0)
  {}
  
  // virtual:
  bool eval(IElement & elt)
  {
    IStorage::IReceiptPtr storageReceipt = elt.storageReceipt();
    if (storageReceipt)
    {
      std::cout
        << indent(indentation_)
        << std::setw(8) << uintEncode(elt.getId())
        << " @ "
        << std::hex << "0x"
        << storageReceipt->position()
        << std::dec
        << " -- " << elt.getName();
      
      IStorage::IReceiptPtr payloadReceipt = elt.payloadReceipt();
      if (payloadReceipt)
      {
        std::cout << ", payload "
                  << payloadReceipt->numBytes()
                  << " bytes";
      }
      
      std::cout << std::endl;
    }
    
    evalPayload(elt.getPayload());
    return false;
  }
  
  // virtual:
  bool evalPayload(IPayload & payload)
  {
    indentation_++;
    
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
    else if (!payload.isDefault())
    {
      std::cout << indent(indentation_);
      
      const VInt * vInt = dynamic_cast<VInt *>(&payload);
      const VUInt * vUInt = dynamic_cast<VUInt *>(&payload);
      const VFloat * vFloat = dynamic_cast<VFloat *>(&payload);
      const VDate * vDate = dynamic_cast<VDate *>(&payload);
      const VString * vString = dynamic_cast<VString *>(&payload);
      const VVoid * vVoid = dynamic_cast<VVoid *>(&payload);
      const VBinary * vBinary = dynamic_cast<VBinary *>(&payload);
      const VEltPosition * vEltPos = dynamic_cast<VEltPosition *>(&payload);
      
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
        std::cout << "float: " << vFloat->get();
      }
      else if (vDate)
      {
        std::cout << "date: " << vDate->get();
      }
      else if (vString)
      {
        std::cout << "string: " << vString->get();
      }
      else if (vVoid)
      {
        std::cout << "void" << vVoid->get();
      }
      else if (vBinary)
      {
        std::cout << "binary data, variable size";
      }
      else if (vEltPos)
      {
        std::cout
          << "element position "
          << std::hex << "0x"
          << vEltPos->position()
          << std::dec;
        
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
            << "(" << uintEncode(elt->getId()) << ")"
            << " @ "
            << std::hex << "0x"
            << storageReceipt->position()
            << std::dec;
        }
      }
      else
      {
        std::cout << "binary data, size " << payload.calcSize();
      }

      std::cout << std::endl;
    }
    
    indentation_--;
    return false;
  }
  
  unsigned int indentation_;
};


//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  if (argc < 2)
  {
    usage(argv, "you forgot to specify a file to examine");
  }
  else if (argc > 2)
  {
    usage(argv, "too many parameters specified");
  }
  
  std::string srcPath(argv[1]);
  
  FileStorage src(srcPath, File::kReadOnly);
  if (!src.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 srcPath +
                 std::string(" for reading")).c_str());
  }
  
  uint64 srcSize = src.file_.size();
  MatroskaDoc doc;
  uint64 bytesRead = doc.loadAndKeepReceipts(src, srcSize);
  if (!bytesRead || doc.segments_.empty())
  {
    usage(argv, (std::string("source file has no matroska segments").c_str()));
  }

  Examiner examiner;
  doc.eval(examiner);
  
  // close open file handles:
  doc = MatroskaDoc();
  src = FileStorage();

  return 0;
}

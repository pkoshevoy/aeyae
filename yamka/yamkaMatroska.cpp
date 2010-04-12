// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:56:33 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaMatroska.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // SegInfo::SegInfo
  // 
  SegInfo::SegInfo()
  {
    timecodeScale_.alwaysSave().payload_.setDefault(1000000);
    duration_.alwaysSave().payload_.setDefault(0.0);
  }

  //----------------------------------------------------------------
  // Video::Video
  // 
  Video::Video()
  {
    aspectRatioType_.payload_.setDefault(0);
  }

  //----------------------------------------------------------------
  // Audio::Audio
  // 
  Audio::Audio()
  {
    sampFreq_.payload_.setDefault(8000.0);
    channels_.payload_.setDefault(1);
  }

  //----------------------------------------------------------------
  // MatroskaDoc::MatroskaDoc
  // 
  MatroskaDoc::MatroskaDoc():
    EbmlDoc()
  {
    EbmlDoc::head_.payload_.docType_.payload_.set(std::string("matroska"));
    EbmlDoc::head_.payload_.docTypeVersion_.payload_.set(1);
    EbmlDoc::head_.payload_.docTypeReadVersion_.payload_.set(1);
  }
  
}

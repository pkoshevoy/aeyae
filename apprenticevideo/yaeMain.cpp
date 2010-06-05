// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri May 28 00:43:26 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>

// yae includes:
#include <yaeAPI.h>
#include <yaeReaderFFMPEG.h>


//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  yae::ReaderFFMPEG * reader = yae::ReaderFFMPEG::create();
  
  for (int i = 1; i < argc; i++)
  {
    std::ostringstream os;
    os << fileUtf8::kProtocolName << "://" << argv[i];
    // os << "file://" << argv[i];
    
    std::string url(os.str());
    if (!reader->open(url.c_str()))
    {
      std::cerr << "ERROR: could not open movie: " << url << std::endl;
      continue;
    }
    
    std::cout << "opened " << url << std::endl;
    
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    for (std::size_t i = 0; i < numVideoTracks; i++)
    {
      reader->selectVideoTrack(i);
      std::cout << "video track " << i << ", ";
      const char * trackName = reader->getSelectedVideoTrackName();
      if (trackName)
      {
	std::cout << trackName;
      }
      else
      {
	std::cout << "no name";
      }
      
      std::cout << std::endl;
    }
    
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    for (std::size_t i = 0; i < numAudioTracks; i++)
    {
      reader->selectAudioTrack(i);
      std::cout << "audio track " << i << ", ";
      const char * trackName = reader->getSelectedAudioTrackName();
      if (trackName)
      {
	std::cout << trackName;
      }
      else
      {
	std::cout << "no name";
      }
      
      std::cout << std::endl;
    }
  }
  
  reader->destroy();
  return 0;
}

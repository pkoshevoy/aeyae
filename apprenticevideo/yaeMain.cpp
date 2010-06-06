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
      
      yae::TTime duration;
      if (reader->getVideoDuration(duration))
      {
	std::cout << ", duration: "
		  << (double(duration.time_) /
		      double(duration.base_))
		  << " seconds";
      }
      
      yae::TTime position;
      if (reader->getVideoPosition(position))
      {
	std::cout << ", position: "
		  << (double(position.time_) /
		      double(position.base_))
		  << " seconds";
      }
      
      yae::VideoTraits t;
      if (reader->getVideoTraits(t))
      {
	std::cout << ", frame rate: " << t.frameRate_ << " Hz"
		  << ", color format: " << t.colorFormat_
		  << ", encoded frame: " << t.encodedWidth_
		  << " x " << t.encodedHeight_
		  << ", visible offset: (" << t.offsetLeft_
		  << ", " << t.offsetTop_ << ")"
		  << ", visible frame: " << t.visibleWidth_
		  << " x " << t.visibleHeight_
		  << ", pixel aspect ratio: " << t.pixelAspectRatio_
		  << ", is upside down: " << t.isUpsideDown_;
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
      
      yae::TTime duration;
      if (reader->getAudioDuration(duration))
      {
	std::cout << ", duration: "
		  << (double(duration.time_) /
		      double(duration.base_))
		  << " seconds";
      }
      
      yae::TTime position;
      if (reader->getAudioPosition(position))
      {
	std::cout << ", position: "
		  << (double(position.time_) /
		      double(position.base_))
		  << " seconds";
      }
      
      yae::AudioTraits t;
      if (reader->getAudioTraits(t))
      {
	std::cout << ", sample rate: " << t.sampleRate_ << " Hz"
		  << ", sample format: " << t.sampleFormat_
		  << ", channel format: " << t.channelFormat_
		  << ", channel layout: " << t.channelLayout_;
      }
      
      std::cout << std::endl;
    }
  }
  
  reader->destroy();
  return 0;
}

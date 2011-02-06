// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 20:34:27 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <string.h>
#include <typeinfo>

// Qt includes:
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QIODevice>
#include <QMutex>
#include <QMutexLocker>

// yae includes:
#include <yaeAudioRendererQt.h>

namespace yae
{

  //----------------------------------------------------------------
  // AudioRendererQt::AudioRendererQt
  // 
  AudioRendererQt::AudioRendererQt():
    reader_(NULL),
    output_(NULL)
  {}
  
  //----------------------------------------------------------------
  // AudioRendererQt::~AudioRendererQt
  // 
  AudioRendererQt::~AudioRendererQt()
  {
    close();
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::create
  // 
  AudioRendererQt *
  AudioRendererQt::create()
  {
    return new AudioRendererQt();
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::destroy
  // 
  void
  AudioRendererQt::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // AudioRendererQt::getName
  // 
  const char *
  AudioRendererQt::getName() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // AudioRendererQt::countAvailableDevices
  // 
  unsigned int
  AudioRendererQt::countAvailableDevices() const
  {
    return 1;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::getDefaultDeviceIndex
  // 
  unsigned int
  AudioRendererQt::getDefaultDeviceIndex() const
  {
    return 0;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::getDeviceName
  // 
  bool
  AudioRendererQt::getDeviceName(unsigned int deviceIndex,
                                 std::string & deviceName) const
  {
    if (deviceIndex >= countAvailableDevices())
    {
      return false;
    }
    
    QAudioDeviceInfo d = QAudioDeviceInfo::defaultOutputDevice();
    deviceName.assign(d.deviceName().toUtf8().constData());
    return true;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::open
  // 
  bool
  AudioRendererQt::open(unsigned int deviceIndex,
                        IReader * reader)
  {
    if (deviceIndex >= countAvailableDevices())
    {
      return false;
    }
    
    if (QIODevice::isOpen())
    {
      QIODevice::close();
    }

    if (output_)
    {
      output_->stop();
      delete output_;
      output_ = NULL;
    }
    
    reader_ = reader;
    if (!reader_)
    {
      return true;
    }
    
    std::size_t selTrack = reader_->getSelectedAudioTrackIndex();
    std::size_t numTracks = reader_->getNumberOfAudioTracks();
    if (selTrack >= numTracks)
    {
      return false;
    }
    
    AudioTraits atraits;
    if (!reader_->getAudioTraits(atraits))
    {
      return false;
    }
    
    QAudioFormat afmt;
    afmt.setCodec(QString::fromUtf8("audio/pcm"));
    afmt.setChannelCount(atraits.channelLayout_);
    afmt.setSampleRate(atraits.sampleRate_);
    
    switch (atraits.sampleFormat_)
    {
      case kAudio8BitOffsetBinary:
        afmt.setSampleSize(8);
        afmt.setSampleType(QAudioFormat::UnSignedInt);
        break;
        
      case kAudio16BitBigEndian:
        afmt.setSampleSize(16);
        afmt.setSampleType(QAudioFormat::SignedInt);
        afmt.setByteOrder(QAudioFormat::BigEndian);
        break;
        
      case kAudio16BitLittleEndian:
        afmt.setSampleSize(16);
        afmt.setSampleType(QAudioFormat::SignedInt);
        afmt.setByteOrder(QAudioFormat::LittleEndian);
        break;
        
      case kAudio24BitLittleEndian:
        afmt.setSampleSize(24);
        afmt.setSampleType(QAudioFormat::SignedInt);
        afmt.setByteOrder(QAudioFormat::LittleEndian);
        break;
        
      case kAudio32BitFloat:
        afmt.setSampleSize(32);
        afmt.setSampleType(QAudioFormat::Float);
        break;
        
      default:
        assert(false);
        return false;
    }
    
    double frameRate = 30.0;
    
    VideoTraits vtraits;
    if (reader_->getVideoTraits(vtraits) &&
        vtraits.frameRate_ > 0.0)
    {
      frameRate = vtraits.frameRate_;
    }
    
    int bufferSize = int(double(afmt.channelCount() *
                                afmt.sampleSize() / 8 *
                                afmt.sampleRate()) /
                         frameRate + 0.5) * 5;
    
    output_ = new QAudioOutput(afmt);
    output_->setBufferSize(bufferSize);
    
    QIODevice::open(QIODevice::ReadOnly);
    output_->start(this);
    
    emit readyRead();
    return true;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::close
  // 
  void
  AudioRendererQt::close()
  {
    open(0, NULL);
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::isSequential
  // 
  bool
  AudioRendererQt::isSequential() const
  {
    return true;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::readData
  // 
  qint64
  AudioRendererQt::readData(char * dst, qint64 dstSize)
  {
    if (!reader_)
    {
      return 0;
    }
    
    if (!audioFrame_)
    {
      if (!reader_->readAudio(audioFrame_))
      {
        return 0;
      }
      
      audioFrameStartOffset_ = 0;
    }
    
    const unsigned char * src = audioFrame_->getBuffer<unsigned char>();
    std::size_t srcSize = audioFrame_->getBufferSize<unsigned char>();
    
    src += audioFrameStartOffset_;
    srcSize -= audioFrameStartOffset_;

    std::size_t readSize = srcSize < dstSize ? srcSize : dstSize;
    memcpy(dst, src, readSize);
    
    if (readSize < srcSize)
    {
      // some frame data is still available:
      audioFrameStartOffset_ += readSize;
    }
    else
    {
      // the frame is finished:
      audioFrameStartOffset_ = 0;
      audioFrame_ = TAudioFramePtr();
    }
    
    return readSize;
  }
  
  //----------------------------------------------------------------
  // AudioRendererQt::writeData
  // 
  qint64
  AudioRendererQt::writeData(const char * src, qint64 srcSize)
  {
    return 0;
  }
  
};

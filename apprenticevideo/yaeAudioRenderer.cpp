// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 20:34:27 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <string.h>

// Qt includes:
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QIODevice>
#include <QMutex>
#include <QMutexLocker>

// yae includes:
#include <yaeAudioRenderer.h>

namespace yae
{

  //----------------------------------------------------------------
  // RingBuffer::RingBuffer
  // 
  RingBuffer::RingBuffer(std::size_t bufferCapacity):
    data_(bufferCapacity),
    head_(0),
    tail_(0)
  {}
  
  //----------------------------------------------------------------
  // RingBuffer::isSequential
  // 
  bool
  RingBuffer::isSequential() const
  {
    return true;
  }
  
  //----------------------------------------------------------------
  // RingBuffer::readData
  // 
  qint64
  RingBuffer::readData(char * dst, qint64 dstSize)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    const unsigned char * src = &data_.front();
    const qint64 dstSizeBegin = dstSize;
    
    while (isOpen() && dstSize > 0)
    {
      cond_.wait(lock);
      {
        std::size_t bufferSize =
          head_ > tail_ ?
          data_.size() - head_ + tail_ :
          tail_ - head_;
        
        std::size_t bytesToRead =
          std::min<std::size_t>(dstSize, bufferSize);
        
        std::size_t n0 = data_.size() - head_;
        if (n0 < bytesToRead)
        {
          std::size_t n1 = bytesToRead - n0;
          
          memcpy(dst, src + head_, n0);
          dst += n0;
          
          memcpy(dst + n0, src, n1);
          dst += n1;
        }
        else
        {
          memcpy(dst, src + head_, bytesToRead);
          dst += bytesToRead;
        }
        
        head_ = (head_ + bytesToRead) % data_.size();
        dstSize -= bytesToRead;
      }
      cond_.notify_one();
    }
    
    qint64 bytesRead = dstSizeBegin - dstSize;
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // RingBuffer::writeData
  // 
  qint64
  RingBuffer::writeData(const char * src, qint64 srcSize)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    unsigned char * dst = &data_.front();
    const qint64 srcSizeBegin = srcSize;
    
    while (isOpen() && srcSize > 0)
    {
      cond_.wait(lock);
      {
        std::size_t bufferSize =
          head_ > tail_ ?
          data_.size() - head_ + tail_ :
          tail_ - head_;
        
        std::size_t bytesToWrite =
          std::min<std::size_t>(srcSize, bufferSize);
        
        std::size_t n0 = data_.size() - tail_;
        if (n0 < bytesToWrite)
        {
          std::size_t n1 = bytesToWrite - n0;
          
          memcpy(dst + tail_, src, n0);
          src += n0;
          
          memcpy(dst, src, n1);
          src += n1;
        }
        else
        {
          memcpy(dst + tail_, src, bytesToWrite);
          dst += bytesToWrite;
        }
        
        tail_ = (tail_ + bytesToWrite) % data_.size();
        srcSize -= bytesToWrite;
      }
      cond_.notify_one();
    }
    
    qint64 bytesWritten = srcSizeBegin - srcSize;
    return bytesWritten;
  }

  
  //----------------------------------------------------------------
  // AudioRenderer::AudioSource
  // 
  AudioRenderer::AudioRenderer():
    reader_(NULL),
    output_(NULL)
  {}
  
  //----------------------------------------------------------------
  // AudioRenderer::setReader
  // 
  void
  AudioRenderer::setReader(IReader * reader)
  {
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
      return;
    }
    
    std::size_t selTrack = reader_->getSelectedAudioTrackIndex();
    std::size_t numTracks = reader_->getNumberOfAudioTracks();
    if (selTrack >= numTracks)
    {
      return;
    }
    
    AudioTraits atraits;
    if (!reader_->getAudioTraits(atraits))
    {
      return;
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
        return;
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
                         frameRate + 0.5);
    
    output_ = new QAudioOutput(afmt);
    output_->setBufferSize(bufferSize);
    
    QIODevice::open(QIODevice::ReadOnly);
    output_->start(this);
    
    emit readyRead();
  }
  
  //----------------------------------------------------------------
  // AudioRenderer::isSequential
  // 
  bool
  AudioRenderer::isSequential() const
  {
    return true;
  }
  
  //----------------------------------------------------------------
  // AudioRenderer::readData
  // 
  qint64
  AudioRenderer::readData(char * dst, qint64 dstSize)
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
  // AudioRenderer::writeData
  // 
  qint64
  AudioRenderer::writeData(const char * src, qint64 srcSize)
  {
    return 0;
  }
  
};

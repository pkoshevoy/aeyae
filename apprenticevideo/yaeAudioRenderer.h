// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 18:37:06 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_H_
#define YAE_AUDIO_RENDERER_H_

// system includes:
#include <vector>

// Qt includes:
#include <QAudioOutput>
#include <QIODevice>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>

namespace yae
{
  
  //----------------------------------------------------------------
  // RingBuffer
  // 
  class RingBuffer : public QIODevice
  {
    Q_OBJECT;
    
  public:
    RingBuffer(std::size_t bufferCapacity);
    
    // virtual:
    bool isSequential() const;
    
    // virtual:
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);
    
  protected:
    // ring-buffer access is synchronized across threads:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;
    
    // data buffer:
    std::vector<unsigned char> data_;
    
    // read position:
    std::size_t head_;
    
    // write position:
    std::size_t tail_;
  };

  //----------------------------------------------------------------
  // AudioRenderer
  // 
  struct AudioRenderer : public QIODevice
  {
    AudioRenderer();
    
    // helper:
    void setReader(IReader * reader);
    
    // virtual:
    bool isSequential() const;
    
    // virtual:
    qint64 readData(char * dst, qint64 dstSize);
    qint64 writeData(const char * src, qint64 srcSize);
    
  protected:
    // audio source:
    IReader * reader_;
    
    // audio output:
    QAudioOutput * output_;

    // previous audio frame;
    TAudioFramePtr audioFrame_;
    std::size_t audioFrameStartOffset_;
  };
  
};

#endif // YAE_AUDIO_RENDERER_H_

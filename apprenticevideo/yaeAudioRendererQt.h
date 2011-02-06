// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 18:37:06 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_QT_H_
#define YAE_AUDIO_RENDERER_QT_H_

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
#include <yaeAudioRenderer.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // AudioRenderer
  // 
  struct YAE_API AudioRendererQt : public QIODevice,
                                   public IAudioRenderer
  {
    Q_OBJECT;
    
  protected:
    AudioRendererQt();
    ~AudioRendererQt();

  public:
    static AudioRendererQt * create();
    virtual void destroy();
    
    //! return a human readable name for this renderer (preferably unique):
    virtual const char * getName() const;

    //! there may be multiple audio rendering devices available:
    virtual unsigned int countAvailableDevices() const;
    
    //! return index of the system default audio rendering device:
    virtual unsigned int getDefaultDeviceIndex() const;
    
    //! get device name and max audio resolution capabilities:
    virtual bool getDeviceName(unsigned int deviceIndex,
                               std::string & deviceName) const;

    //! begin rendering audio frames from a given reader:
    virtual bool open(unsigned int deviceIndex,
                      IReader * reader);

    //! terminate audio rendering:
    virtual void close();
    
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
}


#endif // YAE_AUDIO_RENDERER_QT_H_

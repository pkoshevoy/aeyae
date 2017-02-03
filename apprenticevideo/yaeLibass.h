// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LIBASS_H_
#define YAE_LIBASS_H_

// standard libraries:
#include <list>
#include <string>
#include <vector>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#endif

// libass includes:
extern "C"
{
#include <ass/ass.h>
}

// yae includes:
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"


namespace yae
{
  // forward declarations:
  struct TLibass;


  //----------------------------------------------------------------
  // getFontsConf
  //
  bool
  getFontsConf(std::string & fontsConf, bool & removeAfterUse);


  //----------------------------------------------------------------
  // TFontAttachment
  //
  struct YAE_API TFontAttachment
  {
    TFontAttachment(const char * filename = NULL,
                    const unsigned char * data = NULL,
                    std::size_t size = 0):
      filename_(filename),
      data_(data),
      size_(size)
    {}

    const char * filename_;
    const unsigned char * data_;
    std::size_t size_;
  };


  //----------------------------------------------------------------
  // AssTrack
  //
  struct AssTrack
  {
    AssTrack(TLibass & libass,
             const unsigned char * header,
             const std::size_t headerSize);
    ~AssTrack();

    // call this after seeking to flush earlier events:
    void flushEvents();

    void processData(const unsigned char * data,
                     std::size_t size,
                     int64 pts);

    ASS_Image * renderFrame(int64 now, int * detectChange);

  protected:
    // intentionally disabled:
    AssTrack(const AssTrack &);
    AssTrack & operator = (const AssTrack &);

    //----------------------------------------------------------------
    // Dialogue
    //
    struct Dialogue
    {
      Dialogue(int64 pts = 0,
               const unsigned char * data = NULL,
               std::size_t size = 0):
        pts_(pts),
        data_((const char *)data, (const char *)data + size)
      {}

      inline bool operator == (const Dialogue & sub) const
      {
        return pts_ == sub.pts_ && data_ == sub.data_;
      }

      // presentation timestamp expressed in milliseconds:
      int64 pts_;

      // subtitle dialog line:
      std::string data_;
    };

    TLibass & libass_;
    ASS_Track * track_;
    std::vector<char> header_;
    std::list<Dialogue> buffer_;
    std::size_t bufferSize_;
  };

  //----------------------------------------------------------------
  // TAssTrackPtr
  //
  typedef boost::shared_ptr<AssTrack> TAssTrackPtr;


  //----------------------------------------------------------------
  // TLibassInitDoneCallback
  //
  typedef void(*TLibassInitDoneCallback)(void *, TLibass *);


  //----------------------------------------------------------------
  // TLibass
  //
  struct TLibass
  {
    friend struct AssTrack;
    friend struct Threadable<TLibass>;

    TLibass();
    ~TLibass();

    // initialize libass on a background thread and
    // receive a callback when initialization is finished:
    void asyncInit(TLibassInitDoneCallback callback, void * context);
    void asyncInitStop();

    // check whether libass initialization is finished:
    bool isReady() const;

    // configure the renderer frame size:
    void setFrameSize(int w, int h);

    // add custom fonts to the library:
    void addCustomFont(const TFontAttachment & customFont);

    // add an events new track:
    TAssTrackPtr track(const unsigned char * codecPrivate,
                       const std::size_t codecPrivateSize);

  protected:
    void init();
    void threadLoop();
    void addCustomFonts();

    // libass initialization worker thread:
    mutable boost::mutex mutex_;
    Thread<TLibass> thread_;
    bool initialized_;

    ASS_Library * library_;
    ASS_Renderer * renderer_;
    Queue<TFontAttachment> fonts_;

    void * callbackContext_;
    TLibassInitDoneCallback callback_;
  };

}


#endif // YAE_LIBASS_H_

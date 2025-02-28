// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LIBASS_H_
#define YAE_LIBASS_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/utils/yae_fifo.h"

// standard:
#include <list>
#include <string>
#include <vector>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// libass:
extern "C"
{
#include <ass/ass.h>
}


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
  struct YAEUI_API TFontAttachment
  {
    TFontAttachment(const std::string & filename = std::string(),
                    const Data & data = Data()):
      filename_(filename),
      data_(data)
    {}

    std::string filename_;
    Data data_;
  };


  //----------------------------------------------------------------
  // AssTrack
  //
  struct AssTrack
  {
    AssTrack(TLibass & libass,
             const std::string & trackId,
             const unsigned char * header,
             const std::size_t headerSize);
    ~AssTrack();

    inline const std::string & trackId() const
    { return trackId_; }

    // detect subtitle track changes:
    bool sameHeader(const unsigned char * header,
                    const std::size_t headerSize) const;

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
    std::string trackId_;
    std::vector<char> header_;
    yae::fifo<Dialogue> buffer_;
  };

  //----------------------------------------------------------------
  // TAssTrackPtr
  //
  typedef yae::shared_ptr<AssTrack> TAssTrackPtr;


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
    TAssTrackPtr track(const std::string & trackId,
                       const unsigned char * codecPrivate,
                       const std::size_t codecPrivateSize);

  protected:
    void init();
    void thread_loop();
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

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:55:21 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <assert.h>

// GLEW includes:
#include <GL/glew.h>

// Qt includes:
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>

// yae includes:
#include <yaeReaderFFMPEG.h>
#include <yaePixelFormats.h>
#include <yaePixelFormatTraits.h>
#include <yaeAudioRendererPortaudio.h>
#include <yaeVideoRenderer.h>

// local includes:
#include <yaeMainWindow.h>


namespace yae
{

  //----------------------------------------------------------------
  // MainWindow::MainWindow
  // 
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    reader_(NULL),
    canvas_(NULL),
    audioRenderer_(NULL),
    videoRenderer_(NULL)
  {
    setupUi(this);
    setAcceptDrops(true);

    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
  
    canvas_ = new Canvas(contextFormat);
    reader_ = ReaderFFMPEG::create();
    
    audioRenderer_ = AudioRendererPortaudio::create();
    videoRenderer_ = VideoRenderer::create();
    
    delete centralwidget->layout();
    QVBoxLayout * layout = new QVBoxLayout(centralwidget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(canvas_);
    
    bool ok = true;
    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    assert(ok);
    
    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    assert(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  // 
  MainWindow::~MainWindow()
  {
    audioRenderer_->close();
    audioRenderer_->destroy();
    
    videoRenderer_->close();
    videoRenderer_->destroy();
    
    reader_->destroy();
    delete canvas_;
  }

  //----------------------------------------------------------------
  // MainWindow::canvas
  // 
  Canvas *
  MainWindow::canvas() const
  {
    return canvas_;
  }
  
  //----------------------------------------------------------------
  // MainWindow::load
  // 
  bool
  MainWindow::load(const QString & path)
  {
    std::ostringstream os;
    os << fileUtf8::kProtocolName << "://" << path.toUtf8().constData();
    
    std::string url(os.str());
    
    ReaderFFMPEG * reader = ReaderFFMPEG::create();
    if (!reader->open(url.c_str()))
    {
      std::cerr << "ERROR: could not open movie: " << url << std::endl;
      return false;
    }
    
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    
    reader->threadStop();
    
    std::cout << std::endl
              << "yae: " << url << std::endl
              << "yae: video tracks: " << numVideoTracks << std::endl
              << "yae: audio tracks: " << numAudioTracks << std::endl;
    
    if (numVideoTracks)
    {
      reader->selectVideoTrack(0);
      VideoTraits vtts;
      if (reader->getVideoTraits(vtts))
      {
        // pixel format shortcut:
        const pixelFormat::Traits * ptts =
          pixelFormat::getTraits(vtts.pixelFormat_);

        std::cout << "yae: native format: ";
        if (ptts)
        {
          std::cout << ptts->name_;
        }
        else
        {
          std::cout << "unsupported" << std::endl;
        }
        std::cout << std::endl;
        
#if 1
        bool unsupported = ptts == NULL;

        if (!unsupported)
        {
          unsupported = (ptts->flags_ & pixelFormat::kPaletted) != 0;
        }

        if (!unsupported)
        {
          GLint internalFormatGL;
          GLenum pixelFormatGL;
          GLenum dataTypeGL;
          GLint shouldSwapBytes;
          unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                         internalFormatGL,
                                                         pixelFormatGL,
                                                         dataTypeGL,
                                                         shouldSwapBytes);
          unsupported = (supportedChannels < 1 /* ptts->channels_ */);
        }
        
        if (unsupported)
        {
          vtts.pixelFormat_ = kPixelFormatGRAY8;
          
          if (ptts)
          {
            if ((ptts->flags_ & pixelFormat::kAlpha) &&
                (ptts->flags_ & pixelFormat::kColor))
            {
              vtts.pixelFormat_ = kPixelFormatBGRA;
            }
            else if ((ptts->flags_ & pixelFormat::kColor) ||
                     (ptts->flags_ & pixelFormat::kPaletted))
            {
              if (false && glewIsExtensionSupported("GL_APPLE_ycbcr_422"))
              {
                vtts.pixelFormat_ = kPixelFormatYUYV422;
              }
              else
              {
                vtts.pixelFormat_ = kPixelFormatBGR24;
              }
            }
          }
          
          reader->setVideoTraitsOverride(vtts);
        }
#elif 0
        vtts.pixelFormat_ = kPixelFormatRGB565BE;
        reader->setVideoTraitsOverride(vtts);
#endif
      }
      
      if (reader->getVideoTraitsOverride(vtts))
      {
        const pixelFormat::Traits * ptts =
          pixelFormat::getTraits(vtts.pixelFormat_);

        if (ptts)
        {
          std::cout << "yae: output format: " << ptts->name_
                    << ", par: " << vtts.pixelAspectRatio_
                    << ", " << vtts.visibleWidth_
                    << " x " << vtts.visibleHeight_;
          
          if (vtts.pixelAspectRatio_ != 0.0)
          {
            std::cout << ", dar: "
                      << (double(vtts.visibleWidth_) *
                          vtts.pixelAspectRatio_ /
                          double(vtts.visibleHeight_))
                      << ", " << int(vtts.visibleWidth_ *
                                     vtts.pixelAspectRatio_ +
                                     0.5)
                      << " x " << vtts.visibleHeight_;
          }
          
          std::cout << std::endl;
        }
        else
        {
          // unsupported pixel format:
          reader->selectVideoTrack(numVideoTracks);
        }
      }
    }
    
    if (numAudioTracks)
    {
      reader->selectAudioTrack(0);
    }
    
    // setup renderer shared reference clock:
    videoRenderer_->close();
    audioRenderer_->close();
    
    if (numAudioTracks)
    {
      audioRenderer_->takeThisClock(SharedClock());
      audioRenderer_->obeyThisClock(audioRenderer_->clock());
      
      if (numVideoTracks)
      {
        videoRenderer_->obeyThisClock(audioRenderer_->clock());
      }
    }
    else if (numVideoTracks)
    {
      videoRenderer_->takeThisClock(SharedClock());
      videoRenderer_->obeyThisClock(videoRenderer_->clock());
    }
    
    // update the renderers:
    reader_->close();
    unsigned int audioDevice = audioRenderer_->getDefaultDeviceIndex();
    if (!audioRenderer_->open(audioDevice, reader))
    {
      AudioTraits atts;
      if (reader->getAudioTraits(atts))
      {
        atts.channelLayout_ = kAudioStereo;
        reader->setAudioTraitsOverride(atts);
      }
      
      if (!audioRenderer_->open(audioDevice, reader))
      {
        reader->selectAudioTrack(numAudioTracks);
        
        if (numVideoTracks)
        {
          videoRenderer_->takeThisClock(SharedClock());
          videoRenderer_->obeyThisClock(videoRenderer_->clock());
        }
      }
    }
    
    reader->threadStart();
    videoRenderer_->open(canvas_, reader);
    
    // replace the previous reader:
    reader_->destroy();
    reader_ = reader;
    
    return true;
  }
  
  //----------------------------------------------------------------
  // MainWindow::fileOpen
  // 
  void
  MainWindow::fileOpen()
  {
    QString filename =
      QFileDialog::getOpenFileName(this,
				   "Open file",
				   QString(),
				   "movies ("
                                   "*.avi "
                                   "*.asf "
                                   "*.divx "
                                   "*.flv "
                                   "*.f4v "
                                   "*.m2t "
                                   "*.m2ts "
                                   "*.m4v "
                                   "*.mkv "
                                   "*.mod "
                                   "*.mov "
                                   "*.mpg "
                                   "*.mp4 "
                                   "*.mpeg "
                                   "*.mpts "
                                   "*.ogm "
                                   "*.ogv "
                                   "*.ts "
                                   "*.wmv "
                                   "*.webm "
                                   ")");
    load(filename);
  }
  
  //----------------------------------------------------------------
  // MainWindow::fileExit
  // 
  void
  MainWindow::fileExit()
  {
    reader_->close();
    MainWindow::close();
    qApp->quit();
  }
  
  //----------------------------------------------------------------
  // MainWindow::closeEvent
  // 
  void
  MainWindow::closeEvent(QCloseEvent * e)
  {
    e->accept();
    fileExit();
  }
  
  //----------------------------------------------------------------
  // MainWindow::dragEnterEvent
  // 
  void
  MainWindow::dragEnterEvent(QDragEnterEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }
    
    e->acceptProposedAction();
  }
  
  //----------------------------------------------------------------
  // MainWindow::dropEvent
  // 
  void
  MainWindow::dropEvent(QDropEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }
    
    e->acceptProposedAction();
    
    QString filename = e->mimeData()->urls().front().toLocalFile();
    load(filename);
  }
  
};

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:55:21 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>

// GLEW includes:
#include <GL/glew.h>

// Qt includes:
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>
#include <QSpacerItem>
#include <QDesktopWidget>

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
  // AboutDialog::AboutDialog
  // 
  AboutDialog::AboutDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);
  }
  
  
  //----------------------------------------------------------------
  // MainWindow::MainWindow
  // 
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    fullscreen_(NULL, Qt::Widget | Qt::FramelessWindowHint),
    fullscreenLayout_(NULL),
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
    
    fullscreen_.resize(640, 480);
    
    QPushButton * fullscreenButton = new QPushButton(this);
    fullscreenButton->setText(tr("exit full screen view"));
    
    QGridLayout * fullscreenLayout = new QGridLayout(&fullscreen_);
    fullscreenLayout_ = fullscreenLayout;
    
    fullscreenLayout->setMargin(0);
    fullscreenLayout->setSpacing(0);
    fullscreenLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding),
                              0, 0);
    fullscreenLayout->addWidget(fullscreenButton, 0, 1);
    fullscreenLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding),
                              0, 2);
    
    QActionGroup * aspectRatioGroup = new QActionGroup(this);
    aspectRatioGroup->addAction(actionAspectRatioAuto);
    aspectRatioGroup->addAction(actionAspectRatio1_33);
    aspectRatioGroup->addAction(actionAspectRatio1_78);
    aspectRatioGroup->addAction(actionAspectRatio1_85);
    aspectRatioGroup->addAction(actionAspectRatio2_35);
    actionAspectRatioAuto->setChecked(true);
    
    bool ok = true;
    ok = connect(fullscreenButton, SIGNAL(clicked()),
                 this, SLOT(exitFullScreen()));
    YAE_ASSERT(ok);
    
    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);
    
    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioAuto, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioAuto()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAspectRatio1_33, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_33()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAspectRatio1_78, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_78()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAspectRatio1_85, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_85()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAspectRatio2_35, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_35()));
    YAE_ASSERT(ok);
    
    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);
    
    ok = connect(actionShrinkWrap, SIGNAL(triggered()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(canvas_, SIGNAL(toggleFullScreen()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(canvas_, SIGNAL(exitFullScreen()),
                 this, SLOT(exitFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(canvas_, SIGNAL(togglePause()),
                 this, SLOT(playbackPause()));
    YAE_ASSERT(ok);
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
          unsupported = (supportedChannels < 1 ||
                         supportedChannels != ptts->channels_ &&
                         actionColorConverter->isChecked());
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
          
          std::cout << ", fps: " << vtts.frameRate_
                    << std::endl;
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
    reader_->close();
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
  // MainWindow::playbackAspectRatioAuto
  // 
  void
  MainWindow::playbackAspectRatioAuto()
  {}

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_35
  // 
  void
  MainWindow::playbackAspectRatio2_35()
  {}

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_85
  // 
  void
  MainWindow::playbackAspectRatio1_85()
  {}
  
  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_78
  // 
  void
  MainWindow::playbackAspectRatio1_78()
  {}

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_33
  // 
  void
  MainWindow::playbackAspectRatio1_33()
  {}

  //----------------------------------------------------------------
  // MainWindow::playbackColorConverter
  // 
  void
  MainWindow::playbackColorConverter()
  {}
  
  //----------------------------------------------------------------
  // MainWindow::playbackShrinkWrap
  // 
  void
  MainWindow::playbackShrinkWrap()
  {
    // get image dimensions:
    int vw = int(0.5 + canvas_->imageWidth());
    int vh = int(0.5 + canvas_->imageHeight());
    if (vw < 1 || vh < 1)
    {
      return;
    }
    
    QRect rectWindow = frameGeometry();
    int ww = rectWindow.width();
    int wh = rectWindow.height();
    
    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();
    
    int fx0 = rectCanvas.x() - rectWindow.x();
    int fy0 = rectCanvas.y() - rectWindow.y();
    int fx1 = ((rectWindow.x() + ww) -
               (rectCanvas.x() + cw));
    int fy1 = ((rectWindow.y() + wh) -
               (rectCanvas.y() + ch));
    
    int dx = vw - cw;
    int dy = vh - ch;
    
    // calculate frame width, height overhead:
    int ox = ww - cw;
    int oy = wh - ch;
    
    int ideal_w = ww + dx;
    int ideal_h = wh + dy;
    
    QRect rectMax = QApplication::desktop()->availableGeometry(this);
    int max_w = rectMax.width();
    int max_h = rectMax.height();
    
    if (ideal_w > max_w || ideal_h > max_h)
    {
      // image won't fit on screen, scale it to the largest size that fits:
      double vDAR = canvas_->imageWidth() / canvas_->imageHeight();
      double cDAR = double(max_w - ox) / double(max_h - oy);
      
      if (vDAR > cDAR)
      {
        ideal_w = max_w;
        ideal_h = oy + int(0.5 + double(max_w - ox) / vDAR);
      }
      else
      {
        ideal_h = max_h;
        ideal_w = ox + int(0.5 + double(max_h - oy) * vDAR);
      }
    }
    
    int new_w = std::min(ideal_w, max_w);
    int new_h = std::min(ideal_h, max_h);
    
    int max_x0 = rectMax.x();
    int max_y0 = rectMax.y();
    int max_x1 = max_x0 + max_w - 1;
    int max_y1 = max_y0 + max_h - 1;
    
    int new_x0 = rectWindow.x();
    int new_y0 = rectWindow.y();
    int new_x1 = new_x0 + new_w - 1;
    int new_y1 = new_y0 + new_h - 1;
    
    int shift_x = std::min(0, max_x1 - new_x1);
    int shift_y = std::min(0, max_y1 - new_y1);
    
    int new_x = new_x0 + shift_x;
    int new_y = new_y0 + shift_y;
    
    // apply the new window geometry:
    resize(new_w - (fx0 + fx1), new_h - (fy0 + fy1));
    move(new_x, new_y);
  }
  
  //----------------------------------------------------------------
  // swapLayouts
  // 
  static void
  swapLayouts(QWidget * a, QWidget * b)
  {
    QWidget tmp;
    QLayout * la = a->layout();
    QLayout * lb = b->layout();
    tmp.setLayout(la);
    a->setLayout(lb);
    b->setLayout(la);
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackFullScreen
  // 
  void
  MainWindow::playbackFullScreen()
  {
    if (fullscreen_.layout() != fullscreenLayout_)
    {
      exitFullScreen();
      return;
    }

    // enter full screen rendering:
    swapLayouts(centralwidget, &fullscreen_);
    
    fullscreen_.setWindowTitle(tr("full screen: %1").arg(windowTitle()));
    fullscreen_.showFullScreen();
    actionFullScreen->setChecked(true);
    actionShrinkWrap->setEnabled(false);
  }
  
  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  // 
  void
  MainWindow::exitFullScreen()
  {
    if (fullscreen_.layout() != fullscreenLayout_)
    {
      // exit full screen rendering:
      fullscreen_.hide();
      swapLayouts(centralwidget, &fullscreen_);
      actionFullScreen->setChecked(false);
      actionShrinkWrap->setEnabled(true);
    }
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackPause
  // 
  void
  MainWindow::playbackPause()
  {}

  //----------------------------------------------------------------
  // MainWindow::audioSelectTrack
  // 
  void
  MainWindow::audioSelectTrack()
  {}

  //----------------------------------------------------------------
  // MainWindow::videoSelectTrack
  // 
  void
  MainWindow::videoSelectTrack()
  {}
  
  //----------------------------------------------------------------
  // MainWindow::helpAbout
  // 
  void
  MainWindow::helpAbout()
  {
    AboutDialog about(this);
    about.exec();
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

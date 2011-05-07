// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:55:21 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <list>

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
#include <QMenu>
#include <QShortcut>

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
  // SignalBlocker
  // 
  struct SignalBlocker
  {
    SignalBlocker(QObject * qObj = NULL)
    {
      *this << qObj;
    }
    
    ~SignalBlocker()
    {
      while (!blocked_.empty())
      {
        QObject * qObj = blocked_.front();
        blocked_.pop_front();

        qObj->blockSignals(false);
      }
    }
    
    SignalBlocker & operator << (QObject * qObj)
    {
      if (qObj && !qObj->signalsBlocked())
      {
        qObj->blockSignals(true);
        blocked_.push_back(qObj);
      }
      
      return *this;
    }
    
    std::list<QObject *> blocked_;
  };
  
  //----------------------------------------------------------------
  // MainWindow::MainWindow
  // 
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    reader_(NULL),
    canvas_(NULL),
    audioRenderer_(NULL),
    videoRenderer_(NULL),
    playbackPaused_(false),
    playbackInterrupted_(false)
  {
    setupUi(this);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    
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
    
    timelineControls_ = new TimelineControls(this);
    layout->addWidget(timelineControls_);
    
    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);
    shortcutShowTimeline_ = new QShortcut(this);
    
    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);
    shortcutShowTimeline_->setContext(Qt::ApplicationShortcut);
    
    QActionGroup * aspectRatioGroup = new QActionGroup(this);
    aspectRatioGroup->addAction(actionAspectRatioAuto);
    aspectRatioGroup->addAction(actionAspectRatio1_33);
    aspectRatioGroup->addAction(actionAspectRatio1_78);
    aspectRatioGroup->addAction(actionAspectRatio1_85);
    aspectRatioGroup->addAction(actionAspectRatio2_35);
    actionAspectRatioAuto->setChecked(true);
    
    QActionGroup * cropFrameGroup = new QActionGroup(this);
    cropFrameGroup->addAction(actionCropFrameNone);
    cropFrameGroup->addAction(actionCropFrame1_33);
    cropFrameGroup->addAction(actionCropFrame1_78);
    cropFrameGroup->addAction(actionCropFrame1_85);
    cropFrameGroup->addAction(actionCropFrame2_35);
    actionCropFrameNone->setChecked(true);
    
    bool ok = true;
    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);
    
    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);
    
    ok = connect(shortcutExit_, SIGNAL(activated()),
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

    ok = connect(actionCropFrameNone, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameNone()));
    YAE_ASSERT(ok);
    
    ok = connect(actionCropFrame1_33, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_33()));
    YAE_ASSERT(ok);
    
    ok = connect(actionCropFrame1_78, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_78()));
    YAE_ASSERT(ok);
    
    ok = connect(actionCropFrame1_85, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_85()));
    YAE_ASSERT(ok);
    
    ok = connect(actionCropFrame2_35, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_35()));
    YAE_ASSERT(ok);
    
    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);
    
    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);
    
    ok = connect(actionShrinkWrap, SIGNAL(triggered()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);
    
    ok = connect(actionShowTimeline, SIGNAL(triggered()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);
    
    ok = connect(shortcutShowTimeline_, SIGNAL(activated()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);
    
    ok = connect(actionColorConverter, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);
    
    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(canvas_, SIGNAL(toggleFullScreen()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setInPoint()),
                 timelineControls_, SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setOutPoint()),
                 timelineControls_, SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(userIsSeeking(bool)),
                 this, SLOT(userIsSeeking(bool)));
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
    
    // update the UI:
    delete audioTrackGroup_;
    audioTrackGroup_ = new QActionGroup(this);
    
    delete videoTrackGroup_;
    videoTrackGroup_ = new QActionGroup(this);
    
    delete audioTrackMapper_;
    audioTrackMapper_ = new QSignalMapper(this);
    
    bool ok = connect(audioTrackMapper_, SIGNAL(mapped(int)),
                      this, SLOT(audioSelectTrack(int)));
    YAE_ASSERT(ok);
    
    delete videoTrackMapper_;
    videoTrackMapper_ = new QSignalMapper(this);
    
    ok = connect(videoTrackMapper_, SIGNAL(mapped(int)),
                 this, SLOT(videoSelectTrack(int)));
    YAE_ASSERT(ok);
    
    menuAudio->clear();
    menuVideo->clear();
    
    for (unsigned int i = 0; i < numAudioTracks; i++)
    {
      reader->selectAudioTrack(i);
      QString trackName = tr("Track %1").arg(i);
        
      const char * name = reader->getSelectedAudioTrackName();
      if (name && *name)
      {
        trackName += tr(", %1").arg(QString::fromUtf8(name));
      }
      
      AudioTraits traits;
      if (reader->getAudioTraits(traits))
      {
        trackName +=
          tr(", %1 Hz, %2 channels").
          arg(traits.sampleRate_).
          arg(int(traits.channelLayout_));
      }

      QAction * trackAction = new QAction(trackName, this);
      menuAudio->addAction(trackAction);
      
      trackAction->setCheckable(true);
      trackAction->setChecked(i == 0);
      audioTrackGroup_->addAction(trackAction);
      
      ok = connect(trackAction, SIGNAL(triggered()),
                   audioTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable audio:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuAudio->addAction(trackAction);
      
      trackAction->setCheckable(true);
      trackAction->setChecked(numAudioTracks == 0);
      audioTrackGroup_->addAction(trackAction);
      
      ok = connect(trackAction, SIGNAL(triggered()),
                   audioTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioTrackMapper_->setMapping(trackAction, numAudioTracks);
    }
    
    for (unsigned int i = 0; i < numVideoTracks; i++)
    {
      reader->selectVideoTrack(i);
      QString trackName = tr("Track %1").arg(i);
        
      const char * name = reader->getSelectedVideoTrackName();
      if (name && *name)
      {
        trackName += tr(", %1").arg(QString::fromUtf8(name));
      }
      
      VideoTraits traits;
      if (reader->getVideoTraits(traits))
      {
        trackName +=
          tr(", %1 x %2, %3 fps").
          arg(traits.encodedWidth_).
          arg(traits.encodedHeight_).
          arg(traits.frameRate_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuVideo->addAction(trackAction);
      
      trackAction->setCheckable(true);
      trackAction->setChecked(i == 0);
      videoTrackGroup_->addAction(trackAction);
      
      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable video:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuVideo->addAction(trackAction);
      
      trackAction->setCheckable(true);
      trackAction->setChecked(numVideoTracks == 0);
      videoTrackGroup_->addAction(trackAction);
      
      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, numVideoTracks);
    }
    
    selectVideoTrack(reader, 0);
    selectAudioTrack(reader, 0);
    
    reader_->close();
    stopRenderers();
    
    timelineControls_->resetTimeInOut();
    
    reader->threadStart();
    startRenderers(reader);
    
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
  {
    canvas_->overrideDisplayAspectRatio(0.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_35
  // 
  void
  MainWindow::playbackAspectRatio2_35()
  {
    canvas_->overrideDisplayAspectRatio(2.35);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_85
  // 
  void
  MainWindow::playbackAspectRatio1_85()
  {
    canvas_->overrideDisplayAspectRatio(1.85);
    playbackShrinkWrap();
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_78
  // 
  void
  MainWindow::playbackAspectRatio1_78()
  {
    canvas_->overrideDisplayAspectRatio(16.0 / 9.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_33
  // 
  void
  MainWindow::playbackAspectRatio1_33()
  {
    canvas_->overrideDisplayAspectRatio(4.0 / 3.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameNone
  // 
  void
  MainWindow::playbackCropFrameNone()
  {
    canvas_->cropFrame(0.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_35
  // 
  void
  MainWindow::playbackCropFrame2_35()
  {
    canvas_->cropFrame(2.35);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_85
  // 
  void
  MainWindow::playbackCropFrame1_85()
  {
    canvas_->cropFrame(1.85);
    playbackShrinkWrap();
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_78
  // 
  void
  MainWindow::playbackCropFrame1_78()
  {
    canvas_->cropFrame(16.0 / 9.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_33
  // 
  void
  MainWindow::playbackCropFrame1_33()
  {
    canvas_->cropFrame(4.0 / 3.0);
    playbackShrinkWrap();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackColorConverter
  // 
  void
  MainWindow::playbackColorConverter()
  {
    std::cerr << "playbackColorConverter" << std::endl;
    reader_->threadStop();
    stopRenderers();
    
    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    selectVideoTrack(reader_, videoTrack);
    
    reader_->threadStart();
    startRenderers(reader_);
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackShowTimeline
  // 
  void
  MainWindow::playbackShowTimeline()
  {
    SignalBlocker blockSignals(actionShowTimeline);
    
    QRect mainGeom = geometry();
    int ctrlHeight = timelineControls_->height();
    bool fullScreen = this->isFullScreen();
    
    if (timelineControls_->isVisible())
    {
      actionShowTimeline->setChecked(false);
      timelineControls_->hide();
      
      if (!fullScreen)
      {
        resize(width(), mainGeom.height() - ctrlHeight);
      }
    }
    else
    {
      actionShowTimeline->setChecked(true);

      if (!fullScreen)
      {
        resize(width(), mainGeom.height() + ctrlHeight);
      }
      
      timelineControls_->show();
    }
  }
  
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
    
    int dx = vw - cw;
    int dy = vh - ch;
    
    // calculate width and height overhead:
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
    QRect rectClient = geometry();
    int cdx = rectWindow.width() - rectClient.width();
    int cdy = rectWindow.height() - rectClient.height();
    
    resize(new_w - cdx, new_h - cdy);
    move(new_x, new_y);
  }
  
  //----------------------------------------------------------------
  // swapShortcuts
  // 
  static inline void
  swapShortcuts(QShortcut * a, QAction * b)
  {
    QKeySequence tmp = a->key();
    a->setKey(b->shortcut());
    b->setShortcut(tmp);
  }
  
  //----------------------------------------------------------------
  // MainWindow::playbackFullScreen
  // 
  void
  MainWindow::playbackFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
      return;
    }

    // enter full screen rendering:
    SignalBlocker blockSignals(actionFullScreen);
    
    actionFullScreen->setChecked(true);
    actionShrinkWrap->setEnabled(false);
    menuBar()->hide();
    showFullScreen();
    
    swapShortcuts(shortcutExit_, actionExit);
    swapShortcuts(shortcutFullScreen_, actionFullScreen);
    swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
  }
  
  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  // 
  void
  MainWindow::exitFullScreen()
  {
    if (isFullScreen())
    {
      // exit full screen rendering:
      SignalBlocker blockSignals(actionFullScreen);
      
      actionFullScreen->setChecked(false);
      actionShrinkWrap->setEnabled(true);
      menuBar()->show();
      showNormal();
      
      swapShortcuts(shortcutExit_, actionExit);
      swapShortcuts(shortcutFullScreen_, actionFullScreen);
      swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
    }
  }
  
  //----------------------------------------------------------------
  // MainWindow::togglePlayback
  // 
  void
  MainWindow::togglePlayback()
  {
    std::cerr << "togglePlayback" << std::endl;
    
    if (playbackPaused_)
    {
      // reader_->threadResume();
      startRenderers(reader_);
    }
    else
    {
      // reader_->threadPause();
      stopRenderers();
    }
    
    playbackPaused_ = !playbackPaused_;
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectTrack
  // 
  void
  MainWindow::audioSelectTrack(int index)
  {
    std::cerr << "audioSelectTrack: " << index << std::endl;
    reader_->threadStop();
    stopRenderers();
    selectAudioTrack(reader_, index);
    
    reader_->threadStart();
    startRenderers(reader_);
  }

  //----------------------------------------------------------------
  // MainWindow::videoSelectTrack
  // 
  void
  MainWindow::videoSelectTrack(int index)
  {
    std::cerr << "videoSelectTrack: " << index << std::endl;
    reader_->threadStop();
    stopRenderers();
    
    selectVideoTrack(reader_, index);
    
    reader_->threadStart();
    startRenderers(reader_);
  }
  
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
  // MainWindow::processDropEventUrls
  // 
  void
  MainWindow::processDropEventUrls(const QList<QUrl> & urls)
  {
    QString filename = urls.front().toLocalFile();
    load(filename);
  }
  
  //----------------------------------------------------------------
  // MainWindow::userIsSeeking
  // 
  void
  MainWindow::userIsSeeking(bool seeking)
  {
    if (seeking && !playbackInterrupted_)
    {
      playbackInterrupted_ = !playbackPaused_;
      togglePlayback();
    }
    else if (!seeking && playbackInterrupted_)
    {
      playbackInterrupted_ = false;
      togglePlayback();
    }
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
    processDropEventUrls(e->mimeData()->urls());
  }
  
  //----------------------------------------------------------------
  // MainWindow::keyPressEvent
  // 
  void
  MainWindow::keyPressEvent(QKeyEvent * event)
  {
    int key = event->key();
    if (key == Qt::Key_Space)
    {
      togglePlayback();
    }
    else if (key == Qt::Key_Escape)
    {
      exitFullScreen();
    }
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
    }
    else
    {
      event->ignore();
    }
  }
  
  //----------------------------------------------------------------
  // MainWindow::stopRenderers
  // 
  void
  MainWindow::stopRenderers()
  {
    videoRenderer_->close();
    audioRenderer_->close();
  }
  
  //----------------------------------------------------------------
  // MainWindow::startRenderers
  // 
  void
  MainWindow::startRenderers(IReader * reader)
  {
    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t audioTrack = reader->getSelectedAudioTrackIndex();
    
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    SharedClock sharedClock;
    sharedClock.setObserver(timelineControls_);
    
    if (audioTrack < numAudioTracks)
    {
      audioRenderer_->takeThisClock(sharedClock);
      audioRenderer_->obeyThisClock(audioRenderer_->clock());
      
      if (videoTrack < numVideoTracks)
      {
        videoRenderer_->obeyThisClock(audioRenderer_->clock());
      }
    }
    else if (videoTrack < numVideoTracks)
    {
      videoRenderer_->takeThisClock(sharedClock);
      videoRenderer_->obeyThisClock(videoRenderer_->clock());
    }
    else
    {
      // all tracks disabled!
      return;
    }
    
    // update the renderers:
    unsigned int audioDevice = audioRenderer_->getDefaultDeviceIndex();
    if (!audioRenderer_->open(audioDevice, reader))
    {
      videoRenderer_->takeThisClock(sharedClock);
      videoRenderer_->obeyThisClock(videoRenderer_->clock());
    }
    
    videoRenderer_->open(canvas_, reader);
    timelineControls_->reset(sharedClock, reader);
  }
  
  //----------------------------------------------------------------
  // MainWindow::selectVideoTrack
  // 
  void
  MainWindow::selectVideoTrack(IReader * reader, std::size_t videoTrackIndex)
  {
     reader->selectVideoTrack(videoTrackIndex);
     
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
         std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
         reader->selectVideoTrack(numVideoTracks);
       }
     }
  }
  
  //----------------------------------------------------------------
  // MainWindow::selectAudioTrack
  // 
  void
  MainWindow::selectAudioTrack(IReader * reader, std::size_t audioTrackIndex)
  {
     reader->selectAudioTrack(audioTrackIndex);
  }
  
};

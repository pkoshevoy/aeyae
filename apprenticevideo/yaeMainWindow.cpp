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
#include <yaeAudioRendererQt.h>
#ifdef YAE_HAS_PORTAUDIO
#include <yaeAudioRendererPortaudio.h>
#endif

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
    viewer_(NULL),
    audioRenderer_(NULL)
  {
    setupUi(this);
    setAcceptDrops(true);
    
    reader_ = ReaderFFMPEG::create();
    viewer_ = new Viewer(reader_);
    
#ifdef YAE_HAS_PORTAUDIO
    audioRenderer_ = AudioRendererPortaudio::create();
#else
    audioRenderer_ = AudioRendererQt::create();
#endif
    
    delete centralwidget->layout();
    QVBoxLayout * layout = new QVBoxLayout(centralwidget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(viewer_);

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
    delete viewer_;
    audioRenderer_->close();
    audioRenderer_->destroy();
    reader_->destroy();
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
    
    if (numVideoTracks)
    {
      reader->selectVideoTrack(0);
    }
    
    if (numAudioTracks)
    {
      reader->selectAudioTrack(0);
      
      // FIXME: this is a temporary workaround for blocking on full queue:
      // unselect audio track:
      // reader->selectAudioTrack(numAudioTracks);
    }
    
    reader->threadStart();
    
    // update the renderers:
    reader_->close();
    viewer_->setReader(reader);
    viewer_->loadFrame();
    audioRenderer_->open(audioRenderer_->getDefaultDeviceIndex(), reader);
    
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

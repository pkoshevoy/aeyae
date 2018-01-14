// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 13 15:53:35 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <list>
#include <math.h>

// boost includes:
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

// Qt includes:
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QProcess>
#include <QShortcut>
#include <QSpacerItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

// yae includes:
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video_renderer.h"

// local includes:
#include "yaeMainWindow.h"
#include "yaePortaudioRenderer.h"
#include "yaeTimelineModel.h"
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"
#include "apprenticevideo/yaeVersion.h"


namespace yae
{

  //----------------------------------------------------------------
  // AboutDialog::AboutDialog
  //
  AboutDialog::AboutDialog(QWidget * parent):
    QDialog(parent),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);

    textBrowser->setSearchPaths(QStringList() << ":/");
    textBrowser->setSource(QUrl("qrc:///yaeAbout.html"));
  }


  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow(const TDemuxerInterfacePtr & demuxer):
    QMainWindow(NULL, 0),
    demuxer_(demuxer),
    canvasWidget_(NULL),
    canvas_(NULL)
  {
    setupUi(this);
    setAcceptDrops(true);

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/apprenticevideo/images/apprenticevideo-256.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // setup the canvas widget (QML quick widget):
#ifdef __APPLE__
    QString clickOrTap = tr("click");
#else
    QString clickOrTap = tr("tap");
#endif

    QString greeting = tr("drop video files here");

#ifdef YAE_USE_QOPENGL_WIDGET
    canvasWidget_ = new TCanvasWidget(this);
    canvasWidget_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    canvasWidget_ = new TCanvasWidget(contextFormat, this, canvasWidget_);
#endif
    canvasWidget_->setGreeting(greeting);

#if 0
    canvasWidget_->append(&playlistView_);
    canvasWidget_->append(&timelineView_);
    canvasWidget_->append(&frameCropView_);
    playlistView_.setup(this);
    playlistView_.setModel(&playlistModel_);
    playlistView_.setEnabled(false);
    timelineView_.setup(this, &playlistView_);
    timelineView_.setModel(&timelineModel_);
    frameCropView_.setEnabled(false);

    // add image://thumbnails/... provider:
    boost::shared_ptr<ThumbnailProvider>
      imageProvider(new ThumbnailProvider(readerPrototype_, playlistModel_));
    playlistView_.addImageProvider(QString::fromUtf8("thumbnails"),
                                   imageProvider);
#endif

    canvasWidget_->setFocusPolicy(Qt::StrongFocus);
    canvasWidget_->setAcceptDrops(true);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvasWidget_);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;

    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);

    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    canvas_->cropAutoDetectStop();
    delete canvasWidget_;
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
  // MainWindow::initCanvasWidget
  //
  void
  MainWindow::initCanvasWidget()
  {
    // get a shortcut to the Canvas (owned by the QML canvas widget):
    canvas_ = canvasWidget_;
    YAE_ASSERT(canvas_);
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
#if 0
    // initialize frame crop view:
    TMakeCurrentContext currentContext(canvas_->context());
    frameCropView_.init(&playlistView_);

    CanvasRendererItem & rendererItem =
      frameCropView_.root()->get<CanvasRendererItem>("uncropped");

    onLoadFrame_.reset(new OnFrameLoaded(rendererItem));
    canvasWidget_->addLoadFrameObserver(onLoadFrame_);
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const std::list<QString> & playlist)
  {
  }

  //----------------------------------------------------------------
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path)
  {
    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpen
  //
  void
  MainWindow::fileOpen()
  {
    QString filter =
      tr("movies ("
         "*.avi "
         "*.asf "
         "*.divx "
         "*.eyetv "
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

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);

#ifndef __APPLE__
    QStringList filenames =
      QFileDialog::getOpenFileNames(this,
                                    tr("Select one or more files"),
                                    startHere,
                                    filter);
#else
    QFileDialog dialog(this,
                       tr("Select one or more files"),
                       startHere,
                       filter);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();
#endif

    if (filenames.empty())
    {
      return;
    }

    std::list<QString> playlist;
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
    {
      yae::addToPlaylist(playlist, *i);
    }

    setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // MainWindow::fileExit
  //
  void
  MainWindow::fileExit()
  {
    MainWindow::close();
    qApp->quit();
  }

  //----------------------------------------------------------------
  // MainWindow::helpAbout
  //
  void
  MainWindow::helpAbout()
  {
    static AboutDialog * about = NULL;
    if (!about)
    {
      about = new AboutDialog(this);
      about->setWindowTitle(tr("Apprentice Video Remux (%1)").
                            arg(QString::fromUtf8(YAE_REVISION)));
    }

    about->show();
  }

  //----------------------------------------------------------------
  // MainWindow::processDropEventUrls
  //
  void
  MainWindow::processDropEventUrls(const QList<QUrl> & urls)
  {
    std::list<QString> playlist;
    for (QList<QUrl>::const_iterator i = urls.begin(); i != urls.end(); ++i)
    {
      QUrl url = *i;

#ifdef __APPLE__
      if (url.toString().startsWith("file:///.file/id="))
      {
        std::string strUrl = url.toString().toUtf8().constData();
        strUrl = yae::absoluteUrlFrom(strUrl.c_str());
        url = QUrl::fromEncoded(QByteArray(strUrl.c_str()));
      }
#endif

      QString fullpath = QFileInfo(url.toLocalFile()).canonicalFilePath();
      if (!addToPlaylist(playlist, fullpath))
      {
        QString strUrl = url.toString();
        addToPlaylist(playlist, strUrl);
      }
    }

    setPlaylist(playlist);
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

}

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
#include "yaeReplay.h"
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
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    canvasWidget_(NULL),
    canvas_(NULL)
  {
    setupUi(this);
    setAcceptDrops(true);

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

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

    canvasWidget_->append(&view_);
    view_.setModel(&model_);
    view_.setEnabled(true);
    view_.layoutChanged();

    canvasWidget_->setFocusPolicy(Qt::StrongFocus);
    canvasWidget_->setAcceptDrops(true);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvasWidget_);

#if 1
    actionFullScreen->setShortcut(tr("Ctrl+F"));
#elif defined(__APPLE__)
    actionFullScreen->setShortcut(tr("Ctrl+Shift+F"));
#else
    actionFullScreen->setShortcut(tr("F11"));
#endif

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);

    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);

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

    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(enterFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 actionFullScreen, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(&(canvasWidget_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
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
  // MainWindow::add
  //
  void
  MainWindow::add(const std::set<std::string> & sources,
                  const std::list<ClipInfo> & src_clips)
  {
    typedef boost::shared_ptr<SerialDemuxer> TSerialDemuxerPtr;
    typedef boost::shared_ptr<ParallelDemuxer> TParallelDemuxerPtr;
    std::map<std::string, TParallelDemuxerPtr> parallel_demuxers;
    std::list<ClipInfo> clips = src_clips;

    // these are expressed in seconds:
    static const double buffer_duration = 1.0;
    static const double discont_tolerance = 0.017;

    for (std::set<std::string>::const_iterator
           i = sources.begin(); i != sources.end(); ++i)
    {
      const std::string & source = *i;

      std::list<TDemuxerPtr> demuxers;
      if (!open_primary_and_aux_demuxers(source, demuxers))
      {
        // failed to open the primary resource:
        av_log(NULL, AV_LOG_WARNING,
               "failed to open %s, skipping...",
               source.c_str());
        continue;
      }

      TParallelDemuxerPtr parallel_demuxer(new ParallelDemuxer());

      // wrap each demuxer in a DemuxerBuffer, build a summary:
      for (std::list<TDemuxerPtr>::const_iterator
             i = demuxers.begin(); i != demuxers.end(); ++i)
      {
        const TDemuxerPtr & demuxer = *i;

        TDemuxerInterfacePtr
          buffer(new DemuxerBuffer(demuxer, buffer_duration));

        buffer->update_summary(discont_tolerance);
        parallel_demuxer->append(buffer);
      }

      // summarize the demuxer:
      const DemuxerSummary & summary =
        parallel_demuxer->update_summary(discont_tolerance);

      parallel_demuxers[source] = parallel_demuxer;

      // update the model:
      model_.demuxer_[source] = parallel_demuxer;
      model_.source_[parallel_demuxer] = source;

#if 0
      // show the summary:
      std::cout << "\nparallel:\n" << summary << std::endl;
#endif

      if (src_clips.empty())
      {
        std::string track_id("v:000");
        if (yae::has(summary.decoders_, track_id))
        {
          const Timeline::Track & track = summary.get_track_timeline(track_id);
          clips.push_back(ClipInfo(source,
                                   track_id,
                                   track.pts_.front().to_hhmmss_ms(),
                                   track.pts_.back().to_hhmmss_ms()));
        }
      }
    }

    for (std::list<ClipInfo>::const_iterator
           i = clips.begin(); i != clips.end(); ++i)
    {
      const ClipInfo & trim = *i;

      std::string track_id =
        trim.track_.empty() ? std::string("v:000") : trim.track_;

      if (!al::starts_with(track_id, "v:"))
      {
        // not a video track:
        continue;
      }

      const TDemuxerInterfacePtr & demuxer =
        yae::at(model_.demuxer_, trim.source_);

      const DemuxerSummary & summary = demuxer->summary();

      if (!yae::has(summary.decoders_, track_id))
      {
        // no such track:
        continue;
      }

      const Timeline::Track & track = summary.get_track_timeline(track_id);
      Timespan keep(track.pts_.front(), track.pts_.back());

      const FramerateEstimator & fe = yae::at(summary.fps_, track_id);
      double fps = fe.best_guess();

      if (!trim.t0_.empty() &&
          !parse_time(keep.t0_, trim.t0_.c_str(), NULL, NULL, fps))
      {
        av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t0_.c_str());
      }

      if (!trim.t1_.empty() &&
          !parse_time(keep.t1_, trim.t1_.c_str(), NULL, NULL, fps))
      {
        av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t1_.c_str());
      }

      TClipPtr clip(new Clip(demuxer, track_id, keep));
      model_.clips_.push_back(clip);
      view_.append_clip(clip);
    }

    model_.selected_ = model_.clips_.empty() ? 0 : model_.clips_.size() - 1;
  }

  //----------------------------------------------------------------
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path)
  {
    std::set<std::string> sources;
    sources.insert(std::string(path.toUtf8().constData()));
    add(sources);
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

    std::set<std::string> sources;
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
    {
      const QString & source = *i;
      sources.insert(std::string(source.toUtf8().constData()));
    }

    add(sources);
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
  // MainWindow::requestToggleFullScreen
  //
  void
  MainWindow::requestToggleFullScreen()
  {
    // all this to work-around apparent QML bug where
    // toggling full-screen on double-click leaves Flickable in
    // a state where it never receives the button-up event
    // and ends up interpreting all mouse movement as dragging,
    // very annoying...
    //
    // The workaround is to delay fullscreen toggle to allow
    // Flickable time to receive the button-up event

    QTimer::singleShot(178, this, SLOT(toggleFullScreen()));
  }

  //----------------------------------------------------------------
  // MainWindow::toggleFullScreen
  //
  void
  MainWindow::toggleFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
    }
    else
    {
      enterFullScreen();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::enterFullScreen
  //
  void
  MainWindow::enterFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
      return;
    }

    SignalBlocker blockSignals;
    blockSignals << actionFullScreen;

    actionFullScreen->setChecked(true);
    canvas_->setRenderMode(Canvas::kScaleToFit);

    if (isFullScreen())
    {
      return;
    }

    // enter full screen rendering:
    menuBar()->hide();
    showFullScreen();

    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  //
  void
  MainWindow::exitFullScreen()
  {
    if (!isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals << actionFullScreen;

    actionFullScreen->setChecked(false);
    menuBar()->show();
    showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::processDropEventUrls
  //
  void
  MainWindow::processDropEventUrls(const QList<QUrl> & urls)
  {
    std::set<std::string> sources;
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
      sources.insert(std::string(fullpath.toUtf8().constData()));
    }

    add(sources);
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
  // MainWindow::swapShortcuts
  //
  void
  MainWindow::swapShortcuts()
  {
    yae::swapShortcuts(shortcutExit_, actionExit);
    yae::swapShortcuts(shortcutFullScreen_, actionFullScreen);
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
    if (key == Qt::Key_Escape)
    {
      if (isFullScreen())
      {
        exitFullScreen();
      }
    }
#if 0
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
    }
#endif
    else
    {
      QMainWindow::keyPressEvent(event);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::mousePressEvent
  //
  void
  MainWindow::mousePressEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::RightButton)
    {
      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      // populate the context menu:
      contextMenu_->clear();

      // contextMenu_->addAction(actionSetInPoint);
      // contextMenu_->addAction(actionSetOutPoint);
      // contextMenu_->addSeparator();
      contextMenu_->addAction(actionFullScreen);

      contextMenu_->popup(globalPt);
    }
  }

}

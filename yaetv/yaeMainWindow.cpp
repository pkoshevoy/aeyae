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

// jsoncpp:
#include "json/json.h"

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video_renderer.h"

// local:
#include "yaeMainWindow.h"
#include "yaePortaudioRenderer.h"
#include "yaeTimelineModel.h"
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // save_bookmark
  //
  static void
  save_bookmark(const DVR & dvr,
                const Recording & rec,
                const IReader * reader,
                double position_in_sec)
  {
    if (!reader)
    {
      return;
    }

    Json::Value seen;
    Json::Value & bookmark = seen["bookmark"];

    bookmark["sel_audio"] = Json::UInt64(reader->getSelectedAudioTrackIndex());
    bookmark["sel_video"] = Json::UInt64(reader->getSelectedVideoTrackIndex());

    Json::Value & sel_subtt = bookmark["sel_subtt"];
    sel_subtt = Json::Value(Json::arrayValue);

    Json::UInt64 nsubs = reader->subsCount();
    for (Json::UInt64 i = 0; i < nsubs; i++)
    {
      if (reader->getSubsRender(i))
      {
        sel_subtt.append(i);
      }
    }

    unsigned int cc = reader->getRenderCaptions();
    if (cc)
    {
      bookmark["cc"] = cc;
    }

    bookmark["position_in_sec"] = position_in_sec;

    std::string filepath = rec.get_filepath(dvr.basedir_.string(), ".seen");
    TOpenFile(filepath, "wb").save(seen);
  }

  //----------------------------------------------------------------
  // load_bookmark
  //
  yae::shared_ptr<IBookmark>
  load_bookmark(const DVR & dvr, const Recording & rec)
  {
    yae::shared_ptr<IBookmark> bookmark_ptr;
    std::string filepath = rec.get_filepath(dvr.basedir_.string(), ".seen");
    Json::Value seen;

    if (TOpenFile(filepath, "rb").load(seen))
    {
      const Json::Value & jv = seen["bookmark"];

      bookmark_ptr.reset(new IBookmark());
      IBookmark & bookmark = *bookmark_ptr;

      bookmark.atrack_ = jv.get("sel_audio", 0).asUInt64();
      bookmark.vtrack_ = jv.get("sel_video", 0).asUInt64();

      const Json::Value & sel_subtt = jv["sel_subtt"];
      for (Json::Value::const_iterator
             k = sel_subtt.begin(); k != sel_subtt.end(); ++k)
      {
        bookmark.subs_.push_back(std::size_t((*k).asUInt64()));
      }

      bookmark.cc_ = jv.get("cc", 0).asUInt();

      bookmark.positionInSeconds_ =
        jv.get("position_in_sec", 0.0).asDouble();
    }

    return bookmark_ptr;
  }


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
  // InitTuners
  //
  struct InitTuners : AsyncTaskQueue::Task
  {

    //----------------------------------------------------------------
    // InitTuners
    //
    InitTuners(QObject * target, yae::DVR & dvr):
      target_(target),
      dvr_(dvr)
    {}

    //----------------------------------------------------------------
    // Discover
    //
    struct Discover : public QEvent
    {
      Discover(): QEvent(QEvent::User) {}
    };

    //----------------------------------------------------------------
    // Initialize
    //
    struct Initialize : public QEvent
    {
      Initialize(const std::string & tuner_name):
        QEvent(QEvent::User),
        name_(tuner_name)
      {}

      std::string name_;
    };

    //----------------------------------------------------------------
    // Done
    //
    struct Done : public QEvent
    {
      Done(): QEvent(QEvent::User) {}
    };

    // virtual:
    void run()
    {
      try
      {
        qApp->postEvent(target_, new Discover());

        std::list<std::string> available_tuners;
        dvr_.hdhr_.discover_tuners(available_tuners);

        for (std::list<std::string>::const_iterator
               i = available_tuners.begin(); i != available_tuners.end(); ++i)
        {
          const std::string & tuner_name = *i;
          qApp->postEvent(target_, new Initialize(tuner_name));

          // NOTE: this can take a while if there aren't cached channel scan
          // resuts for this tuner:
          YAE_EXPECT(dvr_.hdhr_.init(tuner_name));
        }
      }
      catch (const std::exception & e)
      {
       yae_wlog("InitTuners::run exception: %s", e.what());
      }
      catch (...)
      {
       yae_wlog("InitTuners::run unknown exception");
      }

      qApp->postEvent(target_, new Done());
    }

  protected:
    QObject * target_;
    yae::DVR & dvr_;
  };


  //----------------------------------------------------------------
  // context_toggle_fullscreen
  //
  static void
  context_toggle_fullscreen(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->requestToggleFullScreen();
  }


  //----------------------------------------------------------------
  // context_query_fullscreen
  //
  static bool
  context_query_fullscreen(void * context, bool & fullscreen)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    fullscreen = mainWindow->isFullScreen();
    return true;
  }


  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow(const std::string & yaetv_dir,
                         const std::string & recordings_dir,
                         const IReaderPtr & reader_prototype):
    QMainWindow(NULL, 0),
    contextMenu_(NULL),
    shortcutExit_(NULL),
    playerWidget_(NULL),
    playerWindow_(this),
    readerPrototype_(reader_prototype),
    canvas_(NULL),
    dvr_(yaetv_dir, recordings_dir)
  {
    setupUi(this);
    setAcceptDrops(false);

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/images/yaetv-logo.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

#ifdef YAE_USE_QOPENGL_WIDGET
    canvas_ = new TCanvasWidget(this);
    canvas_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    canvas_ = new TCanvasWidget(contextFormat, this, canvas_);
#endif

    view_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    view_.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->setFocusPolicy(Qt::StrongFocus);
    canvas_->setAcceptDrops(true);

    // insert canvas widget into the main window layout:
    canvasContainer_->addWidget(canvas_);

    playerWidget_ = new PlayerWidget(this, canvas_);
    canvasContainer_->addWidget(playerWidget_);
    canvasContainer_->setCurrentWidget(canvas_);

    // shortcut:
    PlayerView & playerView = playerWidget_->view();
    playerView.insert_menus(IReaderPtr(), menuBar(), menuHelp->menuAction());

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutExit_->setContext(Qt::ApplicationShortcut);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating some shortcuts as a workaround.
    bool ok = true;

    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);

    ok = connect(shortcutExit_, SIGNAL(activated()),
                 actionExit, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&view_, SIGNAL(playback(TRecordingPtr)),
                 this, SLOT(playbackRecording(TRecordingPtr)));
    YAE_ASSERT(ok);

    ok = connect(&view_, SIGNAL(confirm_delete(TRecordingPtr)),
                 this, SLOT(confirmDelete(TRecordingPtr)));
    YAE_ASSERT(ok);

    ok = connect(&playerWindow_, SIGNAL(windowClosed()),
                 this, SLOT(playerWindowClosed()));
    YAE_ASSERT(ok);

    ok = connect(playerWidget_, SIGNAL(enteringFullScreen()),
                 this, SLOT(playerEnteringFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(playerWidget_, SIGNAL(exitingFullScreen()),
                 this, SLOT(playerExitingFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&playerView, SIGNAL(playback_finished()),
                 this, SLOT(playbackFinished()));
    YAE_ASSERT(ok);

    ok = connect(&playerView, SIGNAL(save_bookmark()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    canvas_->cropAutoDetectStop();
    delete canvas_;
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
    // add image://thumbnails/... provider:
    yae::shared_ptr<ThumbnailProvider, ImageProvider>
      image_provider(new ThumbnailProvider(readerPrototype_));
    view_.addImageProvider(QString::fromUtf8("thumbnails"), image_provider);

    canvas_->initializePrivateBackend();
    canvas_->setGreeting(tr("yaetv"));
    canvas_->append(&view_);

    view_.setModel(&dvr_);
    view_.setEnabled(true);
    view_.layoutChanged();

    // action confirmation view:
    confirm_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    confirm_.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->append(&confirm_);
    confirm_.setStyle(view_.style());
    confirm_.setEnabled(false);

    // spinner view:
    spinner_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    spinner_.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->append(&spinner_);
    spinner_.setStyle(view_.style());
    spinner_.setEnabled(false);

    TAsyncTaskPtr t(new InitTuners(this, dvr_));
    tasks_.push_back(t);
    async_.push_back(t);

    playerWidget_->initItemViews();
  }

  //----------------------------------------------------------------
  // MainWindow::fileExit
  //
  void
  MainWindow::fileExit()
  {
    dvr_.shutdown();

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
      about->setWindowTitle(tr("yaetv (%1)").
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

    canvas_->setRenderMode(Canvas::kScaleToFit);

    if (isFullScreen())
    {
      return;
    }

    // enter full screen rendering:
    playerEnteringFullScreen();
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
    playerExitingFullScreen();

    showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    this->swapShortcuts();
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
  }

  //----------------------------------------------------------------
  // MainWindow::playbackRecording
  //
  void
  MainWindow::playbackRecording(TRecordingPtr rec_ptr)
  {
    const Recording & rec = *rec_ptr;
    std::string path = rec.get_filepath(dvr_.basedir_.string());

    IReaderPtr reader = yae::openFile(readerPrototype_,
                                      QString::fromUtf8(path.c_str()));
    if (!reader)
    {
      return;
    }

    nowPlaying_ = rec_ptr;

    std::string time_str = yae::unix_epoch_time_to_localdate(rec.utc_t0_);
    std::string title = strfmt("%i-%i %s, %s",
                               rec.channel_major_,
                               rec.channel_minor_,
                               rec.title_.c_str(),
                               time_str.c_str());
    playerWindow_.setWindowTitle(QString::fromUtf8(title.c_str()));

    PlayerView & playerView = playerWidget_->view();
    playerView.insert_menus(reader, menuBar(), menuHelp->menuAction());

    if (window()->isFullScreen())
    {
      canvasContainer_->addWidget(playerWidget_);
      canvasContainer_->setCurrentWidget(playerWidget_);
    }
    else
    {
      canvasContainer_->setCurrentWidget(canvas_);
      playerWindow_.show();
      playerWindow_.containerLayout_->addWidget(playerWidget_);
    }

    playerWidget_->show();

    yae::shared_ptr<IBookmark> bookmark = load_bookmark(dvr_, rec);
    playerWindow_.playback(playerWidget_, reader, bookmark.get());
  }

  //----------------------------------------------------------------
  // ConfirmDeleteRecording
  //
  struct ConfirmDeleteRecording : ConfirmView::Action
  {
    ConfirmDeleteRecording(MainWindow & mainWindow, const TRecordingPtr & rec):
      mainWindow_(mainWindow),
      rec_(rec)
    {}

    // virtual:
    void operator()() const
    {
      if (rec_ == mainWindow_.nowPlaying_)
      {
        mainWindow_.playerWindow_.stopAndHide();
        mainWindow_.nowPlaying_.reset();
      }

      const Recording & rec = *rec_;
      DVR & dvr = mainWindow_.dvr_;
      dvr.delete_recording(rec);

      AppView & appView = mainWindow_.view_;
      appView.sync_ui();
      appView.requestRepaint();
    }

    MainWindow & mainWindow_;
    TRecordingPtr rec_;
  };

  //----------------------------------------------------------------
  // MainWindow::confirmDelete
  //
  void
  MainWindow::confirmDelete(TRecordingPtr rec_ptr)
  {
    const Recording & rec = *rec_ptr;
    const AppStyle & style = *(view_.style());

    std::string msg = strfmt("Delete %s?", rec.get_basename().c_str());
    confirm_.message_ = TVarRef::constant(TVar(msg));
    confirm_.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm_.fg_ = style.bg_;

    confirm_.affirmative_.reset(new ConfirmDeleteRecording(*this, rec_ptr));
    ConfirmView::Action & aff = *confirm_.affirmative_;
    aff.message_ = TVarRef::constant(TVar("Delete"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.fg_;

    confirm_.negative_.reset(new ConfirmView::Action());
    ConfirmView::Action & neg = *confirm_.negative_;
    neg.message_ = TVarRef::constant(TVar("Cancel"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm_.setEnabled(true);
  }

  //----------------------------------------------------------------
  // DeclineDeleteRecording
  //
  struct DeclineDeleteRecording : ConfirmView::Action
  {
    DeclineDeleteRecording(MainWindow & mainWindow, const TRecordingPtr & rec):
      mainWindow_(mainWindow),
      rec_(rec)
    {}

    // virtual:
    void operator()() const
    {
      if (rec_ == mainWindow_.nowPlaying_)
      {
        mainWindow_.playerWindow_.stopAndHide();
        mainWindow_.nowPlaying_.reset();
      }

      AppView & appView = mainWindow_.view_;
      appView.sync_ui();
      appView.requestRepaint();
    }

    MainWindow & mainWindow_;
    TRecordingPtr rec_;
  };

  //----------------------------------------------------------------
  // MainWindow::playbackFinished
  //
  void
  MainWindow::playbackFinished()
  {
    TRecordingPtr rec_ptr = nowPlaying_;
    const Recording & rec = *rec_ptr;

    PlayerView & view = playerWidget_->view();
    const IReader * reader = view.get_reader();
    const TimelineModel & timeline = view.timeline_model();
    save_bookmark(dvr_, rec, reader, timeline.timelineStart());

    // shortcuts:
    const AppStyle & style = *(view_.style());
    ConfirmView & confirm = playerWidget_->confirm_;

    std::string msg = strfmt("Delete %s?", rec.get_basename().c_str());
    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.reset(new ConfirmDeleteRecording(*this, rec_ptr));
    ConfirmView::Action & aff = *confirm.affirmative_;
    aff.message_ = TVarRef::constant(TVar("Delete"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.fg_;

    confirm.negative_.reset(new DeclineDeleteRecording(*this, rec_ptr));
    ConfirmView::Action & neg = *confirm.negative_;
    neg.message_ = TVarRef::constant(TVar("Close"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    view.setEnabled(false);
    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::saveBookmark
  //
  void
  MainWindow::saveBookmark()
  {
    if (!nowPlaying_)
    {
      return;
    }

    const Recording & rec = *nowPlaying_;
    const PlayerView & view = playerWidget_->view();
    const IReader * reader = view.get_reader();
    const TimelineModel & timeline = view.timeline_model();
    save_bookmark(dvr_, rec, reader, timeline.currentTime());
  }

  //----------------------------------------------------------------
  // MainWindow::playerWindowClosed
  //
  void
  MainWindow::playerWindowClosed()
  {
    canvasContainer_->setCurrentWidget(canvas_);
    nowPlaying_.reset();
  }

  //----------------------------------------------------------------
  // MainWindow::playerEnteringFullScreen
  //
  void
  MainWindow::playerEnteringFullScreen()
  {
    if (nowPlaying_)
    {
      playerWindow_.hide();
      canvasContainer_->addWidget(playerWidget_);
      canvasContainer_->setCurrentWidget(playerWidget_);
      playerWidget_->show();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playerExitingFullScreen
  //
  void
  MainWindow::playerExitingFullScreen()
  {
    if (nowPlaying_)
    {
      QApplication::processEvents();

      canvasContainer_->setCurrentWidget(canvas_);
      playerWindow_.show();
      playerWindow_.containerLayout_->addWidget(playerWidget_);
      playerWidget_->show();
      playerWindow_.raise();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::event
  //
  bool
  MainWindow::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      InitTuners::Discover * discover_tuners =
        dynamic_cast<InitTuners::Discover *>(e);
      if (discover_tuners)
      {
        const AppStyle & style = *(view_.style());
        spinner_.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
        spinner_.fg_ = style.bg_;
        spinner_.text_color_ = style.bg_;

        spinner_.setEnabled(true);
        spinner_.setText(tr("looking for available tuners"));
        discover_tuners->accept();
        return true;
      }

      InitTuners::Initialize * initialize_tuner =
        dynamic_cast<InitTuners::Initialize *>(e);
      if (initialize_tuner)
      {
        const std::string & tuner_name = initialize_tuner->name_;
        spinner_.setText(tr("initializing tuner channel list: %1").
                         arg(QString::fromUtf8(tuner_name.c_str())));
        initialize_tuner->accept();
        return true;
      }

      InitTuners::Done * done =
        dynamic_cast<InitTuners::Done *>(e);
      if (done)
      {
        if (!dvr_.service_loop_worker_)
        {
          dvr_.service_loop_worker_.reset(new yae::Worker());
        }

        TWorkerPtr service_loop_worker_ptr = dvr_.service_loop_worker_;
        Worker & service_loop_worker = *service_loop_worker_ptr;

        if (service_loop_worker.is_idle())
        {
          yae::shared_ptr<DVR::ServiceLoop, yae::Worker::Task> task;
          task.reset(new DVR::ServiceLoop(dvr_));
          DVR::ServiceLoop & service_loop = *task;
          service_loop_worker.add(task);
        }

        tasks_.pop_front();
        spinner_.setEnabled(!tasks_.empty());
        done->accept();
        return true;
      }
    }

    return QMainWindow::event(e);
  }

  //----------------------------------------------------------------
  // MainWindow::changeEvent
  //
  void
  MainWindow::changeEvent(QEvent * event)
  {
    if (event->type() == QEvent::WindowStateChange)
    {
      if (isFullScreen())
      {
        menuBar()->hide();
      }
      else
      {
        menuBar()->show();
      }
    }

    event->ignore();
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
#if 0
    if (e->button() == Qt::RightButton)
    {
      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      // populate the context menu:
      contextMenu_->clear();

      std::size_t items = 0;
      if (items)
      {
        contextMenu_->addSeparator();
      }

      contextMenu_->popup(globalPt);
    }
#endif
  }

}

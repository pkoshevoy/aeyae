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
#include <QCheckBox>
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
  // PreferencesDialog::PreferencesDialog
  //
  PreferencesDialog::PreferencesDialog(QWidget * parent):
    QDialog(parent),
    Ui::PreferencesDialog(),
    tuners_layout_(NULL),
    dvr_(NULL)
  {
    Ui::PreferencesDialog::setupUi(this);
    tuners_layout_ = new QVBoxLayout(this->tunersScrollAreaContents);

    bool ok = connect(this->storageToolButton, SIGNAL(clicked(bool)),
                      this, SLOT(select_storage_folder()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(finished(int)),
                 this, SLOT(on_finished(int)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PreferencesDialog::~PreferencesDialog
  //
  PreferencesDialog::~PreferencesDialog()
  {
    delete tuners_layout_;
    tuners_layout_ = NULL;
  }

  //----------------------------------------------------------------
  // PreferencesDialog::init
  //
  void
  PreferencesDialog::init(DVR & dvr)
  {
    dvr.get_preferences(preferences_);

    std::string channelmap =
      preferences_.get("channelmap", "us-bcast").asString();

    QString channelmap_qstr = QString::fromUtf8(channelmap.c_str());
    for (int i = 0, n = this->channelMapComboBox->count(); i < n; i++)
    {
      if (this->channelMapComboBox->itemText(i) == channelmap_qstr)
      {
        this->channelMapComboBox->setCurrentIndex(i);
        break;
      }
    }

    // storage:
    std::string basedir =
      preferences_.get("basedir", std::string()).asString();

    if (basedir.empty())
    {
      QString movies = YAE_STANDARD_LOCATION(MoviesLocation);
      basedir = (fs::path(movies.toUtf8().constData()) / "yaetv").string();
    }

    QString basedir_qstr = QString::fromUtf8(basedir.c_str());
    basedir_qstr = QDir::toNativeSeparators(basedir_qstr);
    this->storageLineEdit->setText(basedir_qstr);

    // tuners:
    Json::Value tuners = preferences_.
      get("tuners", Json::Value(Json::objectValue));

    std::list<TunerDevicePtr> devices;
    dvr.hdhr_.discover_devices(devices);

    // clear any prior items:
    tuners_.clear();

    QLayoutItem * layout_item = NULL;
    while ((layout_item = tuners_layout_->takeAt(0)) != 0)
    {
      QWidget * widget = layout_item->widget();
      delete layout_item;
      delete widget;
    }

    for (std::list<TunerDevicePtr>::const_iterator
           i = devices.begin(); i != devices.end(); ++i)
    {
      const TunerDevice & device = *(*(i));
      for (int j = 0, num_tuners = device.num_tuners(); j < num_tuners; j++)
      {
        std::string tuner_name = device.tuner_name(j);
        QCheckBox * cb = new QCheckBox(QString::fromUtf8(tuner_name.c_str()));

        bool enabled = tuners.get(tuner_name, true).asBool();
        cb->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);

        tuners_layout_->addWidget(cb);
        tuners_[tuner_name] = cb;
      }
    }

    tuners_layout_->addSpacerItem(new QSpacerItem(0, // width
                                                  0, // height
                                                  QSizePolicy::Minimum,
                                                  QSizePolicy::Expanding));

    dvr_ = &dvr;
    QDialog::show();
  }

  //----------------------------------------------------------------
  // PreferencesDialog::on_finished
  //
  void
  PreferencesDialog::on_finished(int result)
  {
    if (result != QDialog::Accepted)
    {
      return;
    }

    QString basedir_qstr = this->storageLineEdit->text();
    basedir_qstr = QDir::toNativeSeparators(basedir_qstr);

    preferences_["basedir"] =
      fs::path(basedir_qstr.toUtf8().constData()).string();

    preferences_["channelmap"] =
      this->channelMapComboBox->currentText().toUtf8().constData();

    Json::Value & tuners = preferences_["tuners"];
    for (std::map<std::string, QCheckBox *>::const_iterator
           i = tuners_.begin(); i != tuners_.end(); ++i)
    {
      const std::string & tuner_name = i->first;
      QCheckBox * cb = i->second;
      tuners[tuner_name] = (cb->checkState() == Qt::Checked);
    }

    // update the preferences:
    dvr_->set_preferences(preferences_);
  }

  //----------------------------------------------------------------
  // PreferencesDialog::select_storage_folder
  //
  void
  PreferencesDialog::select_storage_folder()
  {
    QString folder =
      QFileDialog::getExistingDirectory(this,
                                        tr("Select DVR storage folder"),
                                        this->storageLineEdit->text(),
                                        QFileDialog::ShowDirsOnly |
                                        QFileDialog::DontResolveSymlinks);
    if (!folder.isEmpty())
    {
      folder = QDir::toNativeSeparators(folder);
      this->storageLineEdit->setText(folder);
    }
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
  // context_toggle_fullscreen
  //
  static void
  context_toggle_fullscreen(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->playerWidget_->requestToggleFullScreen();
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
    preferencesDialog_(this),
    playerWidget_(NULL),
    playerWindow_(this),
    readerPrototype_(reader_prototype),
    canvas_(NULL),
    dvr_(yaetv_dir, recordings_dir),
    start_live_playback_(this)
  {
    setupUi(this);
    setAcceptDrops(false);

    start_live_playback_.setInterval(1000);

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
    canvas_->setRenderMode(Canvas::kScaleToFit);

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

    ok = connect(actionPreferences, SIGNAL(triggered()),
                 this, SLOT(editPreferences()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(toggleFullScreen()),
                 playerWidget_, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(playerWidget_->canvas_->sigs_), SIGNAL(maybeHideCursor()),
                 &(playerWidget_->canvas_->sigs_), SLOT(hideCursor()));
    YAE_ASSERT(ok);

    ok = connect(&view_, SIGNAL(watch_live(uint32_t, TTime)),
                 this, SLOT(watchLive(uint32_t, TTime)));
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

    ok = connect(&playerView, SIGNAL(playback_finished(TTime)),
                 this, SLOT(playbackFinished(TTime)));
    YAE_ASSERT(ok);

    ok = connect(&playerView, SIGNAL(save_bookmark()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);

    ok = connect(&playerView, SIGNAL(on_back_arrow()),
                 this, SLOT(backToPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(&playerView, SIGNAL(delete_playing_file()),
                 this, SLOT(confirmDeletePlayingRecording()));
    YAE_ASSERT(ok);

    ok = connect(&start_live_playback_, SIGNAL(timeout()),
                 this, SLOT(startLivePlayback()));
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

    playerWidget_->initItemViews();

    if (!dvr_.has_preferences())
    {
      preferencesDialog_.init(dvr_);
      preferencesDialog_.exec();
    }
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
  // MainWindow::editPreferences
  //
  void
  MainWindow::editPreferences()
  {
    preferencesDialog_.init(dvr_);
    preferencesDialog_.show();
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
  // MainWindow::watchLive
  //
  void
  MainWindow::watchLive(uint32_t ch_num, TTime seekPos)
  {
    view_.now_playing_.reset();
    dvr_.watch_live(ch_num);

    // open the player window
    uint16_t major = yae::mpeg_ts::channel_major(ch_num);
    uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);

    std::string title = strfmt("%i-%i, live",
                               major,
                               minor);

    playerWindow_.setWindowTitle(QString::fromUtf8(title.c_str()));

    PlayerView & playerView = playerWidget_->view();
    playerView.insert_menus(IReaderPtr(), menuBar(), menuHelp->menuAction());

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

    // start a timer to attempt to play the live recording:
    start_live_seek_pos_ = seekPos;
    start_live_playback_.start();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackRecording
  //
  IReaderPtr
  MainWindow::playbackRecording(TRecordingPtr rec_ptr)
  {
    if (!rec_ptr)
    {
      return IReaderPtr();
    }

    const Recording & rec = *rec_ptr;
    std::string path = rec.get_filepath(dvr_.basedir_.string());

    IReaderPtr reader = yae::openFile(readerPrototype_,
                                      QString::fromUtf8(path.c_str()));
    if (!reader)
    {
      return IReaderPtr();
    }

    // check if we are switching away from a live channel playback,
    // in which case we should stop live channel recording:
    if (live_rec_ &&
        live_rec_ != rec_ptr &&
        live_rec_->get_basename() != rec.get_basename())
    {
      stopLivePlayback();
    }

    bool show_hour = true;
    bool show_ampm = true;
    std::string time_str = yae::unix_epoch_time_to_localdate(rec.utc_t0_,
                                                             show_hour,
                                                             show_ampm);
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

    return reader;
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
      // shortcuts:
      DVR & dvr = mainWindow_.dvr_;
      AppView & appView = mainWindow_.view_;
      const Recording & rec = *rec_;

      if (appView.now_playing_)
      {
        std::string filename = rec.get_basename() + ".mpg";

        if (appView.now_playing_->filename_ == filename)
        {
          mainWindow_.playerWindow_.stopAndHide();
          appView.now_playing_.reset();
        }
      }

      dvr.delete_recording(rec);
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
      // shortcuts:
      AppView & appView = mainWindow_.view_;
      const Recording & rec = *rec_;

      if (appView.now_playing_)
      {
        std::string filename = rec.get_basename() + ".mpg";

        if (appView.now_playing_->filename_ == filename)
        {
          mainWindow_.playerWindow_.stopAndHide();
          mainWindow_.view_.now_playing_.reset();
        }
      }

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
  MainWindow::playbackFinished(TTime playheadPos)
  {
    TRecordingPtr rec_ptr = view_.now_playing();
    if (live_rec_ || !rec_ptr)
    {
      uint32_t live_ch = dvr_.schedule_.get_live_channel();
      if (live_ch)
      {
        watchLive(live_ch, playheadPos);
      }

      return;
    }

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
  // CancelDeleteRecording
  //
  struct CancelDeleteRecording : ConfirmView::Action
  {
    CancelDeleteRecording(MainWindow & mainWindow, const TRecordingPtr & rec):
      mainWindow_(mainWindow),
      rec_(rec)
    {}

    // virtual:
    void operator()() const
    {
      PlayerView & view = mainWindow_.playerWidget_->view();
      ConfirmView & confirm = mainWindow_.playerWidget_->confirm_;
      view.setEnabled(true);
      confirm.setEnabled(false);
    }

    MainWindow & mainWindow_;
    TRecordingPtr rec_;
  };

  //----------------------------------------------------------------
  // MainWindow::confirmDeletePlayingRecording
  //
  void
  MainWindow::confirmDeletePlayingRecording()
  {
    TRecordingPtr rec_ptr = view_.now_playing();
    if (!rec_ptr)
    {
      return;
    }

    const Recording & rec = *rec_ptr;
    PlayerView & view = playerWidget_->view();

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

    confirm.negative_.reset(new CancelDeleteRecording(*this, rec_ptr));
    ConfirmView::Action & neg = *confirm.negative_;
    neg.message_ = TVarRef::constant(TVar("Cancel"));
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
    TRecordingPtr now_playing = view_.now_playing();
    if (!now_playing)
    {
      return;
    }

    const Recording & rec = *now_playing;
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
    canvasContainer_->addWidget(playerWidget_);
    canvasContainer_->setCurrentWidget(canvas_);
    view_.now_playing_.reset();
    stopLivePlayback();
  }

  //----------------------------------------------------------------
  // MainWindow::playerEnteringFullScreen
  //
  void
  MainWindow::playerEnteringFullScreen()
  {
    if (view_.now_playing_)
    {
      playerWindow_.hide();
      canvasContainer_->addWidget(playerWidget_);
      canvasContainer_->setCurrentWidget(playerWidget_);
      playerWidget_->show();
    }
    else
    {
      canvasContainer_->setCurrentWidget(canvas_);
    }

    // this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::playerExitingFullScreen
  //
  void
  MainWindow::playerExitingFullScreen()
  {
    if (view_.now_playing_)
    {
      QApplication::processEvents();

      canvasContainer_->setCurrentWidget(canvas_);
      playerWindow_.show();
      playerWindow_.containerLayout_->addWidget(playerWidget_);
      playerWidget_->show();
      playerWindow_.raise();
    }
    else
    {
      canvasContainer_->setCurrentWidget(canvas_);
    }

    // this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::backToPlaylist
  //
  void
  MainWindow::backToPlaylist()
  {
    playerWindow_.stopAndHide();
  }

  //----------------------------------------------------------------
  // MainWindow::startLivePlayback
  //
  void
  MainWindow::startLivePlayback()
  {
    uint32_t live_ch = dvr_.schedule_.get_live_channel();
    uint64_t t_gps = unix_epoch_time_to_gps_time(start_live_seek_pos_.get(1));
    live_rec_ = yae::next(live_rec_, view_.rec_by_channel_, live_ch, t_gps);

    IReaderPtr reader = playbackRecording(live_rec_);
    if (reader)
    {
      start_live_playback_.stop();

      TTime t0(0, 0);
      TTime t1(0, 0);
      if (yae::get_timeline(reader.get(), t0, t1))
      {
        double seek_pos =
          start_live_seek_pos_.valid() ?
          std::min(start_live_seek_pos_.sec(), t1.sec() - 10.0) :
          t1.sec() - 10.0;
        seek_pos = std::max(t0.sec(), seek_pos);

        reader->seek(seek_pos);
      }

      const Recording & rec = *live_rec_;
      std::string basepath = rec.get_filepath(dvr_.basedir_, "");
      std::string filename = rec.get_basename() + ".mpg";
      view_.now_playing_.reset(new AppView::Playback(view_.sidebar_sel_,
                                                     filename,
                                                     basepath));
    }
  }

  //----------------------------------------------------------------
  // MainWindow::stopLivePlayback
  //
  void
  MainWindow::stopLivePlayback()
  {
    start_live_playback_.stop();
    dvr_.close_live();
    live_rec_.reset();
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
        playerWidget_->exitFullScreen();
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
    if (e->button() == Qt::RightButton)
    {
      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      PlayerView & playerView = playerWidget_->view();
      playerView.populateContextMenu();
      playerView.contextMenu_->popup(globalPt);
      e->accept();
      return;
    }

    QWidget::mousePressEvent(e);
  }

}

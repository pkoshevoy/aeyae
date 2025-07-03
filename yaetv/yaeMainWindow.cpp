// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 13 15:53:35 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/ffmpeg/yae_live_reader.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video_renderer.h"

// standard:
#include <iostream>
#include <sstream>
#include <list>
#include <math.h>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// Qt:
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
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
                const Recording::Rec & rec,
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

    std::string filepath = rec.get_filepath(dvr.basedir_, ".seen");
    yae::atomic_save(filepath, seen);
  }

  //----------------------------------------------------------------
  // load_bookmark
  //
  yae::shared_ptr<IBookmark>
  load_bookmark(const DVR & dvr, const Recording::Rec & rec)
  {
    yae::shared_ptr<IBookmark> bookmark_ptr;
    std::string filepath = rec.get_filepath(dvr.basedir_, ".seen");
    Json::Value seen;

    if (yae::attempt_load(filepath, seen))
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

    ok = connect(this->radioButtonAuto, SIGNAL(clicked(bool)),
                 this, SLOT(appearance_changed()));
    YAE_ASSERT(ok);

    ok = connect(this->radioButtonDark, SIGNAL(clicked(bool)),
                 this, SLOT(appearance_changed()));
    YAE_ASSERT(ok);

    ok = connect(this->radioButtonLight, SIGNAL(clicked(bool)),
                 this, SLOT(appearance_changed()));
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

    // appearance:
    std::string appearance =
      preferences_.get("appearance", std::string("auto")).asString();

    if (appearance == "dark")
    {
      radioButtonDark->setChecked(true);
    }
    else if (appearance == "light")
    {
      radioButtonLight->setChecked(true);
    }
    else
    {
      radioButtonAuto->setChecked(true);
    }

    // storage:
    std::string basedir =
      preferences_.get("basedir", std::string()).asString();

    if (basedir.empty())
    {
      QString movies = YAE_STANDARD_LOCATION(MoviesLocation);
      basedir = (fs::path(movies.toUtf8().constData()) / "yaetv").string();
    }

    bool allow_recording = preferences_.get("allow_recording", true).asBool();
    this->allowRecordingCheckBox->setChecked(allow_recording);

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
      // restore appearance:
      std::string appearance =
        preferences_.get("appearance", std::string("auto")).asString();

      yae::Application & app = yae::Application::singleton();
      app.set_appearance(appearance);

      return;
    }

    if (radioButtonDark->isChecked())
    {
      preferences_["appearance"] = "dark";
    }
    else if (radioButtonLight->isChecked())
    {
      preferences_["appearance"] = "light";
    }
    else
    {
      preferences_["appearance"] = "auto";
    }

    QString basedir_qstr = this->storageLineEdit->text();
    basedir_qstr = QDir::toNativeSeparators(basedir_qstr);

    preferences_["basedir"] =
      fs::path(basedir_qstr.toUtf8().constData()).string();

    preferences_["allow_recording"] =
      this->allowRecordingCheckBox->isChecked();

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
  // PreferencesDialog::appearance_changed
  //
  void
  PreferencesDialog::appearance_changed()
  {
    yae::Application & app = yae::Application::singleton();
    if (radioButtonDark->isChecked())
    {
      app.set_appearance("dark");
    }
    else if (radioButtonLight->isChecked())
    {
      app.set_appearance("light");
    }
    else
    {
      app.set_appearance("auto");
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
  // LiveReaderFactory
  //
  struct LiveReaderFactory : ReaderFactory
  {
    // virtual:
    IReaderPtr create(const std::string &) const
    {
      return IReaderPtr(LiveReader::create());
    }
  };


  //----------------------------------------------------------------
  // DismissConfirmView
  //
  struct DismissConfirmView : ConfirmItem::Action
  {
    DismissConfirmView(ConfirmView & confirm):
      confirm_(confirm)
    {}

    // virtual:
    void execute() const
    {
      confirm_.setEnabled(false);
    }

    ConfirmView & confirm_;
  };


  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow(const std::string & yaetv_dir,
                         const std::string & recordings_dir):
    popup_(NULL),
    shortcutExit_(NULL),
    preferencesDialog_(NULL),
    playerWidget_(NULL),
    playerWindow_(NULL),
    readerFactory_(new LiveReaderFactory),
    canvas_(NULL),
    dvr_(yaetv_dir, recordings_dir),
    view_(new AppView("MainWindow app view")),
    confirm_(new ConfirmView("MainWindow confirm view")),
    spinner_(new SpinnerView("MainWindow spinner view")),
    start_live_playback_(this)
  {
    setupUi(this);
    setAcceptDrops(false);

    playerWindow_.menubar->addAction(menuFile->menuAction());
    playerWindow_.menubar->addAction(menuHelp->menuAction());

    start_live_playback_.setInterval(1000);

    popup_ = add_menu("contextMenu");

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/images/yaetv-logo.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

#ifndef YAE_USE_QGL_WIDGET
    canvas_ = new TCanvasWidget(this);
    canvas_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    canvas_ = new TCanvasWidget(contextFormat, this, canvas_);
#endif
    canvas_->setObjectName(tr("app view canvas"));

    AppView & app_view = *view_;
    app_view.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    app_view.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->setFocusPolicy(Qt::StrongFocus);
    canvas_->setAcceptDrops(false);
    canvas_->setRenderMode(Canvas::kScaleToFit);

    // insert canvas widget into the main window layout:
    canvasContainer_->addWidget(canvas_);

    playerWidget_ = new PlayerWidget(this, canvas_);
    playerWidget_->greeting_ = tr("loading");
    canvasContainer_->addWidget(playerWidget_);
    canvasContainer_->setCurrentWidget(canvas_);

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

    ok = connect(&(canvas_->sigs_), SIGNAL(escShort()),
                 playerWidget_, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(escLong()),
                 playerWidget_, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(view_.get(), SIGNAL(watch_live(uint32_t, TTime)),
                 this, SLOT(watchLive(uint32_t, TTime)));
    YAE_ASSERT(ok);

    ok = connect(view_.get(), SIGNAL(block_channel(uint32_t)),
                 this, SLOT(confirmBlockChannel(uint32_t)));
    YAE_ASSERT(ok);

    ok = connect(view_.get(), SIGNAL(playback(TRecPtr)),
                 this, SLOT(playbackRecording(TRecPtr)));
    YAE_ASSERT(ok);

    ok = connect(view_.get(), SIGNAL(confirm_delete(TRecPtr)),
                 this, SLOT(confirmDelete(TRecPtr)));
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

    ok = connect(&start_live_playback_, SIGNAL(timeout()),
                 this, SLOT(startLivePlayback()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    spinner_.reset();
    confirm_.reset();
    view_.reset();

    delete playerWidget_;
    playerWidget_ = NULL;

    delete canvas_;
    canvas_ = NULL;

    delete popup_;
    popup_ = NULL;
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
    AppView & app_view = *view_;
    ConfirmView & confirm = *confirm_;
    SpinnerView & spinner = *spinner_;

    // add image://thumbnails/... provider:
    yae::shared_ptr<ThumbnailProvider, ImageProvider>
      image_provider(new ThumbnailProvider(readerFactory_));
    app_view.addImageProvider(QString::fromUtf8("thumbnails"),
                              image_provider);

    TMakeCurrentContext currentContext(canvas_->Canvas::context());
    canvas_->initializePrivateBackend();
    canvas_->setGreeting(tr("hello"));
    canvas_->append(&app_view);

    app_view.setModel(&dvr_);
    app_view.setEnabled(true);
    app_view.layoutChanged();

    // action confirmation view:
    confirm.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    confirm.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->append(&confirm);
    confirm.setStyle(app_view.style());
    confirm.setEnabled(false);

    // spinner view:
    spinner.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    spinner.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvas_->append(&spinner);
    spinner.setStyle(app_view.style());
    spinner.setEnabled(false);

    playerWidget_->initItemViews();
    playerWidget_->view().setEnabled(false);

    // shortcut:
    PlayerUxItem & pl_ux = playerWidget_->get_player_ux();
    pl_ux.insert_menus(IReaderPtr(), menuBar(), menuHelp->menuAction());
    pl_ux.enableBackArrowButton_ = BoolRef::constant(true);
    pl_ux.enableDeleteFileButton_ = BoolRef::constant(true);
    pl_ux.uncache();

    bool ok = true;
    ok = connect(&pl_ux, SIGNAL(playback_finished(TTime)),
                 this, SLOT(playbackFinished(TTime)));
    YAE_ASSERT(ok);

    ok = connect(&pl_ux, SIGNAL(save_bookmark()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);

    ok = connect(&pl_ux, SIGNAL(save_bookmark_at(double)),
                 this, SLOT(saveBookmarkAt(double)));
    YAE_ASSERT(ok);

    ok = connect(&pl_ux, SIGNAL(on_back_arrow()),
                 this, SLOT(backToPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(&pl_ux, SIGNAL(delete_playing_file()),
                 this, SLOT(confirmDeletePlayingRecording()));
    YAE_ASSERT(ok);

    ok = connect(&pl_ux, SIGNAL(toggle_playlist()),
                 &playerWindow_, SLOT(stopAndHide()));
    YAE_ASSERT(ok);

    if (!dvr_.has_preferences())
    {
      PreferencesDialog & d = preferencesDialog();
      d.init(dvr_);
      d.exec();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::themeChanged
  //
  void
  MainWindow::themeChanged(const yae::Application & app)
  {
    AppView & app_view = *view_;
    AppStyle & style = *(app_view.style());
    style.themeChanged(app);
    app_view.requestUncacheEPG();
    app_view.requestUncache();
    app_view.requestRepaint();
  }

  //----------------------------------------------------------------
  // MainWindow::fileExit
  //
  void
  MainWindow::fileExit()
  {
    confirmExit();
  }

  //----------------------------------------------------------------
  // MainWindow::exitConfirmed
  //
  void
  MainWindow::exitConfirmed()
  {
    dvr_.shutdown();
    qApp->quit();
    ::exit(0);
  }

  //----------------------------------------------------------------
  // MainWindow::editPreferences
  //
  void
  MainWindow::editPreferences()
  {
    PreferencesDialog & d = preferencesDialog();
    d.init(dvr_);
    d.show();
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
    view_->now_playing_.reset();
    dvr_.watch_live(ch_num);

    // open the player window
    uint16_t major = yae::mpeg_ts::channel_major(ch_num);
    uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);

    std::string title = strfmt("%i-%i, live",
                               major,
                               minor);

    playerWindow_.setWindowTitle(QString::fromUtf8(title.c_str()));

    PlayerUxItem & pl_ux = playerWidget_->get_player_ux();
    pl_ux.insert_menus(IReaderPtr(), menuBar(), menuHelp->menuAction());

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
  // ConfirmBlockChannel
  //
  struct ConfirmBlockChannel : DismissConfirmView
  {
    ConfirmBlockChannel(MainWindow & mainWindow,
                        ConfirmView & confirm,
                        uint32_t ch_num):
      DismissConfirmView(confirm),
      mainWindow_(mainWindow),
      ch_num_(ch_num)
    {}

    // virtual:
    void execute() const
    {
      // shortcuts:
      DVR & dvr = mainWindow_.dvr_;
      AppView & appView = *(mainWindow_.view_);
      dvr.toggle_blocklist(ch_num_);
      dvr.save_blocklist();
      appView.sync_ui();
      appView.requestRepaint();
      DismissConfirmView::execute();
    }

    MainWindow & mainWindow_;
    uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // MainWindow::confirmBlockChannel
  //
  void
  MainWindow::confirmBlockChannel(uint32_t ch_num)
  {
    uint16_t major = yae::mpeg_ts::channel_major(ch_num);
    uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
    const AppStyle & style = *(view_->style());
    ConfirmView & confirm = *confirm_;

    std::string msg = strfmt("Block Channel %i-%i?  You will be able "
                             "to re-enable it in the Channels view.",
                             major, minor);
    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.
      reset(new ConfirmBlockChannel(*this, confirm, ch_num));
    ConfirmItem::Action & aff = *(confirm.affirmative_);
    aff.message_ = TVarRef::constant(TVar("Block"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.cursor_fg_;

    confirm.negative_.reset(new DismissConfirmView(*confirm_));
    ConfirmItem::Action & neg = *(confirm.negative_);
    neg.message_ = TVarRef::constant(TVar("Cancel"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackRecording
  //
  IReaderPtr
  MainWindow::playbackRecording(TRecPtr rec_ptr)
  {
    if (!rec_ptr)
    {
      return IReaderPtr();
    }

    const Recording::Rec & rec = *rec_ptr;
    std::string mpg_path = rec.get_filepath(dvr_.basedir_, ".mpg");
    bool hwdec = true;

    IReaderPtr reader = yae::openFile(readerFactory_,
                                      QString::fromUtf8(mpg_path.c_str()),
                                      hwdec);
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
                               rec.full_title_.c_str(),
                               time_str.c_str());
    playerWindow_.setWindowTitle(QString::fromUtf8(title.c_str()));

    PlayerUxItem & pl_ux = playerWidget_->get_player_ux();

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

    pl_ux.insert_menus(reader,
                       menuBar(),
                       menuHelp->menuAction());

    pl_ux.insert_menus(reader,
                       playerWindow_.menubar,
                       menuHelp->menuAction());

    return reader;
  }

  //----------------------------------------------------------------
  // ConfirmDeleteRecording
  //
  struct ConfirmDeleteRecording : DismissConfirmView
  {
    ConfirmDeleteRecording(MainWindow & mainWindow,
                           ConfirmView & confirm,
                           const TRecPtr & rec):
      DismissConfirmView(confirm),
      mainWindow_(mainWindow),
      rec_(rec)
    {}

    // virtual:
    void execute() const
    {
      // shortcuts:
      DVR & dvr = mainWindow_.dvr_;
      AppView & appView = *(mainWindow_.view_);
      const Recording::Rec & rec = *rec_;

      if (appView.now_playing_)
      {
        std::string filename = rec.get_filename(dvr.basedir_, ".mpg");

        if (appView.now_playing_->filename_ == filename)
        {
          mainWindow_.playerWindow_.stopAndHide();
          appView.now_playing_.reset();
        }
      }

      dvr.delete_recording(rec);
      appView.requestUncacheEPG();
      appView.requestUncache();
      appView.sync_ui();
      appView.requestRepaint();
      DismissConfirmView::execute();
    }

    MainWindow & mainWindow_;
    TRecPtr rec_;
  };

  //----------------------------------------------------------------
  // MainWindow::confirmDelete
  //
  void
  MainWindow::confirmDelete(TRecPtr rec_ptr)
  {
    const Recording::Rec & rec = *rec_ptr;
    const AppStyle & style = *(view_->style());
    ConfirmView & confirm = *confirm_;

    std::string msg = strfmt("Delete %s?", rec.get_basename().c_str());
    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.
      reset(new ConfirmDeleteRecording(*this, *confirm_, rec_ptr));
    ConfirmItem::Action & aff = *(confirm.affirmative_);
    aff.message_ = TVarRef::constant(TVar("Delete"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.cursor_fg_;

    confirm.negative_.reset(new DismissConfirmView(*confirm_));
    ConfirmItem::Action & neg = *(confirm.negative_);
    neg.message_ = TVarRef::constant(TVar("Cancel"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // DeclineDeleteRecording
  //
  struct DeclineDeleteRecording : DismissConfirmView
  {
    DeclineDeleteRecording(MainWindow & mainWindow,
                           ConfirmView & confirm,
                           const TRecPtr & rec):
      DismissConfirmView(confirm),
      mainWindow_(mainWindow),
      rec_(rec)
    {}

    // virtual:
    void execute() const
    {
      // shortcuts:
      DVR & dvr = mainWindow_.dvr_;
      AppView & appView = *(mainWindow_.view_);
      const Recording::Rec & rec = *rec_;

      if (appView.now_playing_)
      {
        std::string filename = rec.get_filename(dvr.basedir_, ".mpg");

        if (appView.now_playing_->filename_ == filename)
        {
          mainWindow_.playerWindow_.stopAndHide();
          mainWindow_.view_->now_playing_.reset();
        }
      }

      appView.sync_ui();
      appView.requestRepaint();
      DismissConfirmView::execute();
    }

    MainWindow & mainWindow_;
    TRecPtr rec_;
  };

  //----------------------------------------------------------------
  // MainWindow::playbackFinished
  //
  void
  MainWindow::playbackFinished(TTime playheadPos)
  {
    TRecPtr rec_ptr = view_->now_playing();
    if (live_rec_ || !rec_ptr)
    {
      uint32_t live_ch = dvr_.schedule_.get_live_channel();
      if (live_ch)
      {
        watchLive(live_ch, playheadPos);
      }

      return;
    }

    if (dvr_.schedule_.is_recording_now(rec_ptr))
    {
      // probably exhausted the buffer, try to resume playback:
      IReaderPtr reader = playbackRecording(rec_ptr);
      if (reader)
      {
        TTime t0(0, 0);
        TTime t1(0, 0);
        if (yae::get_timeline(reader.get(), t0, t1))
        {
          double seek_pos = std::max(t0.sec(), playheadPos.sec() - 10.0);
          reader->seek(seek_pos);
        }

        return;
      }
    }

    const Recording::Rec & rec = *rec_ptr;

    // shortcuts:
    const AppStyle & style = *(view_->style());
    ConfirmView & confirm = *(playerWidget_->confirm_);

    std::string msg = strfmt("Delete %s?", rec.get_basename().c_str());
    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.
      reset(new ConfirmDeleteRecording(*this, confirm, rec_ptr));
    ConfirmItem::Action & aff = *confirm.affirmative_;
    aff.message_ = TVarRef::constant(TVar("Delete"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.cursor_fg_;

    confirm.negative_.
      reset(new DeclineDeleteRecording(*this, confirm, rec_ptr));
    ConfirmItem::Action & neg = *confirm.negative_;
    neg.message_ = TVarRef::constant(TVar("Close"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::confirmDeletePlayingRecording
  //
  void
  MainWindow::confirmDeletePlayingRecording()
  {
    TRecPtr rec_ptr = view_->now_playing();
    if (!rec_ptr)
    {
      return;
    }

    const Recording::Rec & rec = *rec_ptr;

    // shortcuts:
    const AppStyle & style = *(view_->style());
    ConfirmView & confirm = *(playerWidget_->confirm_);

    std::string msg = strfmt("Delete %s?", rec.get_basename().c_str());
    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.
      reset(new ConfirmDeleteRecording(*this, confirm, rec_ptr));
    ConfirmItem::Action & aff = *confirm.affirmative_;
    aff.message_ = TVarRef::constant(TVar("Delete"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.cursor_fg_;

    confirm.negative_.reset(new DismissConfirmView(confirm));
    ConfirmItem::Action & neg = *confirm.negative_;
    neg.message_ = TVarRef::constant(TVar("Cancel"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::saveBookmark
  //
  void
  MainWindow::saveBookmark()
  {
    TRecPtr now_playing = view_->now_playing();
    if (!now_playing)
    {
      return;
    }

    const PlayerUxItem & pl_ux = playerWidget_->get_player_ux();
    const TimelineModel & timeline = pl_ux.timeline_model();
    saveBookmarkAt(timeline.currentTime());
  }

  //----------------------------------------------------------------
  // MainWindow::saveBookmarkAt
  //
  void
  MainWindow::saveBookmarkAt(double position_in_sec)
  {
    TRecPtr now_playing = view_->now_playing();
    if (!now_playing)
    {
      return;
    }

    const Recording::Rec & rec = *now_playing;
    const PlayerUxItem & pl_ux = playerWidget_->get_player_ux();
    const IReader * reader = pl_ux.get_reader();
    save_bookmark(dvr_, rec, reader, position_in_sec);
  }

  //----------------------------------------------------------------
  // MainWindow::playerWindowClosed
  //
  void
  MainWindow::playerWindowClosed()
  {
    if (canvasContainer_->indexOf(playerWidget_) == -1)
    {
      canvasContainer_->addWidget(playerWidget_);
    }

    canvasContainer_->setCurrentWidget(canvas_);
    view_->now_playing_.reset();
    view_->requestUncache();
    view_->requestRepaint();
    stopLivePlayback();

    PlayerUxItem & pl_ux = playerWidget_->get_player_ux();
    pl_ux.insert_menus(IReaderPtr(), menuBar(), menuHelp->menuAction());

    yae_dlog("MainWindow::playerWindowClosed()");
  }

  //----------------------------------------------------------------
  // MainWindow::playerEnteringFullScreen
  //
  void
  MainWindow::playerEnteringFullScreen()
  {
    if (view_->now_playing_)
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
    if (view_->now_playing_)
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
  // ConfirmExit
  //
  struct ConfirmExit : DismissConfirmView
  {
    ConfirmExit(MainWindow & mainWindow,
                ConfirmView & confirm):
      DismissConfirmView(confirm),
      mainWindow_(mainWindow)
    {}

    // virtual:
    void execute() const
    {
      DismissConfirmView::execute();
      yae::queue_call(mainWindow_, &MainWindow::exitConfirmed);
    }

    MainWindow & mainWindow_;
  };

  //----------------------------------------------------------------
  // MainWindow::confirmExit
  //
  void
  MainWindow::confirmExit()
  {
    std::set<std::string> enabled_tuners;
    if (!dvr_.discover_enabled_tuners(enabled_tuners))
    {
      // there are no enabled tuners, no need to confirm:
      exitConfirmed();
      return;
    }

    const AppStyle & style = *(view_->style());

    const char * msg =
      "NOTE: if you exit the DVR then it will not be able to "
      "update the Program Guide or record any scheduled programs.";

    ConfirmView & confirm =
      (canvasContainer_->currentWidget() == playerWidget_ &&
       window()->isFullScreen()) ?
      *playerWidget_->confirm_ :
      *confirm_;

    confirm.message_ = TVarRef::constant(TVar(msg));
    confirm.bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    confirm.fg_ = style.bg_;

    confirm.affirmative_.reset(new ConfirmExit(*this, confirm));
    ConfirmItem::Action & aff = *confirm.affirmative_;
    aff.message_ = TVarRef::constant(TVar("Exit"));
    aff.bg_ = style.cursor_;
    aff.fg_ = style.cursor_fg_;

    confirm.negative_.reset(new DismissConfirmView(confirm));
    ConfirmItem::Action & neg = *confirm.negative_;
    neg.message_ = TVarRef::constant(TVar("Cancel"));
    neg.bg_ = style.fg_;
    neg.fg_ = style.bg_;

    confirm.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::startLivePlayback
  //
  void
  MainWindow::startLivePlayback()
  {
    AppView & app_view = *view_;
    uint32_t live_ch = dvr_.schedule_.get_live_channel();
    uint64_t t_gps = unix_epoch_time_to_gps_time(start_live_seek_pos_.get(1));
    live_rec_ = yae::next<Recording::Rec>(app_view.rec_by_channel_,
                                          live_ch,
                                          t_gps,
                                          live_rec_);

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

      const Recording::Rec & rec = *live_rec_;
      fs::path title_path = rec.get_title_path(dvr_.basedir_);
      std::string mpg_path = rec.get_title_filepath(title_path, ".mpg");
      std::string basepath = mpg_path.substr(0, mpg_path.size() - 4);
      std::string filename = mpg_path.substr(title_path.string().size() + 1);
      app_view.now_playing_.reset(new DVR::Playback(app_view.sidebar_sel_,
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
    e->ignore();
    fileExit();
  }

  //----------------------------------------------------------------
  // MainWindow::keyPressEvent
  //
  void
  MainWindow::keyPressEvent(QKeyEvent * e)
  {
    // if the event propagates all the way here
    // then let the player handle it:
    if (playerWindow_.isVisible())
    {
#if 0
      QKeyEvent * event = new QKeyEvent(e->type(),
                                        e->key(),
                                        e->modifiers(),
                                        e->text(),
                                        e->isAutoRepeat(),
                                        e->count());

      qApp->postEvent(&playerWidget_->canvas(),
                      event,
                      Qt::HighEventPriority);
#else
      playerWidget_->canvas().event(e);
#endif
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

      popup_->clear();
      if (canvasContainer_->currentWidget() == canvas_)
      {
        canvas_->populateContextMenu(*popup_);
      }
      else
      {
        playerWidget_->canvas_->populateContextMenu(*popup_);
      }

      if (!popup_->isEmpty())
      {
        popup_->popup(globalPt);
        e->accept();
        return;
      }
    }

    QWidget::mousePressEvent(e);
  }

}

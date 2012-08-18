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
#include <math.h>

// GLEW includes:
#include <GL/glew.h>

// Qt includes:
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>
#include <QSpacerItem>
#include <QDesktopWidget>
#include <QMenu>
#include <QShortcut>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QDirIterator>

// yae includes:
#include <yaeReaderFFMPEG.h>
#include <yaePixelFormats.h>
#include <yaePixelFormatTraits.h>
#include <yaeAudioRendererPortaudio.h>
#include <yaeVideoRenderer.h>
#include <yaeVersion.h>
#include <yaeUtils.h>

// local includes:
#include <yaeMainWindow.h>


namespace yae
{

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
  // AboutDialog::AboutDialog
  //
  AboutDialog::AboutDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);
  }

  //----------------------------------------------------------------
  // AspectRatioDialog::AspectRatioDialog
  //
  AspectRatioDialog::AspectRatioDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::AspectRatioDialog()
  {
    Ui::AspectRatioDialog::setupUi(this);
  }

  //----------------------------------------------------------------
  // OpenUrlDialog::OpenUrlDialog
  //
  OpenUrlDialog::OpenUrlDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::OpenUrlDialog()
  {
    Ui::OpenUrlDialog::setupUi(this);
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
  // AutoCropEvent
  //
  struct AutoCropEvent : public QEvent
  {
    AutoCropEvent(const TCropFrame & cropFrame):
      QEvent(QEvent::User),
      cropFrame_(cropFrame)
    {}

    TCropFrame cropFrame_;
  };

#ifdef __APPLE__
  //----------------------------------------------------------------
  // RemoteControlEvent
  //
  struct RemoteControlEvent : public QEvent
  {
    RemoteControlEvent(TRemoteControlButtonId buttonId,
                       bool pressedDown,
                       unsigned int clickCount,
                       bool heldDown):
      QEvent(QEvent::User),
      buttonId_(buttonId),
      pressedDown_(pressedDown),
      clickCount_(clickCount),
      heldDown_(heldDown)
    {}

    TRemoteControlButtonId buttonId_;
    bool pressedDown_;
    unsigned int clickCount_;
    bool heldDown_;
  };
#endif

  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    audioDeviceGroup_(NULL),
    audioDeviceMapper_(NULL),
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    subsTrackGroup_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    subsTrackMapper_(NULL),
    reader_(NULL),
    canvas_(NULL),
    audioRenderer_(NULL),
    videoRenderer_(NULL),
    playbackPaused_(false),
    playbackInterrupted_(false),
    scrollStart_(0.0),
    scrollOffset_(0.0),
    xexpand_(1.0),
    yexpand_(1.0),
    tempo_(1.0)
  {
#ifdef __APPLE__
    appleRemoteControl_ = NULL;
#endif

    setupUi(this);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(playlistWidget_);
    actionPlay->setText(tr("Pause"));

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon = QString::fromUtf8(":/images/apprenticevideo-64.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    timelineControls_->setAuxWidgets(lineEditPlayhead_,
                                     lineEditDuration_,
                                     this);

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);

    canvas_ = new Canvas(contextFormat);
    canvasLayout->addWidget(canvas_);

    reader_ = ReaderFFMPEG::create();
    audioRenderer_ = AudioRendererPortaudio::create();
    videoRenderer_ = VideoRenderer::create();

    // show the timeline:
    actionShowTimeline->setChecked(true);
    timelineWidgets_->show();

    // hide the playlist:
    actionShowPlaylist->setChecked(false);
    playlistDock_->hide();

    // setup the Open URL dialog:
    {
      std::list<std::string> protocols;
      reader_->getUrlProtocols(protocols);

      QString supported = tr("Supported URL protocols include:\n");
      unsigned int column = 0;
      for (std::list<std::string>::const_iterator i = protocols.begin();
           i != protocols.end(); ++i)
      {
        const std::string & name = *i;
        if (column)
        {
          supported += "  ";
        }

        supported += QString::fromUtf8(name.c_str());
        column = (column + 1) % 6;

        if (!column)
        {
          supported += "\n";
        }
      }

      openUrl_ = new OpenUrlDialog(this);
      openUrl_->lineEdit->setToolTip(supported);
    }

    // for scroll-wheel event buffering,
    // used to avoid frequent seeking by small distances:
    scrollWheelTimer_.setSingleShot(true);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);
    shortcutShowPlaylist_ = new QShortcut(this);
    shortcutShowTimeline_ = new QShortcut(this);
    shortcutPlay_ = new QShortcut(this);
    shortcutNext_ = new QShortcut(this);
    shortcutPrev_ = new QShortcut(this);
    shortcutLoop_ = new QShortcut(this);
    shortcutCropNone_ = new QShortcut(this);
    shortcutAutoCrop_ = new QShortcut(this);
    shortcutRemove_ = new QShortcut(this);
    shortcutSelectAll_ = new QShortcut(this);

    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);
    shortcutShowPlaylist_->setContext(Qt::ApplicationShortcut);
    shortcutShowTimeline_->setContext(Qt::ApplicationShortcut);
    shortcutPlay_->setContext(Qt::ApplicationShortcut);
    shortcutNext_->setContext(Qt::ApplicationShortcut);
    shortcutPrev_->setContext(Qt::ApplicationShortcut);
    shortcutLoop_->setContext(Qt::ApplicationShortcut);
    shortcutCropNone_->setContext(Qt::ApplicationShortcut);
    shortcutAutoCrop_->setContext(Qt::ApplicationShortcut);

    shortcutRemove_->setContext(Qt::ApplicationShortcut);
    shortcutSelectAll_->setContext(Qt::ApplicationShortcut);

    shortcutRemove_->setKey(QKeySequence(QKeySequence::Delete));
    shortcutSelectAll_->setKey(QKeySequence(QKeySequence::SelectAll));

    actionRemove_ = new QAction(this);
    actionRemove_->setObjectName(QString::fromUtf8("actionRemove_"));
    actionRemove_->setShortcutContext(Qt::ApplicationShortcut);
    actionRemove_->setText(tr("&Remove Selected"));
    actionRemove_->setShortcut(tr("Delete"));

    actionSelectAll_ = new QAction(this);
    actionSelectAll_->setObjectName(QString::fromUtf8("actionSelectAll_"));
    actionSelectAll_->setShortcutContext(Qt::ApplicationShortcut);
    actionSelectAll_->setText(tr("&Select All"));
    actionSelectAll_->setShortcut(tr("Ctrl+A"));

    QActionGroup * aspectRatioGroup = new QActionGroup(this);
    aspectRatioGroup->addAction(actionAspectRatioAuto);
    aspectRatioGroup->addAction(actionAspectRatio1_33);
    aspectRatioGroup->addAction(actionAspectRatio1_60);
    aspectRatioGroup->addAction(actionAspectRatio1_78);
    aspectRatioGroup->addAction(actionAspectRatio1_85);
    aspectRatioGroup->addAction(actionAspectRatio2_35);
    aspectRatioGroup->addAction(actionAspectRatio2_40);
    aspectRatioGroup->addAction(actionAspectRatioOther);
    actionAspectRatioAuto->setChecked(true);

    QActionGroup * cropFrameGroup = new QActionGroup(this);
    cropFrameGroup->addAction(actionCropFrameNone);
    cropFrameGroup->addAction(actionCropFrame1_33);
    cropFrameGroup->addAction(actionCropFrame1_60);
    cropFrameGroup->addAction(actionCropFrame1_78);
    cropFrameGroup->addAction(actionCropFrame1_85);
    cropFrameGroup->addAction(actionCropFrame2_35);
    cropFrameGroup->addAction(actionCropFrame2_40);
    cropFrameGroup->addAction(actionCropFrameAutoDetect);
    actionCropFrameNone->setChecked(true);

    QActionGroup * playRateGroup = new QActionGroup(this);
    playRateGroup->addAction(actionTempo50);
    playRateGroup->addAction(actionTempo60);
    playRateGroup->addAction(actionTempo70);
    playRateGroup->addAction(actionTempo80);
    playRateGroup->addAction(actionTempo90);
    playRateGroup->addAction(actionTempo100);
    playRateGroup->addAction(actionTempo111);
    playRateGroup->addAction(actionTempo125);
    playRateGroup->addAction(actionTempo143);
    playRateGroup->addAction(actionTempo167);
    playRateGroup->addAction(actionTempo200);
    actionTempo100->setChecked(true);

    QSignalMapper * playRateMapper = new QSignalMapper(this);
    playRateMapper->setMapping(actionTempo50, 50);
    playRateMapper->setMapping(actionTempo60, 60);
    playRateMapper->setMapping(actionTempo70, 70);
    playRateMapper->setMapping(actionTempo80, 80);
    playRateMapper->setMapping(actionTempo90, 90);
    playRateMapper->setMapping(actionTempo100, 100);
    playRateMapper->setMapping(actionTempo111, 111);
    playRateMapper->setMapping(actionTempo125, 125);
    playRateMapper->setMapping(actionTempo143, 143);
    playRateMapper->setMapping(actionTempo167, 167);
    playRateMapper->setMapping(actionTempo200, 200);

    bool ok = true;
    ok = connect(playRateMapper, SIGNAL(mapped(int)),
                 this, SLOT(playbackSetTempo(int)));
    YAE_ASSERT(ok);

    ok = connect(actionTempo50, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo60, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo70, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo80, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo90, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo100, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo111, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo125, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo143, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo167, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo200, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);

    ok = connect(actionOpenURL, SIGNAL(triggered()),
                 this, SLOT(fileOpenURL()));
    YAE_ASSERT(ok);

    ok = connect(actionOpenFolder, SIGNAL(triggered()),
                 this, SLOT(fileOpenFolder()));
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

    ok = connect(actionAspectRatio1_60, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_60()));
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

    ok = connect(actionAspectRatio2_40, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_40()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioOther, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioOther()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameNone, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameNone()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCropNone_, SIGNAL(activated()),
                 this, SLOT(playbackCropFrameNone()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_33, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_33()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_60, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_60()));
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

    ok = connect(actionCropFrame2_40, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_40()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameAutoDetect, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(shortcutAutoCrop_, SIGNAL(activated()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(actionPlay, SIGNAL(triggered()),
                 this, SLOT(togglePlayback()));
    YAE_ASSERT(ok);

    ok = connect(shortcutPlay_, SIGNAL(activated()),
                 this, SLOT(togglePlayback()));
    YAE_ASSERT(ok);

    ok = connect(actionNext, SIGNAL(triggered()),
                 this, SLOT(playbackFinished()));
    YAE_ASSERT(ok);

    ok = connect(shortcutNext_, SIGNAL(activated()),
                 this, SLOT(playbackFinished()));
    YAE_ASSERT(ok);

    ok = connect(actionPrev, SIGNAL(triggered()),
                 this, SLOT(playbackPrev()));
    YAE_ASSERT(ok);

    ok = connect(shortcutPrev_, SIGNAL(activated()),
                 this, SLOT(playbackPrev()));
    YAE_ASSERT(ok);

    ok = connect(actionLoop, SIGNAL(triggered()),
                 this, SLOT(playbackLoop()));
    YAE_ASSERT(ok);

    ok = connect(shortcutLoop_, SIGNAL(activated()),
                 this, SLOT(playbackLoop()));
    YAE_ASSERT(ok);

    ok = connect(actionSetInPoint, SIGNAL(triggered()),
                 this, SIGNAL(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionSetOutPoint, SIGNAL(triggered()),
                 this, SIGNAL(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionRemove_, SIGNAL(triggered()),
                 playlistWidget_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(actionSelectAll_, SIGNAL(triggered()),
                 playlistWidget_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(shortcutRemove_, SIGNAL(activated()),
                 playlistWidget_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSelectAll_, SIGNAL(activated()),
                 playlistWidget_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(playlistWidget_, SIGNAL(currentItemChanged(std::size_t)),
                 this, SLOT(playlistItemChanged(std::size_t)));
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

    ok = connect(actionShowPlaylist, SIGNAL(triggered()),
                 this, SLOT(playbackShowPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(shortcutShowPlaylist_, SIGNAL(activated()),
                 this, SLOT(playbackShowPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(actionShowTimeline, SIGNAL(triggered()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);

    ok = connect(shortcutShowTimeline_, SIGNAL(activated()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipColorConverter, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipLoopFilter, SIGNAL(triggered()),
                 this, SLOT(playbackLoopFilter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipNonReferenceFrames, SIGNAL(triggered()),
                 this, SLOT(playbackNonReferenceFrames()));
    YAE_ASSERT(ok);

    ok = connect(actionVerticalScaling, SIGNAL(triggered()),
                 this, SLOT(playbackVerticalScaling()));
    YAE_ASSERT(ok);

    ok = connect(actionDeinterlace, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);

    ok = connect(canvas_, SIGNAL(toggleFullScreen()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(actionHalfSize, SIGNAL(triggered()),
                 this, SLOT(windowHalfSize()));
    YAE_ASSERT(ok);

    ok = connect(actionFullSize, SIGNAL(triggered()),
                 this, SLOT(windowFullSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDoubleSize, SIGNAL(triggered()),
                 this, SLOT(windowDoubleSize()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
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

    ok = connect(timelineControls_, SIGNAL(moveTimeIn(double)),
                 this, SLOT(moveTimeIn(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(moveTimeOut(double)),
                 this, SLOT(moveTimeOut(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(movePlayHead(double)),
                 this, SLOT(movePlayHead(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(clockStopped()),
                 this, SLOT(playbackFinished()));
    YAE_ASSERT(ok);

    ok = connect(menuAudioDevice, SIGNAL(aboutToShow()),
                 this, SLOT(populateAudioDeviceMenu()));
    YAE_ASSERT(ok);

    ok = connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                 this, SLOT(focusChanged(QWidget *, QWidget *)));
    YAE_ASSERT(ok);

    ok = connect(&scrollWheelTimer_, SIGNAL(timeout()),
                 this, SLOT(scrollWheelTimerExpired()));
    YAE_ASSERT(ok);

    ok = connect(playlistDock_, SIGNAL(visibilityChanged(bool)),
                 this, SLOT(playlistVisibilityChanged(bool)));
    YAE_ASSERT(ok);

    ok = connect(playlistFilter_, SIGNAL(textChanged(const QString &)),
                 playlistWidget_, SLOT(filterChanged(const QString &)));
    YAE_ASSERT(ok);

    ok = connect(playlistFilter_, SIGNAL(textChanged(const QString &)),
                 this, SLOT(fixupNextPrev()));
    YAE_ASSERT(ok);

    // initialize the subtitles menu:
    {
      subsTrackGroup_ = new QActionGroup(this);
      subsTrackMapper_ = new QSignalMapper(this);
      ok = connect(subsTrackMapper_, SIGNAL(mapped(int)),
                   this, SLOT(subsSelectTrack(int)));
      YAE_ASSERT(ok);

      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      trackAction->setChecked(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(0));
    }
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    delete openUrl_;
    openUrl_ = NULL;

    audioRenderer_->close();
    audioRenderer_->destroy();

    videoRenderer_->close();
    videoRenderer_->destroy();

    reader_->destroy();
    canvas_->cropAutoDetectStop();
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
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const std::list<QString> & playlist,
                          bool beginPlaybackImmediately)
  {
    SignalBlocker blockSignals(playlistWidget_);
    playlistWidget_->add(playlist);

    if (beginPlaybackImmediately)
    {
      playback();
    }
  }

  //----------------------------------------------------------------
  // kExtEyetv
  //
  static const QString kExtEyetv = QString::fromUtf8("eyetv");

  //----------------------------------------------------------------
  // getExtIgnoreList
  //
  static QStringList getExtIgnoreList()
  {
    QStringList ext;
    ext
      << QString::fromUtf8("eyetvsched")
      << QString::fromUtf8("eyetvp")
      << QString::fromUtf8("eyetvr")
      << QString::fromUtf8("eyetvi")
      << QString::fromUtf8("doc")
      << QString::fromUtf8("xls")
      << QString::fromUtf8("ppt")
      << QString::fromUtf8("pdf")
      << QString::fromUtf8("htm")
      << QString::fromUtf8("rar")
      << QString::fromUtf8("zip")
      << QString::fromUtf8("7z")
      << QString::fromUtf8("gz")
      << QString::fromUtf8("bz2")
      << QString::fromUtf8("war")
      << QString::fromUtf8("tar")
      << QString::fromUtf8("tgz")
      << QString::fromUtf8("tbz2")
      << QString::fromUtf8("lzma")
      << QString::fromUtf8("url")
      << QString::fromUtf8("eml")
      << QString::fromUtf8("html")
      << QString::fromUtf8("xml")
      << QString::fromUtf8("db")
      << QString::fromUtf8("txt")
      << QString::fromUtf8("srt")
      << QString::fromUtf8("idx")
      << QString::fromUtf8("sub")
      << QString::fromUtf8("ifo")
      << QString::fromUtf8("info")
      << QString::fromUtf8("nfo")
      << QString::fromUtf8("md5")
      << QString::fromUtf8("crc")
      << QString::fromUtf8("sfv")
      << QString::fromUtf8("m3u")
      << QString::fromUtf8("smil")
      << QString::fromUtf8("app")
      << QString::fromUtf8("plist")
      << QString::fromUtf8("framework")
      << QString::fromUtf8("bundle")
      << QString::fromUtf8("rcproject")
      << QString::fromUtf8("ipmeta")
      << QString::fromUtf8("sc")
      << QString::fromUtf8("so")
      << QString::fromUtf8("dylib")
      << QString::fromUtf8("dll")
      << QString::fromUtf8("exe")
      << QString::fromUtf8("com")
      << QString::fromUtf8("cmd")
      << QString::fromUtf8("bat")
      << QString::fromUtf8("sys")
      << QString::fromUtf8("py")
      << QString::fromUtf8("sh")
      << QString::fromUtf8("cpp")
      << QString::fromUtf8("h")
      << QString::fromUtf8("cxx")
      << QString::fromUtf8("hxx")
      << QString::fromUtf8("log")
      << QString::fromUtf8("sqz")
      << QString::fromUtf8("xss")
      << QString::fromUtf8("spfx")
      << QString::fromUtf8("iso")
      << QString::fromUtf8("dmg")
      << QString::fromUtf8("svq")
      << QString::fromUtf8("itdb")
      << QString::fromUtf8("itl")
      << QString::fromUtf8("itc")
      << QString::fromUtf8("ipa");

    return ext;
  }

  //----------------------------------------------------------------
  // kExtIgnoreList
  //
  static QStringList kExtIgnoreList = getExtIgnoreList();

  //----------------------------------------------------------------
  // shouldIgnore
  //
  static bool shouldIgnore(const QString & ext)
  {
    QString extLowered = ext.toLower();
    return kExtIgnoreList.contains(extLowered);
  }

  //----------------------------------------------------------------
  // findFiles
  //
  static void
  findFiles(std::list<QString> & files,
            const QString & startHere,
            bool recursive = true)
  {
    QStringList extFilters;
    if (QFileInfo(startHere).suffix() == kExtEyetv)
    {
      extFilters << QString::fromUtf8("*.mpg");
    }

    QDirIterator iter(startHere,
                      extFilters,
                      QDir::NoDotAndDotDot |
                      QDir::AllEntries |
                      QDir::Readable,
                      QDirIterator::FollowSymlinks);

    while (iter.hasNext())
    {
      iter.next();

      QFileInfo fi = iter.fileInfo();
      QString fn = fi.absoluteFilePath();
      QString ext = fi.suffix();
      // std::cerr << "FN: " << fn.toUtf8().constData() << std::endl;

      if (!shouldIgnore(ext))
      {
        if (fi.isDir() && ext != kExtEyetv)
        {
          if (recursive)
          {
            findFiles(files, fn, recursive);
          }
        }
        else
        {
          files.push_back(fn);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // findFilesAndSort
  //
  static void
  findFilesAndSort(std::list<QString> & files,
                   const QString & startHere,
                   bool recursive = true)
  {
    findFiles(files, startHere, recursive);
    files.sort();
  }

  //----------------------------------------------------------------
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path)
  {
    QString fn = path;
    QFileInfo fi(fn);
    if (fi.suffix() == kExtEyetv)
    {
      std::list<QString> found;
      findFiles(found, path, false);

      if (!found.empty())
      {
        fn = found.front();
      }
    }

    std::string filename(fn.toUtf8().constData());

    actionPlay->setEnabled(false);

    ReaderFFMPEG * reader = ReaderFFMPEG::create();
    if (!reader->open(filename.c_str()))
    {
      std::cerr << "ERROR: could not open movie: " << filename << std::endl;
      return false;
    }

    // disconnect timeline from renderers:
    timelineControls_->observe(SharedClock());

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t subsCount = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackInterval(true);

    std::cout << std::endl
              << "yae: " << filename << std::endl
              << "yae: video tracks: " << numVideoTracks << std::endl
              << "yae: audio tracks: " << numAudioTracks << std::endl
              << "yae: subs tracks: " << subsCount << std::endl;

    if (audioTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = audioTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuAudio->removeAction(action);
      }
    }

    if (videoTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = videoTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuVideo->removeAction(action);
      }
    }

    if (subsTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = subsTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuSubs->removeAction(action);
      }
    }

    // update the UI:
    delete audioTrackGroup_;
    audioTrackGroup_ = new QActionGroup(this);

    delete videoTrackGroup_;
    videoTrackGroup_ = new QActionGroup(this);

    delete subsTrackGroup_;
    subsTrackGroup_ = new QActionGroup(this);

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

    delete subsTrackMapper_;
    subsTrackMapper_ = new QSignalMapper(this);

    ok = connect(subsTrackMapper_, SIGNAL(mapped(int)),
                 this, SLOT(subsSelectTrack(int)));
    YAE_ASSERT(ok);

    for (unsigned int i = 0; i < numAudioTracks; i++)
    {
      reader->selectAudioTrack(i);
      QString trackName = tr("Track %1").arg(i + 1);

      const char * name = reader->getSelectedAudioTrackName();
      if (name && *name && strcmp(name, "und") != 0)
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
      audioTrackMapper_->setMapping(trackAction, int(numAudioTracks));
    }

    for (unsigned int i = 0; i < numVideoTracks; i++)
    {
      reader->selectVideoTrack(i);
      QString trackName = tr("Track %1").arg(i + 1);

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
      videoTrackMapper_->setMapping(trackAction, int(numVideoTracks));
    }

    for (unsigned int i = 0; i < subsCount; i++)
    {
      QString trackName = tr("Track %1").arg(i + 1);

      TSubsFormat subsFmt = kSubsNone;
      const char * name = reader->subsInfo(i, &subsFmt);
      if (name && *name)
      {
        trackName += tr(", %1").arg(QString::fromUtf8(name));
      }

      if (subsFmt != kSubsNone)
      {
        const char * label = getSubsFormatLabel(subsFmt);
        trackName += tr(", %1").arg(QString::fromUtf8(label));
      }

      QAction * trackAction = new QAction(trackName, this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      trackAction->setChecked(i == 0);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable subs:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      trackAction->setChecked(subsCount == 0);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(subsCount));
    }

    selectVideoTrack(reader, 0);
    selectAudioTrack(reader, 0);
    selectSubsTrack(reader, 0);

    reader_->close();
    stopRenderers();

    bool enableLooping = actionLoop->isChecked();
    reader->setPlaybackLooping(enableLooping);

    // reset timeline start, duration, playhead, in/out points:
    timelineControls_->resetFor(reader);

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepareReaderAndRenderers(reader, playbackPaused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // allow renderers to read from output frame queues:
    resumeRenderers();

    // replace the previous reader:
    reader_->destroy();
    reader_ = reader;

    this->setWindowTitle(tr("Apprentice Video: %1").
                         arg(QFileInfo(path).fileName()));
    actionPlay->setEnabled(true);

    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpenFolder
  //
  void
  MainWindow::fileOpenFolder()
  {
    QString startHere =
      QDesktopServices::storageLocation(QDesktopServices::MoviesLocation
                                        // QDesktopServices::HomeLocation
                                        );

    QString folder =
      QFileDialog::getExistingDirectory(this,
                                        tr("Select a video folder"),
                                        startHere,
                                        QFileDialog::ShowDirsOnly |
                                        QFileDialog::DontResolveSymlinks);
    if (folder.isEmpty())
    {
      return;
    }

    // find all files in the folder, sorted alphabetically
    std::list<QString> playlist;

    QFileInfo fi(folder);
    QString ext = fi.suffix();

    if (!shouldIgnore(ext))
    {
      if (fi.isDir() && ext != kExtEyetv)
      {
        findFilesAndSort(playlist, folder, true);
      }
      else
      {
        playlist.push_back(folder);
      }
    }

    setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpenURL
  //
  void
  MainWindow::fileOpenURL()
  {
    openUrl_->lineEdit->setFocus();
    openUrl_->lineEdit->selectAll();

    int r = openUrl_->exec();
    if (r == QDialog::Accepted)
    {
      QString url = openUrl_->lineEdit->text();

      std::list<QString> playlist;
      playlist.push_back(url);

      bool beginPlaybackImmediately = false;
      setPlaylist(playlist, beginPlaybackImmediately);

      // begin playback:
      playback();
    }
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

    QString startHere =
      QDesktopServices::storageLocation(QDesktopServices::MoviesLocation
                                        // QDesktopServices::HomeLocation
                                        );
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
      QString filename = *i;
      QFileInfo fi(filename);

      if (!fi.isReadable())
      {
        continue;
      }

      if (fi.isDir() && fi.suffix() != kExtEyetv)
      {
        findFilesAndSort(playlist, filename);
      }
      else
      {
        playlist.push_back(filename);
      }
    }

    setPlaylist(playlist);
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
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(0.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatioOther
  //
  void
  MainWindow::playbackAspectRatioOther()
  {
    static AspectRatioDialog * aspectRatioDialog = NULL;
    if (!aspectRatioDialog)
    {
      aspectRatioDialog = new AspectRatioDialog(this);
    }

    double w = 0.0;
    double h = 0.0;
    double dar = canvas_->imageAspectRatio(w, h);
    dar = dar != 0.0 ? dar : 1.777777;
    aspectRatioDialog->doubleSpinBox->setValue(dar);

    int r = aspectRatioDialog->exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    dar = aspectRatioDialog->doubleSpinBox->value();
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(dar);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_40
  //
  void
  MainWindow::playbackAspectRatio2_40()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(2.40);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_35
  //
  void
  MainWindow::playbackAspectRatio2_35()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(2.35);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_85
  //
  void
  MainWindow::playbackAspectRatio1_85()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(1.85);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_78
  //
  void
  MainWindow::playbackAspectRatio1_78()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(16.0 / 9.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_60
  //
  void
  MainWindow::playbackAspectRatio1_60()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(1.6);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_33
  //
  void
  MainWindow::playbackAspectRatio1_33()
  {
    canvasSizeBackup();
    canvas_->overrideDisplayAspectRatio(4.0 / 3.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameNone
  //
  void
  MainWindow::playbackCropFrameNone()
  {
    canvasSizeBackup();
    canvas_->cropFrame(0.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_40
  //
  void
  MainWindow::playbackCropFrame2_40()
  {
    canvasSizeBackup();
    canvas_->cropFrame(2.40);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_35
  //
  void
  MainWindow::playbackCropFrame2_35()
  {
    canvasSizeBackup();
    canvas_->cropFrame(2.35);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_85
  //
  void
  MainWindow::playbackCropFrame1_85()
  {
    canvasSizeBackup();
    canvas_->cropFrame(1.85);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_78
  //
  void
  MainWindow::playbackCropFrame1_78()
  {
    canvasSizeBackup();
    canvas_->cropFrame(16.0 / 9.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_60
  //
  void
  MainWindow::playbackCropFrame1_60()
  {
    canvasSizeBackup();
    canvas_->cropFrame(1.6);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_33
  //
  void
  MainWindow::playbackCropFrame1_33()
  {
    canvasSizeBackup();
    canvas_->cropFrame(4.0 / 3.0);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameAutoDetect
  //
  void
  MainWindow::playbackCropFrameAutoDetect()
  {
    canvasSizeBackup();
    canvas_->cropAutoDetect(this, &(MainWindow::autoCropCallback));
  }

  //----------------------------------------------------------------
  // TIgnoreClockStop
  //
  struct TIgnoreClockStop
  {
    TIgnoreClockStop(TimelineControls * tc):
      tc_(tc)
    {
      count_++;
      YAE_ASSERT(count_ < 2);

      if (count_ < 2)
      {
        tc_->ignoreClockStoppedEvent(true);
      }
    }

    ~TIgnoreClockStop()
    {
      count_--;
      if (!count_)
      {
        tc_->ignoreClockStoppedEvent(false);
      }
    }

  private:
    TIgnoreClockStop(const TIgnoreClockStop &);
    TIgnoreClockStop & operator = (const TIgnoreClockStop &);

    static int count_;
    TimelineControls * tc_;
  };

  //----------------------------------------------------------------
  // TIgnoreClockStop::count_
  //
  int
  TIgnoreClockStop::count_ = 0;

  //----------------------------------------------------------------
  // MainWindow::playbackColorConverter
  //
  void
  MainWindow::playbackColorConverter()
  {
    std::cerr << "playbackColorConverter" << std::endl;

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    selectVideoTrack(reader_, videoTrack);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoopFilter
  //
  void
  MainWindow::playbackLoopFilter()
  {
    std::cerr << "playbackLoopFilter" << std::endl;
    reader_->skipLoopFilter(actionSkipLoopFilter->isChecked());
  }

  //----------------------------------------------------------------
  // MainWindow::playbackNonReferenceFrames
  //
  void
  MainWindow::playbackNonReferenceFrames()
  {
    std::cerr << "playbackNonReferenceFrames" << std::endl;
    reader_->skipNonReferenceFrames(actionSkipNonReferenceFrames->isChecked());
  }

  //----------------------------------------------------------------
  // MainWindow::playbackVerticalScaling
  //
  void
  MainWindow::playbackVerticalScaling()
  {
    canvasSizeBackup();
    bool enable = actionVerticalScaling->isChecked();
    canvas_->enableVerticalScaling(enable);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackSetTempo
  //
  void
  MainWindow::playbackSetTempo(int percent)
  {
    tempo_ = double(percent) / 100.0;
    reader_->setTempo(tempo_);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowPlaylist
  //
  void
  MainWindow::playbackShowPlaylist()
  {
    SignalBlocker blockSignals(actionShowPlaylist);

    if (playlistDock_->isVisible())
    {
      actionShowPlaylist->setChecked(false);
      playlistDock_->hide();
    }
    else
    {
      actionShowPlaylist->setChecked(true);
      playlistDock_->show();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowTimeline
  //
  void
  MainWindow::playbackShowTimeline()
  {
    SignalBlocker blockSignals(actionShowTimeline);

    QRect mainGeom = geometry();
    int ctrlHeight = timelineWidgets_->height();
    bool fullScreen = this->isFullScreen();

    if (timelineWidgets_->isVisible())
    {
      actionShowTimeline->setChecked(false);
      timelineWidgets_->hide();

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

      timelineWidgets_->show();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShrinkWrap
  //
  void
  MainWindow::playbackShrinkWrap()
  {
    if (isFullScreen())
    {
      return;
    }

    canvasSizeBackup();

    double scale = std::min<double>(xexpand_, yexpand_);
    canvasSizeSet(scale, scale);
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

    canvasSizeBackup();
    actionFullScreen->setChecked(true);
    actionShrinkWrap->setEnabled(false);
    menuBar()->hide();
    showFullScreen();

    swapShortcuts(shortcutExit_, actionExit);
    swapShortcuts(shortcutFullScreen_, actionFullScreen);
    swapShortcuts(shortcutShowPlaylist_, actionShowPlaylist);
    swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
    swapShortcuts(shortcutPlay_, actionPlay);
    swapShortcuts(shortcutNext_, actionNext);
    swapShortcuts(shortcutPrev_, actionPrev);
    swapShortcuts(shortcutLoop_, actionLoop);
    swapShortcuts(shortcutCropNone_, actionCropFrameNone);
    swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
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
      QTimer::singleShot(100, this, SLOT(canvasSizeRestore()));

      swapShortcuts(shortcutExit_, actionExit);
      swapShortcuts(shortcutFullScreen_, actionFullScreen);
      swapShortcuts(shortcutShowPlaylist_, actionShowPlaylist);
      swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
      swapShortcuts(shortcutPlay_, actionPlay);
      swapShortcuts(shortcutNext_, actionNext);
      swapShortcuts(shortcutPrev_, actionPrev);
      swapShortcuts(shortcutLoop_, actionLoop);
      swapShortcuts(shortcutCropNone_, actionCropFrameNone);
      swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::togglePlayback
  //
  void
  MainWindow::togglePlayback()
  {
    std::cerr << "togglePlayback: "
              << !playbackPaused_ << " -> "
              << playbackPaused_
              << std::endl;

    reader_->setPlaybackInterval(playbackPaused_);

    if (playbackPaused_)
    {
      actionPlay->setText(tr("Pause"));
      prepareReaderAndRenderers(reader_);
      resumeRenderers();
    }
    else
    {
      actionPlay->setText(tr("Play"));
      TIgnoreClockStop ignoreClockStop(timelineControls_);
      stopRenderers();
    }

    playbackPaused_ = !playbackPaused_;
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectDevice
  //
  void
  MainWindow::audioSelectDevice(const QString & audioDevice)
  {
    std::cerr << "audioSelectDevice: "
              << audioDevice.toUtf8().constData() << std::endl;

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    audioDevice_.assign(audioDevice.toUtf8().constData());
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectTrack
  //
  void
  MainWindow::audioSelectTrack(int index)
  {
    std::cerr << "audioSelectTrack: " << index << std::endl;

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectAudioTrack(reader_, index);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::videoSelectTrack
  //
  void
  MainWindow::videoSelectTrack(int index)
  {
    std::cerr << "videoSelectTrack: " << index << std::endl;

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectVideoTrack(reader_, index);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::subsSelectTrack
  //
  void
  MainWindow::subsSelectTrack(int index)
  {
    std::cerr << "subsSelectTrack: " << index << std::endl;

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectSubsTrack(reader_, index);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::playlistItemChanged
  //
  void
  MainWindow::playlistItemChanged(std::size_t index)
  {
    playbackStop();

    PlaylistItem * item = playlistWidget_->lookup(index);
    if (!item)
    {
      canvas_->clear();
    }
    else
    {
      playback();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::windowHalfSize
  //
  void
  MainWindow::windowHalfSize()
  {
    canvasSizeSet(0.5, 0.5);
  }

  //----------------------------------------------------------------
  // MainWindow::windowFullSize
  //
  void
  MainWindow::windowFullSize()
  {
    canvasSizeSet(1.0, 1.0);
  }

  //----------------------------------------------------------------
  // MainWindow::windowDoubleSize
  //
  void
  MainWindow::windowDoubleSize()
  {
    canvasSizeSet(2.0, 2.0);
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
      about->setWindowTitle(tr("Apprentice Video (revision %1)").
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
      QString filename = QFileInfo(i->toLocalFile()).canonicalFilePath();
      QFileInfo fi(filename);

      if (!fi.isReadable())
      {
        continue;
      }

      if (fi.isDir() && fi.suffix() != kExtEyetv)
      {
        findFilesAndSort(playlist, filename);
      }
      else
      {
        playlist.push_back(filename);
      }
    }

    setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // MainWindow::userIsSeeking
  //
  void
  MainWindow::userIsSeeking(bool seeking)
  {
    reader_->setPlaybackInterval(!seeking);

#if 0
    if (seeking && !playbackPaused_)
    {
      playbackInterrupted_ = true;
      togglePlayback();
    }
    else if (!seeking && playbackInterrupted_)
    {
      playbackInterrupted_ = false;
      togglePlayback();
    }
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeIn
  //
  void
  MainWindow::moveTimeIn(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalStart(seconds);

    SignalBlocker blockSignals(actionLoop);
    actionLoop->setChecked(true);
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeOut
  //
  void
  MainWindow::moveTimeOut(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalEnd(seconds);

    SignalBlocker blockSignals(actionLoop);
    actionLoop->setChecked(true);
  }

  //----------------------------------------------------------------
  // MainWindow::movePlayHead
  //
  void
  MainWindow::movePlayHead(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    reader_->seek(seconds);

    if (playbackPaused_)
    {
      bool forOneFrameOnly = true;
      prepareReaderAndRenderers(reader_, forOneFrameOnly);
      resumeRenderers();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::populateAudioDeviceMenu
  //
  void
  MainWindow::populateAudioDeviceMenu()
  {
    menuAudioDevice->clear();
    if (!audioRenderer_)
    {
      return;
    }

    // update the UI:
    delete audioDeviceGroup_;
    audioDeviceGroup_ = new QActionGroup(this);

    delete audioDeviceMapper_;
    audioDeviceMapper_ = new QSignalMapper(this);

    bool ok = connect(audioDeviceMapper_, SIGNAL(mapped(const QString &)),
                      this, SLOT(audioSelectDevice(const QString &)));
    YAE_ASSERT(ok);

    std::string devName;
    unsigned int defaultDev = audioRenderer_->getDefaultDeviceIndex();
    audioRenderer_->getDeviceName(defaultDev, devName);

    std::size_t numDevices = audioRenderer_->countAvailableDevices();
    unsigned int deviceIndex = audioRenderer_->getDeviceIndex(audioDevice_);
    if (deviceIndex >= numDevices)
    {
      // select new default output in case original has disappeared:
      audioRenderer_->getDeviceName(defaultDev, audioDevice_);
    }

    for (std::size_t i = 0; i < numDevices; i++)
    {
      audioRenderer_->getDeviceName((unsigned int)i, devName);

      QString device = QString::fromUtf8(devName.c_str());
      QAction * deviceAction = new QAction(device, this);
      menuAudioDevice->addAction(deviceAction);
      deviceAction->setCheckable(true);
      deviceAction->setChecked(devName == audioDevice_);
      audioDeviceGroup_->addAction(deviceAction);

      ok = connect(deviceAction, SIGNAL(triggered()),
                   audioDeviceMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioDeviceMapper_->setMapping(deviceAction, device);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::focusChanged
  //
  void
  MainWindow::focusChanged(QWidget * prev, QWidget * curr)
  {
    std::cerr << "focus changed: " << prev << " -> " << curr;
    if (curr)
    {
      std::cerr << ", " << curr->objectName().toUtf8().constData()
                << " (" << curr->metaObject()->className() << ")";
    }
    std::cerr << std::endl;
#ifdef __APPLE__
    if (!appleRemoteControl_ && curr)
    {
      appleRemoteControl_ =
        appleRemoteControlOpen(true, // exclusive
                               false, // count clicks
                               false, // simulate hold
                               &MainWindow::appleRemoteControlObserver,
                               this);
    }
    else if (appleRemoteControl_ && !curr)
    {
      appleRemoteControlClose(appleRemoteControl_);
      appleRemoteControl_ = NULL;
    }
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::playbackFinished
  //
  void
  MainWindow::playbackFinished()
  {
    playbackStop();
    playbackNext();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackStop
  //
  void
  MainWindow::playbackStop()
  {
    ReaderFFMPEG * reader = ReaderFFMPEG::create();
    timelineControls_->observe(SharedClock());
    timelineControls_->resetFor(reader);

    reader_->close();
    stopRenderers();

    reader_->destroy();
    reader_ = reader;

    timelineControls_->update();

    this->setWindowTitle(tr("Apprentice Video"));
  }

  //----------------------------------------------------------------
  // MainWindow::playback
  //
  void
  MainWindow::playback(bool forward)
  {
    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t current = playlistWidget_->currentItem();
    PlaylistItem * item = NULL;
    bool ok = false;

    while ((item = playlistWidget_->lookup(current)))
    {
      if (load(item->path_))
      {
        ok = true;
        break;
      }

      if (forward)
      {
        current = playlistWidget_->closestItem(current + 1);
      }
      else
      {
        current = playlistWidget_->closestItem(current - 1,
                                               PlaylistWidget::kBehind);
      }

      playlistWidget_->setCurrentItem(current);
    }

    fixupNextPrev();

    if (!ok && !forward)
    {
      playback(true);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fixupNextPrev
  //
  void
  MainWindow::fixupNextPrev()
  {
    std::size_t index = playlistWidget_->currentItem();
    std::size_t nNext = playlistWidget_->countItemsAhead();
    std::size_t nPrev = playlistWidget_->countItemsBehind();
    std::size_t iNext = playlistWidget_->closestItem(index + 1);
    std::size_t iPrev = playlistWidget_->closestItem(index - 1,
                                                     PlaylistWidget::kBehind);

    PlaylistItem * prev =
      nPrev && iPrev < index ? playlistWidget_->lookup(iPrev) : NULL;

    PlaylistItem * next =
      nNext && iNext > index ? playlistWidget_->lookup(iNext) : NULL;

    actionPrev->setEnabled(iPrev < index);
    actionNext->setEnabled(iNext > index);

    if (prev)
    {
      actionPrev->setText(tr("Go Back To %1").arg(prev->name_));
    }
    else
    {
      actionPrev->setText(tr("Go Back"));
    }

    if (next)
    {
      actionNext->setText(tr("Skip To %1").arg(next->name_));
    }
    else
    {
      actionNext->setText(tr("Skip"));
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playbackNext
  //
  void
  MainWindow::playbackNext()
  {
    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t index = playlistWidget_->currentItem();
    std::size_t iNext = playlistWidget_->closestItem(index + 1);
    if (iNext > index)
    {
      playlistWidget_->setCurrentItem(iNext);
    }

    playback(true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackPrev
  //
  void
  MainWindow::playbackPrev()
  {
    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t index = playlistWidget_->currentItem();
    std::size_t iPrev = playlistWidget_->closestItem(index - 1,
                                                     PlaylistWidget::kBehind);
    if (iPrev < index)
    {
      playlistWidget_->setCurrentItem(iPrev);
    }

    playback(false);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoop
  //
  void
  MainWindow::playbackLoop()
  {
    if (reader_)
    {
      bool enableLooping = actionLoop->isChecked();
      reader_->setPlaybackLooping(enableLooping);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::scrollWheelTimerExpired
  //
  void
  MainWindow::scrollWheelTimerExpired()
  {
    double seconds = scrollStart_ + scrollOffset_;

    bool isLooping = actionLoop->isChecked();
    if (isLooping)
    {
      double t0 = timelineControls_->timelineStart();
      double dt = timelineControls_->timelineDuration();
      seconds = t0 + fmod(seconds - t0, dt);
    }

    timelineControls_->seekTo(seconds);
  }

  //----------------------------------------------------------------
  // MainWindow::playlistVisibilityChanged
  //
  void
  MainWindow::playlistVisibilityChanged(bool visible)
  {
    if (actionShowPlaylist->isEnabled())
    {
      actionShowPlaylist->setChecked(visible);
    }

    QTimer::singleShot(1, this, SLOT(canvasSizeRestore()));
  }

  //----------------------------------------------------------------
  // MainWindow::event
  //
  bool
  MainWindow::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      AutoCropEvent * ac = dynamic_cast<AutoCropEvent *>(e);
      if (ac)
      {
        ac->accept();
        canvas_->cropFrame(ac->cropFrame_);
        canvasSizeSet(1.0, 1.0);
        return true;
      }

#ifdef __APPLE__
      RemoteControlEvent * rc = dynamic_cast<RemoteControlEvent *>(e);
      if (rc)
      {
        rc->accept();
        std::cerr << "remote control: " << rc->buttonId_
                  << ", down: " << rc->pressedDown_
                  << ", clicks: " << rc->clickCount_
                  << ", held down: " << rc->heldDown_
                  << std::endl;

        if (rc->buttonId_ == kRemoteControlPlayButton)
        {
          if (rc->pressedDown_ && !rc->heldDown_)
          {
            togglePlayback();
          }
        }
        else if (rc->buttonId_ == kRemoteControlMenuButton)
        {
          if (rc->pressedDown_)
          {
            if (rc->heldDown_)
            {
              playbackShowTimeline();
            }
            else
            {
              playbackFullScreen();
            }
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeUp)
        {
          if (rc->pressedDown_)
          {
            // raise the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume + 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeDown)
        {
          if (rc->pressedDown_)
          {
            // lower the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume - 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlLeftButton ||
                 rc->buttonId_ == kRemoteControlRightButton)
        {
          if (rc->pressedDown_)
          {
            double offset =
              (rc->buttonId_ == kRemoteControlLeftButton) ? -3.0 : 7.0;

            timelineControls_->seekFromCurrentTime(offset);
          }
        }

        return true;
      }
#endif
    }

    return QMainWindow::event(e);
  }

  //----------------------------------------------------------------
  // MainWindow::wheelEvent
  //
  void
  MainWindow::wheelEvent(QWheelEvent * e)
  {
    // seek back and forth here:
    int delta = e->delta();
    double percent = floor(0.5 + fabs(double(delta)) / 120.0);
    percent = std::max<double>(1.0, percent);
    double offset = percent * ((delta < 0) ? 5.0 : -5.0);

    if (!scrollWheelTimer_.isActive())
    {
      scrollStart_ = timelineControls_->currentTime();
      scrollOffset_ = 0.0;
    }

    scrollOffset_ += offset;
    scrollWheelTimer_.start(200);
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
  // MainWindow::mousePressEvent
  //
  void
  MainWindow::mousePressEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::RightButton)
    {
      std::size_t numVideoTracks = reader_->getNumberOfVideoTracks();
      std::size_t numAudioTracks = reader_->getNumberOfAudioTracks();

      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      // populate the context menu:
      contextMenu_->clear();
      contextMenu_->addAction(actionPlay);

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionPrev);
      contextMenu_->addAction(actionNext);
      contextMenu_->addAction(actionShowPlaylist);

      if (playlistWidget_->underMouse() &&
          playlistWidget_->countItems())
      {
        contextMenu_->addSeparator();
        contextMenu_->addAction(actionRemove_);
        contextMenu_->addAction(actionSelectAll_);
      }

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionLoop);
      contextMenu_->addAction(actionSetInPoint);
      contextMenu_->addAction(actionSetOutPoint);
      contextMenu_->addAction(actionShowTimeline);

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionShrinkWrap);
      contextMenu_->addAction(actionFullScreen);
      contextMenu_->addAction(menuPlaybackSpeed->menuAction());

      if (numVideoTracks || numAudioTracks)
      {
        contextMenu_->addSeparator();

        if (numAudioTracks)
        {
          contextMenu_->addAction(menuAudio->menuAction());
        }

        if (numVideoTracks)
        {
          contextMenu_->addAction(menuVideo->menuAction());
          contextMenu_->addAction(menuSubs->menuAction());
        }
      }

      contextMenu_->popup(globalPt);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::resizeEvent
  //
  void
  MainWindow::resizeEvent(QResizeEvent * e)
  {
    if (!canvas_)
    {
      return;
    }

    // QTimer::singleShot(1, this, SLOT(canvasSizeBackup()));
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeBackup
  //
  void
  MainWindow::canvasSizeBackup()
  {
    if (isFullScreen())
    {
      return;
    }

    int vw = int(0.5 + canvas_->imageWidth());
    int vh = int(0.5 + canvas_->imageHeight());
    if (vw < 1 || vh < 1)
    {
      return;
    }

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);
    std::cerr << "\ncanvas size backup: " << xexpand_ << ", " << yexpand_
              << std::endl;
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeRestore
  //
  void
  MainWindow::canvasSizeRestore()
  {
    canvasSizeSet(xexpand_, yexpand_);
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeRestore
  //
  void
  MainWindow::canvasSizeSet(double xexpand, double yexpand)
  {
    xexpand_ = xexpand;
    yexpand_ = yexpand;

    if (isFullScreen())
    {
      return;
    }

    double iw = canvas_->imageWidth();
    double ih = canvas_->imageHeight();

    int vw = int(0.5 + iw);
    int vh = int(0.5 + ih);

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

    // calculate width and height overhead:
    int ox = ww - cw;
    int oy = wh - ch;

    int ideal_w = ox + int(0.5 + vw * xexpand_);
    int ideal_h = oy + int(0.5 + vh * yexpand_);

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
#if 0
    cw = new_w - ox;
    ch = new_h - oy;
    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);
#endif
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

    std::cerr << "\ncanvas size set: " << xexpand_ << ", " << yexpand_
              << std::endl
              << "canvas resize: " << new_w - cdx << ", " << new_h - cdy
              << std::endl
              << "canvas move to: " << new_x << ", " << new_y
              << std::endl;
    resize(new_w - cdx, new_h - cdy);
    // move(new_x, new_y);
  }

  //----------------------------------------------------------------
  // MainWindow::stopRenderers
  //
  void
  MainWindow::stopRenderers()
  {
    videoRenderer_->close();
    audioRenderer_->close();

    videoRenderer_->pause();
    audioRenderer_->pause();
  }

  //----------------------------------------------------------------
  // MainWindow::prepareReaderAndRenderers
  //
  void
  MainWindow::prepareReaderAndRenderers(IReader * reader, bool forOneFrameOnly)
  {
    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t audioTrack = reader->getSelectedAudioTrackIndex();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    SharedClock sharedClock;
    if (!forOneFrameOnly)
    {
      timelineControls_->observe(sharedClock);
    }

    if (!forOneFrameOnly && audioTrack < numAudioTracks)
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
    if (!forOneFrameOnly)
    {
      unsigned int audioDeviceIndex = adjustAudioTraitsOverride(reader);
      if (!audioRenderer_->open(audioDeviceIndex, reader, forOneFrameOnly))
      {
        videoRenderer_->takeThisClock(sharedClock);
        videoRenderer_->obeyThisClock(videoRenderer_->clock());
      }
    }

    videoRenderer_->open(canvas_, reader, forOneFrameOnly);

    if (!forOneFrameOnly)
    {
      timelineControls_->adjustTo(reader);

      // request playback at currently selected playback rate:
      reader->setTempo(tempo_);

      bool deint = actionDeinterlace->isChecked();
      reader->setDeinterlacing(deint);
    }
    else
    {
      // disable audio tempo filtering:
      reader->setTempo(1.0);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::resumeRenderers
  //
  void
  MainWindow::resumeRenderers()
  {
    audioRenderer_->resume();
    videoRenderer_->resume();
  }

  //----------------------------------------------------------------
  // MainWindow::selectVideoTrack
  //
  void
  MainWindow::selectVideoTrack(IReader * reader, std::size_t videoTrackIndex)
  {
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

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
                       !actionSkipColorConverter->isChecked());
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
            if (glewIsExtensionSupported("GL_APPLE_ycbcr_422"))
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
#elif 1
      vtts.pixelFormat_ = kPixelFormatYUV420P9;
      reader->setVideoTraitsOverride(vtts);
      canvas_->cropAutoDetect(this, &(MainWindow::autoCropCallback));
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

    if (!numVideoTracks && numAudioTracks > 0)
    {
      if (actionShowPlaylist->isEnabled())
      {
        shortcutShowPlaylist_->setEnabled(false);
        actionShowPlaylist->setEnabled(false);
        actionShrinkWrap->setEnabled(false);
        actionFullScreen->setEnabled(false);

        if (actionShowPlaylist->isChecked())
        {
          playlistDock_->hide();
        }

        swapLayouts(canvasContainer_, playlistContainer_);
        menubar->removeAction(menuVideo->menuAction());
        menubar->removeAction(menuSubs->menuAction());

        playlistWidget_->show();
        playlistWidget_->update();
        playlistWidget_->setFocus();
      }
    }
    else
    {
      if (!actionShowPlaylist->isEnabled())
      {
        swapLayouts(canvasContainer_, playlistContainer_);
        menubar->insertMenu(menuHelp->menuAction(), menuVideo);
        menubar->insertMenu(menuHelp->menuAction(), menuSubs);

        if (actionShowPlaylist->isChecked())
        {
          playlistDock_->show();
        }

        shortcutShowPlaylist_->setEnabled(true);
        actionShowPlaylist->setEnabled(true);
        actionShrinkWrap->setEnabled(true);
        actionFullScreen->setEnabled(true);

        playlistWidget_->show();
        playlistWidget_->update();
        this->setFocus();
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
     adjustAudioTraitsOverride(reader);
  }

  //----------------------------------------------------------------
  // MainWindow::selectSubsTrack
  //
  void
  MainWindow::selectSubsTrack(IReader * reader, std::size_t subsTrackIndex)
  {
    const std::size_t nsubs = reader->subsCount();
    for (std::size_t i = 0; i < nsubs; i++)
    {
      bool enable = (i == subsTrackIndex);
      reader->subsRender(i, enable);
    }

    canvas_->setSubs(std::list<TSubsFrame>());
  }

  //----------------------------------------------------------------
  // MainWindow::adjustAudioTraitsOverride
  //
  unsigned int
  MainWindow::adjustAudioTraitsOverride(IReader * reader)
  {
     unsigned int numDevices = audioRenderer_->countAvailableDevices();
     unsigned int deviceIndex = audioRenderer_->getDeviceIndex(audioDevice_);
     if (deviceIndex >= numDevices)
     {
       deviceIndex = audioRenderer_->getDefaultDeviceIndex();
       audioRenderer_->getDeviceName(deviceIndex, audioDevice_);
     }

     AudioTraits native;
     if (reader->getAudioTraits(native))
     {
       AudioTraits supported;
       audioRenderer_->match(deviceIndex, native, supported);

       // FIXME: temporary for debugging on openSuSE
       // supported.channelLayout_ = kAudioStereo;
       std::cerr << "supported: " << supported.channelLayout_ << std::endl
                 << "required:  " << native.channelLayout_ << std::endl;
       reader->setAudioTraitsOverride(supported);
     }

     return deviceIndex;
  }

  //----------------------------------------------------------------
  // MainWindow::autoCropCallback
  //
  void
  MainWindow::autoCropCallback(void * callbackContext, const TCropFrame & cf)
  {
    MainWindow * mainWindow = (MainWindow *)callbackContext;
    qApp->postEvent(mainWindow, new AutoCropEvent(cf));
  }

#ifdef __APPLE__
  //----------------------------------------------------------------
  // appleRemoteControlObserver
  //
  void
  MainWindow::appleRemoteControlObserver(void * observerContext,
                                         TRemoteControlButtonId buttonId,
                                         bool pressedDown,
                                         unsigned int clickCount,
                                         bool heldDown)
  {
    MainWindow * mainWindow = (MainWindow *)observerContext;
    qApp->postEvent(mainWindow, new RemoteControlEvent(buttonId,
                                                       pressedDown,
                                                       clickCount,
                                                       heldDown));
  }
#endif
};

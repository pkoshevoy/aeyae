// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:50:01 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt includes:
#include <QDialog>
#include <QMainWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QShortcut>
#include <QTimer>

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_audio_renderer.h"
#include "yae/video/yae_video_renderer.h"

// local includes:
#ifdef __APPLE__
#include "yaeAppleRemoteControl.h"
#include "yaeAppleUtils.h"
#endif
#include "yaeBookmarks.h"
#ifndef YAE_USE_PLAYER_QUICK_WIDGET
#include "yaeCanvasWidget.h"
#endif
#include "yaePlaylist.h"
#include "yaePlaylistModel.h"
#include "yaePlaylistModelProxy.h"
#include "yaePlaylistView.h"
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
#include "yaeQuickWidget.h"
#endif
#include "yaeTimelineModel.h"
#include "yaeTimelineView.h"

// Qt uic generated files:
#include "ui_yaeAbout.h"
#include "ui_yaeAspectRatioDialog.h"
#include "ui_yaeMainWindow.h"
#include "ui_yaeOpenUrlDialog.h"


namespace yae
{
  //----------------------------------------------------------------
  // TPlayerWidget
  //
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
  typedef TQuickWidget TPlayerWidget;
#elif defined(YAE_USE_QOPENGL_WIDGET)
  typedef CanvasWidget<QOpenGLWidget> TPlayerWidget;
#else
  typedef CanvasWidget<QGLWidget> TPlayerWidget;
#endif

  //----------------------------------------------------------------
  // AboutDialog
  //
  class AboutDialog : public QDialog,
                      public Ui::AboutDialog
  {
    Q_OBJECT;

  public:
    AboutDialog(QWidget * parent = 0);
  };


  //----------------------------------------------------------------
  // AspectRatioDialog
  //
  class AspectRatioDialog : public QDialog,
                            public Ui::AspectRatioDialog
  {
    Q_OBJECT;

  public:
    AspectRatioDialog(QWidget * parent = 0);
  };


  //----------------------------------------------------------------
  // OpenUrlDialog
  //
  class OpenUrlDialog : public QDialog,
                        public Ui::OpenUrlDialog
  {
    Q_OBJECT;

  public:
    OpenUrlDialog(QWidget * parent = 0);
  };

  //----------------------------------------------------------------
  // PlaylistBookmark
  //
  struct PlaylistBookmark : public TBookmark
  {
    PlaylistBookmark();

    TPlaylistItemPtr item_;
    QAction * action_;
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
  // MainWindow
  //
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;

  public:
    MainWindow(const IReaderPtr & readerPrototype);
    ~MainWindow();

    void initPlayerWidget();

    // accessor to the OpenGL rendering canvas:
    Canvas * canvas() const;

  protected:
    // open a movie file for playback:
    bool load(const QString & path, const TBookmark * bookmark = NULL);

  public:
    // specify a playlist of files to load:
    void setPlaylist(const std::list<QString> & playlist,
                     bool beginPlaybackImmediately = true);

    inline bool isPlaybackPaused() const
    { return playbackPaused_; }

    inline bool isPlaylistVisible() const
    { return actionShowPlaylist->isChecked(); }

  signals:
    void setInPoint();
    void setOutPoint();

  public slots:
    // file menu:
    void fileOpen();
    void fileOpenURL();
    void fileOpenFolder();
    void fileExit();

    // bookmarks menu:
    void bookmarksAutomatic();
    void bookmarksPopulate();
    void bookmarksRemoveNowPlaying();
    void bookmarksRemove();
    void bookmarksResumePlayback();
    void bookmarksSelectItem(int index);

    // playback menu:
    void playbackAspectRatioAuto();
    void playbackAspectRatioOther();
    void playbackAspectRatio2_40();
    void playbackAspectRatio2_35();
    void playbackAspectRatio1_85();
    void playbackAspectRatio1_78();
    void playbackAspectRatio1_60();
    void playbackAspectRatio1_33();
    void playbackCropFrameNone();
    void playbackCropFrame2_40();
    void playbackCropFrame2_35();
    void playbackCropFrame1_85();
    void playbackCropFrame1_78();
    void playbackCropFrame1_60();
    void playbackCropFrame1_33();
    void playbackCropFrameAutoDetect();
    void playbackNext();
    void playbackPrev();
    void playbackLoop();
    void playbackVerticalScaling();
    void playbackColorConverter();
    void playbackLoopFilter();
    void playbackNonReferenceFrames();
    void playbackShowPlaylist();
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void playbackSetTempo(int percent);

    // helper:
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();
    void exitPlaylist();
    void togglePlaylist();
    void togglePlayback();
    void skipToInPoint();
    void skipToOutPoint();
    void skipToNextFrame();
    void skipForward();
    void skipBack();

    // audio/video menus:
    void audioDownmixToStereo();
    void audioSelectDevice(const QString & audioDevice);
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);
    void subsSelectTrack(int index);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();

    // chapters menu:
    void updateChaptersMenu();
    void skipToNextChapter();
    void skipToChapter(int index);

    // help menu:
    void helpAbout();

    // helpers:
    void setPlayingItem(const QModelIndex & index);
    void processDropEventUrls(const QList<QUrl> & urls);
    void userIsSeeking(bool seeking);
    void moveTimeIn(double seconds);
    void moveTimeOut(double seconds);
    void movePlayHead(double seconds);
    void populateAudioDeviceMenu();
    void focusChanged(QWidget * prev, QWidget * curr);
    void playbackFinished(const SharedClock & c);
    void playbackStop();
    void playback(bool forward = true);
    void playback(const QModelIndex & index, bool forward = true);
    void scrollWheelTimerExpired();
    void fixupNextPrev();
    void adjustCanvasHeight();
    void canvasSizeBackup();
    void canvasSizeRestore();
    void saveBookmark();
    void gotoBookmark(const PlaylistBookmark & bookmark);

    bool findBookmark(const std::string & groupHash,
                      PlaylistBookmark & bookmark) const;

    bool findBookmark(const TPlaylistItemPtr & item,
                      PlaylistBookmark & bookmark) const;

  protected:
    // virtual:
    bool event(QEvent * e);
    void wheelEvent(QWheelEvent * e);
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // helpers:
    void canvasSizeSet(double xexpand, double yexpand);
    void stopRenderers();
    void prepareReaderAndRenderers(IReader * reader, bool oneFrame = false);
    void resumeRenderers(bool loadNextFrameIfPaused = false);
    void renderOneFrame();
    void selectVideoTrack(IReader * reader, std::size_t videoTrackIndex);
    void selectAudioTrack(IReader * reader, std::size_t audioTrackIndex);
    void selectSubsTrack(IReader * reader, std::size_t subsTrackIndex);
    void adjustMenuActions(IReader * reader,
                           std::vector<TTrackInfo> & audioInfo,
                           std::vector<AudioTraits> & audioTraits,
                           std::vector<TTrackInfo> & videoInfo,
                           std::vector<VideoTraits> & videoTraits,
                           std::vector<TTrackInfo> & subsInfo,
                           std::vector<TSubsFormat> & subsFormat);
    void adjustMenuActions();
    void adjustMenus(IReader * reader);
    void swapShortcuts();

    unsigned int adjustAudioTraitsOverride(IReader * reader);

    static TVideoFramePtr autoCropCallback(void * context,
                                           const TCropFrame & detected,
                                           bool detectionFinished);

#ifdef __APPLE__
    // for Apple Remote:
    static void appleRemoteControlObserver(void * observerContext,
                                           TRemoteControlButtonId buttonId,
                                           bool pressedDown,
                                           unsigned int clickCount,
                                           bool heldDown);
    void * appleRemoteControl_;
#endif

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutFillScreen_;
    QShortcut * shortcutShowPlaylist_;
    QShortcut * shortcutPlay_;
    QShortcut * shortcutNext_;
    QShortcut * shortcutPrev_;
    QShortcut * shortcutLoop_;
    QShortcut * shortcutCropNone_;
    QShortcut * shortcutAutoCrop_;
    QShortcut * shortcutCrop1_33_;
    QShortcut * shortcutCrop1_78_;
    QShortcut * shortcutCrop1_85_;
    QShortcut * shortcutCrop2_40_;
    QShortcut * shortcutNextChapter_;
    QShortcut * shortcutAspectRatioNone_;
    QShortcut * shortcutAspectRatio1_33_;
    QShortcut * shortcutAspectRatio1_78_;

    // playlist shortcuts:
    QAction * actionRemove_;
    QAction * actionSelectAll_;
    QShortcut * shortcutRemove_;
    QShortcut * shortcutSelectAll_;

    // audio device selection widget:
    QActionGroup * audioDeviceGroup_;
    QSignalMapper * audioDeviceMapper_;

    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    QActionGroup * subsTrackGroup_;
    QActionGroup * chaptersGroup_;

    QSignalMapper * audioTrackMapper_;
    QSignalMapper * videoTrackMapper_;
    QSignalMapper * subsTrackMapper_;
    QSignalMapper * chapterMapper_;

    // bookmark selection mechanism:
    std::vector<PlaylistBookmark> bookmarks_;
    QActionGroup * bookmarksGroup_;
    QSignalMapper * bookmarksMapper_;
    QAction * bookmarksMenuSeparator_;

    // file reader prototype factory instance:
    IReaderPtr readerPrototype_;

    // file reader:
    IReaderPtr reader_;
    unsigned int readerId_;

    // frame canvas:
    TPlayerWidget * playerWidget_;
    Canvas * canvas_;
    PlaylistView playlistView_;
    TimelineView timelineView_;
    TimelineModel timelineModel_;
    TPlaylistModel playlistModel_;

    // audio device:
    std::string audioDevice_;

    // audio renderer:
    IAudioRenderer * audioRenderer_;

    // video renderer:
    VideoRenderer * videoRenderer_;

    // a flag indicating whether playback is paused:
    bool playbackPaused_;

    // scroll-wheel timer:
    QTimer scrollWheelTimer_;
    double scrollStart_;
    double scrollOffset_;

    // dialog for opening a URL resource:
    OpenUrlDialog * openUrl_;

    // remember most recently used full screen render mode:
    Canvas::TRenderMode renderMode_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;

    // desired playback tempo:
    double tempo_;

    // selected track info is used to select matching track(s)
    // after loading next file in the playlist:
    TTrackInfo selVideo_;
    VideoTraits selVideoTraits_;

    TTrackInfo selAudio_;
    AudioTraits selAudioTraits_;

    TTrackInfo selSubs_;
    TSubsFormat selSubsFormat_;

    // auto-crop single shot timer:
    QTimer autocropTimer_;

    // auto-bookmark timer:
    QTimer bookmarkTimer_;
  };
}


#endif // YAE_MAIN_WINDOW_H_

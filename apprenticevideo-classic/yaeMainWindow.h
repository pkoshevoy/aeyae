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

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_json.h"
#include "yae/video/yae_audio_renderer.h"
#include "yae/video/yae_reader_factory.h"
#include "yae/video/yae_video_renderer.h"

// yaeui:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaeCanvasWidget.h"

// local:
#include "yaeBookmarks.h"
#include "yaeMainView.h"
#include "yaeTimelineControls.h"
#include "ui_yaeAbout.h"
#include "ui_yaeAspectRatioDialog.h"
#include "ui_yaeMainWindow.h"
#include "ui_yaeOpenUrlDialog.h"


namespace yae
{

  //----------------------------------------------------------------
  // TCanvasWidget
  //
#ifdef YAE_USE_QGL_WIDGET
  typedef CanvasWidget<QGLWidget> TCanvasWidget;
#else
  typedef CanvasWidget<QOpenGLWidget> TCanvasWidget;
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

    std::size_t itemIndex_;
    QAction * action_;
  };

  //----------------------------------------------------------------
  // MainWindow
  //
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;

  public:
    MainWindow(const TReaderFactoryPtr & readerFactory);
    ~MainWindow();

    void initItemViews();

    // accessor to the OpenGL rendering canvas:
    inline const TCanvasWidget & canvas() const
    { return *canvas_; }

    inline TCanvasWidget & canvas()
    { return *canvas_; }

  protected:
    // open a movie file for playback:
    bool load(const QString & path, const TBookmark * bookmark = NULL);

  public:
    // specify a playlist of files to load:
    void setPlaylist(const std::list<QString> & playlist,
                     bool beginPlaybackImmediately = true);

  signals:
    void setInPoint();
    void setOutPoint();

  public slots:
    void setPlaylist(const QString & filename);

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
    void playbackShowTimeline();
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void playbackSetTempo(int percent);

    // helper:
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();
    void togglePlayback();

    // audio/video menus:
    void audioDownmixToStereo();
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);
    void subsSelectTrack(int index);
    void playlistItemChanged(std::size_t index);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();
    void windowDecreaseSize();
    void windowIncreaseSize();

    // chapters menu:
    void updateChaptersMenu();
    void skipToNextChapter();
    void skipToChapter(int index);

    // help menu:
    void helpAbout();

    // helpers:
    void processDropEventUrls(const QList<QUrl> & urls);
    void userIsSeeking(bool seeking);
    void moveTimeIn(double seconds);
    void moveTimeOut(double seconds);
    void movePlayHead(double seconds);
    void updateTimelineDuration();
    void playbackFinished(const SharedClock & c);
    void playbackStop();
    void playback(bool forward = true);
    void scrollWheelTimerExpired();
    void playlistVisibilityChanged(bool visible);
    void fixupNextPrev();
    void adjustCanvasHeight();
    void canvasSizeScaleBy(double scale);
    void canvasSizeBackup();
    void canvasSizeRestore();
    void saveBookmark();
    void gotoBookmark(const PlaylistBookmark & bookmark);

    bool findBookmark(const std::string & groupHash,
                      PlaylistBookmark & bookmark) const;

    bool findBookmark(std::size_t itemIndex,
                      PlaylistBookmark & bookmark) const;

    bool isPlaybackPaused() const;

    // IEventObserver handler:
    void handle_reader_event(const IReaderPtr & reader,
                             const Json::Value & event);

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
    void skipToNextFrame();

    void adjustAudioTraitsOverride(IReader * reader);

    static TVideoFramePtr autoCropCallback(void * context,
                                           const TCropFrame & detected,
                                           bool detectionFinished);

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutFillScreen_;
    QShortcut * shortcutShowPlaylist_;
    QShortcut * shortcutShowTimeline_;
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

    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    QActionGroup * subsTrackGroup_;
    QActionGroup * chaptersGroup_;

    yae::SignalMapper * audioTrackMapper_;
    yae::SignalMapper * videoTrackMapper_;
    yae::SignalMapper * subsTrackMapper_;
    yae::SignalMapper * chapterMapper_;

    // bookmark selection mechanism:
    std::vector<PlaylistBookmark> bookmarks_;
    QActionGroup * bookmarksGroup_;
    yae::SignalMapper * bookmarksMapper_;
    QAction * bookmarksMenuSeparator_;

    // file reader prototype factory instance:
    TReaderFactoryPtr readerFactory_;

    // file reader:
    IReaderPtr reader_;
    unsigned int readerId_;

    // frame canvas:
    TCanvasWidget * canvas_;

    // audio renderer:
    TAudioRendererPtr audioRenderer_;

    // video renderer:
    TVideoRendererPtr videoRenderer_;

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
    std::size_t selClosedCaptions_;

    // update DVR live recordings duration:
    QTimer durationTimer_;

    // auto-crop single shot timer:
    QTimer autocropTimer_;

    // auto-bookmark timer:
    QTimer bookmarkTimer_;

    // player controls view:
    MainView view_;
    ItemViewStyle style_;
  };
}


#endif // YAE_MAIN_WINDOW_H_

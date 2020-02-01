// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 22:22:54 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_WINDOW_H_
#define YAE_PLAYER_WINDOW_H_

// Qt:
#include <QDialog>
#include <QWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QShortcut>

// local:
#include "yaePlayerView.h"
#ifdef __APPLE__
#include "yaeAppleRemoteControl.h"
#include "yaeAppleUtils.h"
#endif
#include "yaeCanvasWidget.h"
#include "yaeFrameCropView.h"

// uic:
#include "ui_yaeAspectRatioDialog.h"
#include "ui_yaePlayerWindow.h"


namespace yae
{
  // forward declarations:
  class PlayerWindow;

  //----------------------------------------------------------------
  // TCanvasWidget
  //
#if defined(YAE_USE_QOPENGL_WIDGET)
  typedef CanvasWidget<QOpenGLWidget> TCanvasWidget;
#else
  typedef CanvasWidget<QGLWidget> TCanvasWidget;
#endif


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
  // PlayerWindow
  //
  class PlayerWindow : public QWindow,
                       public Ui::yaePlayerWindow
  {
    Q_OBJECT;

  public:
    PlayerWindow(const IReaderPtr & readerPrototype);
    ~PlayerWindow();

    void initPlayerWidget();
    void initItemViews();

    // accessor to the player widget:
    inline TCanvasWidget * canvasWidget() const
    { return canvasWidget_; }

    // accessor to the OpenGL rendering canvas:
    Canvas * canvas() const;

  protected:
    // open a movie file for playback:
    bool load(const QString & path, const TBookmark * bookmark = NULL);

  public:
    inline bool isPlaybackPaused() const
    { return playbackPaused_; }

    inline bool isTimelineVisible() const
    { return actionShowTimeline->isChecked(); }

  signals:
    void setInPoint();
    void setOutPoint();

  public slots:
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
    void playbackCropFrameOther();
    void playbackCropFrameAutoDetect();

    void playbackLoop();
    void playbackVerticalScaling();
    void playbackColorConverter();
    void playbackLoopFilter();
    void playbackNonReferenceFrames();
    void playbackDeinterlacing();
    void playbackShowTimeline();
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void playbackSetTempo(int percent);

    // helper:
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();
    void togglePlayback();
    void skipToInPoint();
    void skipToOutPoint();
    void skipToNextFrame();
    void skipForward();
    void skipBack();

    // audio/video menus:
    void audioDownmixToStereo();
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);
    void subsSelectTrack(int index);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();
    void windowDecreaseSize();
    void windowIncreaseSize();

    // helpers:
    void focusChanged(QWidget * prev, QWidget * curr);
    void playbackFinished(const SharedClock & c);
    void playbackStop();
    void scrollWheelTimerExpired();
    void adjustCanvasHeight();
    void canvasSizeBackup();
    void canvasSizeRestore();
    void saveBookmark();
    void gotoBookmark(const PlayerBookmark & bookmark);

    bool findBookmark(const std::string & groupHash,
                      PlayerBookmark & bookmark) const;

    bool findBookmark(const TPlaylistItemPtr & item,
                      PlayerBookmark & bookmark) const;

    void cropped(const TVideoFramePtr & frame, const TCropFrame & crop);
    void dismissFrameCropView();

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

    void adjustAudioTraitsOverride(IReader * reader);

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
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutFillScreen_;
    QShortcut * shortcutShowTimeline_;

    QShortcut * shortcutPlay_;
    QShortcut * shortcutLoop_;

    QShortcut * shortcutCropNone_;
    QShortcut * shortcutAutoCrop_;
    QShortcut * shortcutCrop1_33_;
    QShortcut * shortcutCrop1_78_;
    QShortcut * shortcutCrop1_85_;
    QShortcut * shortcutCrop2_40_;
    QShortcut * shortcutCropOther_;

    QShortcut * shortcutAspectRatioNone_;
    QShortcut * shortcutAspectRatio1_33_;
    QShortcut * shortcutAspectRatio1_78_;

    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    QActionGroup * subsTrackGroup_;

    QSignalMapper * playRateMapper_;
    QSignalMapper * audioTrackMapper_;
    QSignalMapper * videoTrackMapper_;
    QSignalMapper * subsTrackMapper_;

    // file reader prototype factory instance:
    IReaderPtr readerPrototype_;

    // file reader:
    IReaderPtr reader_;
    unsigned int readerId_;

    // frame canvas:
    TCanvasWidget * canvasWidget_;
    Canvas * canvas_;
    PlayerView playerView_;

    // frame editing view:
    FrameCropView frameCropView_;
    yae::shared_ptr<Canvas::ILoadFrameObserver> onLoadFrame_;

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


#endif // YAE_PLAYER_WINDOW_H_

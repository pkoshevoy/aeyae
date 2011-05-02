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
#include <QSignalMapper>
#include <QShortcut>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>
#include <yaeReader.h>
#include <yaeAudioRenderer.h>
#include <yaeVideoRenderer.h>
#include <yaePlaybackControls.h>

// local includes:
#include "ui_yaeMainWindow.h"
#include "ui_yaeAbout.h"


namespace yae
{

  //----------------------------------------------------------------
  // AboutDialog
  // 
  class AboutDialog : public QDialog,
                      public Ui::AboutDialog
  {
    Q_OBJECT;
    
  public:
    AboutDialog(QWidget * parent = 0, Qt::WFlags f = 0);
  };
  
  
  //----------------------------------------------------------------
  // MainWindow
  // 
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;
    
  public:
    MainWindow();
    ~MainWindow();
    
    // accessor to the OpenGL rendering canvas:
    Canvas * canvas() const;
  
    // open a movie file for playback:
    bool load(const QString & path);
    
  public slots:
    // file menu:
    void fileOpen();
    void fileExit();

    // playback menu:
    void playbackAspectRatioAuto();
    void playbackAspectRatio2_35();
    void playbackAspectRatio1_85();
    void playbackAspectRatio1_78();
    void playbackAspectRatio1_33();
    void playbackCropFrameNone();
    void playbackCropFrame2_35();
    void playbackCropFrame1_85();
    void playbackCropFrame1_78();
    void playbackCropFrame1_33();
    void playbackColorConverter();
    void playbackShowControls();
    void playbackShrinkWrap();
    void playbackFullScreen();
    
    // helper:
    void exitFullScreen();
    void playbackPause();
    
    // audio/video menus:
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);

    // help menu:
    void helpAbout();
    
    // helper:
    void processDropEventUrls(const QList<QUrl> & urls);
    
  protected:
    // virtual:
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    
    // helpers:
    void stopRenderers();
    void startRenderers(IReader * reader);
    void selectVideoTrack(IReader * reader, std::size_t videoTrackIndex);
    void selectAudioTrack(IReader * reader, std::size_t audioTrackIndex);
    
    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutShowControls_;
    
    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    
    QSignalMapper * audioTrackMapper_;
    QSignalMapper * videoTrackMapper_;
    
    // file reader:
    IReader * reader_;
    
    // frame canvas:
    Canvas * canvas_;
    
    // audio renderer:
    IAudioRenderer * audioRenderer_;
    
    // video renderer:
    VideoRenderer * videoRenderer_;
    
    // a flag indicating whether playback is paused:
    bool playbackPaused_;
    
    // playback controls:
    PlaybackControls * playbackControls_;
  };
}


#endif // YAE_MAIN_WINDOW_H_

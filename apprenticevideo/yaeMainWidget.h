// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat May  9 19:43:49 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WIDGET_H_
#define YAE_MAIN_WIDGET_H_

// standard:
#include <list>
#include <vector>

// aeyae:
#include "yae/video/yae_reader_factory.h"

// yaeui:
#include "yaePlayerWidget.h"

// local:
#include "yaeBookmarks.h"
#include "yaePlaylist.h"
#include "yaePlaylistModel.h"
#include "yaePlaylistModelProxy.h"
#include "yaePlaylistView.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistBookmark
  //
  struct PlaylistBookmark : public TBookmark
  {
    PlaylistBookmark():
      TBookmark(),
      action_(NULL)
    {}

    TPlaylistItemPtr item_;
    QAction * action_;
  };


  //----------------------------------------------------------------
  // MainWidget
  //
  class MainWidget : public PlayerWidget
  {
    Q_OBJECT;

  public:
    MainWidget(QWidget * parent = NULL,
               TCanvasWidget * shared_ctx = NULL,
               Qt::WindowFlags f = Qt::WindowFlags());
    ~MainWidget();

  protected:
    void init_actions();
    void translate_ui();

  public:
    // virtual:
    void initItemViews();

    // specify a reader factory for opening files:
    void setReaderFactory(const TReaderFactoryPtr & readerFactory);

    // specify a playlist of files to load:
    void setPlaylist(const std::list<QString> & playlist,
                     bool beginPlaybackImmediately = true);

    /*
    inline bool isPlaybackPaused() const
    { return playbackPaused_; }

    inline bool isPlaylistVisible() const
    { return actionShowPlaylist->isChecked(); }

    inline bool isTimelineVisible() const
    { return actionShowTimeline->isChecked(); }
    */

  signals:
    void adjust_menus(IReaderPtr reader);
    void adjust_playlist_style();

  public slots:

    // bookmarks menu:
    void bookmarksAutomatic();
    void bookmarksPopulate();
    void bookmarksRemoveNowPlaying();
    void bookmarksRemove();
    void bookmarksResumePlayback();
    void bookmarksSelectItem(int index);

    // playback menu:
    void playbackShowPlaylist();

    // helpers:
    void adjustPlaylistStyle();
    void exitPlaylist();
    void togglePlaylist();
    void setPlayingItem(const QModelIndex & index);

    void playbackStop();
    void playbackFinished(TTime playheadPos);
    void playback(bool forward = true);
    void playback(const QModelIndex & index, bool forward = true);
    void fixupNextPrev();

    void playbackNext();
    void playbackPrev();

    void saveBookmark();
    void gotoBookmark(const PlaylistBookmark & bookmark);

    bool findBookmark(const std::string & groupHash,
                      PlaylistBookmark & bookmark) const;

    bool findBookmark(const TPlaylistItemPtr & item,
                      PlaylistBookmark & bookmark) const;

    // virtual:
    void swapShortcuts();
    void populateContextMenu();

  protected:
    // open a movie file for playback:
    bool load(const QString & path, const TBookmark * bookmark = NULL);

  public:
    QMenu * menuBookmarks_;

    QAction * actionAutomaticBookmarks_;
    QAction * actionResumeFromBookmark_;
    QAction * actionRemoveBookmarkNowPlaying_;
    QAction * actionRemoveBookmarks_;

    QAction * actionNext_;
    QAction * actionPrev_;

    QAction * actionRepeatPlaylist_;
    QAction * actionShowPlaylist_;
    QAction * actionSelectAll_;
    QAction * actionRemove_;

    QShortcut * shortcutNext_;
    QShortcut * shortcutPrev_;

    QShortcut * shortcutShowPlaylist_;
    QShortcut * shortcutSelectAll_;
    QShortcut * shortcutRemove_;

  protected:
    // file reader prototype factory instance:
    TReaderFactoryPtr readerFactory_;

    // playlist stuff:
    TPlaylistModel playlistModel_;
    yae::shared_ptr<PlaylistView, ItemView> playlistView_;

    // bookmark selection mechanism:
    std::vector<PlaylistBookmark> bookmarks_;
    QActionGroup * bookmarksGroup_;
    QSignalMapper * bookmarksMapper_;
    QAction * bookmarksMenuSeparator_;
  };

}


#endif // YAE_MAIN_WIDGET_H_

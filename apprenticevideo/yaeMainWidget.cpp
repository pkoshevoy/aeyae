// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat May  9 19:43:49 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <list>

// boost:
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#endif

// Qt includes:
#include <QFileInfo>
#include <QProcess>

// aeyae:
#include "yae/utils/yae_benchmark.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"

// yaeui:
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"

// local:
#include "yaeMainWidget.h"

// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // kCreateBookmarksAutomatically
  //
  static const QString kCreateBookmarksAutomatically =
    QString::fromUtf8("CreateBookmarksAutomatically");

  //----------------------------------------------------------------
  // kResumePlaybackFromBookmark
  //
  static const QString kResumePlaybackFromBookmark =
    QString::fromUtf8("ResumePlaybackFromBookmark");


  //----------------------------------------------------------------
  // PlaylistItemFilePath
  //
  struct PlaylistItemFilePath : ThumbnailProvider::GetFilePath
  {
    PlaylistItemFilePath(const TPlaylistModel & playlist):
      playlist_(playlist)
    {}

    // virtual:
    QString operator()(const QString & id) const
    { return playlist_.lookupItemFilePath(id); }

    const TPlaylistModel & playlist_;
  };


  //----------------------------------------------------------------
  // MainWidget::MainWidget
  //
  MainWidget::MainWidget(QWidget * parent,
                         TCanvasWidget * shared_ctx,
                         Qt::WindowFlags f):
    PlayerWidget(parent, shared_ctx, f),
    menuBookmarks_(NULL),

    actionAutomaticBookmarks_(NULL),
    actionResumeFromBookmark_(NULL),
    actionRemoveBookmarkNowPlaying_(NULL),
    actionRemoveBookmarks_(NULL),

    actionNext_(NULL),
    actionPrev_(NULL),

    actionRepeatPlaylist_(NULL),
    actionShowPlaylist_(NULL),
    actionSelectAll_(NULL),
    actionRemove_(NULL),

    shortcutShowPlaylist_(NULL),
    shortcutSelectAll_(NULL),
    shortcutRemove_(NULL),

    shortcutNext_(NULL),
    shortcutPrev_(NULL),

    bookmarksGroup_(NULL),
    bookmarksMapper_(NULL),
    bookmarksMenuSeparator_(NULL)
  {
    init_actions();
    translate_ui();
  }

  //----------------------------------------------------------------
  // MainWidget::~MainWidget
  //
  MainWidget::~MainWidget()
  {
    delete menuBookmarks_;
  }

  //----------------------------------------------------------------
  // MainWidget::init_actions
  //
  void
  MainWidget::init_actions()
  {
    actionAutomaticBookmarks_
      = add<QAction>(this, "actionAutomaticBookmarks_");
    actionAutomaticBookmarks_->setCheckable(true);

    actionResumeFromBookmark_ =
      add<QAction>(this, "actionResumeFromBookmark_");
    actionResumeFromBookmark_->setCheckable(true);

    actionRemoveBookmarkNowPlaying_ =
      add<QAction>(this, "actionRemoveBookmarkNowPlaying_");

    actionRemoveBookmarks_ =
      add<QAction>(this, "actionRemoveBookmarks_");

    actionNext_ = add<QAction>(this, "actionNext_");
    actionNext_->setShortcutContext(Qt::ApplicationShortcut);

    actionPrev_ = add<QAction>(this, "actionPrev_");
    actionPrev_->setShortcutContext(Qt::ApplicationShortcut);

    actionRepeatPlaylist_ = add<QAction>(this, "actionRepeatPlaylist");
    actionRepeatPlaylist_->setCheckable(true);

    actionShowPlaylist_ = add<QAction>(this, "actionShowPlaylist_");
    actionShowPlaylist_->setCheckable(true);
    actionShowPlaylist_->setChecked(false);
    actionShowPlaylist_->setShortcutContext(Qt::ApplicationShortcut);

    actionSelectAll_ = add<QAction>(this, "actionSelectAll_");
    actionSelectAll_->setShortcutContext(Qt::ApplicationShortcut);

    actionRemove_ = add<QAction>(this, "actionRemove_");
    actionRemove_->setShortcutContext(Qt::ApplicationShortcut);

    shortcutShowPlaylist_ = new QShortcut(this);
    shortcutShowPlaylist_->setContext(Qt::ApplicationShortcut);

    shortcutNext_ = new QShortcut(this);
    shortcutNext_->setContext(Qt::ApplicationShortcut);

    shortcutPrev_ = new QShortcut(this);
    shortcutPrev_->setContext(Qt::ApplicationShortcut);

    shortcutRemove_ = new QShortcut(this);
    shortcutRemove_->setContext(Qt::ApplicationShortcut);
    shortcutRemove_->setKey(QKeySequence(QKeySequence::Delete));

    shortcutSelectAll_ = new QShortcut(this);
    shortcutSelectAll_->setContext(Qt::ApplicationShortcut);
    shortcutSelectAll_->setKey(QKeySequence(QKeySequence::SelectAll));


    menuBookmarks_ = new QMenu();
    menuBookmarks_->setObjectName(QString::fromUtf8("menuBookmarks_"));
    menuBookmarks_->addAction(actionAutomaticBookmarks_);
    menuBookmarks_->addAction(actionResumeFromBookmark_);
    menuBookmarks_->addSeparator();
    bookmarksMenuSeparator_ = menuBookmarks_->addSeparator();
    menuBookmarks_->addAction(actionRemoveBookmarkNowPlaying_);
    menuBookmarks_->addAction(actionRemoveBookmarks_);

    PlayerView & player_view = PlayerWidget::view();
    QMenu & playback_menu = *(player_view.menuPlayback_);
    QAction * before = playback_menu.insertSeparator(player_view.actionLoop_);
    playback_menu.insertAction(before, actionPrev_);
    playback_menu.insertAction(before, actionNext_);
    playback_menu.insertAction(before, actionShowPlaylist_);
    playback_menu.insertAction(before, actionRepeatPlaylist_);

    bool ok = true;

    ok = connect(&player_view, SIGNAL(playback_finished(TTime)),
                 this, SLOT(playbackFinished(TTime)));
    YAE_ASSERT(ok);

    // show playlist:
    ok = connect(actionShowPlaylist_, SIGNAL(toggled(bool)),
                 this, SLOT(playbackShowPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(shortcutShowPlaylist_, SIGNAL(activated()),
                 actionShowPlaylist_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&player_view, SIGNAL(toggle_playlist()),
                 actionShowPlaylist_, SLOT(trigger()));
    YAE_ASSERT(ok);

    // select all:
    ok = connect(actionSelectAll_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSelectAll_, SIGNAL(activated()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    // remove selected:
    ok = connect(actionRemove_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(shortcutRemove_, SIGNAL(activated()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    // playlist model:
    ok = connect(&playlistModel_, SIGNAL(itemCountChanged()),
                 this, SLOT(fixupNextPrev()));
    YAE_ASSERT(ok);

    ok = connect(actionRemove_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(actionSelectAll_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(shortcutRemove_, SIGNAL(activated()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSelectAll_, SIGNAL(activated()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(&playlistModel_,
                 SIGNAL(playingItemChanged(const QModelIndex &)),
                 this,
                 SLOT(setPlayingItem(const QModelIndex &)));
    YAE_ASSERT(ok);

    ok = connect(&player_view, SIGNAL(fixup_next_prev()),
                 this, SLOT(fixupNextPrev()));
    YAE_ASSERT(ok);

    // bookmarks:
    ok = connect(actionAutomaticBookmarks_, SIGNAL(triggered()),
                 this, SLOT(bookmarksAutomatic()));
    YAE_ASSERT(ok);

    ok = connect(menuBookmarks_, SIGNAL(aboutToShow()),
                 this, SLOT(bookmarksPopulate()));
    YAE_ASSERT(ok);

    ok = connect(actionRemoveBookmarkNowPlaying_, SIGNAL(triggered()),
                 this, SLOT(bookmarksRemoveNowPlaying()));
    YAE_ASSERT(ok);

    ok = connect(actionRemoveBookmarks_, SIGNAL(triggered()),
                 this, SLOT(bookmarksRemove()));
    YAE_ASSERT(ok);

    ok = connect(actionResumeFromBookmark_, SIGNAL(triggered()),
                 this, SLOT(bookmarksResumePlayback()));
    YAE_ASSERT(ok);

    ok = connect(&player_view, SIGNAL(save_bookmark()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);

    // playlist navigation:
    ok = connect(actionNext_, SIGNAL(triggered()),
                 this, SLOT(playbackNext()));
    YAE_ASSERT(ok);

    ok = connect(actionPrev_, SIGNAL(triggered()),
                 this, SLOT(playbackPrev()));
    YAE_ASSERT(ok);

    ok = connect(shortcutNext_, SIGNAL(activated()),
                 actionNext_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(shortcutPrev_, SIGNAL(activated()),
                 actionPrev_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&player_view, SIGNAL(playback_next()),
                 actionNext_, SLOT(trigger()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWidget::translate_ui
  //
  void
  MainWidget::translate_ui()
  {
    menuBookmarks_->setTitle(tr("&Bookmarks"));

    actionAutomaticBookmarks_->setText
      (tr("Create Playlist Bookmarks Automatically"));
    actionAutomaticBookmarks_->setToolTip
      (tr("Automatically create and update bookmarks during playback, "
          "or whenever playback is paused"));

    actionResumeFromBookmark_->setText
      (tr("Resume Playback From Bookmark"));

    actionRemoveBookmarkNowPlaying_->setText
      (tr("Remove Selected Bookmark"));
    actionRemoveBookmarkNowPlaying_->setToolTip
      (tr("Remove bookmark for the playlist item playing now"));

    actionRemoveBookmarks_->setText
      (tr("Remove All Bookmarks"));
    actionRemoveBookmarks_->setToolTip
      (tr("Remove all bookmarks currently present in the Bookmarks menu"));

    actionNext_->setText(tr("Skip"));
    actionNext_->setShortcut(tr("Alt+Right"));

    actionPrev_->setText(tr("Go Back"));
    actionPrev_->setShortcut(tr("Alt+Left"));

    actionRepeatPlaylist_->setText
      (tr("Repeat Playlist"));
    actionRepeatPlaylist_->setToolTip
      (tr("Repeat playlist from the beginning "
          "after the last playlist item completes playback"));

    actionShowPlaylist_->setText(tr("Show &Playlist"));
    actionShowPlaylist_->setShortcut(tr("Ctrl+P"));

    actionSelectAll_->setText(tr("&Select All"));
    actionSelectAll_->setShortcut(tr("Ctrl+A"));

    actionRemove_->setText(tr("&Remove Selected"));
    actionRemove_->setShortcut(tr("Delete"));
  }

  //----------------------------------------------------------------
  // context_toggle_fullscreen
  //
  static void
  context_toggle_fullscreen(void * context)
  {
    MainWidget * mainWidget = (MainWidget *)context;
    mainWidget->requestToggleFullScreen();
  }

  //----------------------------------------------------------------
  // context_query_fullscreen
  //
  static bool
  context_query_fullscreen(void * context, bool & fullscreen)
  {
    MainWidget * mainWidget = (MainWidget *)context;
    fullscreen = mainWidget->isFullScreen();
    return true;
  }

  //----------------------------------------------------------------
  // context_query_playlist_visible
  //
  static bool
  context_query_playlist_visible(void * context, bool & playlist_visible)
  {
    MainWidget * mainWidget = (MainWidget *)context;
    playlist_visible = mainWidget->actionShowPlaylist_->isChecked();
    return true;
  }

  //----------------------------------------------------------------
  // toggle_playlist
  //
  static void
  toggle_playlist(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->togglePlaylist();
  }

  //----------------------------------------------------------------
  // MainWidget::initItemViews
  //
  void
  MainWidget::initItemViews()
  {
    PlayerWidget::initItemViews();

    TMakeCurrentContext currentContext(canvas_->Canvas::context());
    playlistView_.setup(&(PlayerWidget::view()));
    playlistView_.setModel(&playlistModel_);
    playlistView_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    playlistView_.query_fullscreen_.reset(&context_query_fullscreen, this);

    // add image://thumbnails/... provider:
    boost::shared_ptr<ThumbnailProvider::GetFilePath>
      getFilePath(new PlaylistItemFilePath(playlistModel_));
    yae::shared_ptr<ThumbnailProvider, ImageProvider>
      imageProvider(new ThumbnailProvider(readerPrototype_, getFilePath));
    playlistView_.addImageProvider(QString::fromUtf8("thumbnails"),
                                   imageProvider);

#ifdef __APPLE__
    QString clickOrTap = tr("click");
#else
    QString clickOrTap = tr("tap");
#endif

    QString greeting =
      tr("drop video/music files here\n\n"
         "press Spacebar to pause/resume playback\n\n"
         "%1 %2 to toggle the playlist view\n\n"
         "press Alt %3, Alt %4 to skip through the playlist\n\n"
#ifdef __APPLE__
         "use Apple Remote to change volume or skip along the timeline\n\n"
#endif
         "explore the menus for more options").
      arg(clickOrTap).
      arg(QString::fromUtf8("\xE2""\x98""\xB0")). // hamburger
      arg(QString::fromUtf8("\xE2""\x86""\x90")). // left arrow
      arg(QString::fromUtf8("\xE2""\x86""\x92")); // right arrow

    PlayerWidget::canvas_->setGreeting(greeting);
    PlayerWidget::canvas_->setFocusPolicy(Qt::StrongFocus);
    PlayerWidget::canvas_->prepend(&playlistView_);
    playlistView_.setEnabled(false);

    PlayerView & player_view = PlayerWidget::view_;
    TimelineItem & timeline = *(player_view.timeline_);
    timeline.toggle_playlist_.
      reset(&yae::toggle_playlist, &player_view);
    timeline.query_playlist_visible_.
      reset(&yae::context_query_playlist_visible, this);

    // load preferences:
    bool automaticBookmarks =
      loadBooleanSettingOrDefault(kCreateBookmarksAutomatically, true);
    actionAutomaticBookmarks_->setChecked(automaticBookmarks);

    bool resumeFromBookmark =
      loadBooleanSettingOrDefault(kResumePlaybackFromBookmark, true);
    actionResumeFromBookmark_->setChecked(resumeFromBookmark);

#if 0 // already handled in yaePlayerView.cpp
    bool showTimeline =
      loadBooleanSettingOrDefault(kShowTimeline, false);
    player_view.actionShowTimeline_->setChecked(showTimeline);
#endif
  }

  //----------------------------------------------------------------
  // MainWidget::setReaderPrototype
  //
  void
  MainWidget::setReaderPrototype(const IReaderPtr & readerPrototype)
  {
    readerPrototype_ = readerPrototype;
  }

  //----------------------------------------------------------------
  // MainWidget::setPlaylist
  //
  void
  MainWidget::setPlaylist(const std::list<QString> & playlist,
                          bool beginPlaybackImmediately)
  {
    bool resumeFromBookmark = actionResumeFromBookmark_->isChecked();

    std::list<BookmarkHashInfo> hashInfo;
    {
      BlockSignal block(&playlistModel_,
                        SIGNAL(playingItemChanged(const QModelIndex &)),
                        this,
                        SLOT(setPlayingItem(const QModelIndex &)));

      playlistModel_.add(playlist, resumeFromBookmark ? &hashInfo : NULL);
    }

    if (!beginPlaybackImmediately)
    {
      return;
    }

    if (resumeFromBookmark)
    {
      // look for a matching bookmark, resume playback if a bookmark exist:
      PlaylistBookmark bookmark;
      TPlaylistItemPtr found;

      for (std::list<BookmarkHashInfo>::const_iterator i = hashInfo.begin();
           !found && i != hashInfo.end(); ++i)
      {
        // shortcuts:
        const std::string & groupHash = i->groupHash_;

        if (!yae::loadBookmark(groupHash, bookmark))
        {
          continue;
        }

        // shortcut:
        const std::list<std::string> & itemHashList = i->itemHash_;
        for (std::list<std::string>::const_iterator j = itemHashList.begin();
             !found && j != itemHashList.end(); ++j)
        {
          const std::string & itemHash = *j;
          if (itemHash == bookmark.itemHash_)
          {
            found = playlistModel_.lookup(bookmark.groupHash_,
                                          bookmark.itemHash_);
            bookmark.item_ = found;
          }
        }
      }

      if (found)
      {
        gotoBookmark(bookmark);
        return;
      }
    }

    playback();

    QModelIndex playingIndex = playlistModel_.playingItem();
    playlistView_.ensureVisible(playingIndex);
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksAutomatic
  //
  void
  MainWidget::bookmarksAutomatic()
  {
    saveBooleanSetting(kCreateBookmarksAutomatically,
                       actionAutomaticBookmarks_->isChecked());
  }

  //----------------------------------------------------------------
  // escapeAmpersand
  //
  static QString
  escapeAmpersand(const QString & menuText)
  {
    QString out = menuText;
    out.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
    return out;
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksPopulate
  //
  void
  MainWidget::bookmarksPopulate()
  {
    delete bookmarksGroup_;
    bookmarksGroup_ = NULL;

    delete bookmarksMapper_;
    bookmarksMapper_ = NULL;

    bookmarksMenuSeparator_->setVisible(false);
    actionRemoveBookmarkNowPlaying_->setVisible(false);
    actionRemoveBookmarks_->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;
    }

    bookmarks_.clear();

    QModelIndex itemIndexNowPlaying = playlistModel_.playingItem();
    std::size_t playingBookmarkIndex = std::numeric_limits<std::size_t>::max();

    QModelIndex rootIndex = playlistModel_.makeModelIndex(-1, -1);
    const int numGroups = playlistModel_.rowCount(rootIndex);

    for (int i = 0; i < numGroups; i++)
    {
      QModelIndex groupIndex = playlistModel_.makeModelIndex(i, -1);

      TPlaylistGroupPtr group;
      playlistModel_.lookup(groupIndex, &group);

      // check whether there is a bookmark for an item in this group:
      PlaylistBookmark bookmark;
      if (!yae::loadBookmark(group->hash_, bookmark))
      {
        continue;
      }

      // check whether the item hash matches a group item:
      const int groupSize = playlistModel_.rowCount(groupIndex);
      for (int j = 0; j < groupSize; j++)
      {
        QModelIndex itemIndex = playlistModel_.makeModelIndex(i, j);
        TPlaylistItemPtr item = playlistModel_.lookup(itemIndex);

        if (!item || item->hash_ != bookmark.itemHash_)
        {
          continue;
        }

        // found a match, add it to the bookmarks menu:
        bookmark.item_ = item;

        std::string ts = TTime(bookmark.positionInSeconds_).to_hhmmss(":");

        if (!bookmarksGroup_)
        {
          bookmarksGroup_ = new QActionGroup(this);
          bookmarksMapper_ = new QSignalMapper(this);

          bool ok = connect(bookmarksMapper_, SIGNAL(mapped(int)),
                            this, SLOT(bookmarksSelectItem(int)));
          YAE_ASSERT(ok);

          bookmarksMenuSeparator_->setVisible(true);
          actionRemoveBookmarks_->setEnabled(true);
        }

        QString name =
          escapeAmpersand(group->name_) +
#ifdef __APPLE__
          // right-pointing double angle bracket:
          QString::fromUtf8(" ""\xc2""\xbb"" ") +
#else
          QString::fromUtf8("\t") +
#endif
          escapeAmpersand(item->name_) +
          QString::fromUtf8(", ") +
          QString::fromUtf8(ts.c_str());

        bookmark.action_ = new QAction(name, this);

        bool nowPlaying = (itemIndexNowPlaying == itemIndex);
        if (nowPlaying)
        {
          playingBookmarkIndex = bookmarks_.size();
        }

        bookmark.action_->setCheckable(true);
        bookmark.action_->setChecked(nowPlaying);

        menuBookmarks_->insertAction(bookmarksMenuSeparator_,
                                     bookmark.action_);
        bookmarksGroup_->addAction(bookmark.action_);

        bool ok = connect(bookmark.action_, SIGNAL(triggered()),
                          bookmarksMapper_, SLOT(map()));
        YAE_ASSERT(ok);

        bookmarksMapper_->setMapping(bookmark.action_,
                                     (int)(bookmarks_.size()));

        bookmarks_.push_back(bookmark);
      }
    }

    if (playingBookmarkIndex != std::numeric_limits<std::size_t>::max())
    {
      actionRemoveBookmarkNowPlaying_->setVisible(true);
    }
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksRemoveNowPlaying
  //
  void
  MainWidget::bookmarksRemoveNowPlaying()
  {
    QModelIndex itemIndex = playlistModel_.playingItem();
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlistModel_.lookup(itemIndex, &group);
    if (!item || !group)
    {
      return;
    }

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      if (bookmark.groupHash_ != group->hash_ ||
          bookmark.itemHash_ != item->hash_)
      {
        continue;
      }

      yae::removeBookmark(bookmark.groupHash_);
      break;
    }
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksRemove
  //
  void
  MainWidget::bookmarksRemove()
  {
    delete bookmarksGroup_;
    bookmarksGroup_ = NULL;

    delete bookmarksMapper_;
    bookmarksMapper_ = NULL;

    bookmarksMenuSeparator_->setVisible(false);
    actionRemoveBookmarkNowPlaying_->setVisible(false);
    actionRemoveBookmarks_->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;

      yae::removeBookmark(bookmark.groupHash_);
    }

    bookmarks_.clear();
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksResumePlayback
  //
  void
  MainWidget::bookmarksResumePlayback()
  {
    saveBooleanSetting(kResumePlaybackFromBookmark,
                       actionResumeFromBookmark_->isChecked());
  }

  //----------------------------------------------------------------
  // MainWidget::bookmarksSelectItem
  //
  void
  MainWidget::bookmarksSelectItem(int index)
  {
    if (index >= (int)bookmarks_.size())
    {
      return;
    }

    const PlaylistBookmark & bookmark = bookmarks_[index];
    gotoBookmark(bookmark);
  }


  //----------------------------------------------------------------
  // MainWidget::playbackShowPlaylist
  //
  void
  MainWidget::playbackShowPlaylist()
  {
    bool showPlaylist = actionShowPlaylist_->isChecked();

    std::ostringstream oss;
    YAE_LIFETIME_SHOW(oss);
    YAE_BENCHMARK_SHOW(oss);
    YAE_BENCHMARK_CLEAR();
    YAE_LIFETIME_CLEAR();

#if 0 // ndef NDEBUG
    std::string desktop =
      YAE_STANDARD_LOCATION(DesktopLocation).toUtf8().constData();
    std::string timesheet = (fs::path(desktop) / "aeyae.timesheet").string();
    yae::dump(timesheet, oss.str().c_str(), oss.str().size());
#endif

    playlistView_.setEnabled(showPlaylist);

    PlayerView & player_view = PlayerWidget::view();
    TimelineItem & timeline = *(player_view.timeline_);
    timeline.showPlaylist(showPlaylist);
  }

  //----------------------------------------------------------------
  // MainWidget::exitPlaylist
  //
  void
  MainWidget::exitPlaylist()
  {
    actionShowPlaylist_->setChecked(false);
  }

  //----------------------------------------------------------------
  // MainWidget::togglePlaylist
  //
  void
  MainWidget::togglePlaylist()
  {
    actionShowPlaylist_->trigger();
  }

  //----------------------------------------------------------------
  // MainWidget::setPlayingItem
  //
  void
  MainWidget::setPlayingItem(const QModelIndex & index)
  {
    playbackStop();

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    if (!item)
    {
      canvas_->clear();

      // FIXME: this could be handled as a playlist view state instead:
      canvas_->setGreeting(canvas_->greeting());

      // actionPlay->setEnabled(false);
      fixupNextPrev();
    }
    else
    {
      playback(index);

      QModelIndex playingIndex = playlistModel_.playingItem();
      playlistView_.ensureVisible(playingIndex);
    }
  }

  //----------------------------------------------------------------
  // MainWidget::playbackStop
  //
  void
  MainWidget::playbackStop()
  {
    PlayerView & player_view = PlayerWidget::view();
    player_view.player_->playback_stop();

    this->setWindowTitle(tr("Apprentice Video"));

    player_view.adjustMenuActions();

    IReaderPtr no_reader;
    emit adjust_menus(no_reader);
  }

  //----------------------------------------------------------------
  // MainWidget::playbackFinished
  //
  void
  MainWidget::playbackFinished(TTime playheadPos)
  {
    // remove current bookmark:
    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    TPlaylistItemPtr next = playlistModel_.lookup(iNext);

    if (item && next && (&(item->group_) != &(next->group_)))
    {
      const PlaylistGroup * itemGroup = &(item->group_);
      const PlaylistGroup * nextGroup = &(next->group_);

      if (itemGroup != nextGroup)
      {
        PlaylistBookmark bookmark;

        // if current item was bookmarked, then remove it from bookmarks:
        if (findBookmark(item, bookmark))
        {
          yae::removeBookmark(itemGroup->hash_);
        }

        // if a bookmark exists for the next item group, then use it:
        if (nextGroup && findBookmark(nextGroup->hash_, bookmark))
        {
          gotoBookmark(bookmark);
          return;
        }
      }
    }

    if (!next && actionRepeatPlaylist_->isChecked())
    {
      // repeat the playlist:
      QModelIndex first = playlistModel_.firstItem();
      playback(first);
      return;
    }

    playbackNext();
    saveBookmark();
  }

  //----------------------------------------------------------------
  // MainWidget::playback
  //
  void
  MainWidget::playback(bool forward)
  {
    QModelIndex current = playlistModel_.playingItem();
    playback(current, forward);
  }

  //----------------------------------------------------------------
  // MainWidget::playback
  //
  void
  MainWidget::playback(const QModelIndex & startHere, bool forward)
  {
    QModelIndex current = startHere;
    TPlaylistItemPtr item;
    bool ok = false;

    while ((item = playlistModel_.lookup(current)))
    {
      item->failed_ = !load(item->path_);

      if (!item->failed_)
      {
        ok = true;
        break;
      }

      if (forward)
      {
        current = playlistModel_.nextItem(current);
      }
      else
      {
        current = playlistModel_.prevItem(current);
      }
    }

    playlistView_.ensureVisible(current);

    if (!ok && !forward)
    {
      playback(startHere, true);
      return;
    }

    // update playlist model:
    {
      BlockSignal block(&playlistModel_,
                        SIGNAL(playingItemChanged(const QModelIndex &)),
                        this,
                        SLOT(setPlayingItem(const QModelIndex &)));
      playlistModel_.setPlayingItem(current);
    }

    if (!ok)
    {
      playbackStop();
    }

    fixupNextPrev();
  }

  //----------------------------------------------------------------
  // MainWidget::fixupNextPrev
  //
  void
  MainWidget::fixupNextPrev()
  {
    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    QModelIndex iPrev =
      index.isValid() ?
      playlistModel_.prevItem(index) :
      playlistModel_.lastItem();

    TPlaylistItemPtr prev = playlistModel_.lookup(iPrev);
    TPlaylistItemPtr next = playlistModel_.lookup(iNext);

    actionPrev_->setEnabled(!!prev);
    actionNext_->setEnabled(index.isValid());

    if (prev)
    {
      actionPrev_->setText(tr("Go Back To %1").arg(prev->name_));
    }
    else
    {
      actionPrev_->setText(tr("Go Back"));
    }

    if (next)
    {
      actionNext_->setText(tr("Skip To %1").arg(next->name_));
    }
    else
    {
      actionNext_->setText(tr("Skip"));
    }
  }

  //----------------------------------------------------------------
  // MainWidget::playbackNext
  //
  void
  MainWidget::playbackNext()
  {
    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    playback(iNext, true);
  }

  //----------------------------------------------------------------
  // MainWidget::playbackPrev
  //
  void
  MainWidget::playbackPrev()
  {
    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iPrev =
      index.isValid() ?
      playlistModel_.prevItem(index) :
      playlistModel_.lastItem();

    if (iPrev.isValid())
    {
      playback(iPrev, false);
    }
  }

  //----------------------------------------------------------------
  // MainWidget::saveBookmark
  //
  void
  MainWidget::saveBookmark()
  {
    if (!actionAutomaticBookmarks_->isChecked())
    {
      return;
    }

    const PlayerView & player_view = PlayerWidget::view();
    IReaderPtr reader_ptr = player_view.player_->reader();
    if (!(reader_ptr && reader_ptr->isSeekable()))
    {
      return;
    }

    QModelIndex itemIndex = playlistModel_.playingItem();
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlistModel_.lookup(itemIndex, &group);

    if (group && item)
    {
      const TimelineModel & timeline = player_view.timeline_model();
      double positionInSeconds = timeline.currentTime();
      yae::saveBookmark(group->hash_,
                        item->hash_,
                        reader_ptr.get(),
                        positionInSeconds);

      // refresh the bookmarks list:
      bookmarksPopulate();
    }
  }

  //----------------------------------------------------------------
  // MainWidget::gotoBookmark
  //
  void
  MainWidget::gotoBookmark(const PlaylistBookmark & bookmark)
  {
    QModelIndex index = playlistModel_.lookupModelIndex(bookmark.groupHash_,
                                                        bookmark.itemHash_);

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    if (item)
    {
      item->failed_ = !load(item->path_, &bookmark);

      if (!item->failed_)
      {
        playlistView_.ensureVisible(index);

        // update playlist model:
        BlockSignal block(&playlistModel_,
                          SIGNAL(playingItemChanged(const QModelIndex &)),
                          this,
                          SLOT(setPlayingItem(const QModelIndex &)));
        playlistModel_.setPlayingItem(index);
      }
    }

    fixupNextPrev();
  }

  //----------------------------------------------------------------
  // MainWidget::findBookmark
  //
  bool
  MainWidget::findBookmark(const TPlaylistItemPtr & item,
                           PlaylistBookmark & found) const
  {
    for (std::vector<PlaylistBookmark>::const_iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      const PlaylistBookmark & bookmark = *i;
      if (bookmark.item_ == item)
      {
        found = bookmark;
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // MainWidget::findBookmark
  //
  bool
  MainWidget::findBookmark(const std::string & groupHash,
                           PlaylistBookmark & found) const
  {
    for (std::vector<PlaylistBookmark>::const_iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      const PlaylistBookmark & bookmark = *i;
      if (bookmark.groupHash_ == groupHash)
      {
        found = bookmark;
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // MainWidget::adjustCanvasHeight
  //
  void
  MainWidget::adjustCanvasHeight()
  {
    bool showPlaylist = actionShowPlaylist_->isChecked();
    if (showPlaylist)
    {
      return;
    }

    PlayerWidget::adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWidget::swapShortcuts
  //
  void
  MainWidget::swapShortcuts()
  {
    PlayerWidget::swapShortcuts();

    yae::swapShortcuts(shortcutNext_, actionNext_);
    yae::swapShortcuts(shortcutPrev_, actionPrev_);

    emit swap_shortcuts();
  }

  //----------------------------------------------------------------
  // MainWidget::populateContextMenu
  //
  void
  MainWidget::populateContextMenu()
  {
    PlayerWidget::populateContextMenu();

    PlayerView & player_view = PlayerWidget::view();
    QMenu & context_menu = *(player_view.contextMenu_);
    QAction * separator = context_menu.
      insertSeparator(player_view.actionLoop_);
    context_menu.insertAction(separator, actionPrev_);
    context_menu.insertAction(separator, actionNext_);
    context_menu.insertAction(separator, actionShowPlaylist_);
    context_menu.insertAction(separator, actionRepeatPlaylist_);

    if (playlistModel_.hasItems())
    {
      context_menu.insertSeparator(separator);
      context_menu.insertAction(separator, actionSelectAll_);
      context_menu.insertAction(separator, actionRemove_);
    }

    // insert the bookmarks menu:
    QAction * next_item = NULL;
    separator = yae::find_last_separator(context_menu, next_item);
    if (separator && next_item)
    {
      context_menu.insertAction(next_item, menuBookmarks_->menuAction());
    }
    else
    {
      context_menu.addAction(menuBookmarks_->menuAction());
    }
  }

  //----------------------------------------------------------------
  // MainWidget::processKeyEvent
  //
  bool
  MainWidget::processKeyEvent(QKeyEvent * event)
  {
    if (PlayerWidget::processKeyEvent(event))
    {
      return true;
    }

    int key = event->key();
    if (key == Qt::Key_Escape && actionShowPlaylist_->isChecked())
    {
      actionShowPlaylist_->trigger();
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // hasFileExt
  //
  inline static bool
  hasFileExt(const QString & fn, const char * ext)
  {
    return fn.endsWith(QString::fromUtf8(ext), Qt::CaseInsensitive);
  }

  //----------------------------------------------------------------
  // canaryTest
  //
  static bool
  canaryTest(const QString & fn)
  {
    bool modFile =
      hasFileExt(fn, ".xm") ||
      hasFileExt(fn, ".ult") ||
      hasFileExt(fn, ".s3m") ||
      hasFileExt(fn, ".ptm") ||
      hasFileExt(fn, ".plm") ||
      hasFileExt(fn, ".mus") ||
      hasFileExt(fn, ".mtm") ||
      hasFileExt(fn, ".mod") ||
      hasFileExt(fn, ".med") ||
      hasFileExt(fn, ".mdl") ||
      hasFileExt(fn, ".md") ||
      hasFileExt(fn, ".lbm") ||
      hasFileExt(fn, ".it") ||
      hasFileExt(fn, ".hsp") ||
      hasFileExt(fn, ".dsm") ||
      hasFileExt(fn, ".dmf") ||
      hasFileExt(fn, ".dat") ||
      hasFileExt(fn, ".cfg") ||
      hasFileExt(fn, ".bmw") ||
      hasFileExt(fn, ".ams") ||
      hasFileExt(fn, ".669");

    if (!modFile)
    {
      return true;
    }

    QProcess canary;

    QString exePath = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QString::fromUtf8("--canary");
    args << fn;

    // send in the canary:
    canary.start(exePath, args);

    if (!canary.waitForFinished(30000))
    {
      // failed to finish in 30 seconds, assume it hanged:
      return false;
    }

    int exitCode = canary.exitCode();
    QProcess::ExitStatus canaryStatus = canary.exitStatus();

    if (canaryStatus != QProcess::NormalExit || exitCode)
    {
      // dead canary:
      return false;
    }

    // seems to be safe enough to at least open the file,
    // may still die during playback:
    return true;
  }

  //----------------------------------------------------------------
  // MainWidget::load
  //
  bool
  MainWidget::load(const QString & path, const TBookmark * bookmark)
  {
    IReaderPtr reader_ptr =
      canaryTest(path) ? yae::openFile(readerPrototype_, path) : IReaderPtr();

    if (!reader_ptr)
    {
#if 0
      yae_debug
        << "ERROR: could not open file: " << path.toUtf8().constData();
#endif
      return false;
    }

    IReader & reader = *reader_ptr;
    PlayerView & player_view = PlayerWidget::view();
    bool start_from_zero_time = true;
    player_view.playback(reader_ptr, bookmark, start_from_zero_time);

    window()->setWindowTitle(tr("Apprentice Video: %1").
                             arg(QFileInfo(path).fileName()));

    emit adjust_menus(reader_ptr);

    std::size_t numAudioTracks = reader.getNumberOfAudioTracks();
    std::size_t numVideoTracks = reader.getNumberOfVideoTracks();
    std::size_t videoTrackIndex = reader.getSelectedVideoTrackIndex();

    VideoTraits vtts;
    bool gotVideoTraits = reader.getVideoTraits(vtts);

    if ((videoTrackIndex >= numVideoTracks && numAudioTracks > 0) ||
        // audio files with embeded album art poster frame
        // typically show up with ridiculous frame rate,
        // so I'll consider that as an audio file trait:
        (gotVideoTraits && vtts.frameRate_ > 240.0))
    {
      playlistView_.setStyleId(PlaylistView::kListView);
    }
    else if (numVideoTracks)
    {
      playlistView_.setStyleId(PlaylistView::kGridView);
    }

    return true;
  }

}

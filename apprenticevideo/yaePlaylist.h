// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:37:40 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_H_
#define YAE_PLAYLIST_H_

// std includes:
#include <memory>
#include <set>
#include <vector>

// Qt includes:
#include <QObject>
#include <QString>

// yae includes:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_video.h"
#include "yae/utils/yae_tree.h"

// local includes:
#include "yaeBookmarks.h"
#include "yaePlaylistKey.h"


namespace yae
{

  //----------------------------------------------------------------
  // TPlaylistTree
  //
  typedef Tree<PlaylistKey, QString> TPlaylistTree;

  //----------------------------------------------------------------
  // PlaylistNode
  //
  struct PlaylistNode
  {
    PlaylistNode();
    PlaylistNode(const PlaylistNode & other);
    virtual ~PlaylistNode();

    std::size_t row_;

    // last-modified timestamp expressed in milliseconds since
    // 1970-01-01T00:00:00.000, Coordinated Universal Time
    //
    // initial value is set to current time when the node is constructed:
    qint64 msecUtcUpdated_;
  };

  //----------------------------------------------------------------
  // PlaylistGroup
  //
  struct PlaylistGroup;

  //----------------------------------------------------------------
  // PlaylistItem
  //
  struct PlaylistItem : public PlaylistNode
  {
    PlaylistItem(PlaylistGroup & group);

    typedef PlaylistKey TKey;

    inline const TKey & key() const
    { return key_; }

    // reference to the parent group holding this item:
    PlaylistGroup & group_;

    // playlist item key within the fringe group it belongs to:
    PlaylistKey key_;

    // absolute path to the playlist item:
    QString path_;

    // human friendly text describing this playlist item:
    QString name_;

    // file extension:
    QString ext_;

    // a flag indicating whether this item is currently selected:
    bool selected_;

    // a flag indicating whether this item failed to load:
    bool failed_;

    // a hash string identifying this item:
    std::string hash_;
  };

  //----------------------------------------------------------------
  // TPlaylistItemPtr
  //
  typedef yae::shared_ptr<PlaylistItem, PlaylistNode> TPlaylistItemPtr;

  //----------------------------------------------------------------
  // PlaylistGroup
  //
  struct PlaylistGroup : public PlaylistNode
  {
    PlaylistGroup();

    typedef std::list<PlaylistKey> TKey;

    inline const TKey & key() const
    { return keyPath_; }

    // complete key path to the fringe group that corresponds to this
    // playlist group:
    TKey keyPath_;

    // human friendly text describing this playlist item group:
    QString name_;

    // playlist items belonging to this group:
    std::vector<TPlaylistItemPtr> items_;

    // number of items stored in other playlist groups preceding this group:
    std::size_t offset_;

    // a flag indicating whether this group is collapsed for brevity:
    bool collapsed_;

    // a hash string identifying this group:
    std::string hash_;
  };

  //----------------------------------------------------------------
  // TPlaylistGroupPtr
  //
  typedef yae::shared_ptr<PlaylistGroup, PlaylistNode> TPlaylistGroupPtr;

  //----------------------------------------------------------------
  // TObservePlaylistGroup
  //
  typedef void(*TObservePlaylistGroup)(void * context, int groupRow);

  //----------------------------------------------------------------
  // TObservePlaylistItem
  //
  typedef void(*TObservePlaylistItem)(void * ctxt, int groupRow, int itemRow);

  //----------------------------------------------------------------
  // Playlist
  //
  class Playlist : public QObject,
                   public PlaylistNode
  {
    Q_OBJECT;

  public:
    Playlist();

    // use this to add items to the playlist;
    // optionally pass back a list of group bookmark hashes
    // that were added to the playlist during this call:
    void add(const std::list<QString> & playlist,

             // optionally pass back a list of hashes for the added items:
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

    // return index of the playing item:
    std::size_t playingItem() const;

    // return index of the current item:
    std::size_t currentItem() const;

    // return number of items in the playlist:
    inline std::size_t numItems() const
    { return numItems_; }

    // lookup a playlist item by index:
    TPlaylistGroupPtr lookupGroup(std::size_t index) const;
    TPlaylistItemPtr lookup(std::size_t index,
                            TPlaylistGroupPtr * group = NULL) const;

    // lookup a playlist item by group hash and item hash:
    TPlaylistGroupPtr lookupGroup(const std::string & groupHash) const;
    TPlaylistItemPtr lookup(const std::string & groupHash,
                            const std::string & itemHash,
                            TPlaylistGroupPtr * returnGroup = NULL) const;

    // lookup the url/file path an item identified by a groupHash/itemHash id:
    QString lookupItemFilePath(const QString & id) const;

    TPlaylistItemPtr lookup(TPlaylistGroupPtr & parent,
                            int groupRow,
                            int itemRow) const;

    std::size_t lookupIndex(int groupRow, int itemRow) const;

    enum TDirection {
      kBehind = 0,
      kAhead = 1
    };

    // lookup non-excluded group closest (in a given direction)
    // to the specified item index:
    TPlaylistGroupPtr closestGroup(std::size_t itemIndex,
                                   TDirection where = kAhead) const;

    // lookup non-excluded item closest (in a given direction)
    // to the specified item index:
    std::size_t closestItem(std::size_t itemIndex,
                            TDirection where = kAhead,
                            TPlaylistGroupPtr * returnGroup = NULL) const;

    inline std::size_t nextItem(std::size_t itemIndex,
                                TDirection where = kAhead,
                                TPlaylistGroupPtr * returnGroup = NULL) const
    {
      std::size_t i =
        (where == kAhead) ? itemIndex + 1 :
        (itemIndex > 0) ? itemIndex - 1 : 0;

      return closestItem(i, where, returnGroup);
    }

    // playlist navigation controls:
    void setPlayingItem(std::size_t index, bool force = false);

    // selection set management:
    void selectAll();
    void unselectAll();
    void selectGroup(PlaylistGroup & group);
    void unselectGroup(PlaylistGroup & group);
    void unselectItem(std::size_t indexSel);

    void selectItem(std::size_t indexSel, bool exclusive = true);
    void selectItems(std::size_t i0, std::size_t i1, bool exclusive);
    void selectItems(const std::set<std::size_t> & items, bool exclusive);

    std::size_t selectionAnchor();
    void discardSelectionAnchor();

    void removeSelected();
    void removeItems(int groupRow, int itemRow = -1);

    // accessors:
    inline const std::vector<TPlaylistGroupPtr> & groups() const
    { return groups_; }

    // returns true if current index has changed:
    bool setCurrentItem(std::size_t index);
    bool setCurrentItem(int groupRow, int itemRow);
    void getCurrentItem(int & groupRow, int & itemRow) const;

    // return true if selected status has changed:
    bool toggleSelectedItem(std::size_t index);
    bool setSelectedItem(std::size_t index);
    bool setSelectedItem(PlaylistItem & item, bool selected);

  signals:
    void itemCountChanged();

    void addingGroup(int groupRow);
    void addedGroup(int groupRow);

    void addingItem(int groupRow, int itemRow);
    void addedItem(int groupRow, int itemRow);

    void removingGroup(int groupRow);
    void removedGroup(int groupRow);

    void removingItem(int groupRow, int itemRow);
    void removedItem(int groupRow, int itemRow);

    void playingChanged(std::size_t now, std::size_t prev);
    void currentChanged(int groupRow, int itemRow);
    void selectedChanged(int groupRow, int itemRow);

  protected:
    // helpers:
    void updateOffsets();
    void setPlayingItem(std::size_t index, const TPlaylistItemPtr & prev);

    // this exist purely for code de-duplication:
    void removeItem(PlaylistGroup & group,
                    int itemRow,
                    int & playingNow,
                    int & currentNow);

    void removeItemsFinalize(const TPlaylistItemPtr & playingOld,
                             int playingNow,
                             int currentNow);

    // a playlist tree:
    TPlaylistTree tree_;

    // a list of playlist item groups, derived from playlist tree fringes:
    std::vector<TPlaylistGroupPtr> groups_;

    // total number of items:
    std::size_t numItems_;

    // playing item index:
    std::size_t playing_;

    // index of currently highlighted item:
    std::size_t current_;

    // index of current item at the start of extended selection:
    std::size_t selectionAnchor_;
  };

}

#endif // YAE_PLAYLIST_H_

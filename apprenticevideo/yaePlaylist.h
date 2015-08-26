// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:37:40 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_H_
#define YAE_PLAYLIST_H_

// std includes:
#include <set>
#include <vector>

// Qt includes:
#include <QString>

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/utils/yae_tree.h"

// local includes:
#include "yaeBookmarks.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistKey
  //
  struct PlaylistKey
  {
    PlaylistKey(const QString & key = QString(),
                const QString & ext = QString());

    bool operator == (const PlaylistKey & k) const;
    bool operator < (const PlaylistKey & k) const;
    bool operator > (const PlaylistKey & k) const;

    QString key_;
    QString ext_;
  };

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
  };

  //----------------------------------------------------------------
  // PlaylistItem
  //
  struct PlaylistItem : public PlaylistNode
  {
    PlaylistItem();

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

    // a flag indicating whether this item is excluded from the list:
    bool excluded_;

    // a flag indicating whether this item failed to load:
    bool failed_;

    // a hash string identifying this item:
    std::string hash_;
  };

  //----------------------------------------------------------------
  // PlaylistGroup
  //
  struct PlaylistGroup : public PlaylistNode
  {
    PlaylistGroup();

    // complete key path to the fringe group that corresponds to this
    // playlist group:
    std::list<PlaylistKey> keyPath_;

    // human friendly text describing this playlist item group:
    QString name_;

    // playlist items belonging to this group:
    std::vector<PlaylistItem> items_;

    // number of items stored in other playlist groups preceding this group:
    std::size_t offset_;

    // a flag indicating whether this group is collapsed for brevity:
    bool collapsed_;

    // a flag indicating whether this group is excluded from the list:
    bool excluded_;

    // a hash string identifying this group:
    std::string hash_;
  };

  //----------------------------------------------------------------
  // Playlist
  //
  struct Playlist : public PlaylistNode
  {
    Playlist();

    // use this to add items to the playlist;
    // optionally pass back a list of group bookmark hashes
    // that were added to the playlist during this call:
    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

    // return index of the playing item:
    std::size_t playingItem() const;

    // return number of items in the playlist:
    std::size_t countItems() const;

    // this is used to check whether previous/next navigation is possible:
    std::size_t countItemsAhead() const;
    std::size_t countItemsBehind() const;

    // lookup a playlist item by index:
    PlaylistGroup * lookupGroup(std::size_t index) const;
    PlaylistItem * lookup(std::size_t index,
                          PlaylistGroup ** group = NULL) const;

    // lookup a playlist item by group hash and item hash:
    PlaylistGroup * lookupGroup(const std::string & groupHash) const;
    PlaylistItem * lookup(const std::string & groupHash,
                          const std::string & itemHash,
                          std::size_t * returnItemIndex = NULL,
                          PlaylistGroup ** returnGroup = NULL) const;

    PlaylistItem * lookup(PlaylistGroup *& parent,
                          int groupRow,
                          int itemRow) const;

    enum TDirection {
      kBehind = 0,
      kAhead = 1
    };

    // lookup non-excluded group closest (in a given direction)
    // to the specified item index:
    PlaylistGroup * closestGroup(std::size_t itemIndex,
                                 TDirection where = kAhead) const;

    // lookup non-excluded item closest (in a given direction)
    // to the specified item index:
    std::size_t closestItem(std::size_t itemIndex,
                            TDirection where = kAhead,
                            PlaylistGroup ** group = NULL) const;

    // item filter:
    bool filterChanged(const QString & filter);

    // playlist navigation controls:
    void setPlayingItem(std::size_t index, bool force = false);

    // selection set management:
    void selectAll();
    void selectGroup(PlaylistGroup * group);
    void selectItem(std::size_t indexSel, bool exclusive = true);
    void removeSelected();
    void removeItems(std::size_t groupIndex, std::size_t itemIndex);

    // accessors:
    inline const std::vector<PlaylistGroup> & groups() const
    { return groups_; }

    inline std::size_t countItemsShown() const
    { return numShown_; }

    inline std::size_t countGroupsShown() const
    { return numShownGroups_; }

    inline std::size_t currentItem() const
    { return current_; }

    void changeCurrentItem(int itemsPerRow, int delta);

  protected:
    // helpers:
    bool applyFilter();
    void updateOffsets();

    // a playlist tree:
    TPlaylistTree tree_;

    // a list of playlist item groups, derived from playlist tree fringes:
    std::vector<PlaylistGroup> groups_;

    // total number of items:
    std::size_t numItems_;

    // number of non-excluded items:
    std::size_t numShown_;

    // number of non-excluded item groups:
    std::size_t numShownGroups_;

    // playing item index:
    std::size_t playing_;

    // index of currently highlighted item:
    std::size_t current_;

    // playlist filter:
    std::list<QString> keywords_;
  };

}

#endif // YAE_PLAYLIST_H_

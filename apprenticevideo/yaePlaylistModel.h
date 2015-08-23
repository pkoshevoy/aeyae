// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jul  1 20:33:02 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_MODEL_H_
#define YAE_PLAYLIST_MODEL_H_

// Qt includes:
#include <QAbstractItemModel>

// local includes:
#include "yaePlaylist.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistModel
  //
  class PlaylistModel : public QAbstractItemModel
  {
    Q_OBJECT;

  public:

    //----------------------------------------------------------------
    // Roles
    //
    enum Roles {
      kRoleType = Qt::UserRole + 1,
      kRolePath,
      kRoleLabel,
      kRoleBadge,
      kRoleGroupHash,
      kRoleItemHash,
      kRoleThumbnail,
      kRoleCollapsed,
      kRoleExcluded,
      kRoleSelected,
      kRolePlaying,
      kRoleFailed,
      kRoleItemCount
   };

    PlaylistModel(QObject * parent = NULL);

    // virtual:
    QModelIndex index(int row,
                      int column,
                      const QModelIndex & parent = QModelIndex()) const;

    // virtual:
    QModelIndex parent(const QModelIndex & child) const;

    // virtual:
    QHash<int, QByteArray> roleNames() const;

    // virtual:
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;

    // virtual: returns true if parent has any children:
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

    // virtual:
    QVariant data(const QModelIndex & index, int role) const;

    // virtual:
    bool setData(const QModelIndex & index, const QVariant & value, int role);

    // lookup a node associated with a given model index:
    PlaylistNode * getNode(const QModelIndex & index,
                           const PlaylistNode *& parentNode) const;

    // popular playlist methods:
    inline const Playlist & playlist() const
    { return playlist_; }

    inline std::size_t currentItem() const
    { return playlist_.currentItem(); }

    // return number of items in the playlist:
    inline std::size_t countItems() const
    { return playlist_.countItems(); }

    // this is used to check whether previous/next navigation is possible:
    inline std::size_t countItemsAhead() const
    { return playlist_.countItemsAhead(); }

    inline std::size_t countItemsBehind() const
    { return playlist_.countItemsBehind(); }

    inline PlaylistGroup * lookupGroup(std::size_t index) const
    { return playlist_.lookupGroup(index); }

    inline PlaylistItem * lookup(std::size_t index,
                                 PlaylistGroup ** group = NULL) const
    { return playlist_.lookup(index, group); }

    inline PlaylistGroup * lookupGroup(const std::string & groupHash) const
    { return playlist_.lookupGroup(groupHash); }

    inline PlaylistItem *
    lookup(const std::string & groupHash,
           const std::string & itemHash,
           std::size_t * returnItemIndex = NULL,
           PlaylistGroup ** returnGroup = NULL) const
    {
      return playlist_.lookup(groupHash,
                              itemHash,
                              returnItemIndex,
                              returnGroup);
    }

    inline PlaylistGroup *
    closestGroup(std::size_t itemIndex,
                 Playlist::TDirection where = Playlist::kAhead) const
    {
      return playlist_.closestGroup(itemIndex, where);
    }

    inline std::size_t
    closestItem(std::size_t itemIndex,
                Playlist::TDirection where = Playlist::kAhead,
                PlaylistGroup ** group = NULL) const
    {
      return playlist_.closestItem(itemIndex, where, group);
    }

    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

  public slots:
    // item filter:
    bool filterChanged(const QString & filter);

    // playlist navigation controls:
    void setCurrentItem(std::size_t index, bool force = false);

    // selection set management:
    void selectAll();

#if 0
    // FIXME: what about items that are unselected as the result?
    void selectGroup(PlaylistGroup * group);
    void selectItem(std::size_t indexSel, bool exclusive = true);
#endif

    void removeSelected();
    void removeItems(std::size_t groupIndex, std::size_t itemIndex);

    QModelIndex modelIndexForItem(std::size_t itemIndex) const;

  signals:
    // this signal may be emitted if the user activates an item,
    // or otherwise changes the playlist to invalidate the
    // existing current item:
    void currentItemChanged(std::size_t index);

  protected:
    void emitDataChanged(Roles role, const QModelIndex & index);

    void emitDataChanged(Roles role,
                         const QModelIndex & first,
                         const QModelIndex & last);

    mutable Playlist playlist_;
  };
}


#endif // YAE_PLAYLIST_MODEL_H_

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

    Q_PROPERTY(quint64 itemCount
               READ itemCount
               NOTIFY itemCountChanged);

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
      kRoleSelected,
      kRolePlaying,
      kRoleFailed,
      kRoleTimestamp,
      kRoleFlatIndex,
      kRoleFilterKey,
      kRoleItemCount,
      kRoleLayoutHint
    };

    //----------------------------------------------------------------
    // LayoutHint
    //
    enum LayoutHint {
      kLayoutHintGroupList = 1,
      kLayoutHintItemList,
      kLayoutHintItemListRow,
      kLayoutHintItemGrid,
      kLayoutHintItemGridCell
    };
#ifdef YAE_USE_QT5
    Q_ENUM(LayoutHint);
#else
    Q_ENUMS(LayoutHint);
#endif

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

    // optionally pass back a list of hashes for the added items:
    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

    // helper: create a model index for a given
    // group row index and item row index:
    QModelIndex makeModelIndex(int groupRow, int itemRow) const;

    // inverse of makeModelIndex
    static void mapToGroupRowItemRow(const QModelIndex & index,
                                     int & groupRow,
                                     int & itemRow);
  public slots:
    // no-op
    Q_INVOKABLE void setItemFilter(const QString & filter);

    // selection set management:
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectItems(int groupRow, int itemRow, int selFlags);
    Q_INVOKABLE void unselectAll();

    Q_INVOKABLE void setCurrentItem(int groupRow, int itemRow);
    Q_INVOKABLE void setCurrentItem(const QModelIndex & index);

    Q_INVOKABLE void setPlayingItem(int groupRow, int itemRow);
    Q_INVOKABLE void setPlayingItem(const QModelIndex & index);

    Q_INVOKABLE void removeItems(int groupRow, int itemRow);
    Q_INVOKABLE void removeItems(const QModelIndex & index);
    Q_INVOKABLE void removeSelected();

    // property: number of items in the playlist:
    inline quint64 itemCount() const
    { return (quint64)playlist_.numItems(); }

    // helper: model index for the first group/item:
    inline QModelIndex firstItem() const
    { return makeModelIndex(0, 0); }

    inline QModelIndex lastItem() const
    {
      return
        playlist_.numItems() ?
        mapToModelIndex(playlist_.numItems() - 1) :
        firstItem();
    }

    // check whether there are any items:
    inline bool hasItems() const
    { return firstItem().isValid(); }

    // popular playlist methods:
    QModelIndex playingItem() const
    { return mapToModelIndex(playlist_.playingItem()); }

    QModelIndex currentItem() const
    { return mapToModelIndex(playlist_.currentItem()); }

    // return index of the item closest to a given index
    // in the specified traversal direction:
    QModelIndex nextItem(const QModelIndex & index,
                         Playlist::TDirection where) const;

    inline QModelIndex nextItem(const QModelIndex & index) const
    { return nextItem(index, Playlist::kAhead); }

    inline QModelIndex prevItem(const QModelIndex & index) const
    { return nextItem(index, Playlist::kBehind); }

    // lookup item index for a given pair of group/item hashes:
    QModelIndex lookupModelIndex(const std::string & groupHash,
                                 const std::string & itemHash) const;

    // lookup a given item:
    TPlaylistItemPtr lookup(const QModelIndex & modelIndex,
                            TPlaylistGroupPtr * returnGroup = NULL) const;

    inline TPlaylistItemPtr
    lookup(const std::string & groupHash,
           const std::string & itemHash,
           TPlaylistGroupPtr * returnGroup = NULL) const
    {
      return playlist_.lookup(groupHash, itemHash, returnGroup);
    }

    // lookup the url/file path an item identified by a groupHash/itemHash id:
    QString lookupItemFilePath(const QString & id) const;

    // convert to/from model index and flat item vector index:
    std::size_t mapToItemIndex(const QModelIndex & modelIndex) const;
    QModelIndex mapToModelIndex(std::size_t itemIndex) const;

  signals:
    void itemCountChanged();

    // this signal may be emitted if the user activates an item,
    // or otherwise changes the playlist to invalidate the
    // existing playing item:
    void playingItemChanged(const QModelIndex & index);

    // highlight item change notification:
    void currentItemChanged(int groupRow, int itemRow);

  protected slots:
    void onAddingGroup(int groupRow);
    void onAddedGroup(int groupRow);

    void onAddingItem(int groupRow, int itemRow);
    void onAddedItem(int groupRow, int itemRow);

    void onRemovingGroup(int groupRow);
    void onRemovedGroup(int groupRow);

    void onRemovingItem(int groupRow, int itemRow);
    void onRemovedItem(int groupRow, int itemRow);

    void onPlayingChanged(std::size_t now, std::size_t prev);
    void onCurrentChanged(int groupRow, int itemRow);
    void onSelectedChanged(int groupRow, int itemRow);

  protected:
    // lookup a node associated with a given model index:
    PlaylistNode * getNode(const QModelIndex & index,
                           const PlaylistNode *& parentNode) const;

    void setPlayingItem(std::size_t index);

    void emitDataChanged(Roles role, const QModelIndex & index);

    void emitDataChanged(Roles role,
                         const QModelIndex & first,
                         const QModelIndex & last);

  public:
    mutable Playlist playlist_;
  };
}

//----------------------------------------------------------------
// yae::PlaylistModel::LayoutHint
//
Q_DECLARE_METATYPE(yae::PlaylistModel::LayoutHint);


#endif // YAE_PLAYLIST_MODEL_H_

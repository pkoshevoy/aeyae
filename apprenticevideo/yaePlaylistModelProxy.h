// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Sep 20 09:46:54 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_MODEL_PROXY_H_
#define YAE_PLAYLIST_MODEL_PROXY_H_

// standard libraries:
#include <list>
#include <map>

// Qt includes:
#include <QSortFilterProxyModel>

// local includes:
#include "yaePlaylistModel.h"

//----------------------------------------------------------------
// YAE_USE_PLAYLIST_MODEL_PROXY
//
#define YAE_USE_PLAYLIST_MODEL_PROXY 1


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistModelProxy
  //
  class PlaylistModelProxy : public QSortFilterProxyModel
  {
    Q_OBJECT;

    Q_PROPERTY(QString itemFilter
               READ itemFilter
               WRITE setItemFilter
               NOTIFY itemFilterChanged);

    Q_PROPERTY(quint64 itemCount
               READ itemCount
               NOTIFY itemCountChanged);

    Q_PROPERTY(SortBy sortBy
               READ sortBy
               WRITE setSortBy
               NOTIFY sortByChanged);

    Q_PROPERTY(Qt::SortOrder sortOrder
               READ sortOrder
               WRITE setSortOrder
               NOTIFY sortOrderChanged);

    // not supported:
    void setSourceModel(QAbstractItemModel *) YAE_FINAL {}

  public:

    enum SortBy
    {
      SortByName = 0,
      SortByTime = 1
    };
#ifdef YAE_USE_QT5
    Q_ENUM(SortBy);
#else
    Q_ENUMS(SortBy);
#endif

    PlaylistModelProxy(QObject * parent = NULL);

    // optionally pass back a list of hashes for the added items:
    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

    // helper: create a proxy model index for a given
    // proxy group row index and proxy item row index:
    QModelIndex makeModelIndex(int groupRow, int itemRow) const;

    // inverse of makeModelIndex:
    inline static void mapToGroupRowItemRow(const QModelIndex & proxyIndex,
                                            int & groupRow,
                                            int & itemRow)
    { PlaylistModel::mapToGroupRowItemRow(proxyIndex, groupRow, itemRow); }

    const QString & itemFilter() const;
    Q_INVOKABLE void setItemFilter(const QString & filter);

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

    SortBy sortBy() const;
    void setSortBy(SortBy s);
    void setSortOrder(Qt::SortOrder o);

    // count included (not filtered out) items:
    quint64 itemCount() const;

    // helper: model index for the first group/item:
    inline QModelIndex firstItem() const
    { return makeModelIndex(0, 0); }

    QModelIndex lastItem() const;

    // check whether there are any non-filtered-out items:
    inline bool hasItems() const
    { return firstItem().isValid(); }

    // return proxy model index of the currently playing item:
    QModelIndex playingItem() const;

    // return proxy model index of the current item:
    QModelIndex currentItem() const;

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
    TPlaylistItemPtr lookup(const QModelIndex & proxyIndex,
                            TPlaylistGroupPtr * returnGroup = NULL) const;

    inline TPlaylistItemPtr
    lookup(const std::string & groupHash,
           const std::string & itemHash,
           TPlaylistGroupPtr * retGroup = NULL) const
    {
      QModelIndex proxyIndex = lookupModelIndex(groupHash, itemHash);
      return lookup(proxyIndex, retGroup);
    }

    // lookup the url/file path an item identified by a groupHash/itemHash id:
    QString lookupItemFilePath(const QString & id) const;

  signals:
    void itemFilterChanged();
    void itemCountChanged();
    void sortByChanged();
    void sortOrderChanged();

    // this signal may be emitted if the user activates an item,
    // or otherwise changes the playlist to invalidate the
    // existing playing item:
    void playingItemChanged(const QModelIndex & index);

    // highlight item change notification:
    void currentItemChanged(int groupRow, int itemRow);

  protected slots:
    void onSourcePlayingChanged(const QModelIndex & index);
    void onSourceCurrentChanged(int groupRow, int itemRow);

  protected:
    // virtual:
    bool lessThan(const QModelIndex & sourceLeft,
                  const QModelIndex & sourceRight) const;

    // virtual:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex & sourceParent) const;

    // reference to playlist model being proxied:
    PlaylistModel model_;

    // item filter, as it was given to setItemFilter:
    QString itemFilter_;

    // playlist filter keywords:
    std::list<QString> keywords_;
  };

  //----------------------------------------------------------------
  // TPlaylistModel
  //
#if (YAE_USE_PLAYLIST_MODEL_PROXY)
  typedef PlaylistModelProxy TPlaylistModel;
#else
  typedef PlaylistModel TPlaylistModel;
#endif

}


#endif // YAE_PLAYLIST_MODEL_PROXY_H_

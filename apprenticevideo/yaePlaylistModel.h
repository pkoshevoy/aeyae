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
namespace mvc
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
      kRoleImage,
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

    // lookup a node associated with a given model index:
    PlaylistNode * getNode(const QModelIndex & index) const;

    template <typename TNode>
    inline TNode *
    getNode(const QModelIndex & index) const
    {
      PlaylistNode * found = getNode(index);
      TNode * node = dynamic_cast<TNode *>(found);
      return node;
    }

    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

    mutable Playlist playlist_;
  };

}
}

#endif // YAE_PLAYLIST_MODEL_H_

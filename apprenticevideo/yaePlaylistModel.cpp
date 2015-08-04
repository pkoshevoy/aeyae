// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jul  1 20:33:02 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include <yaePlaylistModel.h>


namespace yae
{
namespace mvc
{

  //----------------------------------------------------------------
  // PlaylistModel::PlaylistModel
  //
  PlaylistModel::PlaylistModel(QObject * parent):
    QAbstractItemModel(parent)
  {}

  //----------------------------------------------------------------
  // PlaylistModel::index
  //
  QModelIndex
  PlaylistModel::index(int row, int column, const QModelIndex & parent) const
  {
    if (row < 0 || column < 0)
    {
      return QModelIndex();
    }

    if (!parent.isValid())
    {
      const std::size_t n = playlist_.groups_.size();
      return row < n ? createIndex(row, column, &playlist_) : QModelIndex();
    }

    const PlaylistNode * parentNode = NULL;
    PlaylistNode * node = getNode(parent, parentNode);
    PlaylistGroup * group = dynamic_cast<PlaylistGroup *>(node);

    if (group)
    {
      const std::size_t n = group->items_.size();
      return row < n ? createIndex(row, column, group) : QModelIndex();
    }

    YAE_ASSERT(false);
    return QModelIndex();
  }

  //----------------------------------------------------------------
  // PlaylistModel::parent
  //
  QModelIndex
  PlaylistModel::parent(const QModelIndex & child) const
  {
    PlaylistNode * parent =
      static_cast<PlaylistNode *>(child.internalPointer());

    if (!parent || &playlist_ == parent)
    {
      return QModelIndex();
    }

    const PlaylistGroup * group =
      dynamic_cast<PlaylistGroup *>(parent);

    if (group)
    {
      return createIndex(group->row_, 0, &playlist_);
    }

    return QModelIndex();
  }

  //----------------------------------------------------------------
  // PlaylistModel::roleNames
  //
  QHash<int, QByteArray>
  PlaylistModel::roleNames() const
  {
    QHash<int, QByteArray> roles;

    roles[kRoleType] = "type";
    roles[kRolePath] = "path";
    roles[kRoleLabel] = "label";
    roles[kRoleBadge] = "badge";
    roles[kRoleGroupHash] = "groupHash";
    roles[kRoleItemHash] = "itemHash";
    roles[kRoleCollapsed] = "collapsed";
    roles[kRoleExcluded] = "excluded";
    roles[kRoleSelected] = "selected";
    roles[kRolePlaying] = "playing";
    roles[kRoleFailed] = "failed";
    roles[kRoleItemCount] = "itemCount";

    return roles;
  }

  //----------------------------------------------------------------
  // PlaylistModel::rowCount
  //
  int
  PlaylistModel::rowCount(const QModelIndex & parent) const
  {
    const PlaylistNode * parentNode = NULL;
    const PlaylistNode * node = getNode(parent, parentNode);

    if (&playlist_ == node)
    {
      return playlist_.groups_.size();
    }

    const PlaylistGroup * group =
      dynamic_cast<const PlaylistGroup *>(node);

    if (group)
    {
      return group->items_.size();
    }

    return 0;
  }

  //----------------------------------------------------------------
  // PlaylistModel::columnCount
  //
  int
  PlaylistModel::columnCount(const QModelIndex & parent) const
  {
    return parent.column() > 0 ? 0 : 1;
  }

  //----------------------------------------------------------------
  // PlaylistModel::hasChildren
  //
  bool
  PlaylistModel::hasChildren(const QModelIndex & parent) const
  {
    const PlaylistNode * parentNode = NULL;
    const PlaylistNode * node = getNode(parent, parentNode);

    if (&playlist_ == node)
    {
      return true;
    }

    const PlaylistGroup * group =
      dynamic_cast<const PlaylistGroup *>(node);

    if (group)
    {
      return !group->items_.empty();
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistModel::data
  //
  QVariant
  PlaylistModel::data(const QModelIndex & index, int role) const
  {
    const PlaylistNode * parentNode = NULL;
    const PlaylistNode * node = getNode(index, parentNode);

    const PlaylistGroup * group =
      dynamic_cast<const PlaylistGroup *>(node);

    if (group)
    {
      if (role == kRoleLabel || role == Qt::DisplayRole)
      {
        return QVariant(group->name_);
      }

      if (role == kRoleGroupHash)
      {
        return QVariant(QString::fromUtf8(group->bookmarkHash_.c_str()));
      }

      if (role == kRoleCollapsed)
      {
        return QVariant(group->collapsed_);
      }

      if (role == kRoleExcluded)
      {
        return QVariant(group->excluded_);
      }

      if (role == kRoleItemCount)
      {
        return QVariant((qulonglong)(group->items_.size()));
      }

      return QVariant();
    }

    const PlaylistItem * item =
      dynamic_cast<const PlaylistItem *>(node);

    if (item)
    {
      const PlaylistGroup * parentGroup =
        dynamic_cast<const PlaylistGroup *>(parentNode);

      if (role == Qt::DisplayRole || role == kRoleLabel)
      {
        return QVariant(item->name_);
      }

      if (role == kRolePath)
      {
        return QVariant(item->path_);
      }

      if (role == kRoleBadge)
      {
        return QVariant(item->ext_);
      }

      if (role == kRoleGroupHash)
      {
        return QVariant(QString::fromUtf8(parentGroup->bookmarkHash_.c_str()));
      }

      if (role == kRoleItemHash)
      {
        return QVariant(QString::fromUtf8(item->bookmarkHash_.c_str()));
      }

      if (role == kRoleExcluded)
      {
        return QVariant(item->excluded_);
      }

      if (role == kRoleSelected)
      {
        return QVariant(item->selected_);
      }

      if (role == kRolePlaying)
      {
        return QVariant(playlist_.currentItem() == item->row_);
      }

      if (role == kRoleFailed)
      {
        return QVariant(item->failed_);
      }

      return QVariant();
    }

    return QVariant();
  }

  //----------------------------------------------------------------
  // PlaylistModel::getNode
  //
  PlaylistNode *
  PlaylistModel::getNode(const QModelIndex & index,
                         const PlaylistNode *& parentNode) const
  {
    if (!index.isValid())
    {
      parentNode = NULL;
      return &playlist_;
    }

    PlaylistNode * parent =
      static_cast<PlaylistNode *>(index.internalPointer());
    parentNode = parent;

    if (&playlist_ == parent)
    {
      const std::size_t n = playlist_.groups_.size();
      std::size_t row = index.row();
      return (row < n) ? &(playlist_.groups_[row]) : NULL;
    }

    PlaylistGroup * group =
      dynamic_cast<PlaylistGroup *>(parent);

    if (group)
    {
      const std::size_t n = group->items_.size();
      std::size_t row = index.row();
      return (row < n) ? &(group->items_[row]) : NULL;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // PlaylistModel::add
  //
  void
  PlaylistModel::add(const std::list<QString> & playlist,
                     std::list<BookmarkHashInfo> * returnAddedHashes)
  {
#if 0
    Playlist newlist(playlist_);
    newlist.add(playlist, returnAddedHashes);

    typedef std::vector<PlaylistGroup>::const_iterator TIter;

    TIter ia = playlist_.groups_.begin();
    TIter iaEnd = playlist_.groups_.end();
    TIter ib = newlist.groups_.begin();
    TIter ibEnd = newlist.groups_.end();

    // figure out what new groups were added:
    while (ia != iaEnd && ib != ibEnd)
    {
      const PlaylistGroup & a = *ia;
      const PlaylistGroup & b = *ib;

      if (a.bookmarkHash_ != b.bookmarkHash_)
      {
        // new list (b) contains a group that was not in original (a),
        // try to find where it ends:
      }
    }

#elif 0
    int n0 = playlist_.groups_.size();
    emit rowsAboutToBeRemoved(QModelIndex(), 0, n0);
    playlist_.add(playlist, returnAddedHashes);
    emit rowsRemoved(QModelIndex(), 0, n0);

    int n1 = playlist_.groups_.size();
    emit rowsInserted(QModelIndex(), 0, n1);
#else
    // emit modelAboutToBeReset();
    beginResetModel();
    playlist_.add(playlist, returnAddedHashes);
    endResetModel();
    // emit modelReset();
#endif
  }

}
}

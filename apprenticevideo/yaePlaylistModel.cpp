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

  //----------------------------------------------------------------
  // PlaylistModel::PlaylistModel
  //
  PlaylistModel::PlaylistModel(QObject * parent):
    QAbstractItemModel(parent),
    sel_(this, this)
  {
    bool ok = true;
    ok = connect(&sel_, SIGNAL(currentChanged(const QModelIndex &,
                                              const QModelIndex &)),
                 this, SLOT(currentIndexChanged(const QModelIndex &,
                                                const QModelIndex &)));
    YAE_ASSERT(ok);
  }

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
      const std::size_t n = playlist_.groups().size();
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
    roles[kRoleThumbnail] = "thumbnail";
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
      return playlist_.groups().size();
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
        return QVariant(QString::fromUtf8(group->hash_.c_str()));
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
        return QVariant(QString::fromUtf8(parentGroup->hash_.c_str()));
      }

      if (role == kRoleItemHash)
      {
        return QVariant(QString::fromUtf8(item->hash_.c_str()));
      }

      if (role == kRoleThumbnail)
      {
        std::ostringstream oss;
        oss << "image://thumbnails/"
            << parentGroup->hash_
            << '/'
            << item->hash_;
        return QVariant(QString::fromUtf8(oss.str().c_str()));
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
        std::size_t itemIndex = parentGroup->offset_ + item->row_;
        return QVariant(playlist_.playingItem() == itemIndex);
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
  // PlaylistModel::setData
  //
  bool
  PlaylistModel::setData(const QModelIndex & index,
                         const QVariant & value,
                         int role)
  {
    const PlaylistNode * parentNode = NULL;
    PlaylistNode * node = getNode(index, parentNode);
    PlaylistGroup * group = dynamic_cast<PlaylistGroup *>(node);

    // std::cerr << "PlaylistModel::setData, role: " << role << std::endl;

    if (group)
    {
      if (role == kRoleCollapsed)
      {
        group->collapsed_ = value.toBool();
        emitDataChanged(kRoleCollapsed, index);
        return true;
      }
    }

    const PlaylistItem * item =
      dynamic_cast<const PlaylistItem *>(node);

    if (item)
    {
      const PlaylistGroup * parentGroup =
        dynamic_cast<const PlaylistGroup *>(parentNode);

      if (role == kRolePlaying)
      {
        setPlayingItem(parentGroup->offset_ + item->row_);
      }
    }

    return QAbstractItemModel::setData(index, value, role);
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
      const std::size_t n = playlist_.groups().size();
      std::size_t row = index.row();
      return
        (row < n) ?
        const_cast<PlaylistGroup *>(&(playlist_.groups()[row])) :
        NULL;
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

    TIter ia = playlist_.groups().begin();
    TIter iaEnd = playlist_.groups().end();
    TIter ib = newlist.groups().begin();
    TIter ibEnd = newlist.groups().end();

    // figure out what new groups were added:
    while (ia != iaEnd && ib != ibEnd)
    {
      const PlaylistGroup & a = *ia;
      const PlaylistGroup & b = *ib;

      if (a.hash_ != b.hash_)
      {
        // new list (b) contains a group that was not in original (a),
        // try to find where it ends:
      }
    }

#elif 0
    int n0 = playlist_.groups().size();
    emit rowsAboutToBeRemoved(QModelIndex(), 0, n0);
    playlist_.add(playlist, returnAddedHashes);
    emit rowsRemoved(QModelIndex(), 0, n0);

    int n1 = playlist_.groups().size();
    emit rowsInserted(QModelIndex(), 0, n1);
#else
    // emit modelAboutToBeReset();
    beginResetModel();
    playlist_.add(playlist, returnAddedHashes);
    endResetModel();

    // FIXME: should this be playingItem or currentItem?
    sel_.setCurrentIndex(modelIndexForItem(playlist_.playingItem()),
                         QItemSelectionModel::ClearAndSelect);

    QModelIndex currSel = sel_.currentIndex();
    // emit modelReset();
#endif

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModel::filterChanged
  //
  bool
  PlaylistModel::filterChanged(const QString & filter)
  {
    // FIXME: write me!
    YAE_ASSERT(false);

    emit itemCountChanged();

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistModel::setPlayingItem
  //
  void
  PlaylistModel::setPlayingItem(std::size_t itemIndex)
  {
    QModelIndex prev = modelIndexForItem(playlist_.playingItem());

    playlist_.setPlayingItem(itemIndex, true);

    QModelIndex curr = modelIndexForItem(playlist_.playingItem());
    // sel_.setCurrentIndex(curr, QItemSelectionModel::NoUpdate);

    if (prev != curr)
    {
      emitDataChanged(kRolePlaying, prev);
      emitDataChanged(kRolePlaying, curr);

      // FIXME: how to ensure the item is visible in the view?
      emit playingItemChanged(playlist_.playingItem());
    }
  }

  //----------------------------------------------------------------
  // PlaylistModel::selectAll
  //
  void
  PlaylistModel::selectAll()
  {
    playlist_.selectAll();

    for (std::vector<PlaylistGroup>::const_iterator
           i = playlist_.groups().begin(); i != playlist_.groups().end(); ++i)
    {
      const PlaylistGroup & group = *i;
      std::size_t groupSize = group.items_.size();

      QModelIndex i0 = modelIndexForItem(group.offset_);
      QModelIndex i1 = modelIndexForItem(group.offset_ + groupSize - 1);
      emitDataChanged(kRoleSelected, i0, i1);
    }
  }
#if 0
  //----------------------------------------------------------------
  // PlaylistModel::selectGroup
  //
  void
  PlaylistModel::selectGroup(PlaylistGroup * group)
  {
    // FIXME: what about items that are unselected as the result?

    playlist_.selectGroup(group);

    std::size_t groupSize = group->items_.size();
    QModelIndex i0 = modelIndexForItem(group->offset_);
    QModelIndex i1 = modelIndexForItem(group->offset_ + groupSize - 1);
    emitDataChanged(kRoleSelected, i0, i1);
  }

  //----------------------------------------------------------------
  // PlaylistModel::selectItem
  //
  void
  PlaylistModel::selectItem(std::size_t indexSel, bool exclusive)
  {
    // FIXME: what about items that are unselected as the result?

    playlist_.selectItem(indexSel, exclusive);

    QModelIndex index = modelIndexForItem(indexSel);
    emitDataChanged(kRoleSelected, i0, i1);
  }
#endif
  //----------------------------------------------------------------
  // PlaylistModel::removeSelected
  //
  void
  PlaylistModel::removeSelected()
  {
    // FIXME: write me!
    YAE_ASSERT(false);

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModel::removeItems
  //
  void
  PlaylistModel::removeItems(std::size_t groupIndex, std::size_t itemIndex)
  {
    // FIXME: write me!
    YAE_ASSERT(false);

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModel::modelIndexForItem
  //
  QModelIndex
  PlaylistModel::modelIndexForItem(std::size_t itemIndex) const
  {
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlist_.lookup(itemIndex, &group);

    if (!item)
    {
      return QModelIndex();
    }

    return createIndex(item->row_, 0, group);
  }

  //----------------------------------------------------------------
  // PlaylistModel::makeModelIndex
  //
  QModelIndex
  PlaylistModel::makeModelIndex(int groupRow, int itemRow) const
  {
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlist_.lookup(group, groupRow, itemRow);

    return
      item ? createIndex(item->row_, 0, group) :
      group ? createIndex(group->row_, 0, &playlist_) :
      QModelIndex();
  }

  //----------------------------------------------------------------
  // PlaylistModel::itemSelectionModel
  //
  QItemSelectionModel *
  PlaylistModel::itemSelectionModel()
  {
    return &sel_;
  }

  //----------------------------------------------------------------
  // PlaylistModel::setCurrentItem
  //
  void
  PlaylistModel::setCurrentItem(int groupRow, int itemRow, int cmd)
  {
    QModelIndex index = makeModelIndex(groupRow, itemRow);
    sel_.setCurrentIndex(index, (QItemSelectionModel::SelectionFlags)cmd);
  }

  //----------------------------------------------------------------
  // PlaylistModel::setPlayingItem
  //
  void
  PlaylistModel::setPlayingItem(int groupRow, int itemRow)
  {
    QModelIndex index = makeModelIndex(groupRow, itemRow);
    sel_.setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    setData(index, QVariant(true), kRolePlaying);
  }

  //----------------------------------------------------------------
  // PlaylistModel::currentIndexChanged
  //
  void
  PlaylistModel::currentIndexChanged(const QModelIndex & current,
                                     const QModelIndex & prev)
  {
    const PlaylistNode * parentNode = NULL;
    PlaylistNode * node = getNode(current, parentNode);

    const PlaylistGroup * group =
      dynamic_cast<const PlaylistGroup *>(parentNode);

    if (group)
    {
      PlaylistItem * item = dynamic_cast<PlaylistItem *>(node);
      int groupRow = group ? group->row_ : -1;
      int itemRow = item ? item->row_ : -1;
      emit currentItemChanged(group->row_, item->row_);
      return;
    }

    group = dynamic_cast<const PlaylistGroup *>(node);
    if (group)
    {
      emit currentItemChanged(group->row_, -1);
      return;
    }

    emit currentItemChanged(-1, -1);
  }

  //----------------------------------------------------------------
  // PlaylistModel::emitDataChanged
  //
  void
  PlaylistModel::emitDataChanged(Roles role, const QModelIndex & index)
  {
    if (!index.isValid())
    {
      return;
    }

    emit dataChanged(index, index, QVector<int>(1, role));
  }

  //----------------------------------------------------------------
  // PlaylistModel::emitDataChanged
  //
  void
  PlaylistModel::emitDataChanged(Roles role,
                                 const QModelIndex & first,
                                 const QModelIndex & last)
  {
    emit dataChanged(first, last, QVector<int>(1, role));
  }

}

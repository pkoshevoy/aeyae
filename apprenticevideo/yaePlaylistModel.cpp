// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jul  1 20:33:02 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/utils/yae_benchmark.h"

// local includes:
#include "yaePlaylistModel.h"

// Qt includes:
#include <QItemSelectionModel>


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistModel::PlaylistModel
  //
  PlaylistModel::PlaylistModel(QObject * parent):
    QAbstractItemModel(parent)
  {
    bool ok = true;

    ok = connect(&playlist_, SIGNAL(itemCountChanged()),
                 this, SIGNAL(itemCountChanged()));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(addingGroup(int)),
                 this, SLOT(onAddingGroup(int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(addedGroup(int)),
                 this, SLOT(onAddedGroup(int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(addingItem(int, int)),
                 this, SLOT(onAddingItem(int, int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(addedItem(int, int)),
                 this, SLOT(onAddedItem(int, int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(removingGroup(int)),
                 this, SLOT(onRemovingGroup(int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(removedGroup(int)),
                 this, SLOT(onRemovedGroup(int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(removingItem(int, int)),
                 this, SLOT(onRemovingItem(int, int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(removedItem(int, int)),
                 this, SLOT(onRemovedItem(int, int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(playingChanged(std::size_t, std::size_t)),
                 this, SLOT(onPlayingChanged(std::size_t, std::size_t)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(currentChanged(int, int)),
                 this, SLOT(onCurrentChanged(int, int)));
    YAE_ASSERT(ok);

    ok = connect(&playlist_, SIGNAL(selectedChanged(int, int)),
                 this, SLOT(onSelectedChanged(int, int)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlaylistModel::index
  //
  QModelIndex
  PlaylistModel::index(int row, int column, const QModelIndex & parent) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::index");

    if (row < 0 || column < 0)
    {
      return QModelIndex();
    }

    if (!parent.isValid())
    {
      const std::size_t n = playlist_.groups().size();
      PlaylistNode * playlistNode = &playlist_;
      return ((row < int(n)) ?
              createIndex(row, column, playlistNode) :
              QModelIndex());
    }

    const PlaylistNode * parentNode = NULL;
    PlaylistNode * node = getNode(parent, parentNode);
    PlaylistGroup * group = dynamic_cast<PlaylistGroup *>(node);

    if (group)
    {
      const std::size_t n = group->items_.size();
      return row < int(n) ? createIndex(row, column, group) : QModelIndex();
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
    // YAE_BENCHMARK(benchmark, "... Model::parent");

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
      PlaylistNode * playlistNode = &playlist_;
      return createIndex(group->row_, 0, playlistNode);
    }

    return QModelIndex();
  }

  //----------------------------------------------------------------
  // PlaylistModel::roleNames
  //
  QHash<int, QByteArray>
  PlaylistModel::roleNames() const
  {
    // YAE_BENCHMARK(benchmark, "... Model::roleNames");

    QHash<int, QByteArray> roles;

    roles[kRoleType] = "type";
    roles[kRolePath] = "path";
    roles[kRoleLabel] = "label";
    roles[kRoleBadge] = "badge";
    roles[kRoleGroupHash] = "groupHash";
    roles[kRoleItemHash] = "itemHash";
    roles[kRoleThumbnail] = "thumbnail";
    roles[kRoleCollapsed] = "collapsed";
    roles[kRoleCurrent] = "current";
    roles[kRoleSelected] = "selected";
    roles[kRolePlaying] = "playing";
    roles[kRoleFailed] = "failed";
    roles[kRoleTimestamp] = "msecTimestampUtc";
    roles[kRoleFlatIndex] = "flatIndex";
    roles[kRoleFilterKey] = "filterKey";
    roles[kRoleItemCount] = "itemCount";

    return roles;
  }

  //----------------------------------------------------------------
  // PlaylistModel::rowCount
  //
  int
  PlaylistModel::rowCount(const QModelIndex & parent) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::rowCount");

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
    // YAE_BENCHMARK(benchmark, "... Model::hasChildren");

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
    // YAE_BENCHMARK(benchmark, "... Model::data");

    const PlaylistNode * parentNode = NULL;
    const PlaylistNode * node = getNode(index, parentNode);

    const PlaylistGroup * group =
      dynamic_cast<const PlaylistGroup *>(node);

    if (group)
    {
      if (role == Qt::DisplayRole ||
          role == kRoleLabel ||
          role == kRoleFilterKey)
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

      if (role == kRoleTimestamp)
      {
        return QVariant(group->msecUtcUpdated_);
      }

      if (role == kRoleFlatIndex)
      {
        return QVariant((quint64)(group->offset_));
      }

      if (role == kRoleItemCount)
      {
        return QVariant((quint64)(group->items_.size()));
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

      if (role == kRoleCurrent)
      {
        int groupRow = -1;
        int itemRow = -1;
        playlist_.getCurrentItem(groupRow, itemRow);
        return QVariant(int(parentGroup->row_) == groupRow &&
                        int(item->row_) == itemRow);
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

      if (role == kRoleTimestamp)
      {
        return QVariant(item->msecUtcUpdated_);
      }

      if (role == kRoleFlatIndex)
      {
        return QVariant((quint64)(parentGroup->offset_ + item->row_));
      }

      if (role == kRoleFilterKey)
      {
        QString text =
          parentGroup->name_ + QString::fromUtf8(" ") +
          item->name_ + QString::fromUtf8(".") +
          item->ext_;

        std::size_t itemIndex = parentGroup->offset_ + item->row_;
        if (itemIndex == playlist_.playingItem())
        {
          text += tr("NOW PLAYING");
        }

        return text;
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
    // YAE_BENCHMARK(benchmark, "... Model::setData");

    const PlaylistNode * parentNode = NULL;
    PlaylistNode * node = getNode(index, parentNode);
    PlaylistGroup * group = dynamic_cast<PlaylistGroup *>(node);

    // yae_debug << "PlaylistModel::setData, role: " << role;

    if (group)
    {
      if (role == kRoleCollapsed)
      {
        group->collapsed_ = value.toBool();
        emitDataChanged(kRoleCollapsed, index);
        return true;
      }
    }

    PlaylistItem * item = dynamic_cast<PlaylistItem *>(node);
    if (item)
    {
      const PlaylistGroup * parentGroup =
        dynamic_cast<const PlaylistGroup *>(parentNode);
      YAE_ASSERT(parentGroup);

      if (role == kRolePlaying && parentGroup)
      {
        setPlayingItem(parentGroup->offset_ + item->row_);
        return true;
      }

      if (role == kRoleSelected)
      {
        playlist_.setSelectedItem(*item, value.toBool());
        return true;
      }
    }

    if (role == kRolePlaying && !item && !group)
    {
      setPlayingItem(playlist_.numItems());
      return true;
    }

    return QAbstractItemModel::setData(index, value, role);
  }

  //----------------------------------------------------------------
  // PlaylistModel::add
  //
  void
  PlaylistModel::add(const std::list<QString> & playlist,
                     std::list<BookmarkHashInfo> * returnAddedHashes)
  {
    // YAE_BENCHMARK(benchmark, "... Model::add");

    emit currentItemChanged(-1, -1);

    playlist_.add(playlist, returnAddedHashes);
  }

  //----------------------------------------------------------------
  // PlaylistModel::makeModelIndex
  //
  QModelIndex
  PlaylistModel::makeModelIndex(int groupRow, int itemRow) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::makeModelIndex");

    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlist_.lookup(group, groupRow, itemRow);

    if (item)
    {
      PlaylistNode * parent = group.get();
      return createIndex(item->row_, 0, parent);
    }

    if (group)
    {
      PlaylistNode * playlistNode = &playlist_;
      return createIndex(group->row_, 0, playlistNode);
    }

    return QModelIndex();
  }

  //----------------------------------------------------------------
  // PlaylistModel::mapToGroupRowItemRow
  //
  void
  PlaylistModel::mapToGroupRowItemRow(const QModelIndex & modelIndex,
                                      int & groupRow,
                                      int & itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::mapToGroupRowItemRow");

    groupRow = -1;
    itemRow = -1;

    if (!modelIndex.isValid())
    {
      return;
    }

    QModelIndex parentIndex = modelIndex.parent();
    if (parentIndex.isValid())
    {
      groupRow = parentIndex.row();
      itemRow = modelIndex.row();
    }
    else
    {
      groupRow = modelIndex.row();
    }
  }

  //----------------------------------------------------------------
  // PlaylistModel::setItemFilter
  //
  void
  PlaylistModel::setItemFilter(const QString & filter)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setItemFilter");

    yae_debug
      << "PlaylistModel::setItemFilter("
      << filter.toUtf8().constData()
      << ")";
  }

  //----------------------------------------------------------------
  // PlaylistModel::selectAll
  //
  void
  PlaylistModel::selectAll()
  {
    // YAE_BENCHMARK(benchmark, "... Model::selectAll");

    playlist_.selectAll();
  }

  //----------------------------------------------------------------
  // PlaylistModel::selectItems
  //
  void
  PlaylistModel::selectItems(int groupRow, int itemRow, int selectionFlags)
  {
    // YAE_BENCHMARK(benchmark, "... Model::selectItems");

    std::size_t itemIndex = playlist_.lookupIndex(groupRow, itemRow);

    if (selectionFlags == QItemSelectionModel::SelectCurrent)
    {
      std::size_t anchorIndex = playlist_.selectionAnchor();

      // extend/shrink selection from selection anchor item to given item
      std::size_t i0 = itemIndex < anchorIndex ? itemIndex : anchorIndex;
      std::size_t i1 = itemIndex < anchorIndex ? anchorIndex : itemIndex;

      bool exclusive = true;
      playlist_.selectItems(i0, i1, exclusive);
      return;
    }

    if (selectionFlags == QItemSelectionModel::ToggleCurrent)
    {
      playlist_.toggleSelectedItem(itemIndex);
      return;
    }

    playlist_.selectItem(itemIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModel::unselectAll
  //
  void
  PlaylistModel::unselectAll()
  {
    // YAE_BENCHMARK(benchmark, "... Model::unselectAll");

    playlist_.unselectAll();
  }

  //----------------------------------------------------------------
  // PlaylistModel::setCurrentItem
  //
  void
  PlaylistModel::setCurrentItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setCurrentItem");

    playlist_.setCurrentItem(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::setCurrentItem
  //
  void
  PlaylistModel::setCurrentItem(const QModelIndex & index)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setCurrentItem index");

    int groupRow = -1;
    int itemRow = -1;
    mapToGroupRowItemRow(index, groupRow, itemRow);
    setCurrentItem(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::setPlayingItem
  //
  void
  PlaylistModel::setPlayingItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setPlayingItem");

    QModelIndex index = makeModelIndex(groupRow, itemRow);
    setData(index, QVariant(true), kRolePlaying);

    // FIXME: should this be called here?
    setCurrentItem(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::setPlayingItem
  //
  void
  PlaylistModel::setPlayingItem(const QModelIndex & index)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setPlayingItem index");

    int groupRow = -1;
    int itemRow = -1;
    mapToGroupRowItemRow(index, groupRow, itemRow);
    setPlayingItem(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::removeItems
  //
  void
  PlaylistModel::removeItems(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::removeItems");

    playlist_.removeItems(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::removeItems
  //
  void
  PlaylistModel::removeItems(const QModelIndex & index)
  {
    // YAE_BENCHMARK(benchmark, "... Model::removeItems index");

    int groupRow = -1;
    int itemRow = -1;
    mapToGroupRowItemRow(index, groupRow, itemRow);
    removeItems(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::removeSelected
  //
  void
  PlaylistModel::removeSelected()
  {
    // YAE_BENCHMARK(benchmark, "... Model::removeSelected");

    playlist_.removeSelected();
  }

  //----------------------------------------------------------------
  // PlaylistModel::nextItem
  //
  QModelIndex
  PlaylistModel::nextItem(const QModelIndex & index,
                          Playlist::TDirection where) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::nextItem");

    std::size_t itemIndex = mapToItemIndex(index);
    if (itemIndex >= playlist_.numItems())
    {
      return makeModelIndex(-1, -1);
    }

    if (where == Playlist::kAhead)
    {
      return mapToModelIndex(itemIndex + 1);
    }

    if (itemIndex > 0)
    {
      return mapToModelIndex(itemIndex - 1);
    }

    return makeModelIndex(-1, -1);
  }

  //----------------------------------------------------------------
  // PlaylistModel::lookupModelIndex
  //
  QModelIndex
  PlaylistModel::lookupModelIndex(const std::string & groupHash,
                                  const std::string & itemHash) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::lookupModelIndex");

    TPlaylistGroupPtr group;
    TPlaylistItemPtr found = lookup(groupHash, itemHash, &group);

    if (!group && !found)
    {
      return makeModelIndex(-1, -1);
    }

    if (!found)
    {
      return makeModelIndex(group->row_, -1);
    }

    const PlaylistItem & item = *found;
    return makeModelIndex(group->row_, item.row_);
  }

  //----------------------------------------------------------------
  // PlaylistModel::lookup
  //
  TPlaylistItemPtr
  PlaylistModel::lookup(const QModelIndex & modelIndex,
                        TPlaylistGroupPtr * returnGroup) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::lookup");

    int groupRow = -1;
    int itemRow = -1;
    mapToGroupRowItemRow(modelIndex, groupRow, itemRow);

    const std::size_t numGroups = playlist_.groups().size();
    if (groupRow < 0 || groupRow >= int(numGroups))
    {
      YAE_ASSERT(groupRow < int(numGroups));
      return TPlaylistItemPtr();
    }

    TPlaylistGroupPtr groupPtr = playlist_.groups()[groupRow];
    if (returnGroup)
    {
      *returnGroup = groupPtr;
    }

    const PlaylistGroup & group = *groupPtr;
    const std::size_t numItems = group.items_.size();
    if (itemRow < 0 || itemRow >= int(numItems))
    {
      YAE_ASSERT(itemRow < int(numItems));
      return TPlaylistItemPtr();
    }

    return group.items_[itemRow];
  }

  //----------------------------------------------------------------
  // PlaylistModel::lookupItemFilePath
  //
  QString
  PlaylistModel::lookupItemFilePath(const QString & id) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::lookupItemFilePath");

    return playlist_.lookupItemFilePath(id);
  }

  //----------------------------------------------------------------
  // PlaylistModel::mapToItemIndex
  //
  std::size_t
  PlaylistModel::mapToItemIndex(const QModelIndex & modelIndex) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::mapToItemIndex");

    if (!modelIndex.isValid())
    {
      return std::numeric_limits<std::size_t>::max();
    }

    PlaylistNode * parent =
      static_cast<PlaylistNode *>(modelIndex.internalPointer());

    if (&playlist_ == parent)
    {
      std::size_t numGroups = playlist_.groups().size();
      std::size_t groupRow = modelIndex.row();

      if (groupRow >= numGroups)
      {
        return std::numeric_limits<std::size_t>::max();
      }

      const PlaylistGroup & group = *(playlist_.groups()[groupRow]);
      return group.offset_;
    }

    PlaylistGroup * group =
      dynamic_cast<PlaylistGroup *>(parent);

    if (group)
    {
      std::size_t numItems = group->items_.size();
      std::size_t itemRow = modelIndex.row();

      if (itemRow >= numItems)
      {
        return playlist_.numItems();
      }

      return group->offset_ + itemRow;
    }

    return std::numeric_limits<std::size_t>::max();
  }

  //----------------------------------------------------------------
  // PlaylistModel::mapToModelIndex
  //
  QModelIndex
  PlaylistModel::mapToModelIndex(std::size_t itemIndex) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::mapToModelIndex");

    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlist_.lookup(itemIndex, &group);

    if (!item)
    {
      return QModelIndex();
    }

    PlaylistNode * parent = group.get();
    return createIndex(item->row_, 0, parent);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onAddingGroup
  //
  void
  PlaylistModel::onAddingGroup(int groupRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onAddingGroup");

#if 0
    yae_debug << "PlaylistModel::onAddingGroup, groupRow: " << groupRow;
#endif
    QModelIndex parent = makeModelIndex(-1, -1);
    beginInsertRows(parent, groupRow, groupRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onAddedGroup
  //
  void
  PlaylistModel::onAddedGroup(int groupRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onAddedGroup");

#if 0
    yae_debug << "PlaylistModel::onAddedGroup, groupRow: " << groupRow;
#endif
    endInsertRows();
  }

  //----------------------------------------------------------------
  // PlaylistModel::onAddingItem
  //
  void
  PlaylistModel::onAddingItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onAddingItem");

#if 0
    yae_debug
      << "PlaylistModel::onAddingItem, groupRow: " << groupRow
      << ", itemRow: " << itemRow;
#endif
    QModelIndex parent = makeModelIndex(groupRow, -1);
    beginInsertRows(parent, itemRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onAddedItem
  //
  void
  PlaylistModel::onAddedItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onAddedItem");

#if 0
    yae_debug
      << "PlaylistModel::onAddedItem, groupRow: " << groupRow
      << ", itemRow: " << itemRow;
#endif
    endInsertRows();
  }

  //----------------------------------------------------------------
  // PlaylistModel::onRemovingGroup
  //
  void
  PlaylistModel::onRemovingGroup(int groupRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onRemovingGroup");

#if 0
    yae_debug << "PlaylistModel::onRemovingGroup, groupRow: " << groupRow;
#endif
    QModelIndex parent = makeModelIndex(-1, -1);
    beginRemoveRows(parent, groupRow, groupRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onRemovedGroup
  //
  void
  PlaylistModel::onRemovedGroup(int groupRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onRemovedGroup");

#if 0
    yae_debug << "PlaylistModel::onRemovedGroup, groupRow: " << groupRow;
#endif
    endRemoveRows();
  }

  //----------------------------------------------------------------
  // PlaylistModel::onRemovingItem
  //
  void
  PlaylistModel::onRemovingItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onRemovingItem");

#if 0
    yae_debug
      << "PlaylistModel::onRemovingItem, groupRow: " << groupRow
      << ", itemRow: " << itemRow;
#endif
    QModelIndex parent = makeModelIndex(groupRow, -1);
    beginRemoveRows(parent, itemRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onRemovedItem
  //
  void
  PlaylistModel::onRemovedItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onRemovedItem");

#if 0
    yae_debug
      << "PlaylistModel::onRemovedItem, groupRow: " << groupRow
      << ", itemRow: " << itemRow;
#endif
    endRemoveRows();
  }

  //----------------------------------------------------------------
  // PlaylistModel::onPlayingChanged
  //
  void
  PlaylistModel::onPlayingChanged(std::size_t now, std::size_t prev)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onPlayingChanged");

#if 0
    yae_debug
      << "PlaylistModel::onPlayingChanged: " << now
      << ", prev " << prev;
#endif
    QModelIndex ix0 = mapToModelIndex(prev);
    QModelIndex ix1 = mapToModelIndex(now);
    emitDataChanged(kRolePlaying, ix0);
    emitDataChanged(kRolePlaying, ix1);
    emit playingItemChanged(ix1);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onCurrentChanged
  //
  void
  PlaylistModel::onCurrentChanged(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onCurrentChanged");

#if 0
    yae_debug
      << "PlaylistModel::onCurrentChanged: ("
      << groupRow << ", " << itemRow << ")";
#endif
    emit currentItemChanged(groupRow, itemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModel::onSelectedChanged
  //
  void
  PlaylistModel::onSelectedChanged(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Model::onSelectedChanged");

#if 0
    yae_debug
      << "PlaylistModel::onSelectedChanged: ("
      << groupRow << ", " << itemRow << ")";
#endif
    QModelIndex index = makeModelIndex(groupRow, itemRow);
    emitDataChanged(kRoleSelected, index);
  }

  //----------------------------------------------------------------
  // PlaylistModel::getNode
  //
  PlaylistNode *
  PlaylistModel::getNode(const QModelIndex & index,
                         const PlaylistNode *& parentNode) const
  {
    // YAE_BENCHMARK(benchmark, "... Model::getNode");

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
      return (row < n) ? playlist_.groups()[row].get() : NULL;
    }

    PlaylistGroup * group =
      dynamic_cast<PlaylistGroup *>(parent);

    if (group)
    {
      const std::size_t n = group->items_.size();
      std::size_t row = index.row();
      return (row < n) ? group->items_[row].get() : NULL;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // PlaylistModel::setPlayingItem
  //
  void
  PlaylistModel::setPlayingItem(std::size_t itemIndex)
  {
    // YAE_BENCHMARK(benchmark, "... Model::setPlayingItem");

    playlist_.setPlayingItem(itemIndex, true);
  }

  //----------------------------------------------------------------
  // PlaylistModel::emitDataChanged
  //
  void
  PlaylistModel::emitDataChanged(Roles role, const QModelIndex & index)
  {
    // YAE_BENCHMARK(benchmark, "... Model::emitDataChanged");

    if (!index.isValid())
    {
      return;
    }

#ifdef YAE_USE_QT5
    emit dataChanged(index, index, QVector<int>(1, role));
#else
    (void)role;
    emit dataChanged(index, index);
#endif
  }

  //----------------------------------------------------------------
  // PlaylistModel::emitDataChanged
  //
  void
  PlaylistModel::emitDataChanged(Roles role,
                                 const QModelIndex & first,
                                 const QModelIndex & last)
  {
    // YAE_BENCHMARK(benchmark, "... Model::emitDataChanged 2");

#ifdef YAE_USE_QT5
    emit dataChanged(first, last, QVector<int>(1, role));
#else
    (void)role;
    emit dataChanged(first, last);
#endif
  }
}

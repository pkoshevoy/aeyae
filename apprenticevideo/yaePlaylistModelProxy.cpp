// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Sep 20 11:47:26 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yaePlaylistModelProxy.h"
#include "yaeUtilsQt.h"

// Qt includes:
#include <QItemSelectionModel>


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistModelProxy::PlaylistModelProxy
  //
  PlaylistModelProxy::PlaylistModelProxy(QObject * parent):
    QSortFilterProxyModel(parent)
  {
    bool ok = true;

    ok = connect(&model_, SIGNAL(itemCountChanged()),
                 this, SIGNAL(itemCountChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(playingItemChanged(const QModelIndex &)),
                 this, SLOT(onSourcePlayingChanged(const QModelIndex &)));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(currentItemChanged(int, int)),
                 this, SLOT(onSourceCurrentChanged(int, int)));
    YAE_ASSERT(ok);

    QSortFilterProxyModel::setSourceModel(&model_);
    QSortFilterProxyModel::setFilterRole(PlaylistModel::kRoleFilterKey);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::add
  //
  void
  PlaylistModelProxy::add(const std::list<QString> & playlist,
                          std::list<BookmarkHashInfo> * returnAddedHashes)
  {
    model_.add(playlist, returnAddedHashes);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::makeModelIndex
  //
  QModelIndex
  PlaylistModelProxy::makeModelIndex(int groupRow, int itemRow) const
  {
    QModelIndex rootIndex = QSortFilterProxyModel::index(-1, -1);
    const int numGroups = rowCount(rootIndex);

    if (groupRow < 0 || groupRow >= numGroups)
    {
      return rootIndex;
    }

    QModelIndex groupIndex =
      QSortFilterProxyModel::index(groupRow, 0, rootIndex);
    if (itemRow < 0)
    {
      return groupIndex;
    }

    const int groupSize = rowCount(groupIndex);
    if (itemRow >= groupSize)
    {
      return rootIndex;
    }

    return QSortFilterProxyModel::index(itemRow, 0, groupIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setItemFilter
  //
  void
  PlaylistModelProxy::setItemFilter(const QString & filter)
  {
    std::list<QString> keywords;
    splitIntoWords(filter, keywords);
    if (keywords.empty())
    {
      QSortFilterProxyModel::setFilterRegExp(QString());
      return;
    }

    QString pattern = QString("^");
    for (std::list<QString>::const_iterator i = keywords.begin();
         i != keywords.end(); ++i)
    {
      pattern += QString::fromUtf8("(?=.*");
      pattern += *i;
      pattern += QString::fromUtf8(")");
    }
    pattern += QString::fromUtf8(".+");

    QRegExp rx(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);
    QSortFilterProxyModel::setFilterRegExp(rx);

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::selectAll
  //
  void
  PlaylistModelProxy::selectAll()
  {
    QModelIndex rootIndex = makeModelIndex(-1, -1);
    const int numGroups = rowCount(rootIndex);

    for (int i = 0; i < numGroups; i++)
    {
      QModelIndex groupIndex = makeModelIndex(i, -1);
      const int groupSize = rowCount(groupIndex);

      for (int j = 0; j < groupSize; j++)
      {
        QModelIndex proxyIndex = makeModelIndex(i, j);
        QModelIndex sourceIndex = mapToSource(proxyIndex);
        std::size_t itemIndex = model_.mapToItemIndex(sourceIndex);

        model_.playlist_.selectItem(itemIndex, false);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::selectItems
  //
  void
  PlaylistModelProxy::selectItems(int groupRow, int itemRow, int selFlags)
  {
    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);

    if (selFlags == QItemSelectionModel::SelectCurrent)
    {
      std::set<std::size_t> selset;

      std::size_t anchorIndex = model_.playlist_.selectionAnchor();
      QModelIndex sourceAnchorIndex = model_.mapToModelIndex(anchorIndex);
      QModelIndex proxyAnchorIndex = mapFromSource(sourceAnchorIndex);

      // extend/shrink selection from selection anchor item to given item
      int groupRows[2] = { -1 };
      int itemRows[2] = { -1 };

      mapToGroupRowItemRow(proxyAnchorIndex, groupRows[0], itemRows[0]);
      mapToGroupRowItemRow(proxyIndex, groupRows[1], itemRows[1]);

      // simplify selection set traversion by establishing ascending order:
      if (groupRows[1] < groupRows[0])
      {
        std::swap(groupRows[0], groupRows[1]);
        std::swap(itemRows[0], itemRows[1]);
      }
      else if (itemRows[1] < itemRows[0])
      {
        std::swap(itemRows[0], itemRows[1]);
      }

      for (int groupRow = groupRows[0]; groupRow <= groupRows[1]; groupRow++)
      {
        for (int itemRow = itemRows[0]; itemRow <= itemRows[1]; itemRow++)
        {
          QModelIndex proxyModelIndex = makeModelIndex(groupRow, itemRow);
          QModelIndex sourceModelIndex = mapToSource(proxyModelIndex);
          std::size_t itemIndex = model_.mapToItemIndex(sourceModelIndex);
          selset.insert(itemIndex);
        }
      }

      bool exclusive = true;
      model_.playlist_.selectItems(selset, exclusive);

      return;
    }

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    std::size_t itemIndex = model_.mapToItemIndex(sourceIndex);

    if (selFlags == QItemSelectionModel::ToggleCurrent)
    {
      model_.playlist_.toggleSelectedItem(itemIndex);
      return;
    }

    model_.playlist_.selectItem(itemIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::unselectAll
  //
  void
  PlaylistModelProxy::unselectAll()
  {
    QModelIndex rootIndex = makeModelIndex(-1, -1);
    const int numGroups = rowCount(rootIndex);

    for (int i = 0; i < numGroups; i++)
    {
      QModelIndex groupIndex = makeModelIndex(i, -1);
      const int groupSize = rowCount(groupIndex);

      for (int j = 0; j < groupSize; j++)
      {
        QModelIndex proxyIndex = makeModelIndex(i, j);
        QModelIndex sourceIndex = mapToSource(proxyIndex);
        std::size_t itemIndex = model_.mapToItemIndex(sourceIndex);

        model_.playlist_.unselectItem(itemIndex);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setCurrentItem
  //
  void
  PlaylistModelProxy::setCurrentItem(int groupRow, int itemRow)
  {
    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    setCurrentItem(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setCurrentItem
  //
  void
  PlaylistModelProxy::setCurrentItem(const QModelIndex & proxyIndex)
  {
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.setCurrentItem(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setPlayingItem
  //
  void
  PlaylistModelProxy::setPlayingItem(int groupRow, int itemRow)
  {
    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    setPlayingItem(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setPlayingItem
  //
  void
  PlaylistModelProxy::setPlayingItem(const QModelIndex & proxyIndex)
  {
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.setPlayingItem(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeItems
  //
  void
  PlaylistModelProxy::removeItems(int groupRow, int itemRow)
  {
    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    removeItems(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeItems
  //
  void
  PlaylistModelProxy::removeItems(const QModelIndex & proxyIndex)
  {
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.removeItems(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeSelected
  //
  void
  PlaylistModelProxy::removeSelected()
  {
    model_.playlist_.removeSelected();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::itemCount
  //
  quint64
  PlaylistModelProxy::itemCount() const
  {
    quint64 numShown = 0;

    const int numGroups = rowCount(makeModelIndex(-1, -1));
    for (int i = 0; i < numGroups; i++)
    {
      const int numItems = rowCount(makeModelIndex(i, -1));
      numShown += numItems;
    }

    return numShown;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::lastItem
  //
  QModelIndex
  PlaylistModelProxy::lastItem() const
  {
    int numGroups = rowCount(makeModelIndex(-1, -1));
    if (!numGroups)
    {
      return firstItem();
    }

    int groupRow = numGroups - 1;
    int groupItems = rowCount(makeModelIndex(groupRow, -1));
    return makeModelIndex(groupRow, groupItems - 1);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::playingItem
  //
  QModelIndex
  PlaylistModelProxy::playingItem() const
  {
    QModelIndex sourceIndex = model_.playingItem();
    QModelIndex proxyIndex = mapFromSource(sourceIndex);
    return proxyIndex;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::nextItem
  //
  QModelIndex
  PlaylistModelProxy::nextItem(const QModelIndex & index,
                               Playlist::TDirection where) const
  {
    int groupRow = -1;
    int itemRow = -1;
    mapToGroupRowItemRow(index, groupRow, itemRow);

    if (groupRow == -1 || itemRow == -1)
    {
      return makeModelIndex(-1, -1);
    }

    if (where == Playlist::kAhead)
    {
      QModelIndex parent = index.parent();
      int numItems = rowCount(parent);

      if (index.row() + 1 < numItems)
      {
        // next sibling:
        return makeModelIndex(groupRow, itemRow + 1);
      }

      // next group:
      return makeModelIndex(groupRow + 1, 0);
    }

    // sanity check:
    YAE_ASSERT(where == Playlist::kBehind);

    if (itemRow > 0)
    {
      // previous sibling:
      return makeModelIndex(groupRow, itemRow - 1);
    }

    if (groupRow > 0)
    {
      // previous group:
      QModelIndex parent = makeModelIndex(groupRow - 1, -1);
      int numItems = rowCount(parent);
      return makeModelIndex(groupRow - 1, numItems - 1);
    }

    return makeModelIndex(-1, -1);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::lookupModelIndex
  //
  QModelIndex
  PlaylistModelProxy::lookupModelIndex(const std::string & groupHash,
                                       const std::string & itemHash) const
  {
    QModelIndex sourceIndex = model_.lookupModelIndex(groupHash, itemHash);
    QModelIndex proxyIndex = mapFromSource(sourceIndex);
    return proxyIndex;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::lookup
  //
  TPlaylistItemPtr
  PlaylistModelProxy::lookup(const QModelIndex & proxyIndex,
                             TPlaylistGroupPtr * returnGroup) const
  {
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return model_.lookup(sourceIndex, returnGroup);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::lookupItemFilePath
  //
  QString
  PlaylistModelProxy::lookupItemFilePath(const QString & id) const
  {
    return model_.playlist_.lookupItemFilePath(id);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::onSourcePlayingChanged
  //
  void
  PlaylistModelProxy::onSourcePlayingChanged(const QModelIndex & sourceIndex)
  {
    QModelIndex proxyIndex = mapFromSource(sourceIndex);
    emit playingItemChanged(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::onSourceCurrentChanged
  //
  void
  PlaylistModelProxy::onSourceCurrentChanged(int groupRow, int itemRow)
  {
    QModelIndex sourceIndex = model_.makeModelIndex(groupRow, itemRow);
    QModelIndex proxyIndex = mapFromSource(sourceIndex);

    int proxyGroupRow = -1;
    int proxyItemRow = -1;
    mapToGroupRowItemRow(proxyIndex, proxyGroupRow, proxyItemRow);
    emit currentItemChanged(proxyGroupRow, proxyItemRow);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::filterAcceptsRow
  //
  bool
  PlaylistModelProxy::filterAcceptsRow(int sourceRow,
                                       const QModelIndex & sourceParent) const
  {
    bool acceptable = QSortFilterProxyModel::filterAcceptsRow(sourceRow,
                                                              sourceParent);
    if (acceptable)
    {
      return true;
    }

    if (sourceParent.isValid())
    {
      return acceptable;
    }

    // must check whether any children of this group match the filter,
    // and reject the group if none of the children match.
    QModelIndex groupIndex = model_.makeModelIndex(sourceRow, -1);
    const int groupSize = model_.rowCount(groupIndex);
    for (int i = 0; i < groupSize; i++)
    {
      if (filterAcceptsRow(i, groupIndex))
      {
        return true;
      }
    }

    return false;
  }

}

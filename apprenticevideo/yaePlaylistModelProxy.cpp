// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Sep 20 11:47:26 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_benchmark.h"

// local:
#include "yaePlaylistModelProxy.h"
#include "yaeUtilsQt.h"

// Qt:
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
    QSortFilterProxyModel::setDynamicSortFilter(false);
    QSortFilterProxyModel::setFilterRole(PlaylistModel::kRoleFilterKey);

    setSortBy(PlaylistModelProxy::SortByName);
    setSortOrder(Qt::AscendingOrder);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::add
  //
  void
  PlaylistModelProxy::add(const std::list<QString> & playlist,
                          std::list<BookmarkHashInfo> * returnAddedHashes)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::add");

    model_.add(playlist, returnAddedHashes);
    Qt::SortOrder so = QSortFilterProxyModel::sortOrder();
    QSortFilterProxyModel::invalidate();
    QSortFilterProxyModel::sort(0, so);

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::makeModelIndex
  //
  QModelIndex
  PlaylistModelProxy::makeModelIndex(int groupRow, int itemRow) const
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::makeModelIndex");

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
  // PlaylistModelProxy::itemFilter
  //
  const QString &
  PlaylistModelProxy::itemFilter() const
  {
    return itemFilter_;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setItemFilter
  //
  void
  PlaylistModelProxy::setItemFilter(const QString & filter)
  {
    if (itemFilter_ == filter)
    {
      return;
    }

    // YAE_BENCHMARK(benchmark, "... Proxy::setItemFilter");

    itemFilter_ = filter;
    emit itemFilterChanged();

    keywords_.clear();
    splitIntoWords(filter, keywords_);

    QSortFilterProxyModel::invalidateFilter();
    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::selectAll
  //
  void
  PlaylistModelProxy::selectAll()
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::selectAll");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::selectItems");

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
      else if (groupRows[0] == groupRows[1] && itemRows[1] < itemRows[0])
      {
        std::swap(itemRows[0], itemRows[1]);
      }

      for (int groupRow = groupRows[0]; groupRow <= groupRows[1]; groupRow++)
      {
        int groupSize = rowCount(makeModelIndex(groupRow, -1));
        int i0 = (groupRow == groupRows[0]) ? itemRows[0] : 0;
        int i1 = (groupRow == groupRows[1]) ? itemRows[1] : groupSize - 1;

        for (int itemRow = i0; itemRow <= i1; itemRow++)
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
    // YAE_BENCHMARK(benchmark, "... Proxy::unselectAll");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::setCurrentItem");

    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    setCurrentItem(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setCurrentItem
  //
  void
  PlaylistModelProxy::setCurrentItem(const QModelIndex & proxyIndex)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::setCurrentItem index");

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.setCurrentItem(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setPlayingItem
  //
  void
  PlaylistModelProxy::setPlayingItem(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::setPlayingItem");

    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    setPlayingItem(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setPlayingItem
  //
  void
  PlaylistModelProxy::setPlayingItem(const QModelIndex & proxyIndex)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::setPlayingItem index");

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.setPlayingItem(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeItems
  //
  void
  PlaylistModelProxy::removeItems(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::removeItems");

    QModelIndex proxyIndex = makeModelIndex(groupRow, itemRow);
    removeItems(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeItems
  //
  void
  PlaylistModelProxy::removeItems(const QModelIndex & proxyIndex)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::removeItems index");

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    model_.removeItems(sourceIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::removeSelected
  //
  void
  PlaylistModelProxy::removeSelected()
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::removeSelected");

    model_.playlist_.removeSelected();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::sortBy
  //
  PlaylistModelProxy::SortBy
  PlaylistModelProxy::sortBy() const
  {
    const int sr = QSortFilterProxyModel::sortRole();

    PlaylistModelProxy::SortBy sb =
      (sr == PlaylistModel::kRoleTimestamp) ?
      PlaylistModelProxy::SortByTime :
      PlaylistModelProxy::SortByName;

    return sb;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setSortBy
  //
  void
  PlaylistModelProxy::setSortBy(PlaylistModelProxy::SortBy sb)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::setSortBy");

    const int sr = QSortFilterProxyModel::sortRole();

    int srNew =
      (sb == PlaylistModelProxy::SortByTime) ?
      PlaylistModel::kRoleTimestamp :
      PlaylistModel::kRoleFlatIndex;

    if (sr == srNew)
    {
      return;
    }

    QSortFilterProxyModel::invalidate();
    QSortFilterProxyModel::setSortRole(srNew);
    Qt::SortOrder so = QSortFilterProxyModel::sortOrder();
    QSortFilterProxyModel::sort(0, so);

    emit sortByChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::setSortOrder
  //
  void
  PlaylistModelProxy::setSortOrder(Qt::SortOrder soNew)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::setSortOrder");

    const Qt::SortOrder so = QSortFilterProxyModel::sortOrder();

    if (so == soNew)
    {
      return;
    }

    QSortFilterProxyModel::invalidate();
    QSortFilterProxyModel::sort(0, soNew);

    emit sortOrderChanged();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::itemCount
  //
  quint64
  PlaylistModelProxy::itemCount() const
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::itemCount");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::lastItem");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::playingItem");

    QModelIndex sourceIndex = model_.playingItem();
    QModelIndex proxyIndex = mapFromSource(sourceIndex);
    return proxyIndex;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::currentItem
  //
  QModelIndex
  PlaylistModelProxy::currentItem() const
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::currentItem");

    QModelIndex sourceIndex = model_.currentItem();
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
    // YAE_BENCHMARK(benchmark, "... Proxy::nextItem");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::lookupModelIndex");

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
    // YAE_BENCHMARK(benchmark, "... Proxy::lookup");

    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return model_.lookup(sourceIndex, returnGroup);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::lookupItemFilePath
  //
  QString
  PlaylistModelProxy::lookupItemFilePath(const QString & id) const
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::lookupItemFilePath");

    return model_.playlist_.lookupItemFilePath(id);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::onSourcePlayingChanged
  //
  void
  PlaylistModelProxy::onSourcePlayingChanged(const QModelIndex & sourceIndex)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::onSourcePlayingChanged");

    QModelIndex proxyIndex = mapFromSource(sourceIndex);
    emit playingItemChanged(proxyIndex);
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::onSourceCurrentChanged
  //
  void
  PlaylistModelProxy::onSourceCurrentChanged(int groupRow, int itemRow)
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::onSourceCurrentChanged");

    QModelIndex sourceIndex = model_.makeModelIndex(groupRow, itemRow);
    QModelIndex proxyIndex = mapFromSource(sourceIndex);

    int proxyGroupRow = -1;
    int proxyItemRow = -1;
    mapToGroupRowItemRow(proxyIndex, proxyGroupRow, proxyItemRow);
    emit currentItemChanged(proxyGroupRow, proxyItemRow);
  }

  //----------------------------------------------------------------
  // keywordsMatch
  //
  static bool
  keywordsMatch(const std::list<QString> & keywords, const QString  & text)
  {
    for (std::list<QString>::const_iterator i = keywords.begin();
         i != keywords.end(); ++i)
    {
      const QString & keyword = *i;
      if (!text.contains(keyword, Qt::CaseInsensitive))
      {
        return false;
      }
    }

#if 0
    yae_debug << "KEYWORDS MATCH: " << text.toUtf8().constData();
#endif
    return true;
  }

  //----------------------------------------------------------------
  // getFlatIndex
  //
  inline static void
  getRoleFlatIndex(const QModelIndex & left,
                   const QModelIndex & right,
                   std::size_t & l_ix,
                   std::size_t & r_ix)
  {
    l_ix = left.data(PlaylistModel::kRoleFlatIndex).value<std::size_t>();
    r_ix = right.data(PlaylistModel::kRoleFlatIndex).value<std::size_t>();
  }

  //----------------------------------------------------------------
  // getRoleTimestamp
  //
  inline static void
  getRoleTimestamp(const QModelIndex & left,
                   const QModelIndex & right,
                   qint64 & l_ts,
                   qint64 & r_ts)
  {
    l_ts = left.data(PlaylistModel::kRoleTimestamp).value<qint64>();
    r_ts = right.data(PlaylistModel::kRoleTimestamp).value<qint64>();
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::PlaylistModelProxy::lessThan
  //
  bool
  PlaylistModelProxy::lessThan(const QModelIndex & sourceLeft,
                               const QModelIndex & sourceRight) const
  {
    // make it a stable sort!
    const bool sortByName = (this->sortBy() == PlaylistModelProxy::SortByName);
    if (sortByName)
    {
      std::size_t l_ix, r_ix;
      yae::getRoleFlatIndex(sourceLeft, sourceRight, l_ix, r_ix);

      if (l_ix != r_ix)
      {
        return l_ix < r_ix;
      }

      qint64 l_ts, r_ts;
      yae::getRoleTimestamp(sourceLeft, sourceRight, l_ts, r_ts);

      return l_ts < r_ts;
    }

    qint64 l_ts, r_ts;
    yae::getRoleTimestamp(sourceLeft, sourceRight, l_ts, r_ts);

    if (l_ts != r_ts)
    {
      return l_ts < r_ts;
    }

    std::size_t l_ix, r_ix;
    yae::getRoleFlatIndex(sourceLeft, sourceRight, l_ix, r_ix);

    return l_ix < r_ix;
  }

  //----------------------------------------------------------------
  // PlaylistModelProxy::filterAcceptsRow
  //
  bool
  PlaylistModelProxy::filterAcceptsRow(int sourceRow,
                                       const QModelIndex & sourceParent) const
  {
    // YAE_BENCHMARK(benchmark, "... Proxy::filterAcceptsRow");

    if (sourceParent.isValid())
    {
      QModelIndex sourceIndex = model_.index(sourceRow, 0, sourceParent);

      QString filterKey =
        model_.data(sourceIndex, PlaylistModel::kRoleFilterKey).toString();

      bool acceptable = keywordsMatch(keywords_, filterKey);
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

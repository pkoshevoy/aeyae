// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:41:07 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <iostream>
#include <algorithm>
#include <limits>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/filesystem.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// Qt includes:
#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

// local includes:
#include "yaePlaylist.h"
#include "yaeUtilsQt.h"

// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // dump
  //
  static void
  dump(const Playlist & playlist, const char * message)
  {
    std::ostringstream oss;
    oss << "\n\n" << message;

    const std::vector<TPlaylistGroupPtr> & groups = playlist.groups();
    for (std::size_t i = 0; i < groups.size(); i++)
    {
      const PlaylistGroup & group = *(groups[i]);
      oss
        << "\ngroup " << i
        << ", time: "
        << (QDateTime::fromMSecsSinceEpoch(group.msecUtcUpdated_).
            toString(Qt::ISODate)).toUtf8().constData()
        << ", row: " << group.row_
        << ", offset: " << group.offset_
        << ", size: " << group.items_.size()
        << ", name: " << group.name_.toUtf8().constData()
        << "\n";

      for (std::size_t j = 0; j < group.items_.size(); j++)
      {
        const PlaylistItem & item = *(group.items_[j]);
        oss
          << "  item " << j
          << ", time: "
          << (QDateTime::fromMSecsSinceEpoch(item.msecUtcUpdated_).
              toString(Qt::ISODate)).toUtf8().constData()
          << ", row: " << item.row_
          << ", selected: " << item.selected_
          << ", name: " << item.name_.toUtf8().constData()
          << ", ext: " << item.ext_.toUtf8().constData()
          << "\n";
      }
    }

    yae_debug << oss.str();
  }


  //----------------------------------------------------------------
  // PlaylistNode::PlaylistNode
  //
  PlaylistNode::PlaylistNode():
    row_(~0),
    msecUtcUpdated_(QDateTime::currentMSecsSinceEpoch())
  {}

  //----------------------------------------------------------------
  // PlaylistNode::PlaylistNode
  //
  PlaylistNode::PlaylistNode(const PlaylistNode & other):
    row_(other.row_),
    msecUtcUpdated_(other.msecUtcUpdated_)
  {}

  //----------------------------------------------------------------
  // PlaylistNode::~PlaylistNode
  //
  PlaylistNode::~PlaylistNode()
  {}


  //----------------------------------------------------------------
  // PlaylistItem::PlaylistItem
  //
  PlaylistItem::PlaylistItem(PlaylistGroup & group):
    group_(group),
    selected_(false),
    failed_(false)
  {}


  //----------------------------------------------------------------
  // PlaylistGroup::PlaylistGroup
  //
  PlaylistGroup::PlaylistGroup():
    offset_(0),
    collapsed_(false)
  {}


  //----------------------------------------------------------------
  // Playlist::Playlist
  //
  Playlist::Playlist():
    numItems_(0),
    playing_(0),
    current_(0),
    selectionAnchor_(0)
  {
    add(std::list<QString>());
  }

  //----------------------------------------------------------------
  // init_lookup
  //
  template <typename TNode>
  static void
  init_lookup
  (std::map<typename TNode::TKey, yae::shared_ptr<TNode, PlaylistNode> > & lut,
   const std::vector<yae::shared_ptr<TNode, PlaylistNode> > & nodes)
  {
    typedef yae::shared_ptr<TNode, PlaylistNode> TNodePtr;
    typedef std::vector<TNodePtr> TNodes;
    typedef typename TNodes::const_iterator TNodesIter;

    lut.clear();

    for (TNodesIter i = nodes.begin(); i != nodes.end(); ++i)
    {
      const TNodePtr & node = *i;
      lut[node->key()] = node;
    }
  }

  //----------------------------------------------------------------
  // lookup
  //
  template <typename TNode>
  static yae::shared_ptr<TNode, PlaylistNode>
  lookup(const std::map
         <typename TNode::TKey, yae::shared_ptr<TNode, PlaylistNode> > & lut,
         const typename TNode::TKey & key)
  {
    typedef yae::shared_ptr<TNode, PlaylistNode> TNodePtr;
    typedef std::map<typename TNode::TKey, TNodePtr> TNodeMap;

    typename TNodeMap::const_iterator found = lut.find(key);
    return (found == lut.end()) ? TNodePtr() : found->second;
  }

  //----------------------------------------------------------------
  // Playlist::add
  //
  void
  Playlist::add(const std::list<QString> & playlist,

                // optionally pass back a list of hashes for the added items:
                std::list<BookmarkHashInfo> * returnBookmarkHashList)
  {
#if 0
    yae_debug << "Playlist::add";
#endif

    std::size_t currentNow = current_;

    // try to keep track of the original playing item,
    // its index may change:
    yae::weak_ptr<PlaylistItem, PlaylistNode> playingOld = lookup(playing_);

    // a temporary playlist tree used for deciding which of the
    // newly added items should be selected for playback:
    TPlaylistTree tmpTree;

    for (std::list<QString>::const_iterator i = playlist.begin();
         i != playlist.end(); ++i)
    {
      QString path = *i;

      std::list<PlaylistKey> keys;
      if (!getKeyPath(keys, path))
      {
        continue;
      }

      if (!keys.empty())
      {
        tmpTree.set(keys, path);
        tree_.set(keys, path);
      }
    }

    typedef TPlaylistTree::FringeGroup TFringeGroup;
    typedef std::map<PlaylistKey, QString> TSiblings;

    // lookup the first new item:
    const QString * firstNewItemPath = tmpTree.findFirstFringeItemValue();

    // return hash keys of newly added groups and items:
    if (returnBookmarkHashList)
    {
      std::list<TFringeGroup> fringeGroups;
      tmpTree.get(fringeGroups);

      for (std::list<TFringeGroup>::const_iterator i = fringeGroups.begin();
           i != fringeGroups.end(); ++i)
      {
        // shortcuts:
        const std::list<PlaylistKey> & keyPath = i->fullPath_;
        const TSiblings & siblings = i->siblings_;

        returnBookmarkHashList->push_back(BookmarkHashInfo());
        BookmarkHashInfo & hashInfo = returnBookmarkHashList->back();
        hashInfo.groupHash_ = getKeyPathHash(keyPath);

        for (TSiblings::const_iterator j = siblings.begin();
             j != siblings.end(); ++j)
        {
          const PlaylistKey & key = j->first;
          hashInfo.itemHash_.push_back(getKeyHash(key));
        }
      }
    }

    // flatten the tree into a list of play groups:
    std::list<TFringeGroup> fringeGroups;
    tree_.get(fringeGroups);

    // remove leading redundant keys from abbreviated paths:
    while (!fringeGroups.empty() &&
           is_size_two_or_more(fringeGroups.front().abbreviatedPath_))
    {
      const PlaylistKey & head = fringeGroups.front().abbreviatedPath_.front();

      bool same = true;
      std::list<TFringeGroup>::iterator i = fringeGroups.begin();
      for (++i; same && i != fringeGroups.end(); ++i)
      {
        const std::list<PlaylistKey> & abbreviatedPath = i->abbreviatedPath_;
        const PlaylistKey & key = abbreviatedPath.front();
        same = is_size_two_or_more(abbreviatedPath) && (key == head);
      }

      if (!same)
      {
        break;
      }

      // remove the head of abbreviated path of each group:
      for (i = fringeGroups.begin(); i != fringeGroups.end(); ++i)
      {
        i->abbreviatedPath_.pop_front();
      }
    }

    // setup a map for fast group lookup:
    std::map<PlaylistGroup::TKey, TPlaylistGroupPtr> groupMap;
    init_lookup<PlaylistGroup>(groupMap, groups_);

    // update the group vector:
    numItems_ = 0;
    std::size_t groupCount = 0;

    std::vector<TPlaylistGroupPtr> groupVec;
    for (std::list<TFringeGroup>::const_iterator i = fringeGroups.begin();
         i != fringeGroups.end(); ++i)
    {
      // shortcut:
      const TFringeGroup & fringeGroup = *i;

      TPlaylistGroupPtr groupPtr =
        yae::lookup<PlaylistGroup>(groupMap, fringeGroup.fullPath_);
      bool newGroup = !groupPtr;

      if (newGroup)
      {
        emit addingGroup(groupCount);

        groupPtr.reset(new PlaylistGroup());
        groupMap[fringeGroup.fullPath_] = groupPtr;
        groups_.insert(groups_.begin() + groupCount, groupPtr);
      }

      PlaylistGroup & group = *groupPtr;
      group.offset_ = numItems_;
      group.row_ = groupCount;
      groupCount++;

      if (newGroup)
      {
        group.keyPath_ = fringeGroup.fullPath_;
        group.name_ = toWords(fringeGroup.abbreviatedPath_);
        group.hash_ = getKeyPathHash(group.keyPath_);

        emit addedGroup(group.row_);
     }

      // setup a map for fast item lookup:
      std::map<PlaylistItem::TKey, TPlaylistItemPtr> itemMap;
      init_lookup<PlaylistItem>(itemMap, group.items_);

      // update the items vector:
      std::size_t groupSize = 0;

      // shortcuts:
      const TSiblings & siblings = fringeGroup.siblings_;

      for (TSiblings::const_iterator j = siblings.begin();
           j != siblings.end(); ++j, ++numItems_)
      {
        const PlaylistKey & key = j->first;
        const QString & value = j->second;

        TPlaylistItemPtr itemPtr = yae::lookup<PlaylistItem>(itemMap, key);
        bool newItem = !itemPtr;

        if (newItem)
        {
          emit addingItem(group.row_, groupSize);

          itemPtr.reset(new PlaylistItem(group));
          itemMap[key] = itemPtr;
          group.items_.insert(group.items_.begin() + groupSize, itemPtr);
        }

        PlaylistItem & item = *itemPtr;
        item.row_ = groupSize;
        groupSize++;

        if (newItem)
        {
          item.key_ = key;
          item.path_ = value;
          item.name_ = toWords(key.key_);
          item.ext_ = key.ext_;
          item.hash_ = getKeyHash(item.key_);

          QFileInfo fi(item.path_);
          if (fi.exists())
          {
            item.msecUtcUpdated_ = fi.lastModified().toMSecsSinceEpoch();
          }
        }

        if (firstNewItemPath && *firstNewItemPath == item.path_)
        {
          currentNow = numItems_;
        }

        if (newItem)
        {
          emit addedItem(group.row_, item.row_);
        }
      }
    }

    updateOffsets();
    setPlayingItem(currentNow, playingOld.lock());
    setCurrentItem(currentNow);
    selectItem(currentNow);

    emit itemCountChanged();

#if 0
    dump(*this, "Playlist::add:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::playingItem
  //
  std::size_t
  Playlist::playingItem() const
  {
    return playing_;
  }

  //----------------------------------------------------------------
  // Playlist::currentItem
  //
  std::size_t
  Playlist::currentItem() const
  {
    return current_;
  }

  //----------------------------------------------------------------
  // Playlist::closestGroup
  //
  TPlaylistGroupPtr
  Playlist::closestGroup(std::size_t index,
                         Playlist::TDirection where) const
  {
    (void) where;

    // no items have been excluded:
    return lookupGroup(index);
  }

  //----------------------------------------------------------------
  // Playlist::closestItem
  //
  std::size_t
  Playlist::closestItem(std::size_t index,
                        Playlist::TDirection where,
                        TPlaylistGroupPtr * returnGroup) const
  {
    (void) where;

    // no items have been excluded:
    if (returnGroup)
    {
      *returnGroup = lookupGroup(index);
    }

    return index;
  }

  //----------------------------------------------------------------
  // Playlist::setPlayingItem
  //
  void
  Playlist::setPlayingItem(std::size_t index, bool force)
  {
    if (index == playing_ && !force)
    {
      return;
    }

    TPlaylistItemPtr prev = lookup(playing_);
    setPlayingItem(index, prev);

    setCurrentItem(playing_);
    selectItem(playing_);
  }

  //----------------------------------------------------------------
  // Playlist::selectAll
  //
  void
  Playlist::selectAll()
  {
    discardSelectionAnchor();

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      selectGroup(group);
    }

#if 0
    dump(*this, "Playlist::selectAll:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::unselectAll
  //
  void
  Playlist::unselectAll()
  {
    discardSelectionAnchor();

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      unselectGroup(group);
    }

#if 0
    dump(*this, "Playlist::unselectAll:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::selectGroup
  //
  void
  Playlist::selectGroup(PlaylistGroup & group)
  {
    discardSelectionAnchor();

    for (std::vector<TPlaylistItemPtr>::iterator i = group.items_.begin();
         i != group.items_.end(); ++i)
    {
      PlaylistItem & item = *(*i);
      setSelectedItem(item, true);
    }

#if 0
    dump(*this, "Playlist::selectGroup:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::unselectGroup
  //
  void
  Playlist::unselectGroup(PlaylistGroup & group)
  {
    discardSelectionAnchor();

    for (std::vector<TPlaylistItemPtr>::iterator i = group.items_.begin();
         i != group.items_.end(); ++i)
    {
      PlaylistItem & item = *(*i);
      setSelectedItem(item, false);
    }

#if 0
    dump(*this, "Playlist::unselectGroup:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::unselectItem
  //
  void
  Playlist::unselectItem(std::size_t index)
  {
    discardSelectionAnchor();

    TPlaylistItemPtr itemPtr = lookup(index);
    if (!itemPtr)
    {
      return;
    }

    PlaylistItem & item = *itemPtr;
    setSelectedItem(item, false);
  }

  //----------------------------------------------------------------
  // Playlist::selectItem
  //
  void
  Playlist::selectItem(std::size_t index, bool exclusive)
  {
    discardSelectionAnchor();

    if (exclusive)
    {
      selectItems(index, index, exclusive);
      return;
    }

    TPlaylistItemPtr itemPtr = lookup(index);
    if (!itemPtr)
    {
      return;
    }

    PlaylistItem & item = *itemPtr;
    setSelectedItem(item, true);
  }

  //----------------------------------------------------------------
  // Playlist::selectItems
  //
  void
  Playlist::selectItems(std::size_t i0, std::size_t i1, bool exclusive)
  {
    std::set<std::size_t> items;
    for (std::size_t i = i0; i <= i1; i++)
    {
      items.insert(i);
    }

    selectItems(items, exclusive);
  }

  //----------------------------------------------------------------
  // Playlist::selectItems
  //
  void
  Playlist::selectItems(const std::set<std::size_t> & items, bool exclusive)
  {
    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);

      for (std::vector<TPlaylistItemPtr>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *(*j);
        std::size_t index = group.offset_ + item.row_;

        if (items.find(index) != items.end())
        {
          setSelectedItem(item, true);
        }
        else if (exclusive)
        {
          setSelectedItem(item, false);
        }
      }
    }

#if 0
    dump(*this, "Playlist::selectItems:\n");
#endif
  }

  //----------------------------------------------------------------
  // Playlist::selectionAnchor
  //
  std::size_t
  Playlist::selectionAnchor()
  {
    if (selectionAnchor_ >= numItems_)
    {
      selectionAnchor_ = current_;
    }

    return selectionAnchor_;
  }

  //----------------------------------------------------------------
  // Playlist::discardSelectionAnchor
  //
  void
  Playlist::discardSelectionAnchor()
  {
    selectionAnchor_ = numItems_;
  }

  //----------------------------------------------------------------
  // Playlist::removeSelected
  //
  void
  Playlist::removeSelected()
  {
#if 0
    yae_debug << "Playlist::removeSelected";
#endif

    int playingNow = playing_;
    int currentNow = current_;

    // try to keep track of the original playing item,
    // its index may change:

    // NOTE: must keep alive the group as well, because PlaylistItem
    // has a reference to its group, so if the group is deleted
    // it becomes a dangling reference:
    TPlaylistGroupPtr keepAlivePlayingOldGroup;

    yae::weak_ptr<PlaylistItem, PlaylistNode> playingOld =
      lookup(playing_, &keepAlivePlayingOldGroup);

    for (int groupRow = groups_.size() - 1; groupRow >= 0; groupRow--)
    {
      PlaylistGroup & group = *(groups_[groupRow]);
      YAE_ASSERT(groupRow == int(group.row_));

      for (int itemRow = group.items_.size() - 1; itemRow >= 0; itemRow--)
      {
        PlaylistItem & item = *(group.items_[itemRow]);
        YAE_ASSERT(itemRow == int(item.row_));

        if (!item.selected_)
        {
          continue;
        }

        // remove one item:
        removeItem(group, itemRow, playingNow, currentNow);
      }

      // if the group is empty and has a key path, remove it:
      if (group.items_.empty() && !group.keyPath_.empty())
      {
        emit removingGroup(groupRow);

        std::vector<TPlaylistGroupPtr>::iterator
          iter = groups_.begin() + groupRow;
        groups_.erase(iter);

        emit removedGroup(groupRow);
      }
    }

    removeItemsFinalize(playingOld.lock(), playingNow, currentNow);
  }

  //----------------------------------------------------------------
  // Playlist::removeItems
  //
  void
  Playlist::removeItems(int groupRow, int itemRow)
  {
#if 0
    yae_debug << "Playlist::removeItems";
#endif

    int playingNow = playing_;
    int currentNow = current_;

    TPlaylistGroupPtr groupPtr;
    TPlaylistItemPtr itemPtr = lookup(groupPtr, groupRow, itemRow);

    if (!groupPtr)
    {
      YAE_ASSERT(false);
      return;
    }

    PlaylistGroup & group = *groupPtr;
    if (!(itemPtr || itemRow < 0 || itemRow >= int(group.items_.size())))
    {
      YAE_ASSERT(false);
      return;
    }

    // try to keep track of the original playing item,
    // its index may change:

    // NOTE: must keep alive the group as well, because PlaylistItem
    // has a reference to its group, so if the group is deleted
    // it becomes a dangling reference:
    TPlaylistGroupPtr keepAlivePlayingOldGroup;

    yae::weak_ptr<PlaylistItem, PlaylistNode> playingOld =
      lookup(playing_, &keepAlivePlayingOldGroup);

    if (itemPtr)
    {
      // remove one item:
      removeItem(group, itemRow, playingNow, currentNow);
    }
    else
    {
      // remove entire group:
      for (int i = group.items_.size() - 1; i >= 0; i--)
      {
        removeItem(group, i, playingNow, currentNow);
      }
    }

    // if the group is empty and has a key path, remove it:
    if (group.items_.empty() && !group.keyPath_.empty())
    {
      int groupRow = group.row_;
      emit removingGroup(groupRow);

      std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin() + groupRow;
      groups_.erase(i);

      emit removedGroup(groupRow);
    }

    removeItemsFinalize(playingOld.lock(), playingNow, currentNow);
  }

  //----------------------------------------------------------------
  // Playlist::setCurrentItem
  //
  bool
  Playlist::setCurrentItem(std::size_t index)
  {
#if 0
    yae_debug << "Playlist::setCurrentItem: (" << index << ")";

    if (index == current_)
    {
      return false;
    }
#endif
    current_ = index;

    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = lookup(current_, &group);
    emit currentChanged(group ? group->row_ : -1,
                        item ? item->row_ : -1);
    return true;
  }

  //----------------------------------------------------------------
  // Playlist::setCurrentItem
  //
  bool
  Playlist::setCurrentItem(int groupRow, int itemRow)
  {
    std::size_t index = lookupIndex(groupRow, itemRow);

    index = closestItem(index);

    return setCurrentItem(index);
  }

  //----------------------------------------------------------------
  // Playlist::getCurrentItem
  //
  void
  Playlist::getCurrentItem(int & groupRow, int & itemRow) const
  {
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = lookup(current_, &group);
    groupRow = group ? group->row_ : -1;
    itemRow = item ? item->row_ : -1;
  }

  //----------------------------------------------------------------
  // Playlist::toggleSelectedItem
  //
  bool
  Playlist::toggleSelectedItem(std::size_t index)
  {
    discardSelectionAnchor();

    TPlaylistItemPtr itemPtr = lookup(index);
    if (!itemPtr)
    {
      return false;
    }

    PlaylistItem & item = *itemPtr;
    return setSelectedItem(item, !item.selected_);
  }

  //----------------------------------------------------------------
  // Playlist::setSelectedItem
  //
  bool
  Playlist::setSelectedItem(std::size_t index)
  {
    discardSelectionAnchor();

    TPlaylistItemPtr item = lookup(index);
    if (!item)
    {
      return false;
    }

    return setSelectedItem(*item, true);
  }

  //----------------------------------------------------------------
  // Playlist::setSelectedItem
  //
  bool
  Playlist::setSelectedItem(PlaylistItem & item, bool selected)
  {
    if (item.selected_ == selected)
    {
      return false;
    }

    item.selected_ = selected;
    emit selectedChanged(item.group_.row_, item.row_);

    return true;
  }

  //----------------------------------------------------------------
  // lookupLastGroupIndex
  //
  // return index of the last non-excluded group:
  //
  static std::size_t
  lookupLastGroupIndex(const std::vector<TPlaylistGroupPtr> & groups)
  {
    std::size_t index = groups.size();
    return index ? index - 1 : index;
  }

  //----------------------------------------------------------------
  // lookupLastGroup
  //
  inline static TPlaylistGroupPtr
  lookupLastGroup(const std::vector<TPlaylistGroupPtr> & groups)
  {
    std::size_t numGroups = groups.size();
    std::size_t i = lookupLastGroupIndex(groups);

    TPlaylistGroupPtr found;

    if (i < numGroups)
    {
      found = groups[i];
    }

    return found;
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  TPlaylistGroupPtr
  Playlist::lookupGroup(std::size_t index) const
  {
#if 0
    yae_debug << "Playlist::lookupGroup: " << index;
#endif

    if (groups_.empty())
    {
      return TPlaylistGroupPtr();
    }

    if (index >= numItems_)
    {
      return TPlaylistGroupPtr();
    }

    const std::size_t numGroups = groups_.size();
    std::size_t i0 = 0;
    std::size_t i1 = numGroups;

    while (i0 != i1)
    {
      std::size_t i = i0 + (i1 - i0) / 2;

      const PlaylistGroup & group = *(groups_[i]);
      std::size_t numItems = group.items_.size();
      std::size_t groupEnd = group.offset_ + numItems;

      if (index < groupEnd)
      {
        i1 = std::min<std::size_t>(i, i1 - 1);
      }
      else
      {
        i0 = std::max<std::size_t>(i, i0 + 1);
      }
    }

    if (i0 < numGroups)
    {
      const TPlaylistGroupPtr & group = groups_[i0];
      std::size_t numItems = group->items_.size();
      std::size_t groupEnd = group->offset_ + numItems;

      if (index < groupEnd)
      {
        return group;
      }
    }

    YAE_ASSERT(false);
    return lookupLastGroup(groups_);
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  TPlaylistItemPtr
  Playlist::lookup(std::size_t index, TPlaylistGroupPtr * returnGroup) const
  {
#if 0
    yae_debug << "Playlist::lookup: " << index;
#endif

    TPlaylistGroupPtr group = lookupGroup(index);
    if (returnGroup)
    {
      *returnGroup = group;
    }

    if (group)
    {
      std::size_t groupSize = group->items_.size();
      std::size_t i = index - group->offset_;

      YAE_ASSERT(i < groupSize || index == numItems_);

      return
        (i < groupSize) ?
        group->items_[i] :
        TPlaylistItemPtr();
    }

    return TPlaylistItemPtr();
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  TPlaylistGroupPtr
  Playlist::lookupGroup(const std::string & groupHash) const
  {
    if (groupHash.empty())
    {
      return TPlaylistGroupPtr();
    }

    const std::size_t numGroups = groups_.size();
    for (std::size_t i = 0; i < numGroups; i++)
    {
      const TPlaylistGroupPtr & group = groups_[i];
      if (groupHash == group->hash_)
      {
        return group;
      }
    }

    return TPlaylistGroupPtr();
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  TPlaylistItemPtr
  Playlist::lookup(const std::string & groupHash,
                   const std::string & itemHash,
                   TPlaylistGroupPtr * returnGroup) const
  {
    TPlaylistGroupPtr group = lookupGroup(groupHash);
    if (!group || itemHash.empty())
    {
      return TPlaylistItemPtr();
    }

    if (returnGroup)
    {
      *returnGroup = group;
    }

    std::size_t groupSize = group->items_.size();
    for (std::size_t i = 0; i < groupSize; i++)
    {
      const TPlaylistItemPtr & item = group->items_[i];
      if (itemHash == item->hash_)
      {
        return item;
      }
    }

    return TPlaylistItemPtr();
  }

  //----------------------------------------------------------------
  // Playlist::lookupItemFilePath
  //
  QString
  Playlist::lookupItemFilePath(const QString & id) const
  {
    std::string groupHashItemHash(id.toUtf8().constData());
    std::size_t slash = groupHashItemHash.find_first_of('/');
    std::string groupHash = groupHashItemHash.substr(0, slash);
    std::string itemHash = groupHashItemHash.substr(slash + 1);

    TPlaylistItemPtr found = lookup(groupHash, itemHash);
    if (!found)
    {
      return QString();
    }

    const PlaylistItem & item = *found;
    return item.path_;
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  TPlaylistItemPtr
  Playlist::lookup(TPlaylistGroupPtr & group, int groupRow, int itemRow) const
  {
    if (groupRow < 0 || groupRow >= int(groups_.size()))
    {
      group = TPlaylistGroupPtr();
      return TPlaylistItemPtr();
    }

    group = groups_[groupRow];
    if (itemRow < 0 || itemRow >= int(group->items_.size()))
    {
      return TPlaylistItemPtr();
    }

    const TPlaylistItemPtr & item = group->items_[itemRow];
    return item;
  }

  //----------------------------------------------------------------
  // Playlist::lookupIndex
  //
  std::size_t
  Playlist::lookupIndex(int groupRow, int itemRow) const
  {
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = lookup(group, groupRow, itemRow);

    if (!group && !item)
    {
      return numItems_;
    }

    std::size_t index = 0;

    if (group)
    {
      index += group->offset_;
    }

    if (item)
    {
      index += item->row_;
    }

    return index;
  }

  //----------------------------------------------------------------
  // Playlist::updateOffsets
  //
  void
  Playlist::updateOffsets()
  {
#if 0
    yae_debug << "Playlist::updateOffsets";
#endif

    std::size_t offset = 0;

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      group.offset_ = offset;
      group.row_ = i - groups_.begin();

      qint64 lastModified = std::numeric_limits<qint64>::min();
      for (std::vector<TPlaylistItemPtr>::iterator j = group.items_.begin(),
             j0 = j, j1 = group.items_.end(); j != j1; ++j)
      {
        PlaylistItem & item = *(*j);
        item.row_ = j - j0;
        lastModified = std::max<qint64>(lastModified, item.msecUtcUpdated_);
      }

      group.msecUtcUpdated_ = lastModified;
      offset += group.items_.size();
    }

    numItems_ = offset;
  }

  //----------------------------------------------------------------
  // Playlist::setPlayingItem
  //
  void
  Playlist::setPlayingItem(std::size_t index, const TPlaylistItemPtr & prev)
  {
#if 0
    yae_debug << "Playlist::setPlayingItem: " << index;
#endif

    std::size_t indexOld = playing_;

    if (prev)
    {
      indexOld = prev->group_.offset_;
      indexOld += prev->row_;
    }

    playing_ = (index < numItems_) ? index : numItems_;

    TPlaylistItemPtr playingNow = lookup(playing_);
    if (!playingNow || playingNow != prev)
    {
      emit playingChanged(playing_, indexOld);
    }
  }

  //----------------------------------------------------------------
  // Playlist::removeItem
  //
  void
  Playlist::removeItem(PlaylistGroup & group,
                       int itemRow,
                       int & playingNow,
                       int & currentNow)
  {
    if (itemRow >= int(group.items_.size()))
    {
      YAE_ASSERT(false);
      return;
    }

    emit removingItem(group.row_, itemRow);

    // remove one item:
    std::vector<TPlaylistItemPtr>::iterator i = group.items_.begin() + itemRow;
    PlaylistItem & item = *(*i);

    int index = group.offset_ + itemRow;

    if (index < playingNow)
    {
      // adjust the playing index:
      playingNow--;
    }

    if (index < currentNow)
    {
      // adjust the current index:
      currentNow--;
    }

    // 1. remove the item from the tree:
    std::list<PlaylistKey> keyPath = group.keyPath_;
    keyPath.push_back(item.key_);
    tree_.remove(keyPath);

    // 2. remove the item from the group:
    group.items_.erase(i);

    emit removedItem(group.row_, itemRow);
  }

  //----------------------------------------------------------------
  // Playlist::removeItemsFinalize
  //
  void
  Playlist::removeItemsFinalize(const TPlaylistItemPtr & playingOld,
                                int playingNow,
                                int currentNow)
  {
    updateOffsets();

    int lastIndex = (int)numItems_ - 1;

    playingNow = std::max<int>(0, std::min<int>(playingNow, lastIndex));
    currentNow = std::max<int>(0, std::min<int>(currentNow, lastIndex));

    playingNow = closestItem(playingNow, kBehind);
    currentNow = closestItem(currentNow, kBehind);

    setPlayingItem(playingNow, playingOld);
    setCurrentItem(currentNow);
    selectItem(currentNow);

    emit itemCountChanged();
  }
}

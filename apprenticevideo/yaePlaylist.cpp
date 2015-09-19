// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:41:07 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <algorithm>
#include <limits>

// boost includes:
#include <boost/filesystem.hpp>

// Qt includes:
#include <QCryptographicHash>
#include <QFileInfo>
#include <QUrl>

// yae includes:
#include <yae/utils/yae_utils.h>

// local includes:
#include <yaePlaylist.h>
#include <yaeUtilsQt.h>

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
    std::cerr << "\n\n" << message;

    const std::vector<TPlaylistGroupPtr> & groups = playlist.groups();
    for (std::size_t i = 0; i < groups.size(); i++)
    {
      const PlaylistGroup & group = *(groups[i]);
      std::cerr
        << "\ngroup " << i
        << ", row: " << group.row_
        << ", offset: " << group.offset_
        << ", size: " << group.items_.size()
        << std::endl;

      for (std::size_t j = 0; j < group.items_.size(); j++)
      {
        const PlaylistItem & item = *(group.items_[j]);
        std::cerr
          << "  item " << j
          << ", row: " << item.row_
          << ", selected: " << item.selected_
          << std::endl;
      }
    }
  }

  //----------------------------------------------------------------
  // getKeyPathHash
  //
  static std::string
  getKeyPathHash(const std::list<PlaylistKey> & keyPath)
  {
    QCryptographicHash crypto(QCryptographicHash::Sha1);
    for (std::list<PlaylistKey>::const_iterator i = keyPath.begin();
         i != keyPath.end(); ++i)
    {
      const PlaylistKey & key = *i;
      crypto.addData(key.key_.toUtf8());
      crypto.addData(key.ext_.toUtf8());
    }

    std::string groupHash(crypto.result().toHex().constData());
    return groupHash;
  }

  //----------------------------------------------------------------
  // getKeyHash
  //
  static std::string
  getKeyHash(const PlaylistKey & key)
  {
    QCryptographicHash crypto(QCryptographicHash::Sha1);
    crypto.addData(key.key_.toUtf8());
    crypto.addData(key.ext_.toUtf8());

    std::string itemHash(crypto.result().toHex().constData());
    return itemHash;
  }


  //----------------------------------------------------------------
  // PlaylistKey::PlaylistKey
  //
  PlaylistKey::PlaylistKey(const QString & key, const QString & ext):
    key_(key),
    ext_(ext)
  {}

  //----------------------------------------------------------------
  // PlaylistKey::operator
  //
  bool
  PlaylistKey::operator == (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return false;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return !diff;
  }

  //----------------------------------------------------------------
  // PlaylistKey::operator <
  //
  bool
  PlaylistKey::operator < (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return diff < 0;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return diff < 0;
  }

  //----------------------------------------------------------------
  // PlaylistKey::operator >
  //
  bool PlaylistKey::operator > (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return diff > 0;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return diff > 0;
  }

  //----------------------------------------------------------------
  // PlaylistNode::PlaylistNode
  //
  PlaylistNode::PlaylistNode():
    row_(~0)
  {}

  //----------------------------------------------------------------
  // PlaylistNode::PlaylistNode
  //
  PlaylistNode::PlaylistNode(const PlaylistNode & other):
    row_(other.row_)
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
    excluded_(false),
    failed_(false)
  {}


  //----------------------------------------------------------------
  // PlaylistGroup::PlaylistGroup
  //
  PlaylistGroup::PlaylistGroup():
    offset_(0),
    collapsed_(false),
    excluded_(false)
  {}


  //----------------------------------------------------------------
  // Playlist::Playlist
  //
  Playlist::Playlist():
    numItems_(0),
    numShown_(0),
    numShownGroups_(0),
    playing_(0),
    current_(0),
    selectionAnchor_(0)
  {
    add(std::list<QString>());
  }

  //----------------------------------------------------------------
  // toWords
  //
  static QString
  toWords(const std::list<PlaylistKey> & keys)
  {
    std::list<QString> words;
    for (std::list<PlaylistKey>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
    {
      if (!words.empty())
      {
        // right-pointing double angle bracket:
        words.push_back(QString::fromUtf8(" ""\xc2""\xbb"" "));
      }

      const PlaylistKey & key = *i;
      splitIntoWords(key.key_, words);

      if (!key.ext_.isEmpty())
      {
        words.push_back(key.ext_);
      }
    }

    return toQString(words, true);
  }

  //----------------------------------------------------------------
  // init_lookup
  //
  template <typename TNode>
  static void
  init_lookup(std::map<typename TNode::TKey, std::shared_ptr<TNode> > & lookup,
              const std::vector<std::shared_ptr<TNode> > & nodes)
  {
    typedef std::shared_ptr<TNode> TNodePtr;
    typedef std::vector<std::shared_ptr<TNode> > TNodes;
    typedef typename TNodes::const_iterator TNodesIter;

    lookup.clear();

    for (TNodesIter i = nodes.begin(); i != nodes.end(); ++i)
    {
      const TNodePtr & node = *i;
      lookup[node->key()] = node;
    }
  }

  //----------------------------------------------------------------
  // lookup
  //
  template <typename TNode>
  static std::shared_ptr<TNode>
  lookup(const std::map<typename TNode::TKey, std::shared_ptr<TNode> > & nodes,
         const typename TNode::TKey & key)
  {
    typedef std::shared_ptr<TNode> TNodePtr;
    typedef std::map<typename TNode::TKey, std::shared_ptr<TNode> > TNodeMap;

    typename TNodeMap::const_iterator found = nodes.find(key);
    return (found == nodes.end()) ? TNodePtr() : found->second;
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
    std::cerr << "Playlist::add" << std::endl;
#endif

    std::size_t currentNow = current_;

    // try to keep track of the original playing item,
    // its index may change:
    std::weak_ptr<PlaylistItem> playingOld = lookup(playing_);

    // a temporary playlist tree used for deciding which of the
    // newly added items should be selected for playback:
    TPlaylistTree tmpTree;

    for (std::list<QString>::const_iterator i = playlist.begin();
         i != playlist.end(); ++i)
    {
      QString path = *i;
      QString humanReadablePath = path;

      QFileInfo fi(path);
      if (fi.exists())
      {
        path = fi.absoluteFilePath();
        humanReadablePath = path;
      }
      else
      {
        QUrl url;

#if YAE_QT4
        url.setEncodedUrl(path.toUtf8(), QUrl::StrictMode);
#elif YAE_QT5
        url.setUrl(path, QUrl::StrictMode);
#endif

        if (url.isValid())
        {
          humanReadablePath = url.toString();
        }
      }

      fi = QFileInfo(humanReadablePath);
      QString name = toWords(fi.completeBaseName());

      if (name.isEmpty())
      {
#if 0
        std::cerr << "IGNORING: " << i->toUtf8().constData() << std::endl;
#endif
        continue;
      }

      // tokenize it, convert into a tree key path:
      std::list<PlaylistKey> keys;
      while (true)
      {
        QString key = fi.fileName();
        if (key.isEmpty())
        {
          break;
        }

        QFileInfo parseKey(key);
        QString base;
        QString ext;

        if (keys.empty())
        {
          base = parseKey.completeBaseName();
          ext = parseKey.suffix();
        }
        else
        {
          base = parseKey.fileName();
        }

        if (keys.empty() && ext.compare(kExtEyetv, Qt::CaseInsensitive) == 0)
        {
          // handle Eye TV archive more gracefully:
          QString program;
          QString episode;
          QString timestamp;
          if (!parseEyetvInfo(path, program, episode, timestamp))
          {
            break;
          }

          if (episode.isEmpty())
          {
            key = timestamp + " " + program;
            keys.push_front(PlaylistKey(key, QString()));
          }
          else
          {
            key = timestamp + " " + episode;
            keys.push_front(PlaylistKey(key, QString()));
          }

          key = program;
          keys.push_front(PlaylistKey(key, QString()));
        }
        else
        {
          key = prepareForSorting(base);
          keys.push_front(PlaylistKey(key, ext));
        }

        QString next = fi.absolutePath();
        fi = QFileInfo(next);
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
           isSizeTwoOrMore(fringeGroups.front().abbreviatedPath_))
    {
      const PlaylistKey & head = fringeGroups.front().abbreviatedPath_.front();

      bool same = true;
      std::list<TFringeGroup>::iterator i = fringeGroups.begin();
      for (++i; same && i != fringeGroups.end(); ++i)
      {
        const std::list<PlaylistKey> & abbreviatedPath = i->abbreviatedPath_;
        const PlaylistKey & key = abbreviatedPath.front();
        same = isSizeTwoOrMore(abbreviatedPath) && (key == head);
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
    init_lookup(groupMap, groups_);

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
        yae::lookup(groupMap, fringeGroup.fullPath_);
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
      init_lookup(itemMap, group.items_);

      // update the items vector:
      std::size_t groupSize = 0;

      // shortcuts:
      const TSiblings & siblings = fringeGroup.siblings_;

      for (TSiblings::const_iterator j = siblings.begin();
           j != siblings.end(); ++j, ++numItems_)
      {
        const PlaylistKey & key = j->first;
        const QString & value = j->second;

        TPlaylistItemPtr itemPtr = yae::lookup(itemMap, key);
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

    if (applyFilter())
    {
      currentNow = closestItem(currentNow);
    }

    updateOffsets();
    setPlayingItem(currentNow, playingOld.lock());
    setCurrentItem(currentNow);
    selectItem(currentNow);

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
  // Playlist::countItems
  //
  std::size_t
  Playlist::countItems() const
  {
    return numItems_;
  }

  //----------------------------------------------------------------
  // Playlist::countItemsAhead
  //
  std::size_t
  Playlist::countItemsAhead() const
  {
    return (playing_ < numItems_) ? (numItems_ - playing_) : 0;
  }

  //----------------------------------------------------------------
  // Playlist::countItemsBehind
  //
  std::size_t
  Playlist::countItemsBehind() const
  {
    return (playing_ < numItems_) ? playing_ : numItems_;
  }

  //----------------------------------------------------------------
  // Playlist::closestGroup
  //
  TPlaylistGroupPtr
  Playlist::closestGroup(std::size_t index,
                         Playlist::TDirection where) const
  {
    if (numItems_ == numShown_)
    {
      // no items have been excluded:
      return lookupGroup(index);
    }

    TPlaylistGroupPtr prev;

    for (std::vector<TPlaylistGroupPtr>::const_iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      const TPlaylistGroupPtr & groupPtr = *i;
      const PlaylistGroup & group = *groupPtr;

      if (group.excluded_)
      {
        continue;
      }

      std::size_t groupSize = group.items_.size();
      std::size_t groupEnd = group.offset_ + groupSize;
      if (groupEnd <= index)
      {
        prev = groupPtr;
      }

      if (index < groupEnd)
      {
        if (index >= group.offset_)
        {
          // make sure the group has an un-excluded item
          // in the range that we are interested in:
          const int step = where == kAhead ? 1 : -1;

          for (std::size_t j = index - group.offset_; j < groupSize; j += step)
          {
            const PlaylistItem & item = *(group.items_[j]);
            if (!item.excluded_)
            {
              return groupPtr;
            }
          }
        }
        else if (where == kAhead)
        {
          return groupPtr;
        }

        if (where == kBehind)
        {
          break;
        }
      }
    }

    if (where == kBehind)
    {
      return prev;
    }

    return TPlaylistGroupPtr();
  }

  //----------------------------------------------------------------
  // Playlist::closestItem
  //
  std::size_t
  Playlist::closestItem(std::size_t index,
                        Playlist::TDirection where,
                        TPlaylistGroupPtr * returnGroup) const
  {
    if (numItems_ == numShown_)
    {
      // no items have been excluded:
      if (returnGroup)
      {
        *returnGroup = lookupGroup(index);
      }

      return index;
    }

    TPlaylistGroupPtr group = closestGroup(index, where);
    if (returnGroup)
    {
      *returnGroup = group;
    }

    if (!group)
    {
      if (where == kAhead)
      {
        return numItems_;
      }

      // nothing left behind, try looking ahead for the first un-excluded item:
      return closestItem(index, kAhead, returnGroup);
    }

    // find the closest item within this group:
    const std::vector<TPlaylistItemPtr> & items = group->items_;
    const std::size_t groupSize = items.size();
    const int step = where == kAhead ? 1 : -1;

    std::size_t i =
      index < group->offset_ ? 0 :
      index - group->offset_ >= groupSize ? groupSize - 1 :
      index - group->offset_;

    for (; i < groupSize; i += step)
    {
      const PlaylistItem & item = *(items[i]);
      if (!item.excluded_)
      {
        return (group->offset_ + i);
      }
    }

    if (where == kAhead)
    {
      return numItems_;
    }

    // nothing left behind, try looking ahead for the first un-excluded item:
    return closestItem(index, kAhead, returnGroup);
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
    std::cerr << "KEYWORDS MATCH: " << text.toUtf8().constData() << std::endl;
#endif
    return true;
  }

  //----------------------------------------------------------------
  // Playlist::filterChanged
  //
  bool
  Playlist::filterChanged(const QString & filter)
  {
    keywords_.clear();
    splitIntoWords(filter, keywords_);

    if (applyFilter())
    {
      updateOffsets();
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // Playlist::applyFilter
  //
  bool
  Playlist::applyFilter()
  {
    bool exclude = !keywords_.empty();
    bool changed = false;
    std::size_t index = 0;

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);

      std::size_t groupSize = group.items_.size();
      std::size_t numExcluded = 0;

      for (std::vector<TPlaylistItemPtr>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j, index++)
      {
        PlaylistItem & item = *(*j);

        if (!exclude)
        {
          if (item.excluded_)
          {
            item.excluded_ = false;
            changed = true;
          }

          continue;
        }

        QString text =
          group.name_ + QString::fromUtf8(" ") +
          item.name_ + QString::fromUtf8(".") +
          item.ext_;

        if (index == playing_)
        {
          text += QObject::tr("NOW PLAYING");
        }

        if (!keywordsMatch(keywords_, text))
        {
          if (!item.excluded_)
          {
            item.excluded_ = true;
            changed = true;
          }

          numExcluded++;
        }
        else if (item.excluded_)
        {
          item.excluded_ = false;
          changed = true;
        }
      }

      if (!group.keyPath_.empty())
      {
        group.excluded_ = (groupSize == numExcluded);
      }
    }

    return changed;
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
      if (group.excluded_)
      {
        continue;
      }

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
      if (item.excluded_)
      {
        continue;
      }

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
#if 0
    std::cerr << "Playlist::selectItems: " << i0 << ", " << i1 << std::endl;
#endif

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      if (group.excluded_)
      {
        continue;
      }

      std::size_t groupEnd = group.offset_ + group.items_.size();

      for (std::vector<TPlaylistItemPtr>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *(*j);
        if (item.excluded_)
        {
          continue;
        }

        std::size_t index = group.offset_ + item.row_;
        if (i0 <= index && index <= i1)
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
    dump(*this, "Playlist::selectItem:\n");
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
    std::cerr << "Playlist::removeSelected" << std::endl;
#endif

    int playingNow = playing_;
    int currentNow = current_;
    bool playingRemoved = false;

    // try to keep track of the original playing item,
    // its index may change:
    std::weak_ptr<PlaylistItem> playingOld = lookup(playing_);

    for (int groupRow = groups_.size() - 1; groupRow >= 0; groupRow--)
    {
      PlaylistGroup & group = *(groups_[groupRow]);
      YAE_ASSERT(groupRow == group.row_);

      if (group.excluded_)
      {
        continue;
      }

      for (int itemRow = group.items_.size() - 1; itemRow >= 0; itemRow--)
      {
        PlaylistItem & item = *(group.items_[itemRow]);
        YAE_ASSERT(itemRow == item.row_);

        if (item.excluded_ || !item.selected_)
        {
          continue;
        }

        // remove one item:
        removeItem(group, itemRow, playingNow, currentNow);
      }

      // if the group is empty and has a key path, remove it:
      if (group.items_.empty() && !group.keyPath_.empty() && !group.excluded_)
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
    std::cerr << "Playlist::removeItems" << std::endl;
#endif

    int playingNow = playing_;
    int currentNow = current_;

    TPlaylistGroupPtr groupPtr;
    TPlaylistItemPtr itemPtr = lookup(groupPtr, groupRow, itemRow);

    if (!groupPtr || (itemRow < numItems_ && !itemPtr))
    {
      YAE_ASSERT(false);
      return;
    }

    PlaylistGroup & group = *groupPtr;

    // try to keep track of the original playing item,
    // its index may change:
    std::weak_ptr<PlaylistItem> playingOld = lookup(playing_);

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
    std::cerr
      << "Playlist::setCurrentItem: ("
      << index << ")"
      << std::endl;

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
    for (std::vector<TPlaylistGroupPtr>::const_reverse_iterator
           i = groups.rbegin(); i != groups.rend(); ++i, --index)
    {
      const PlaylistGroup & group = *(*i);
      if (!group.excluded_)
      {
#if 0
        std::cerr << "lookupGroup, last: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return index - 1;
      }
    }

    return groups.size();
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
    std::cerr << "Playlist::lookupGroup: " << index << std::endl;
#endif

    if (groups_.empty())
    {
      return NULL;
    }

    if (index >= numItems_)
    {
      YAE_ASSERT(index == numItems_);
      return NULL;
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
    std::cerr << "Playlist::lookup: " << index << std::endl;
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
                   std::size_t * returnItemIndex,
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
        if (*returnItemIndex)
        {
          *returnItemIndex = group->offset_ + i;
        }

        return item;
      }
    }

    if (*returnItemIndex)
    {
      *returnItemIndex = std::numeric_limits<std::size_t>::max();
    }

    return TPlaylistItemPtr();
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  TPlaylistItemPtr
  Playlist::lookup(TPlaylistGroupPtr & group, int groupRow, int itemRow) const
  {
    if (groupRow < 0 || groupRow >= groups_.size())
    {
      group = TPlaylistGroupPtr();
      return TPlaylistItemPtr();
    }

    group = groups_[groupRow];
    if (itemRow < 0 || itemRow >= group->items_.size())
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
    std::cerr << "Playlist::updateOffsets" << std::endl;
#endif

    std::size_t offset = 0;
    numShown_ = 0;
    numShownGroups_ = 0;

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      group.offset_ = offset;
      group.row_ = i - groups_.begin();

      if (!group.excluded_)
      {
        numShownGroups_++;
      }

      for (std::vector<TPlaylistItemPtr>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *(*j);
        item.row_ = j - group.items_.begin();

        if (!item.excluded_)
        {
          numShown_++;
        }
      }

      offset += group.items_.size();
    }

    numItems_ = offset;

    emit itemCountChanged();
  }

  //----------------------------------------------------------------
  // Playlist::setPlayingItem
  //
  void
  Playlist::setPlayingItem(std::size_t index, const TPlaylistItemPtr & prev)
  {
#if 0
    std::cerr << "Playlist::setPlayingItem" << std::endl;
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
    if (itemRow >= group.items_.size())
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
  }
}

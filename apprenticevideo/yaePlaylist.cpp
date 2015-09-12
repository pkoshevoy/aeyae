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
  PlaylistItem::PlaylistItem():
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
    current_(0)
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
  // TPlaylistGroupKey
  //
  typedef std::list<PlaylistKey> TPlaylistGroupKey;

  //----------------------------------------------------------------
  // TPlaylistGroupMap
  //
  typedef std::map<TPlaylistGroupKey, TPlaylistGroupPtr> TPlaylistGroupMap;

  //----------------------------------------------------------------
  // TPlaylistGroupVec
  //
  typedef std::vector<TPlaylistGroupPtr> TPlaylistGroupVec;

  //----------------------------------------------------------------
  // setup_group_map
  //
  static void
  setup_group_map(TPlaylistGroupMap & groupMap,
                  const TPlaylistGroupVec & groups)
  {
    for (TPlaylistGroupVec::const_iterator i = groups.begin();
         i != groups.end(); ++i)
    {
      const PlaylistGroup & group = *(*i);
      groupMap[group.keyPath_] = (*i);
    }
  }

  //----------------------------------------------------------------
  // get_group_ptr
  //
  static TPlaylistGroupPtr
  get_group_ptr(TPlaylistGroupMap & groups, const TPlaylistGroupKey & key)
  {
    TPlaylistGroupMap::iterator lower_bound = groups.lower_bound(key);

    if (lower_bound == groups.end() ||
        groups.key_comp()(key, lower_bound->first))
    {
      TPlaylistGroupPtr group(new PlaylistGroup());
      groups.insert(lower_bound, std::make_pair(key, group));
      return group;
    }

    return lower_bound->second;
  }

  //----------------------------------------------------------------
  // Playlist::add
  //
  void
  Playlist::add(const std::list<QString> & playlist,
                std::list<BookmarkHashInfo> * returnBookmarkHashList)
  {
#if 0
    std::cerr << "Playlist::add" << std::endl;
#endif

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
    TPlaylistGroupMap groupMap;
    setup_group_map(groupMap, groups_);

    // re-create the group vector:
    groups_.clear();
    numItems_ = 0;

    std::vector<TPlaylistGroupPtr> groupVec;
    for (std::list<TFringeGroup>::const_iterator i = fringeGroups.begin();
         i != fringeGroups.end(); ++i)
    {
      // shortcut:
      const TFringeGroup & fringeGroup = *i;
      groups_.push_back(get_group_ptr(groupMap, fringeGroup.fullPath_));

      PlaylistGroup & group = *(groups_.back());
      group.items_.clear();
      group.row_ = groups_.size() - 1;
      group.keyPath_ = fringeGroup.fullPath_;
      group.name_ = toWords(fringeGroup.abbreviatedPath_);
      group.offset_ = numItems_;
      group.hash_ = getKeyPathHash(group.keyPath_);

      // shortcuts:
      const TSiblings & siblings = fringeGroup.siblings_;

      for (TSiblings::const_iterator j = siblings.begin();
           j != siblings.end(); ++j, ++numItems_)
      {
        const PlaylistKey & key = j->first;
        const QString & value = j->second;

        group.items_.push_back(PlaylistItem());
        PlaylistItem & playlistItem = group.items_.back();

        playlistItem.row_ = group.items_.size() - 1;
        playlistItem.key_ = key;
        playlistItem.path_ = value;

        playlistItem.name_ = toWords(key.key_);
        playlistItem.ext_ = key.ext_;
        playlistItem.hash_ = getKeyHash(playlistItem.key_);

        if (firstNewItemPath && *firstNewItemPath == playlistItem.path_)
        {
          current_ = numItems_;
        }
      }
    }

    if (applyFilter())
    {
      current_ = closestItem(current_);
    }

    updateOffsets();
    setPlayingItem(current_, true);
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
  PlaylistGroup *
  Playlist::closestGroup(std::size_t index,
                         Playlist::TDirection where) const
  {
    if (numItems_ == numShown_)
    {
      // no items have been excluded:
      return lookupGroup(index);
    }

    const PlaylistGroup * prev = NULL;

    for (std::vector<TPlaylistGroupPtr>::const_iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      const PlaylistGroup & group = *(*i);
      if (group.excluded_)
      {
        continue;
      }

      std::size_t groupSize = group.items_.size();
      std::size_t groupEnd = group.offset_ + groupSize;
      if (groupEnd <= index)
      {
        prev = &group;
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
            const PlaylistItem & item = group.items_[j];
            if (!item.excluded_)
            {
              return const_cast<PlaylistGroup *>(&group);
            }
          }
        }
        else if (where == kAhead)
        {
          return const_cast<PlaylistGroup *>(&group);
        }

        if (where == kBehind)
        {
          break;
        }
      }
    }

    if (where == kBehind)
    {
      return const_cast<PlaylistGroup *>(prev);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::closestItem
  //
  std::size_t
  Playlist::closestItem(std::size_t index,
                        Playlist::TDirection where,
                        PlaylistGroup ** returnGroup) const
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

    PlaylistGroup * group = closestGroup(index, where);
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
    const std::vector<PlaylistItem> & items = group->items_;
    const std::size_t groupSize = items.size();
    const int step = where == kAhead ? 1 : -1;

    std::size_t i =
      index < group->offset_ ? 0 :
      index - group->offset_ >= groupSize ? groupSize - 1 :
      index - group->offset_;

    for (; i < groupSize; i += step)
    {
      const PlaylistItem & item = items[i];
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

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j, index++)
      {
        PlaylistItem & item = *j;

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
#if 0
    std::cerr << "Playlist::setPlayingItem" << std::endl;
#endif

    if (index != playing_ || force)
    {
      playing_ = (index < numItems_) ? index : numItems_;
      current_ = playing_;
      selectItem(playing_);
    }
  }

  //----------------------------------------------------------------
  // Playlist::selectAll
  //
  void
  Playlist::selectAll()
  {
    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      if (group.excluded_)
      {
        continue;
      }

      selectGroup(&group);
    }
  }

  //----------------------------------------------------------------
  // Playlist::selectGroup
  //
  void
  Playlist::selectGroup(PlaylistGroup * group)
  {
    for (std::vector<PlaylistItem>::iterator i = group->items_.begin();
         i != group->items_.end(); ++i)
    {
      PlaylistItem & item = *i;
      if (item.excluded_)
      {
        continue;
      }

      item.selected_ = true;
    }
  }

  //----------------------------------------------------------------
  // Playlist::selectItem
  //
  void
  Playlist::selectItem(std::size_t indexSel, bool exclusive)
  {
#if 0
    std::cerr << "Playlist::selectItem: " << indexSel << std::endl;
#endif
    bool itemSelected = false;

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *(*i);
      if (group.excluded_)
      {
        continue;
      }

      std::size_t groupEnd = group.offset_ + group.items_.size();

      if (exclusive)
      {
        for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
             j != group.items_.end(); ++j)
        {
          PlaylistItem & item = *j;
          if (item.excluded_)
          {
            continue;
          }

          item.selected_ = false;
        }
      }

      if (group.offset_ <= indexSel && indexSel < groupEnd)
      {
        PlaylistItem & item = group.items_[indexSel - group.offset_];
        item.selected_ = true;
        itemSelected = true;

        if (!exclusive)
        {
          // done:
          break;
        }
      }
    }

    YAE_ASSERT(itemSelected || indexSel == numItems_);
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

    std::size_t oldIndex = 0;
    std::size_t newIndex = 0;
    std::size_t newPlaying = playing_;
    bool playingRemoved = false;

    for (std::vector<TPlaylistGroupPtr>::iterator i = groups_.begin();
         i != groups_.end(); )
    {
      PlaylistGroup & group = *(*i);

      if (group.excluded_)
      {
        std::size_t groupSize = group.items_.size();
        oldIndex += groupSize;
        newIndex += groupSize;
        ++i;
        continue;
      }

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); oldIndex++)
      {
        PlaylistItem & item = *j;

        if (item.excluded_ || !item.selected_)
        {
          ++j;
          newIndex++;
          continue;
        }

        if (oldIndex < playing_)
        {
          // adjust the playing index:
          newPlaying--;
        }
        else if (oldIndex == playing_)
        {
          // playing item has changed:
          playingRemoved = true;
        }

        current_ = newIndex;

        // 1. remove the item from the tree:
        std::list<PlaylistKey> keyPath = group.keyPath_;
        keyPath.push_back(item.key_);
        tree_.remove(keyPath);

        // 2. remove the item from the group:
        j = group.items_.erase(j);
      }

      // if the group is empty and has a key path, remove it:
      if (!group.items_.empty() || group.keyPath_.empty() || group.excluded_)
      {
        ++i;
        continue;
      }

      i = groups_.erase(i);
    }

    updateOffsets();

    if (current_ >= numItems_)
    {
      current_ = numItems_ ? numItems_ - 1 : 0;
    }

    // must account for the excluded items:
    current_ = closestItem(current_, kBehind);

    if (current_ < numItems_)
    {
      PlaylistItem * item = lookup(current_);
      item->selected_ = true;
    }

    if (playingRemoved)
    {
      setPlayingItem(current_, true);
    }
    else
    {
      playing_ = newPlaying;
      current_ = playing_;
    }
  }

  //----------------------------------------------------------------
  // Playlist::removeItems
  //
  void
  Playlist::removeItems(std::size_t groupIndex, std::size_t itemIndex)
  {
    bool playingRemoved = false;
    std::size_t newPlaying = playing_;

    PlaylistGroup & group = *(groups_[groupIndex]);
    if (group.excluded_)
    {
      YAE_ASSERT(false);
      return;
    }

    if (itemIndex < numItems_)
    {
      // remove one item:
      std::vector<PlaylistItem>::iterator iter =
        group.items_.begin() + (itemIndex - group.offset_);

      // remove item from the tree:
      {
        PlaylistItem & item = *iter;
        std::list<PlaylistKey> keyPath = group.keyPath_;
        keyPath.push_back(item.key_);
        tree_.remove(keyPath);
      }

      if (itemIndex < playing_)
      {
        // adjust the playing index:
        newPlaying = playing_ - 1;
      }
      else if (itemIndex == playing_)
      {
        // playing item has changed:
        playingRemoved = true;
        newPlaying = playing_;
      }

      if (itemIndex < current_)
      {
        current_--;
      }

      group.items_.erase(iter);
    }
    else
    {
      // remove entire group:
      for (std::vector<PlaylistItem>::iterator iter = group.items_.begin();
           iter != group.items_.end(); ++iter)
      {
        // remove item from the tree:
        PlaylistItem & item = *iter;
        std::list<PlaylistKey> keyPath = group.keyPath_;
        keyPath.push_back(item.key_);
        tree_.remove(keyPath);
      }

      std::size_t groupSize = group.items_.size();
      std::size_t groupEnd = group.offset_ + groupSize;
      if (groupEnd < playing_)
      {
        // adjust the playing index:
        newPlaying = playing_ - groupSize;
      }
      else if (group.offset_ <= playing_)
      {
        // playing item has changed:
        playingRemoved = true;
        newPlaying = group.offset_;
      }

      if (groupEnd < current_)
      {
        current_ -= groupSize;
      }
      else if (group.offset_ <= current_)
      {
        current_ = group.offset_;
      }

      group.items_.clear();
    }

    // if the group is empty and has a key path, remove it:
    if (group.items_.empty() && !group.keyPath_.empty())
    {
      std::vector<TPlaylistGroupPtr>::iterator
        iter = groups_.begin() + groupIndex;
      groups_.erase(iter);
    }

    updateOffsets();

    if (newPlaying >= numItems_)
    {
      newPlaying = numItems_ ? numItems_ - 1 : 0;
    }

    if (current_ >= numItems_)
    {
      current_ = numItems_ ? numItems_ - 1 : 0;
    }

    // must account for the excluded items:
    newPlaying = closestItem(newPlaying, kBehind);
    current_ = closestItem(current_, kBehind);

    if (current_ < numItems_)
    {
      PlaylistItem * item = lookup(current_);
      item->selected_ = true;
    }

    if (playingRemoved)
    {
      setPlayingItem(newPlaying, true);
    }
    else
    {
      playing_ = newPlaying;
      current_ = playing_;
    }
  }

  //----------------------------------------------------------------
  // Playlist::changeCurrentItem
  //
  void
  Playlist::changeCurrentItem(int itemsPerRow, int delta)
  {
    // FIXME: move this into Playlist
    std::cerr
      << "FIXME: PlaylistModel::changeCurrentItem("
      << itemsPerRow << ", " << delta << ")"
      << std::endl;
#if 0
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlist_.lookup(playlist_.currentItem(), &group);
    std::size_t groupSize = group ? group->items_.size() : 0;
    if (delta < 0)
    {
      if (!item)
      {
        if (playlist_.countItemsShown())
        {
          setCurrentItem(playlist_.countItems() - 1);
        }
      }
      else if (item->row_ < -delta)
      {
        if (group->offset_)
        {
          // skip to the end of previous group:
          setCurrentItem(group->offset_ - 1);
        }
        else if (item->row_)
        {
          // skip to the beginning of the playlist:
          setCurrentItem(0);
        }
      }
      else
      {
        setCurrentItem(group->offset_ + item->row_ + delta);
      }
    }
    else if (delta > 0)
    {
      if (item->row_ + delta < groupSize)
      {
        setCurrentItem(group->offset_ + item->row_ + delta);
      }
      else
      {
        // skip to the start of next group:
        setCurrentItem(group->offset_ + groupSize);
      }
    }
#endif
  }

#if 0
  //----------------------------------------------------------------
  // lookupFirstGroupIndex
  //
  // return index of the first non-excluded group:
  //
  static std::size_t
  lookupFirstGroupIndex(const std::vector<TPlaylistGroupPtr> & groups)
  {
    std::size_t index = 0;
    for (std::vector<TPlaylistGroupPtr>::const_iterator i = groups.begin();
         i != groups.end(); ++i, ++index)
    {
      const PlaylistGroup & group = *i;
      if (!group.excluded_)
      {
#if 0
        std::cerr << "lookupGroup, first: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return index;
      }
    }

    return groups.size();
  }
#endif

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
  inline static PlaylistGroup *
  lookupLastGroup(const std::vector<TPlaylistGroupPtr> & groups)
  {
    std::size_t numGroups = groups.size();
    std::size_t i = lookupLastGroupIndex(groups);
    const PlaylistGroup * found = i < numGroups ? groups[i].get() : NULL;
    return const_cast<PlaylistGroup *>(found);
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  PlaylistGroup *
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
      const PlaylistGroup & group = *(groups_[i0]);
      std::size_t numItems = group.items_.size();
      std::size_t groupEnd = group.offset_ + numItems;

      if (index < groupEnd)
      {
        return const_cast<PlaylistGroup *>(&group);
      }
    }

    YAE_ASSERT(false);
    return lookupLastGroup(groups_);
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  PlaylistItem *
  Playlist::lookup(std::size_t index, PlaylistGroup ** returnGroup) const
  {
#if 0
    std::cerr << "Playlist::lookup: " << index << std::endl;
#endif

    PlaylistGroup * group = lookupGroup(index);
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
        const_cast<PlaylistItem *>(&group->items_[i]) :
        NULL;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  PlaylistGroup *
  Playlist::lookupGroup(const std::string & groupHash) const
  {
    if (groupHash.empty())
    {
      return NULL;
    }

    const std::size_t numGroups = groups_.size();
    for (std::size_t i = 0; i < numGroups; i++)
    {
      const PlaylistGroup & group = *(groups_[i]);
      if (groupHash == group.hash_)
      {
        return const_cast<PlaylistGroup *>(&group);
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  PlaylistItem *
  Playlist::lookup(const std::string & groupHash,
                   const std::string & itemHash,
                   std::size_t * returnItemIndex,
                   PlaylistGroup ** returnGroup) const
  {
    PlaylistGroup * group = lookupGroup(groupHash);
    if (!group || itemHash.empty())
    {
      return NULL;
    }

    if (returnGroup)
    {
      *returnGroup = group;
    }

    std::size_t groupSize = group->items_.size();
    for (std::size_t i = 0; i < groupSize; i++)
    {
      PlaylistItem & item = group->items_[i];
      if (itemHash == item.hash_)
      {
        if (*returnItemIndex)
        {
          *returnItemIndex = group->offset_ + i;
        }

        return const_cast<PlaylistItem *>(&item);
      }
    }

    if (*returnItemIndex)
    {
      *returnItemIndex = std::numeric_limits<std::size_t>::max();
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  PlaylistItem *
  Playlist::lookup(PlaylistGroup *& group, int groupRow, int itemRow) const
  {
    if (groupRow < 0 || groupRow >= groups_.size())
    {
      group = NULL;
      return NULL;
    }

    group = const_cast<PlaylistGroup *>(groups_[groupRow].get());
    if (itemRow < 0 || itemRow >= group->items_.size())
    {
      return NULL;
    }

    const PlaylistItem & item = group->items_[itemRow];
    return const_cast<PlaylistItem *>(&item);
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

      if (!group.excluded_)
      {
        numShownGroups_++;
      }

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;

        if (!item.excluded_)
        {
          numShown_++;
        }
      }

      offset += group.items_.size();
    }

    numItems_ = offset;
  }
}

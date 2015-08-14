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
namespace mvc
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
    current_(0),
    highlighted_(0)
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
      QString name = yae::toWords(fi.completeBaseName());

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
    groups_.clear();
    numItems_ = 0;

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

    for (std::list<TFringeGroup>::const_iterator i = fringeGroups.begin();
         i != fringeGroups.end(); ++i)
    {
      // shortcut:
      const TFringeGroup & fringeGroup = *i;

      groups_.push_back(PlaylistGroup());
      PlaylistGroup & group = groups_.back();
      group.row_ = groups_.size() - 1;
      group.keyPath_ = fringeGroup.fullPath_;
      group.name_ = yae::mvc::toWords(fringeGroup.abbreviatedPath_);
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

        playlistItem.name_ = yae::toWords(key.key_);
        playlistItem.ext_ = key.ext_;
        playlistItem.hash_ = getKeyHash(playlistItem.key_);

        if (firstNewItemPath && *firstNewItemPath == playlistItem.path_)
        {
          highlighted_ = numItems_;
        }
      }
    }

    if (applyFilter())
    {
      highlighted_ = closestItem(highlighted_);
    }

    setCurrentItem(highlighted_, true);
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
    return (current_ < numItems_) ? (numItems_ - current_) : 0;
  }

  //----------------------------------------------------------------
  // Playlist::countItemsBehind
  //
  std::size_t
  Playlist::countItemsBehind() const
  {
    return (current_ < numItems_) ? current_ : numItems_;
  }

  //----------------------------------------------------------------
  // Playlist::closestGroup
  //
  PlaylistGroup *
  Playlist::closestGroup(std::size_t index,
                         Playlist::TDirection where)
  {
    if (numItems_ == numShown_)
    {
      // no items have been excluded:
      return lookupGroup(index);
    }

    PlaylistGroup * prev = NULL;

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
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
            PlaylistItem & item = group.items_[j];
            if (!item.excluded_)
            {
              return &group;
            }
          }
        }
        else if (where == kAhead)
        {
          return &group;
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

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::closestItem
  //
  std::size_t
  Playlist::closestItem(std::size_t index,
                        Playlist::TDirection where,
                        PlaylistGroup ** returnGroup)
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

    return applyFilter();
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

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;

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

        if (index == current_)
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
  // Playlist::playbackNext
  //
  void
  Playlist::setCurrentItem(std::size_t index, bool force)
  {
#if 0
    std::cerr << "Playlist::setCurrentItem" << std::endl;
#endif

    if (index != current_ || force)
    {
      current_ = (index < numItems_) ? index : numItems_;
      highlighted_ = current_;
      selectItem(current_);
    }
  }

  //----------------------------------------------------------------
  // Playlist::selectAll
  //
  void
  Playlist::selectAll()
  {
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
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

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
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
    std::size_t newCurrent = current_;
    bool currentRemoved = false;

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); )
    {
      PlaylistGroup & group = *i;

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

        if (oldIndex < current_)
        {
          // adjust the current index:
          newCurrent--;
        }
        else if (oldIndex == current_)
        {
          // current item has changed:
          currentRemoved = true;
        }

        highlighted_ = newIndex;

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

    if (highlighted_ >= numItems_)
    {
      highlighted_ = numItems_ ? numItems_ - 1 : 0;
    }

    // must account for the excluded items:
    highlighted_ = closestItem(highlighted_, kBehind);

    if (highlighted_ < numItems_)
    {
      PlaylistItem * item = lookup(highlighted_);
      item->selected_ = true;
    }

    if (currentRemoved)
    {
      setCurrentItem(highlighted_, true);
    }
    else
    {
      current_ = newCurrent;
      highlighted_ = current_;
    }
  }

  //----------------------------------------------------------------
  // Playlist::removeItems
  //
  void
  Playlist::removeItems(std::size_t groupIndex, std::size_t itemIndex)
  {
    bool currentRemoved = false;
    std::size_t newCurrent = current_;

    PlaylistGroup & group = groups_[groupIndex];
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

      if (itemIndex < current_)
      {
        // adjust the current index:
        newCurrent = current_ - 1;
      }
      else if (itemIndex == current_)
      {
        // current item has changed:
        currentRemoved = true;
        newCurrent = current_;
      }

      if (itemIndex < highlighted_)
      {
        highlighted_--;
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
      if (groupEnd < current_)
      {
        // adjust the current index:
        newCurrent = current_ - groupSize;
      }
      else if (group.offset_ <= current_)
      {
        // current item has changed:
        currentRemoved = true;
        newCurrent = group.offset_;
      }

      if (groupEnd < highlighted_)
      {
        highlighted_ -= groupSize;
      }
      else if (group.offset_ <= highlighted_)
      {
        highlighted_ = group.offset_;
      }

      group.items_.clear();
    }

    // if the group is empty and has a key path, remove it:
    if (group.items_.empty() && !group.keyPath_.empty())
    {
      std::vector<PlaylistGroup>::iterator iter = groups_.begin() + groupIndex;
      groups_.erase(iter);
    }

    if (newCurrent >= numItems_)
    {
      newCurrent = numItems_ ? numItems_ - 1 : 0;
    }

    if (highlighted_ >= numItems_)
    {
      highlighted_ = numItems_ ? numItems_ - 1 : 0;
    }

    // must account for the excluded items:
    newCurrent = closestItem(newCurrent, kBehind);
    highlighted_ = closestItem(highlighted_, kBehind);

    if (highlighted_ < numItems_)
    {
      PlaylistItem * item = lookup(highlighted_);
      item->selected_ = true;
    }

    if (currentRemoved)
    {
      setCurrentItem(newCurrent, true);
    }
    else
    {
      current_ = newCurrent;
      highlighted_ = current_;
    }
  }

#if 0
  //----------------------------------------------------------------
  // lookupFirstGroupIndex
  //
  // return index of the first non-excluded group:
  //
  static std::size_t
  lookupFirstGroupIndex(const std::vector<PlaylistGroup> & groups)
  {
    std::size_t index = 0;
    for (std::vector<PlaylistGroup>::const_iterator i = groups.begin();
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
  lookupLastGroupIndex(const std::vector<PlaylistGroup> & groups)
  {
    std::size_t index = groups.size();
    for (std::vector<PlaylistGroup>::const_reverse_iterator
           i = groups.rbegin(); i != groups.rend(); ++i, --index)
    {
      const PlaylistGroup & group = *i;
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
  lookupLastGroup(std::vector<PlaylistGroup> & groups)
  {
    std::size_t numGroups = groups.size();
    std::size_t i = lookupLastGroupIndex(groups);
    PlaylistGroup * found = i < numGroups ? &groups[i] : NULL;
    return found;
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  PlaylistGroup *
  Playlist::lookupGroup(std::size_t index)
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
      return lookupLastGroup(groups_);
    }

    const std::size_t numGroups = groups_.size();
    std::size_t i0 = 0;
    std::size_t i1 = numGroups;

    while (i0 != i1)
    {
      std::size_t i = i0 + (i1 - i0) / 2;

      PlaylistGroup & group = groups_[i];
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
      PlaylistGroup & group = groups_[i0];
      std::size_t numItems = group.items_.size();
      std::size_t groupEnd = group.offset_ + numItems;

      if (index < groupEnd)
      {
        return &group;
      }
    }

    YAE_ASSERT(false);
    return lookupLastGroup(groups_);
  }

  //----------------------------------------------------------------
  // Playlist::lookup
  //
  PlaylistItem *
  Playlist::lookup(std::size_t index, PlaylistGroup ** returnGroup)
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
      return i < groupSize ? &group->items_[i] : NULL;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Playlist::lookupGroup
  //
  PlaylistGroup *
  Playlist::lookupGroup(const std::string & groupHash)
  {
    if (groupHash.empty())
    {
      return NULL;
    }

    const std::size_t numGroups = groups_.size();
    for (std::size_t i = 0; i < numGroups; i++)
    {
      PlaylistGroup & group = groups_[i];
      if (groupHash == group.hash_)
      {
        return &group;
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
                   PlaylistGroup ** returnGroup)
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

        return &item;
      }
    }

    if (*returnItemIndex)
    {
      *returnItemIndex = std::numeric_limits<std::size_t>::max();
    }

    return NULL;
  }
}
}

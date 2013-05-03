// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr  7 23:41:07 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <algorithm>

// Qt includes:
#include <QCursor>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QUrl>

// yae includes:
#include <yaePlaylistWidget.h>
#include <yaeUtils.h>


namespace yae
{

  //----------------------------------------------------------------
  // kGroupNameHeight
  //
  static const int kGroupNameHeight = 24;

  //----------------------------------------------------------------
  // kGroupItemHeight
  //
  static const int kGroupItemHeight = 20;

  //----------------------------------------------------------------
  // kGroupArrowSize
  //
  static const int kGroupArrowSize = 7;

  //----------------------------------------------------------------
  // kPixmapClear
  //
  static QPixmap & kPixmapClear()
  {
    static QPixmap p = QPixmap(":/images/clear.png");
    return p;
  }

  //----------------------------------------------------------------
  // kPixmapClearDark
  //
  static QPixmap & kPixmapClearDark()
  {
    static QPixmap p = QPixmap(":/images/clear-dark.png");
    return p;
  }


  //----------------------------------------------------------------
  // PlaylistItem::PlaylistItem
  //
  PlaylistItem::PlaylistItem():
    selected_(false),
    excluded_(false)
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
  // PlaylistWidget::PlaylistWidget
  //
  PlaylistWidget::PlaylistWidget(QWidget * parent, Qt::WindowFlags):
    QAbstractScrollArea(parent),
    mouseState_(PlaylistWidget::kNotReady),
    rubberBand_(QRubberBand::Rectangle, this),
    numItems_(0),
    numShown_(0),
    numShownGroups_(0),
    current_(0),
    highlighted_(0)
  {
    setMouseTracking(true);
    add(std::list<QString>());

    repaintTimer_.setSingleShot(true);
    bool ok = connect(&repaintTimer_, SIGNAL(timeout()),
                      this, SLOT(update()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlaylistWidget::minimumSizeHint
  //
  QSize
  PlaylistWidget::minimumSizeHint() const
  {
    int w = minimumWidth();
    int h = kGroupNameHeight;

    if (numShown_)
    {
      h += kGroupNameHeight + kGroupItemHeight;
    }

    return QSize(w, h);
  }

  //----------------------------------------------------------------
  // PlaylistWidget::sizeHint
  //
  QSize
  PlaylistWidget::sizeHint() const
  {
    int w = minimumWidth();
    int h = int(kGroupNameHeight * numShownGroups_ +
                kGroupItemHeight * numShown_);

    return QSize(w, h);
  }

  //----------------------------------------------------------------
  // PlaylistWidget::add
  //
  void
  PlaylistWidget::add(const std::list<QString> & playlist)
  {
#if 0
    std::cerr << "PlaylistWidget::add" << std::endl;
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
        url.setEncodedUrl(path.toUtf8(), QUrl::StrictMode);

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
      std::list<QString> keys;
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

        key = prepareForSorting(base);
        if (!ext.isEmpty())
        {
          key += QChar::fromAscii('.');
          key += ext;
        }

        keys.push_front(key);

        QString next = fi.absolutePath();
        fi = QFileInfo(next);
      }

      tmpTree.set(keys, path);
      tree_.set(keys, path);
    }

    // lookup the first new item:
    const QString * firstNewItemPath = tmpTree.findFirstFringeItemValue();

    // flatten the tree into a list of play groups:
    typedef TPlaylistTree::FringeGroup TFringeGroup;
    std::list<TFringeGroup> fringeGroups;
    tree_.get(fringeGroups);
    groups_.clear();
    numItems_ = 0;

    for (std::list<TFringeGroup>::const_iterator i = fringeGroups.begin();
         i != fringeGroups.end(); ++i)
    {
      // shortcut:
      const TFringeGroup & fringeGroup = *i;

      groups_.push_back(PlaylistGroup());
      PlaylistGroup & group = groups_.back();
      group.keyPath_ = fringeGroup.fullPath_;
      group.name_ = toWords(fringeGroup.abbreviatedPath_);
      group.offset_ = numItems_;

      // shortcuts:
      typedef std::map<QString, QString> TSiblings;
      const TSiblings & siblings = fringeGroup.siblings_;

      for (TSiblings::const_iterator j = siblings.begin();
           j != siblings.end(); ++j, ++numItems_)
      {
        const QString & key = j->first;
        const QString & value = j->second;

        group.items_.push_back(PlaylistItem());
        PlaylistItem & playlistItem = group.items_.back();

        playlistItem.key_ = key;
        playlistItem.path_ = value;

        QFileInfo fi(key);
        playlistItem.name_ = toWords(fi.completeBaseName());
        playlistItem.ext_ = fi.suffix();

        if (firstNewItemPath && *firstNewItemPath == playlistItem.path_)
        {
          highlighted_ = numItems_;
        }
      }
    }

    // add a tail group:
    {
      groups_.push_back(PlaylistGroup());
      PlaylistGroup & group = groups_.back();
      group.name_ = tr("end of playlist");
      group.offset_ = numItems_;
    }

    if (applyFilter())
    {
      highlighted_ = closestItem(highlighted_);
    }

    updateGeometries();
    setCurrentItem(highlighted_, true);
  }

  //----------------------------------------------------------------
  // PlaylistWidget::currentItem
  //
  std::size_t
  PlaylistWidget::currentItem() const
  {
    return current_;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::countItems
  //
  std::size_t
  PlaylistWidget::countItems() const
  {
    return numItems_;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::countItemsAhead
  //
  std::size_t
  PlaylistWidget::countItemsAhead() const
  {
    return (current_ < numItems_) ? (numItems_ - current_) : 0;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::countItemsBehind
  //
  std::size_t
  PlaylistWidget::countItemsBehind() const
  {
    return (current_ < numItems_) ? current_ : numItems_;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::closestGroup
  //
  PlaylistGroup *
  PlaylistWidget::closestGroup(std::size_t index,
                               PlaylistWidget::TDirection where)
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
      if (groupEnd < index)
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
  // PlaylistWidget::closestItem
  //
  std::size_t
  PlaylistWidget::closestItem(std::size_t index,
                              PlaylistWidget::TDirection where,
                              PlaylistGroup ** returnGroup)
  {
    if (numItems_ == numShown_)
    {
      // no items have been excluded:
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
  // PlaylistWidget::filterChanged
  //
  void
  PlaylistWidget::filterChanged(const QString & filter)
  {
    m_keywords.clear();
    splitIntoWords(filter, m_keywords);

    if (applyFilter())
    {
      updateGeometries();
      update();
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::applyFilter
  //
  bool
  PlaylistWidget::applyFilter()
  {
    bool exclude = !m_keywords.empty();
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
          text += tr("NOW PLAYING");
        }

        if (!keywordsMatch(m_keywords, text))
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
  // PlaylistWidget::playbackNext
  //
  void
  PlaylistWidget::setCurrentItem(std::size_t index, bool force)
  {
#if 0
    std::cerr << "PlaylistWidget::setCurrentItem" << std::endl;
#endif

    if (index != current_ || force)
    {
      current_ = (index < numItems_) ? index : numItems_;
      selectItem(current_);
      scrollTo(current_);

      emit currentItemChanged(current_);
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::selectAll
  //
  void
  PlaylistWidget::selectAll()
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
  // PlaylistWidget::selectGroup
  //
  void
  PlaylistWidget::selectGroup(PlaylistGroup * group)
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

    update();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::selectItem
  //
  void
  PlaylistWidget::selectItem(std::size_t indexSel, bool exclusive)
  {
#if 0
    std::cerr << "PlaylistWidget::selectItem: " << indexSel << std::endl;
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
    update();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::removeSelected
  //
  void
  PlaylistWidget::removeSelected()
  {
#if 0
    std::cerr << "PlaylistWidget::removeSelected" << std::endl;
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
        std::list<QString> keyPath = group.keyPath_;
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

    updateGeometries();

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
      scrollTo(highlighted_);
    }

    if (currentRemoved)
    {
      setCurrentItem(highlighted_, true);
    }
    else
    {
      current_ = newCurrent;
    }

    update();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::removeItems
  //
  void
  PlaylistWidget::removeItems(std::size_t groupIndex, std::size_t itemIndex)
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
        std::list<QString> keyPath = group.keyPath_;
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
        std::list<QString> keyPath = group.keyPath_;
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

    updateGeometries();

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
    }

    update();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::paintEvent
  //
  void
  PlaylistWidget::paintEvent(QPaintEvent * e)
  {
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    QPalette palette = this->palette();
    QBrush background = palette.base();
    painter.fillRect(e->rect(), background);

    QPoint viewOffset = getViewOffset();
    QRect localRegion = e->rect().translated(viewOffset);
    painter.translate(-viewOffset);

    QPoint mousePos = mapFromGlobal(QCursor::pos()) + viewOffset;
    draw(painter, localRegion, mousePos);

    painter.end();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::mousePressEvent
  //
  void
  PlaylistWidget::mousePressEvent(QMouseEvent * e)
  {
    e->ignore();
    mouseState_ = PlaylistWidget::kNotReady;

    if (e->button() == Qt::LeftButton)
    {
      e->accept();

      QPoint viewOffset = getViewOffset();
      QPoint pt = e->pos() + viewOffset;

      // check whether the [x] button was clicked:
      {
        std::size_t groupIndex = groups_.size();
        std::size_t itemIndex = numItems_;
        if (isMouseOverRemoveButton(pt, groupIndex, itemIndex))
        {
          // handle item removal on mouse button release:
          mouseState_ = PlaylistWidget::kRemoveItem;
          return;
        }
      }

      if (pt.x() <= 10 + kGroupArrowSize)
      {
        // lookup the exact group:
        PlaylistGroup * group = lookupGroup(pt, false);

        if (group)
        {
          mouseState_ = PlaylistWidget::kToggleCollapsedGroup;
          group->collapsed_ = !group->collapsed_;
          updateGeometries();
          update();
          return;
        }
      }

      int mod = e->modifiers();
      bool extendSelection = (mod & Qt::ShiftModifier);
      bool toggleSelection = !extendSelection && (mod & Qt::ControlModifier);

      if (!extendSelection)
      {
        anchor_ = pt;

        rubberBand_.setGeometry(QRect(anchor_ - viewOffset, QSize()));
        rubberBand_.show();
      }

      mouseState_ = PlaylistWidget::kUpdateSelection;
      updateSelection(e->pos(), toggleSelection);
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::mouseReleaseEvent
  //
  void
  PlaylistWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    e->ignore();

    if (e->button() == Qt::LeftButton)
    {
      e->accept();

      if (mouseState_ == PlaylistWidget::kRemoveItem)
      {
        QPoint viewOffset = getViewOffset();
        QPoint pt = e->pos() + viewOffset;

        std::size_t groupIndex = groups_.size();
        std::size_t itemIndex = numItems_;
        if (isMouseOverRemoveButton(pt, groupIndex, itemIndex))
        {
          removeItems(groupIndex, itemIndex);
        }
      }

      rubberBand_.hide();
    }

    mouseState_ = PlaylistWidget::kNotReady;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::mouseMoveEvent
  //
  void
  PlaylistWidget::mouseMoveEvent(QMouseEvent * e)
  {
    if (e->buttons() & Qt::LeftButton &&
        mouseState_ == PlaylistWidget::kUpdateSelection)
    {
      e->accept();

      bool toggleSelection = false;
      bool scrollToItem = true;
      bool allowGroupSelection = false;

      updateSelection(e->pos(),
                      toggleSelection,
                      scrollToItem,
                      allowGroupSelection);
    }
    else if (!e->buttons())
    {
      e->accept();

      // avoid repainting the [x] item removal button unless
      // the mouse is hovering near it:
      int vw = viewport()->width();
      QPoint vo = getViewOffset();
      QPoint pt = e->pos() + vo;
      if (vw - pt.x() < 17)
      {
        update();
        repaintTimer_.start(300);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::mouseDoubleClickEvent
  //
  void
  PlaylistWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::LeftButton && !e->modifiers())
    {
      e->accept();

      QPoint viewOffset = getViewOffset();
      QPoint pt = e->pos() + viewOffset;

      PlaylistGroup * group = lookupGroup(pt);
      std::size_t index = lookupItemIndex(group, pt);
      if (index < numItems_)
      {
        highlighted_ = index;
        current_ = index;
        emit currentItemChanged(current_);
        update();
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::wheelEvent
  //
  void
  PlaylistWidget::wheelEvent(QWheelEvent * e)
  {
    if (!e->modifiers() && (!e->buttons() || e->buttons() & Qt::LeftButton))
    {
      QScrollBar * sb = verticalScrollBar();
      int val = sb->value();
      int min = sb->minimum();
      int max = sb->maximum();
      int delta = -(e->delta());

      if (val == min && delta < 0 ||
          val == max && delta > 0)
      {
        // prevent wheel event from propagating to the parent widget:
        e->accept();
        return;
      }

      QAbstractScrollArea::wheelEvent(e);

      if (e->buttons() & Qt::LeftButton)
      {
        bool toggleSelection = false;
        bool scrollToItem = false;
        bool allowGroupSelection = false;

        updateSelection(e->pos(),
                        toggleSelection,
                        scrollToItem,
                        allowGroupSelection);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::keyPressEvent
  //
  void
  PlaylistWidget::keyPressEvent(QKeyEvent * e)
  {
    e->ignore();
    int key = e->key();

    bool stepUp = (key == Qt::Key_Up);
    bool stepDn = (key == Qt::Key_Down);
    bool pageUp = (key == Qt::Key_PageUp);
    bool pageDn = (key == Qt::Key_PageDown);

    bool groupCollapse = (key == Qt::Key_Left);
    bool groupExpand = (key == Qt::Key_Right);

    bool enter = (key == Qt::Key_Enter || key == Qt::Key_Return);

    int mod = e->modifiers();
    bool modAlt   = mod & Qt::AltModifier;
    bool modCtrl  = mod & Qt::ControlModifier;
    bool modShift = mod & Qt::ShiftModifier;
    bool modNone  = !modShift && !modCtrl && !modAlt;

    if (modNone)
    {
      // change highlighted item:
      PlaylistGroup * group = NULL;
      PlaylistItem * found = NULL;

      if (modNone && stepUp && highlighted_ > 0)
      {
        lookup(highlighted_, &group);

        if (group)
        {
          if (group->collapsed_ && group->offset_)
          {
            highlighted_ = closestItem(group->offset_ - 1, kBehind);
            lookup(highlighted_, &group);

            if (group->collapsed_)
            {
              highlighted_ = group->offset_;
            }
          }
          else
          {
            highlighted_ = closestItem(highlighted_ - 1, kBehind);
          }
        }

        found = lookup(highlighted_, &group);
      }
      else if (modNone && stepDn && highlighted_ < numItems_)
      {
        lookup(highlighted_, &group);

        if (group)
        {
          if (group->collapsed_)
          {
            highlighted_ = closestItem(group->offset_ + group->items_.size());
          }
          else
          {
            highlighted_ = closestItem(highlighted_ + 1);
          }
        }

        found = lookup(highlighted_, &group);
      }
      else if (modNone && (pageUp || pageDn))
      {
        PlaylistGroup * hiGroup = NULL;
        PlaylistItem * hiItem = lookup(highlighted_, &hiGroup);

        if (hiGroup || hiItem)
        {
          int vh = viewport()->height() - kGroupNameHeight;
          QPoint viewOffset = getViewOffset();

          QPoint p0 =
            hiGroup->collapsed_ || !hiItem  ?
            hiGroup->bbox_.center() :
            hiItem->bbox_.center();

          QPoint p1 =
            pageUp ?
            QPoint(p0.x(), p0.y() - vh) :
            QPoint(p0.x(), p0.y() + vh);

          group = lookupGroup(p1);
          std::size_t index = lookupItemIndex(group, p1);

          highlighted_ = (index < numItems_) ? index : group->offset_;
          highlighted_ = closestItem(highlighted_, pageDn ? kAhead : kBehind);

          found = lookup(highlighted_, &group);
        }
      }
      else if (modNone && enter)
      {
        setCurrentItem(highlighted_);
        e->accept();
      }
      else if (modNone && (groupExpand || groupCollapse))
      {
        PlaylistGroup * hiGroup = NULL;
        lookup(highlighted_, &hiGroup);

        if (hiGroup)
        {
          bool expandable = !hiGroup->keyPath_.empty();

          if (expandable && hiGroup->collapsed_ && groupExpand)
          {
            hiGroup->collapsed_ = false;
            updateGeometries();
            highlighted_ = hiGroup->offset_;
            selectItem(closestItem(hiGroup->offset_));
            e->accept();
          }
          else if (expandable && groupCollapse && !hiGroup->collapsed_)
          {
            hiGroup->collapsed_ = true;
            updateGeometries();
            selectGroup(hiGroup);
            highlighted_ = hiGroup->offset_;
            e->accept();
          }
          else if (groupCollapse && hiGroup->offset_ > 0)
          {
            highlighted_ = closestItem(hiGroup->offset_ - 1, kBehind);
            lookup(highlighted_, &hiGroup);

            if (hiGroup->collapsed_)
            {
              highlighted_ = hiGroup->offset_;
            }

            found = lookup(highlighted_, &group);
          }
          else if (groupExpand && highlighted_ < numItems_)
          {
            highlighted_ = closestItem(highlighted_ + 1);
            found = lookup(highlighted_, &group);
          }
        }
      }

      if (group || found)
      {
        // update the anchor:
        anchor_ =
          group->collapsed_ || !found ?
          group->bbox_.center() :
          found->bbox_.center();

        // update the selection set:
        QPoint viewOffset = getViewOffset();
        QPoint mousePt(anchor_.x() - viewOffset.x(),
                       anchor_.y() - viewOffset.y());

        bool toggleSelection = false;
        bool scrollToItem = true;
        updateSelection(mousePt, toggleSelection, scrollToItem);
        e->accept();
      }
    }
    else if (modShift && (stepUp || stepDn || pageUp || pageDn))
    {
      // update selection set:
      PlaylistGroup * group = NULL;
      PlaylistItem * found = NULL;

      if (stepUp && highlighted_ > 0)
      {
        highlighted_ = closestItem(highlighted_ - 1, kBehind);
        found = lookup(highlighted_, &group);
      }
      else if (stepDn && highlighted_ < numItems_)
      {
        highlighted_ = closestItem(highlighted_ + 1);
        found = lookup(highlighted_, &group);
      }
      else if (pageUp || pageDn)
      {
        PlaylistGroup * hiGroup = NULL;
        PlaylistItem * hiItem = lookup(highlighted_, &hiGroup);

        if (hiGroup || hiItem)
        {
          int vh = viewport()->height() - kGroupNameHeight;
          QPoint viewOffset = getViewOffset();

          QPoint p0 =
            hiGroup->collapsed_ || !hiItem ?
            hiGroup->bbox_.center() :
            hiItem->bbox_.center();

          QPoint p1 =
            pageUp ?
            QPoint(p0.x(), p0.y() - vh) :
            QPoint(p0.x(), p0.y() + vh);

          group = lookupGroup(p1);
          std::size_t index = lookupItemIndex(group, p1);

          highlighted_ = (index < numItems_) ? index : group->offset_;
          highlighted_ = closestItem(highlighted_, pageDn ? kAhead : kBehind);

          found = lookup(highlighted_, &group);
        }
      }

      if (group || found)
      {
        if (group->collapsed_)
        {
          group->collapsed_ = false;
          updateGeometries();
        }

        QPoint viewOffset = getViewOffset();
        QPoint pt = found ? found->bbox_.center() : group->bbox_.center();
        QPoint mousePt(pt.x() - viewOffset.x(),
                       pt.y() - viewOffset.y());

        bool toggleSelection = false;
        bool scrollToItem = true;
        updateSelection(mousePt, toggleSelection, scrollToItem);
      }

      e->accept();
    }

    if (!e->isAccepted())
    {
      QAbstractScrollArea::keyPressEvent(e);
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::resizeEvent
  //
  void
  PlaylistWidget::resizeEvent(QResizeEvent * e)
  {
#if 0
    std::cerr << "PlaylistWidget::resizeEvent" << std::endl;
#endif

    (void) e;
    updateGeometries();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::updateGeometries
  //
  void
  PlaylistWidget::updateGeometries()
  {
#if 0
    std::cerr << "PlaylistWidget::updateGeometries" << std::endl;
#endif

    std::size_t offset = 0;
    int width = viewport()->width();
    int y = 0;

    numShown_ = 0;
    numShownGroups_ = 0;

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      group.offset_ = offset;

      int headerHeight = 0;
      if (!group.excluded_)
      {
        headerHeight = kGroupNameHeight;
        numShownGroups_++;
      }

      group.bbox_.setX(0);
      group.bbox_.setY(y);
      group.bbox_.setWidth(width);
      group.bbox_.setHeight(headerHeight);
      y += headerHeight;

      group.bboxItems_.setX(0);
      group.bboxItems_.setWidth(width);
      group.bboxItems_.setY(y);

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;
        item.bbox_.setX(0);
        item.bbox_.setY(y);
        item.bbox_.setWidth(width);
        item.bbox_.setHeight(0);

        if (!item.excluded_)
        {
          numShown_++;

          if (!group.collapsed_)
          {
            item.bbox_.setHeight(kGroupItemHeight);
            y += kGroupItemHeight;
          }
        }
      }

      group.bboxItems_.setHeight(y - group.bboxItems_.y());
      offset += group.items_.size();
    }

    numItems_ = offset;
    updateScrollBars();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::updateScrollBars
  //
  void
  PlaylistWidget::updateScrollBars()
  {
    QSize viewportSize = viewport()->size();
    if (!viewportSize.isValid())
    {
      viewportSize = QSize(0, 0);
    }

    int viewportHeight = viewportSize.height();
    int viewportWidth = viewportSize.width();

    int contentHeight = 0;
    int contentWidth = 0;

    if (!groups_.empty())
    {
      PlaylistGroup & group = groups_.back();
      contentWidth = group.bbox_.width();

      if (!group.items_.empty())
      {
        PlaylistItem & item = group.items_.back();
        contentHeight = item.bbox_.y() + item.bbox_.height();
      }
      else
      {
        contentHeight = group.bbox_.y() + group.bbox_.height();
      }
    }

    verticalScrollBar()->setSingleStep(kGroupItemHeight);
    verticalScrollBar()->setPageStep(viewportHeight);
    verticalScrollBar()->setRange(0, qMax(0, contentHeight - viewportHeight));

    horizontalScrollBar()->setSingleStep(kGroupItemHeight);
    horizontalScrollBar()->setPageStep(viewportSize.width());
    horizontalScrollBar()->setRange(0, qMax(0, contentWidth - viewportWidth));
  }

  //----------------------------------------------------------------
  // legibleTextColorForGivenBackground
  //
  static QColor
  legibleTextColorForGivenBackground(const QColor & bg)
  {
    qreal h, s, v;
    bg.getHsvF(&h, &s, &v);

    qreal t = (1 - s) * v;
    v = t < 0.6 ? 1 : 0;

    QColor fg;
    fg.setHsvF(h, 0, v);
    return fg;
  }

  //----------------------------------------------------------------
  // bboxRemoveButton
  //
  static QRect
  bboxRemoveButton(const QRect & bbox)
  {
    static const int w = 10;
    return QRect(bbox.x() + bbox.width() - w - 3,
                 bbox.y() + (bbox.height() - w) / 2,
                 w,
                 w);
  }

  //----------------------------------------------------------------
  // drawRemoveButton
  //
  static int
  drawRemoveButton(QPainter & painter,
                   const QRect & bbox,
                   const QPixmap & button,
                   const QColor & bgColor)
  {
    QRect bx = bboxRemoveButton(bbox);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bx, 2, 2);
    painter.drawPixmap(bx.x() + (bx.width() - button.width()) / 2,
                       bx.y() + (bx.height() - button.height()) / 2,
                       button);
    return bx.x();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::draw
  //
  void
  PlaylistWidget::draw(QPainter & painter,
                       const QRect & region,
                       const QPoint & mousePos)
  {
    static const QColor zebraBg[] = {
      QColor(0xf0, 0xf0, 0xf0, 0),
      QColor(0xff, 0xff, 0xff)
    };

    QPalette palette = this->palette();
    QColor selectedColorBg = palette.color(QPalette::Highlight);
    QColor selectedColorFg = palette.color(QPalette::HighlightedText);
    QColor foregroundColor = palette.color(QPalette::WindowText);
    static QColor headerColor = QColor("#202020");
    static QColor activeColor = QColor("#ffffff");
    static QColor loBgGroup = QColor("#c1c1c1");
    static QColor hiBgGroup = QColor("#939393");

    QFont textFont = painter.font();
    textFont.setPixelSize(10);

    QFont headerFont = textFont;
    QFont smallFont = textFont;
    smallFont.setPixelSize(9);

    QFont tinyFont = textFont;
    tinyFont.setPixelSize(7);

    painter.setFont(textFont);

    std::size_t index = 0;
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      std::size_t groupSize = group.items_.size();

      if (group.excluded_)
      {
        index += groupSize;
        continue;
      }

      bool isHighlightedGroup =
        group.offset_ <= highlighted_ &&
        highlighted_ < (group.offset_ + groupSize);

      if (overlapExists(group.bbox_, region))
      {
        QRect bbox(group.bbox_.x(),
                   0,
                   group.bbox_.width(),
                   group.bbox_.height());

        const QColor & bg = isHighlightedGroup ? hiBgGroup : loBgGroup;
        painter.setBrush(bg);
        painter.setBrushOrigin(bbox.topLeft());
        painter.setPen(Qt::NoPen);
        painter.drawRect(bbox);

        if (!group.keyPath_.empty())
        {
          if (overlapExists(group.bbox_, mousePos) &&
              group.bbox_.right() - mousePos.x() < 17)
          {
            drawRemoveButton(painter, bbox, kPixmapClear(), bg);
          }

          bbox.adjust(0, 0, -17, 0);

          double w = kGroupArrowSize;
          double h = bbox.height();
          double s = w / 2.0;
          double x = 7.5;
          double y = h / 2.0;

          QPointF arrow[3];
          if (group.collapsed_)
          {
            arrow[0] = QPointF(0.5 + x, y - s);
            arrow[1] = QPointF(0.5 + x + w, y);
            arrow[2] = QPointF(0.5 + x, y + s);
          }
          else
          {
            arrow[0] = QPointF(x,     0.5 + y - s);
            arrow[1] = QPointF(x + s, 0.5 + y + s);
            arrow[2] = QPointF(x + w, 0.5 + y - s);
          }

          painter.setBrush(activeColor);
          painter.setPen(Qt::NoPen);
          painter.drawPolygon(arrow, 3);

          if (isHighlightedGroup)
          {
            painter.setPen(activeColor);
          }
          else
          {
            painter.setPen(headerColor);
          }

          QRect bx = bbox.adjusted(10 + w, 0, 0, 0);

          if (isHighlightedGroup)
          {
            painter.setFont(headerFont);
            drawTextWithShadowToFit(painter,
                                    bx,
                                    Qt::AlignVCenter | Qt::AlignCenter,
                                    group.name_,
                                    QColor("#404040"),
                                    false); // underline shadow
          }
          else
          {
            painter.setFont(headerFont);
            drawTextToFit(painter,
                          bx,
                          Qt::AlignVCenter | Qt::AlignCenter,
                          group.name_);
          }
          painter.setFont(textFont);
        }
        else
        {
          QRect bx = bbox.adjusted(2, 1, -2, -1);
          painter.setFont(smallFont);

          if (group.offset_ == highlighted_ && numItems_)
          {
            painter.setPen(Qt::white);
          }
          else
          {
            painter.setPen(headerColor);
          }

          QString text =
            (numShown_ == 1 ? tr("%1 item,  %2") : tr("%1 items,  %2")).
            arg(numShown_).
            arg(group.name_);

          drawTextToFit(painter,
                        bx,
                        Qt::AlignBottom | Qt::AlignRight,
                        text);
          painter.setFont(textFont);
        }
      }

      painter.translate(0, group.bbox_.height());

      if (group.collapsed_)
      {
        index += groupSize;
        continue;
      }

      if (!overlapExists(group.bboxItems_, region))
      {
        painter.translate(0, group.bboxItems_.height());
        index += groupSize;
        continue;
      }

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j, index++)
      {
        PlaylistItem & item = *j;
        std::size_t zebraIndex = index % 2;

        if (item.excluded_)
        {
          continue;
        }

        if (!overlapExists(item.bbox_, region))
        {
          painter.translate(0, item.bbox_.height());
          continue;
        }

        QRect bbox(item.bbox_.x(),
                   0,
                   item.bbox_.width(),
                   item.bbox_.height());

        QColor bg = zebraBg[zebraIndex];
        QColor fg = foregroundColor;

        if (item.selected_)
        {
          bg = selectedColorBg;
          fg = selectedColorFg;
        }

        painter.fillRect(bbox, bg);

        if (overlapExists(item.bbox_, mousePos) &&
            item.bbox_.right() - mousePos.x() < 17)
        {
          const QColor & bg = item.selected_ ? selectedColorBg : loBgGroup;
          int x0 = drawRemoveButton(painter, bbox, kPixmapClear(), bg);
          bbox.setRight(x0);
        }

        bbox.adjust(3, 0, -3, 0);

        if (index == current_)
        {
          QString nowPlaying = tr("NOW PLAYING");

          painter.setFont(tinyFont);
          QFontMetrics fm = painter.fontMetrics();
          QSize sz = fm.size(Qt::TextSingleLine, nowPlaying);
          QRect bx = bbox.adjusted(1, 1, -1, -1);

          // add a little padding:
          sz.setWidth(sz.width() + 8);

          if (bx.width() > sz.width())
          {
            bx.setX(bx.x() + bx.width() - sz.width());
            bx.setWidth(sz.width());
          }

          if (bx.height() > sz.height())
          {
            bx.setHeight(sz.height());
          }

          int radius = std::min<int>(sz.width(), sz.height()) / 2;
          painter.setBrush(selectedColorBg);
          painter.setPen(Qt::NoPen);
          painter.drawRoundedRect(bx, radius, radius);

          painter.setPen(legibleTextColorForGivenBackground(selectedColorBg));
          drawTextToFit(painter,
                        bx,
                        Qt::AlignVCenter | Qt::AlignCenter,
                        nowPlaying);
          painter.setFont(textFont);
        }

        painter.setPen(fg);

        QRect bboxText = bbox.adjusted(0, 0, 0, -1);
        QString text = tr("%1, %2").arg(item.name_).arg(item.ext_);
        drawTextToFit(painter,
                      bboxText,
                      Qt::AlignBottom | Qt::AlignLeft,
                      text);

        painter.translate(0, item.bbox_.height());
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::updateSelection
  //
  void
  PlaylistWidget::updateSelection(const QPoint & mousePos,
                                  bool toggleSelection,
                                  bool scrollToItem,
                                  bool allowGroupSelection)
  {
#if 0
    std::cerr << "PlaylistWidget::updateSelection" << std::endl;
#endif

    QPoint viewOffset = getViewOffset();
    QPoint p1 = mousePos + viewOffset;

    rubberBand_.setGeometry(QRect(anchor_ - viewOffset,
                                  p1 - viewOffset).normalized());

    QRect bboxSel = QRect(anchor_, p1).normalized();
    selectItems(bboxSel, toggleSelection);

    PlaylistGroup * group = lookupGroup(p1);
    if (group)
    {
      std::size_t index = lookupItemIndex(group, p1);

      PlaylistItem * item =
        (index < numItems_) ?
        &group->items_[index - group->offset_] :
        NULL;

      if (item)
      {
        highlighted_ = index;
      }
      else if (allowGroupSelection)
      {
        selectGroup(group);
        highlighted_ = group->offset_;
      }

      if (!scrollToItem)
      {
        update();
        return;
      }

      scrollTo(group, item);

      QPoint viewOffsetNew = getViewOffset();
      int dy = viewOffsetNew.y() - viewOffset.y();
      int dx = viewOffsetNew.x() - viewOffset.x();
      if (dy)
      {
        // move the cursor:
        QPoint pt = this->mapToGlobal(mousePos);
        pt -= QPoint(dx, dy);
        QCursor::setPos(pt);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::selectItems
  //
  void
  PlaylistWidget::selectItems(const QRect & bboxSel,
                              bool toggleSelection)
  {
#if 0
    std::cerr << "PlaylistWidget::selectItems" << std::endl;
#endif

    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      if (group.excluded_)
      {
        continue;
      }

      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;
        if (item.excluded_)
        {
          continue;
        }

        if (!group.collapsed_ && overlapExists(item.bbox_, bboxSel))
        {
          item.selected_ = toggleSelection ? !item.selected_ : true;
#if 0
          std::cerr << "selectItems, found: "
                    << item.name_.toUtf8().constData()
                    << std::endl;
#endif
        }
        else if (!toggleSelection)
        {
          item.selected_ = false;
        }
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistWidget::scrollTo
  //
  void
  PlaylistWidget::scrollTo(const PlaylistGroup * group,
                           const PlaylistItem * item)
  {
    if (!group && !item)
    {
      return;
    }

    QPoint viewOffset = getViewOffset();
    QRect area = viewport()->rect().translated(viewOffset);
    QRect rect = item && !group->collapsed_ ? item->bbox_ : group->bbox_;

    if (item && item == &(group->items_.front()) &&
        group->bbox_.y() < viewOffset.y())
    {
      // when scrolling to the first item in the group
      // scroll to the group header instead:
      rect = group->bbox_;
    }

    QScrollBar * hsb = horizontalScrollBar();
    QScrollBar * vsb = verticalScrollBar();

    if (rect.left() < area.left())
    {
      hsb->setValue(hsb->value() + rect.left() - area.left());
    }
    else if (rect.right() > area.right())
    {
      hsb->setValue(hsb->value() + qMin(rect.right() - area.right(),
                                        rect.left() - area.left()));
    }

    if (rect.top() < area.top())
    {
      vsb->setValue(vsb->value() + rect.top() - area.top());
    }
    else if (rect.bottom() > area.bottom())
    {
      vsb->setValue(vsb->value() + qMin(rect.bottom() - area.bottom(),
                                        rect.top() - area.top()));
    }

    update();
  }

  //----------------------------------------------------------------
  // PlaylistWidget::scrollTo
  //
  void
  PlaylistWidget::scrollTo(std::size_t index, PlaylistItem ** returnItem)
  {
    PlaylistGroup * group = NULL;
    PlaylistItem * item = lookup(index, &group);

    if (returnItem)
    {
      *returnItem = item;
    }

    if (item && group->collapsed_)
    {
      group->collapsed_ = false;
      updateGeometries();
    }

    scrollTo(group, item);
  }

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

  //----------------------------------------------------------------
  // lookupFirstGroup
  //
  inline static PlaylistGroup *
  lookupFirstGroup(std::vector<PlaylistGroup> & groups)
  {
    std::size_t numGroups = groups.size();
    std::size_t i = lookupFirstGroupIndex(groups);
    PlaylistGroup * found = i < numGroups ? &groups[i] : NULL;
    return found;
  }

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
  // PlaylistWidget::lookupGroupIndex
  //
  std::size_t
  PlaylistWidget::lookupGroupIndex(const QPoint & pt, bool findClosest) const
  {
#if 0
    std::cerr << "PlaylistWidget::lookupGroupIndex" << std::endl;
#endif

    if (groups_.empty())
    {
      return numItems_;
    }

    const int y = pt.y();
    const std::size_t numGroups = groups_.size();

    std::size_t i0 = 0;
    std::size_t i1 = numGroups;

    while (i0 != i1)
    {
      std::size_t i = i0 + (i1 - i0) / 2;

      const PlaylistGroup & group = groups_[i];

      int y1 =
        findClosest && !group.collapsed_ ?
        group.bboxItems_.y() + group.bboxItems_.height() :
        group.bbox_.y() + group.bbox_.height();

      if (y < y1)
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
      const PlaylistGroup & group = groups_[i0];
      if (!group.excluded_ &&
          (overlapExists(group.bbox_, pt) ||
           findClosest && !group.collapsed_ &&
           overlapExists(group.bboxItems_, pt)))
      {
#if 0
        std::cerr << "lookupGroup, found: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return i0;
      }
    }

    if (!findClosest)
    {
      return numItems_;
    }

    return
      (y < 0) ?
      lookupFirstGroupIndex(groups_) :
      lookupLastGroupIndex(groups_);
  }

  //----------------------------------------------------------------
  // PlaylistWidget::lookupGroup
  //
  PlaylistGroup *
  PlaylistWidget::lookupGroup(const QPoint & pt, bool findClosest)
  {
#if 0
    std::cerr << "PlaylistWidget::lookupGroup" << std::endl;
#endif

    const std::size_t numGroups = groups_.size();
    const std::size_t index = lookupGroupIndex(pt, findClosest);

    if (index < numGroups)
    {
      PlaylistGroup & group = groups_[index];
      return &group;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::lookupItemIndex
  //
  std::size_t
  PlaylistWidget::lookupItemIndex(const PlaylistGroup * group,
                                  const QPoint & pt) const
  {
#if 0
    std::cerr << "PlaylistWidget::lookupItemIndex" << std::endl;
#endif

    if (!group ||
        group->collapsed_ ||
        group->excluded_ ||
        group->items_.empty())
    {
      return numItems_;
    }

    const std::vector<PlaylistItem> & items = group->items_;

    if (overlapExists(group->bboxItems_, pt))
    {
      const int y = pt.y();
      const std::size_t groupSize = items.size();

      std::size_t i0 = 0;
      std::size_t i1 = groupSize;

      while (i0 != i1)
      {
        std::size_t i = i0 + (i1 - i0) / 2;
        const PlaylistItem & item = items[i];

        int y1 = item.bbox_.y() + item.bbox_.height();

        if (y < y1)
        {
          i1 = std::min<std::size_t>(i, i1 - 1);
        }
        else
        {
          i0 = std::max<std::size_t>(i, i0 + 1);
        }
      }

      if (i0 < groupSize)
      {
        const PlaylistItem & item = items[i0];

        if (overlapExists(item.bbox_, pt))
        {
#if 0
          std::cerr << "lookupItemIndex, found: "
                    << item.name_.toUtf8().constData()
                    << std::endl;
#endif
          return group->offset_ + i0;
        }
      }
    }

    if (group->bboxItems_.y() + group->bboxItems_.height() < pt.y())
    {
      // return the last non-excluded item:
      std::size_t index = group->offset_ + items.size() - 1;

      for (std::vector<PlaylistItem>::const_reverse_iterator
             j = items.rbegin(); j != items.rend(); --j, --index)
      {
        const PlaylistItem & item = *j;
        if (!item.excluded_)
        {
#if 0
          std::cerr << "lookupItemIndex, last: "
                    << item.name_.toUtf8().constData()
                    << std::endl;
#endif
          return index;
        }
      }
    }

    return numItems_;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::lookup
  //
  PlaylistItem *
  PlaylistWidget::lookup(PlaylistGroup * group, const QPoint & pt)
  {
    std::size_t index = lookupItemIndex(group, pt);
    if (index < numItems_)
    {
      std::size_t i = index - group->offset_;
      return &(group->items_[i]);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::lookup
  //
  PlaylistItem *
  PlaylistWidget::lookup(const QPoint & pt, PlaylistGroup ** returnGroup)
  {
    PlaylistGroup * group = lookupGroup(pt);
    if (returnGroup)
    {
      *returnGroup = group;
    }

    PlaylistItem * item = lookup(group, pt);
    return item;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::lookupGroup
  //
  PlaylistGroup *
  PlaylistWidget::lookupGroup(std::size_t index)
  {
#if 0
    std::cerr << "PlaylistWidget::lookupGroup: " << index << std::endl;
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

    // ignore the last group, it's an empty playlist tail banner:
    const std::size_t numGroups = groups_.size() - 1;

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
  // PlaylistWidget::lookup
  //
  PlaylistItem *
  PlaylistWidget::lookup(std::size_t index, PlaylistGroup ** returnGroup)
  {
#if 0
    std::cerr << "PlaylistWidget::lookup: " << index << std::endl;
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
  // PlaylistWidget::isMouseOverRemoveButton
  //
  bool
  PlaylistWidget::isMouseOverRemoveButton(const QPoint & pt,
                                          std::size_t & groupIndex,
                                          std::size_t & itemIndex) const
  {
    const std::size_t numGroups = groups_.size();

    itemIndex = numItems_;
    groupIndex = lookupGroupIndex(pt);
    if (groupIndex >= numGroups)
    {
      return false;
    }

    const PlaylistGroup & group = groups_[groupIndex];
    QRect bx = bboxRemoveButton(group.bbox_);

    if (overlapExists(bx, pt))
    {
      return true;
    }

    itemIndex = lookupItemIndex(&group, pt);
    if (itemIndex >= numItems_)
    {
      return false;
    }

    const PlaylistItem & item = group.items_[itemIndex - group.offset_];
    bx = bboxRemoveButton(item.bbox_);

    if (overlapExists(bx, pt))
    {
      return true;
    }

    return false;
  }
}

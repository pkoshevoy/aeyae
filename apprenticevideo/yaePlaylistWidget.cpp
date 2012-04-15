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
  // overlapExists
  // 
  static bool
  overlapExists(const QRect & a, const QRect & b)
  {
    if (a.height() && a.width() &&
        b.height() && b.width())
    {
      return a.intersects(b);
    }
    
    // ignore overlap with an empty region:
    return false;
  }
  
  //----------------------------------------------------------------
  // overlapExists
  // 
  static bool
  overlapExists(const QRect & a, const QPoint & b)
  {
    if (a.height() && a.width())
    {
      return a.contains(b);
    }
    
    // ignore overlap with an empty region:
    return false;
  }
  
  //----------------------------------------------------------------
  // shortenTextToFit
  // 
  static bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight)
  {
    static const QString ellipsis("...");
    
    // in case style sheet is used, get fontmetrics from painter:
    QFontMetrics fm = painter.fontMetrics();
    
    const int bboxWidth = bbox.width();
    
    textLeft.clear();
    textRight.clear();
    
    QSize sz = fm.size(Qt::TextSingleLine, text);
    int textWidth = sz.width();
    if (textWidth <= bboxWidth || bboxWidth <= 0)
    {
      // text fits, nothing to do:
      if (textAlignment & Qt::AlignLeft)
      {
        textLeft = text;
      }
      else
      {
        textRight = text;
      }
      
      return false;
    }
    
    // scale back the estimate to avoid cutting out too much of text,
    // because not all characters have the same width:
    const double stepScale = 0.78;
    const int textLen = text.size();
    
    int numToRemove = 0;
    int currLen = textLen - numToRemove;
    int aLen = currLen / 2;
    int bLen = currLen - aLen;
    
    while (currLen > 1)
    {
      // estimate (conservatively) how much text to remove:
      double excess = double(textWidth) / double(bboxWidth) - 1.0;
      if (excess <= 0.0)
      {
        break;
      }
      
      double excessLen =
        std::max<double>(1.0, 
                         stepScale * double(currLen) * 
                         excess / (excess + 1.0));
      
      numToRemove += int(excessLen);
      currLen = textLen - numToRemove;
      
      aLen = currLen / 2;
      bLen = currLen - aLen;
      QString tmp = text.left(aLen) + ellipsis + text.right(bLen);
      
      sz = fm.size(Qt::TextSingleLine, tmp);
      textWidth = sz.width();
    }
    
    if (currLen < 2)
    {
      // too short, give up:
      aLen = 0;
      bLen = 0;
    }
    
    if (textAlignment & Qt::AlignLeft)
    {
      textLeft = text.left(aLen) + ellipsis;
      textRight = text.right(bLen);
    }
    else
    {
      textLeft = text.left(aLen);
      textRight = ellipsis + text.right(bLen);
    }
    
    return true;
  }

  //----------------------------------------------------------------
  // drawTextToFit
  // 
  static void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText = NULL)
  {
    QString textLeft;
    QString textRight;
    
    if (!shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }
    
    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;
    
    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);
    
    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);
    
    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
  }
  
  //----------------------------------------------------------------
  // drawTextShadow
  // 
  static void
  drawTextShadow(QPainter & painter,
                 const QRect & bbox,
                 int textAlignment,
                 const QString & text)
  {
    painter.drawText(bbox.translated(-1, 0), textAlignment, text);
    painter.drawText(bbox.translated(1, 0), textAlignment, text);
    painter.drawText(bbox.translated(0, -1), textAlignment, text);
    painter.drawText(bbox.translated(0, 1), textAlignment, text);
  }
  
  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  // 
  static void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & fgPen,
                          const QPen & bgPen,
                          QRect * bboxText = NULL)
  {
    QRect bbox(bboxBig.x() + 1,
               bboxBig.y() + 1,
               bboxBig.width() - 1,
               bboxBig.height() - 1);
    
    QString textLeft;
    QString textRight;
    
    if (!shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.setPen(bgPen);
      drawTextShadow(painter, bbox, textAlignment, text);
      
      painter.setPen(fgPen);
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }
    
    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;
    
    painter.setPen(bgPen);
    drawTextShadow(painter, bbox, vertAlignment | Qt::AlignLeft, textLeft);
    drawTextShadow(painter, bbox, vertAlignment | Qt::AlignRight, textRight);
    
    painter.setPen(fgPen);
    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);
    
    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);
    
    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
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
    current_(0),
    highlighted_(0)
  {
    setPlaylist(std::list<QString>());
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::setPlaylist
  // 
  void
  PlaylistWidget::setPlaylist(const std::list<QString> & playlist)
  {
#if 0
    std::cerr << "PlaylistWidget::setPlaylist" << std::endl;
#endif
    
    // path of the first new playlist item:
    QString firstNewItemPath;
    
    for (std::list<QString>::const_iterator i = playlist.begin();
         i != playlist.end(); ++i)
    {
      QString path = *i;
      
      QFileInfo fi(path);
      if (fi.exists())
      {
        path = fi.absoluteFilePath();
      }
      
      if (firstNewItemPath.isEmpty())
      {
        firstNewItemPath = path;
      }
      
      QString name = toWords(fi.baseName());
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
        
        keys.push_front(key);
        
        QString dir = fi.path();
        fi = QFileInfo(dir);
      }
      
      tree_.set(keys, path);
    }
    
    // flatten the tree into a list of play groups:
    typedef TPlaylistTree::FringeGroup TFringeGroup;
    std::list<TFringeGroup> fringeGroups;
    tree_.get(fringeGroups);
    groups_.clear();
    numItems_ = 0;
    current_ = 0;
    highlighted_ = 0;
    
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
        playlistItem.name_ = toWords(fi.baseName());
        playlistItem.ext_ = fi.completeSuffix();
        
        if (playlistItem.path_ == firstNewItemPath)
        {
          highlighted_ = numItems_;
        }
      }
    }
    
    // add a tail group:
    {
      groups_.push_back(PlaylistGroup());
      PlaylistGroup & group = groups_.back();
      group.name_ = tr("END OF PLAYLIST");
      group.offset_ = numItems_;
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
        
        if (!exclusive)
        {
          // done:
          break;
        }
      }
    }
    
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
      
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           !group.excluded_ && j != group.items_.end(); oldIndex++)
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
    
    draw(painter, localRegion);
    
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
    bool modShift = mod & Qt::ShiftModifier;
    bool modNone  = !modShift && !modAlt;
    
    if (modNone || modAlt)
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
            highlighted_ = group->offset_ - 1;
            lookup(highlighted_, &group);
            
            if (group->collapsed_)
            {
              highlighted_ = group->offset_;
            }
          }
          else
          {
            highlighted_--;
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
            highlighted_ = group->offset_ + group->items_.size();
          }
          else
          {
            highlighted_++;
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
            hiItem ?
            hiItem->bbox_.center() :
            hiGroup->bbox_.center();
          
          QPoint p1 =
            pageUp ?
            QPoint(p0.x(), p0.y() - vh) :
            QPoint(p0.x(), p0.y() + vh);
          
          group = lookupGroup(p1);
          std::size_t index = lookupItemIndex(group, p1);
          
          highlighted_ = (index < numItems_) ? index : group->offset_;
          
          found = lookup(highlighted_, &group);
        }
      }
      else if (modAlt && (stepUp || stepDn) && numItems_)
      {
        highlighted_ = stepUp ? 0 : (numItems_ - 1);
        found = lookup(highlighted_, &group);
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
            selectItem(hiGroup->offset_);
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
          else if (groupCollapse && highlighted_ > 0)
          {
            highlighted_ = hiGroup->offset_ - 1;
            lookup(highlighted_, &hiGroup);
            
            if (hiGroup->collapsed_)
            {
              highlighted_ = hiGroup->offset_;
            }
            
            found = lookup(highlighted_, &group);
          }
          else if (groupExpand && highlighted_ < numItems_)
          {
            highlighted_++;
            found = lookup(highlighted_, &group);
          }
        }
      }
      
      if (group || found)
      {
        // update the anchor:
        anchor_ =
          found && !group->collapsed_?
          found->bbox_.center() :
          group->bbox_.center();
        
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
        highlighted_--;
        found = lookup(highlighted_, &group);
      }
      else if (stepDn && highlighted_ < numItems_)
      {
        highlighted_++;
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
            hiItem ?
            hiItem->bbox_.center() :
            hiGroup->bbox_.center();
          
          QPoint p1 =
            pageUp ?
            QPoint(p0.x(), p0.y() - vh) :
            QPoint(p0.x(), p0.y() + vh);
          
          group = lookupGroup(p1);
          std::size_t index = lookupItemIndex(group, p1);
          
          highlighted_ = (index < numItems_) ? index : group->offset_;
          
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
    else
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

    int offset = 0;
    int width = viewport()->width();
    std::size_t y = 0;
    
    numShown_ = 0;
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      group.offset_ = offset;
      
      int headerHeight = group.excluded_ ? 0 : kGroupNameHeight;
      group.bbox_.setX(0);
      group.bbox_.setY(y);
      group.bbox_.setWidth(width);
      group.bbox_.setHeight(headerHeight);
      y += headerHeight;
      
      group.bboxItems_.setX(0);
      group.bboxItems_.setWidth(width);
      group.bboxItems_.setY(y);
      
      if (!group.excluded_)
      {
        for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
             !group.collapsed_ && j != group.items_.end(); ++j)
        {
          PlaylistItem & item = *j;
          item.bbox_.setX(0);
          item.bbox_.setY(y);
          item.bbox_.setWidth(width);

          if (!item.excluded_)
          {
            item.bbox_.setHeight(kGroupItemHeight);
            y += kGroupItemHeight;
            numShown_++;
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
  // headerBrush
  // 
  static const QBrush & headerBrush(int height)
  {
    static QBrush * brush = NULL;
    if (!brush)
    {
      QLinearGradient gradient(0, 1, 0, height - 1);
      gradient.setColorAt(0.0,  QColor("#1b1c20"));
      gradient.setColorAt(0.49, QColor("#1b1f2f"));
      gradient.setColorAt(0.5,  QColor("#070d1e"));
      gradient.setColorAt(1.0,  QColor("#152141"));
      gradient.setSpread(QGradient::PadSpread);
      brush = new QBrush(gradient);
    }
    
    return *brush;
  }

  //----------------------------------------------------------------
  // PlaylistWidget::draw
  // 
  void
  PlaylistWidget::draw(QPainter & painter, const QRect & region)
  {
    static const QColor zebraBg[] = {
      QColor(0, 0, 0, 0),
      QColor(0xf4, 0xf4, 0xf4)
    };
    
    QPalette palette = this->palette();
    
    QColor selectedColorBg = palette.color(QPalette::Highlight);
    QColor selectedColorFg = palette.color(QPalette::HighlightedText);
    QColor foregroundColor = palette.color(QPalette::WindowText);
    QColor headerColor = QColor("#40a0ff");
    
    QFont textFont = painter.font();
    textFont.setPixelSize(10);
    
    painter.setFont(textFont);
    
    QFont tinyFont = textFont;
    tinyFont.setPixelSize(7);
    
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
        
        painter.setBrush(headerBrush(bbox.height()));
        painter.setBrushOrigin(bbox.topLeft());
        painter.setPen(Qt::NoPen);
        painter.drawRect(bbox);
        
        if (!group.keyPath_.empty())
        {
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
          
          if (isHighlightedGroup)
          {
            painter.setBrush(Qt::white);
          }
          else
          {
            painter.setBrush(headerColor);
          }
          
          painter.setPen(Qt::NoPen);
          painter.drawPolygon(arrow, 3);
          
          QRect bx = bbox.adjusted(10 + w, 0, 0, 0);
          drawTextWithShadowToFit(painter,
                                  bx,
                                  Qt::AlignVCenter | Qt::AlignCenter,
                                  group.name_,
                                  headerColor,
                                  QColor("#102040"));
        }
        else
        {
          QRect bx = bbox.adjusted(2, 1, -2, -1);
          painter.setFont(tinyFont);
          
          if (group.offset_ == highlighted_ && numItems_)
          {
            painter.setPen(Qt::white);
          }
          else
          {
            painter.setPen(headerColor);
          }
          
          QString text = tr("%1 ITEMS,  %2").arg(numShown_).arg(group.name_);
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
          
          painter.setPen(Qt::white);
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
  // lookupFirstGroup
  // 
  // return the fist non-excluded group:
  // 
  static PlaylistGroup *
  lookupFirstGroup(std::vector<PlaylistGroup> & groups)
  {
    for (std::vector<PlaylistGroup>::iterator i = groups.begin();
         i != groups.end(); ++i)
    {
      PlaylistGroup & group = *i;
      if (!group.excluded_)
      {
#if 0
        std::cerr << "lookupGroup, first: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return &group;
      }
    }
    
    return NULL;
  }
  
  //----------------------------------------------------------------
  // lookupLastGroup
  // 
  // return the last non-excluded group:
  // 
  static PlaylistGroup *
  lookupLastGroup(std::vector<PlaylistGroup> & groups)
  {
    for (std::vector<PlaylistGroup>::reverse_iterator i = groups.rbegin();
         i != groups.rend(); ++i)
    {
      PlaylistGroup & group = *i;
      if (!group.excluded_)
      {
#if 0
        std::cerr << "lookupGroup, last: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return &group;
      }
    }
    
    return NULL;
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
    
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      
      if (group.excluded_)
      {
        continue;
      }
      
      if (overlapExists(group.bbox_, pt) ||
          findClosest && !group.collapsed_ &&
          overlapExists(group.bboxItems_, pt))
      {
#if 0
        std::cerr << "lookupGroup, found: "
                  << group.name_.toUtf8().constData()
                  << std::endl;
#endif
        return &group;
      }
    }
    
    if (groups_.empty() || !findClosest)
    {
      return NULL;
    }
    
    return
      (pt.y() < 0) ?
      lookupFirstGroup(groups_) :
      lookupLastGroup(groups_);
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::lookupItemIndex
  // 
  std::size_t
  PlaylistWidget::lookupItemIndex(PlaylistGroup * group, const QPoint & pt)
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
    
    std::vector<PlaylistItem> & items = group->items_;
    std::size_t index = group->offset_;

    if (overlapExists(group->bboxItems_, pt))
    {
      for (std::vector<PlaylistItem>::iterator j = items.begin();
           j != items.end(); ++j, ++index)
      {
        PlaylistItem & item = *j;
        if (item.excluded_)
        {
          continue;
        }
      
        if (overlapExists(item.bbox_, pt))
        {
#if 0
          std::cerr << "lookupItemIndex, found: "
                    << item.name_.toUtf8().constData()
                    << std::endl;
#endif
          return index;
        }
      }
    }
    
    if (group->bboxItems_.y() + group->bboxItems_.height() < pt.y())
    {
      // return the last non-excluded item:
      index = group->offset_ + items.size() - 1;
      for (std::vector<PlaylistItem>::reverse_iterator j = items.rbegin();
           j != items.rend(); --j, --index)
      {
        PlaylistItem & item = *j;
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
    
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      std::size_t numItems = group.items_.size();
      
      if (index < group.offset_ + numItems)
      {
        return &group;
      }
    }
    
    YAE_ASSERT(index == numItems_);
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
  
}

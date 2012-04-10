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
  // kPlayPauseIconWidth
  // 
  static const int kPlayPauseIconWidth = 14;
  
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
  // PlaylistItem::PlaylistItem
  // 
  PlaylistItem::PlaylistItem():
    selected_(false)
  {}
  
  
  //----------------------------------------------------------------
  // PlaylistGroup::PlaylistGroup
  // 
  PlaylistGroup::PlaylistGroup():
    offset_(0)
  {}
  
  
  //----------------------------------------------------------------
  // PlaylistWidget::PlaylistWidget
  // 
  PlaylistWidget::PlaylistWidget(QWidget * parent, Qt::WindowFlags):
    QAbstractScrollArea(parent),
    rubberBand_(QRubberBand::Rectangle, this),
    numItems_(0),
    current_(0),
    highlighted_(0)
  {}
  
  //----------------------------------------------------------------
  // PlaylistWidget::setPlaylist
  // 
  void
  PlaylistWidget::setPlaylist(const std::list<QString> & playlist)
  {
    if (playlist.empty())
    {
      return;
    }
    
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
    
    updateGeometries();
    setCurrentItem(highlighted_);
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
  PlaylistWidget::setCurrentItem(std::size_t index)
  {
    if (index != current_)
    {
      current_ = (index < numItems_) ? index : numItems_;
      update();
      
      emit currentItemChanged(current_);
    }
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::playbackPaused
  // 
  void
  PlaylistWidget::playbackPaused(bool paused)
  {
    paused_ = paused;
    update();
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::selectAll
  // 
  void
  PlaylistWidget::selectAll(bool selected)
  {
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;
        item.selected_ = selected;
      }
    }
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::selectNone
  // 
  void
  PlaylistWidget::selectNone()
  {
    selectAll(false);
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::removeSelected
  // 
  void
  PlaylistWidget::removeSelected()
  {
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); )
    {
      PlaylistGroup & group = *i;
      
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); )
      {
        PlaylistItem & item = *j;
        if (!item.selected_)
        {
          ++j;
          continue;
        }
        
        // 1. remove the item from the tree:
        std::list<QString> keyPath = group.keyPath_;
        keyPath.push_back(item.key_);
        tree_.remove(keyPath);
        
        // 2. remove the item from the group:
        j = group.items_.erase(j);
      }
      
      // if the group is empty, remove it:
      if (!group.items_.empty())
      {
        ++i;
        continue;
      }
      
      tree_.remove(group.keyPath_);
      i = groups_.erase(i);
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
    if (e->button() == Qt::LeftButton)
    {
      e->accept();
      
      int mod = e->modifiers();
      bool extendSelection = (mod & Qt::ShiftModifier);
      bool toggleSelection = !extendSelection && (mod & Qt::ControlModifier);
      
      QPoint viewOffset = getViewOffset();
      QPoint pt = e->pos() + viewOffset;
      
      if (!extendSelection)
      {
        anchor_ = pt;
        
        rubberBand_.setGeometry(QRect(anchor_ - viewOffset, QSize()));
        rubberBand_.show();
      }
      
      updateSelection(e->pos(), toggleSelection);
    }
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::mouseReleaseEvent
  // 
  void
  PlaylistWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::LeftButton)
    {
      e->accept();
      rubberBand_.hide();
    }
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::mouseMoveEvent
  // 
  void
  PlaylistWidget::mouseMoveEvent(QMouseEvent * e)
  {
    if (e->buttons() & Qt::LeftButton)
    {
      e->accept();
      
      bool toggleSelection = false;
      bool scrollToItem = true;
      updateSelection(e->pos(), toggleSelection, scrollToItem);
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
        paused_ = false;
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
        updateSelection(e->pos());
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
        highlighted_--;
        found = lookup(highlighted_, &group);
      }
      else if (modNone && stepDn && highlighted_ + 1 < numItems_)
      {
        highlighted_++;
        found = lookup(highlighted_, &group);
      }
      else if (modNone && (pageUp || pageDn))
      {
        PlaylistItem * item = lookup(highlighted_);
        if (item)
        {
          int vh = viewport()->height();
          
          QPoint viewOffset = getViewOffset();
          QPoint p0 = item->bbox_.center();
          QPoint p1 =
            pageUp ?
            QPoint(p0.x(), p0.y() - vh) :
            QPoint(p0.x(), p0.y() + vh);

          group = lookupGroup(p1);
          std::size_t index = lookupItemIndex(group, p1);
          
          highlighted_ = (index < numItems_) ? index : 0;
          found = lookup(highlighted_);
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
      
      if (found)
      {
        // update the anchor:
        anchor_ = found->bbox_.center();
        
        // update the selection set:
        QPoint viewOffset = getViewOffset();
        QPoint mousePt(anchor_.x() - viewOffset.x(),
                       anchor_.y() - viewOffset.y());

        bool toggleSelection = false;
        bool scrollToItem = true;
        updateSelection(mousePt, toggleSelection, scrollToItem);
        e->accept();
      }
      else if (group)
      {
        scrollTo(group, NULL);
        e->accept();
      }
    }
    else if (modShift && (stepUp || stepDn))
    {
      // update selection set:
      PlaylistGroup * group = NULL;
      PlaylistItem * found = NULL;
      
      if (stepUp && highlighted_ > 0)
      {
        highlighted_--;
        found = lookup(highlighted_, &group);
      }
      else if (stepDn && highlighted_ + 1 < numItems_)
      {
        highlighted_++;
        found = lookup(highlighted_, &group);
      }
      
      if (found)
      {
        QPoint viewOffset = getViewOffset();
        QPoint pt = found->bbox_.center();
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
    (void) e;
    updateGeometries();
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::updateGeometries
  // 
  void
  PlaylistWidget::updateGeometries()
  {
    int offset = 0;
    int width = viewport()->width();
    
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      group.bbox_.setX(0);
      group.bbox_.setY(offset);
      group.bbox_.setWidth(width);
      group.bbox_.setHeight(kGroupNameHeight);
      offset += kGroupNameHeight;
      
      group.bboxItems_.setX(0);
      group.bboxItems_.setY(offset);
      group.bboxItems_.setWidth(width);
      
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;
        item.bbox_.setX(0);
        item.bbox_.setY(offset);
        item.bbox_.setWidth(width);
        item.bbox_.setHeight(kGroupItemHeight);
        offset += kGroupItemHeight;
      }
      
      group.bboxItems_.setHeight(offset - group.bboxItems_.y());
    }
    
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
      
      PlaylistItem & item = group.items_.back();
      contentHeight = item.bbox_.y() + item.bbox_.height();
    }
    
    verticalScrollBar()->setSingleStep(kGroupItemHeight);
    verticalScrollBar()->setPageStep(viewportHeight);
    verticalScrollBar()->setRange(0, qMax(0, contentHeight - viewportHeight));
    
    horizontalScrollBar()->setSingleStep(kGroupItemHeight);
    horizontalScrollBar()->setPageStep(viewportSize.width());
    horizontalScrollBar()->setRange(0, qMax(0, contentWidth - viewportWidth));
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::draw
  // 
  void
  PlaylistWidget::draw(QPainter & painter, const QRect & region)
  {
    static QPixmap iconPlay = QPixmap(":/images/iconPlay.png");
    static QPixmap iconPause = QPixmap(":/images/iconPause.png");
    
    static const QColor headerColorBg(0x40, 0x80, 0xff);
    static const QColor activeColorBg(0xff, 0x80, 0x40);
    static const QColor brightColorFg(0xff, 0xff, 0xff);
    static const QColor zebraBg[] = {
      QColor(0, 0, 0, 0),
      QColor(0xf4, 0xf4, 0xf4)
    };
    
    QPalette palette = this->palette();
    
    QColor selectedColorBg = palette.color(QPalette::Highlight);
    QColor selectedColorFg = palette.color(QPalette::HighlightedText);
    QColor foregroundColor = palette.color(QPalette::WindowText);
    QFont textFont = painter.font();
    
    std::size_t index = 0;
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      
      if (group.bbox_.intersects(region))
      {
        painter.fillRect(group.bbox_, headerColorBg);
        painter.setPen(brightColorFg);
        
        drawTextToFit(painter,
                      group.bbox_,
                      Qt::AlignVCenter | Qt::AlignCenter,
                      group.name_);
      }
      
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j, index++)
      {
        PlaylistItem & item = *j;
        std::size_t zebraIndex = index % 2;
        
        if (!item.bbox_.intersects(region))
        {
          continue;
        }
        
        QColor bg = zebraBg[zebraIndex];
        QColor fg = foregroundColor;
        const QPixmap * icon = NULL;
        bool underline = false;
        
        if (index == current_)
        {
          icon = paused_ ? &iconPause : &iconPlay;
          underline = true;
        }
        
        if (item.selected_)
        {
          bg = selectedColorBg;
          fg = selectedColorFg;
        }
        
        painter.setPen(fg);
        painter.fillRect(item.bbox_, bg);
        QString text = tr("%1, %2").arg(item.name_).arg(item.ext_);
        
        QRect bboxText = item.bbox_;
        if (icon)
        {
          int yoffset = (item.bbox_.height() - icon->height()) / 2;
          QPoint pt(item.bbox_.x(), item.bbox_.y() + yoffset);
          painter.drawPixmap(pt, *icon);
        }
        
        int xoffset = kPlayPauseIconWidth;
        bboxText.setX(item.bbox_.x() + xoffset);
        bboxText.setWidth(item.bbox_.width() - xoffset);
        
        QRect bboxTextOut;
        drawTextToFit(painter,
                      bboxText,
                      Qt::AlignVCenter | Qt::AlignLeft,
                      text,
                      &bboxTextOut);
        
        if (underline)
        {
          QColor fg = zebraBg[1].darker(150);
          QPoint p0 = bboxTextOut.bottomLeft() + QPoint(0, 1);
          QPoint p1 = bboxTextOut.bottomRight() + QPoint(0, 1);
          
          painter.setRenderHint(QPainter::Antialiasing, false);
          painter.setPen(fg);
          painter.drawLine(p0, p1);
          painter.setRenderHint(QPainter::Antialiasing);
        }
      }
    }
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::updateSelection
  // 
  void
  PlaylistWidget::updateSelection(const QPoint & mousePos,
                                  bool toggleSelection,
                                  bool scrollToItem)
  {
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
      
      highlighted_ = index;
      
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
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      for (std::vector<PlaylistItem>::iterator j = group.items_.begin();
           j != group.items_.end(); ++j)
      {
        PlaylistItem & item = *j;
        
        if (item.bbox_.intersects(bboxSel))
        {
          item.selected_ = toggleSelection ? !item.selected_ : true;
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
    QRect rect = item ? item->bbox_ : group->bbox_;
    
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
    
    scrollTo(group, item);
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::lookupGroup
  // 
  PlaylistGroup *
  PlaylistWidget::lookupGroup(const QPoint & pt)
  {
    for (std::vector<PlaylistGroup>::iterator i = groups_.begin();
         i != groups_.end(); ++i)
    {
      PlaylistGroup & group = *i;
      if (group.bboxItems_.contains(pt) ||
          group.bbox_.contains(pt))
      {
        return &group;
      }
    }
    
    if (!groups_.empty())
    {
      QRect bbox = groups_.front().bbox_;
      if (pt.y() <= bbox.y())
      {
        return &groups_.front();
      }
      
      bbox = groups_.back().bboxItems_;
      if (bbox.y() + bbox.height() < pt.y())
      {
        return &groups_.back();
      }
    }
    
    return NULL;
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::lookupItemIndex
  // 
  std::size_t
  PlaylistWidget::lookupItemIndex(PlaylistGroup * group, const QPoint & pt)
  {
    if (group)
    {
      std::size_t index = group->offset_;
      for (std::vector<PlaylistItem>::iterator j = group->items_.begin();
           j != group->items_.end(); ++j, ++index)
      {
        PlaylistItem & item = *j;
        if (item.bbox_.contains(pt))
        {
          return index;
        }
      }
      
      if (!group->items_.empty())
      {
        QRect bbox = group->items_.back().bbox_;
        if (bbox.y() + bbox.height() < pt.y())
        {
          index = group->offset_ + group->items_.size() - 1;
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
    
    return NULL;
  }
  
  //----------------------------------------------------------------
  // PlaylistWidget::lookup
  // 
  PlaylistItem *
  PlaylistWidget::lookup(std::size_t index, PlaylistGroup ** returnGroup)
  {
    PlaylistGroup * group = lookupGroup(index);
    if (returnGroup)
    {
      *returnGroup = group;
    }
    
    if (group)
    {
      std::size_t i = index - group->offset_;
      return &group->items_[i];
    }
    
    return NULL;
  }
  
}

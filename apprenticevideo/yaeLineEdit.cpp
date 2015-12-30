// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar  4 17:11:18 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/api/yae_api.h"

// local includes:
#include "yaeLineEdit.h"

// Qt includes:
#include <QToolButton>
#include <QPixmap>
#include <QStyle>

namespace yae
{

  //----------------------------------------------------------------
  // LineEdit::LineEdit
  //
  LineEdit::LineEdit(QWidget * parent):
    QLineEdit(parent)
  {
    static QPixmap pixmap = QPixmap(":/images/clear.png");

    clear_ = new QToolButton(this);
    clear_->setObjectName("clear_");
    clear_->setIcon(QIcon(pixmap));
    clear_->setIconSize(pixmap.size());
    clear_->setCursor(Qt::ArrowCursor);
    clear_->setMinimumSize(QSize(11, 11));
    clear_->setFocusPolicy(Qt::NoFocus);
    clear_->hide();

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int minHeight = 11 + frameWidth * 2 + 4;

    setStyleSheet(QString::fromUtf8("QLineEdit { padding-right: %1px; }").
                  arg(minHeight));
    setMinimumWidth(minHeight);
    setMinimumHeight(minHeight);

    bool ok = connect(clear_, SIGNAL(clicked()),
                      this, SLOT(clear()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(textChanged(const QString &)),
                 this, SLOT(textChanged(const QString &)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // LineEdit::resizeEvent
  //
  void
  LineEdit::resizeEvent(QResizeEvent *)
  {
    QRect bx = rect();

    int w = bx.width();
    int h = bx.height();
    int f = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int p = 2 + f;

    clear_->setGeometry(w - h + p - 1, p, h - 2 * p, h - 2 * p);
  }

  //----------------------------------------------------------------
  // LineEdit::textChanged
  //
  void
  LineEdit::textChanged(const QString & text)
  {
    clear_->setVisible(!text.isEmpty());
  }
}

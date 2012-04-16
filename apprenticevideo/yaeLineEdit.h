// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar  4 17:11:18 MDT 2012
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LINE_EDIT_H_
#define YAE_LINE_EDIT_H_

// Qt includes:
#include <QLineEdit>

// forward declarations:
class QToolButton;

namespace yae
{
  //----------------------------------------------------------------
  // LineEdit
  // 
  class LineEdit : public QLineEdit
  {
    Q_OBJECT;
    
  public:
    LineEdit(QWidget * parent = 0);
    
  protected:
    void resizeEvent(QResizeEvent *);

  protected slots:
    void textChanged(const QString & text);
    
  protected:
    QToolButton * clear_;
  };
}


#endif // YAE_LINE_EDIT_H_

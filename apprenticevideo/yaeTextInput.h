// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 20 20:13:45 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXT_INPUT_H_
#define YAE_TEXT_INPUT_H_

// Qt library:
#include <QEvent>
#include <QFont>
#include <QObject>

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // TextInput
  //
  class TextInput : public QObject, public Item
  {
    Q_OBJECT;

    TextInput(const TextInput &);
    TextInput & operator = (const TextInput &);

  public:
    TextInput(const char * id);
    ~TextInput();

    // virtual:
    bool event(QEvent * event);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // accessor to current text payload:
    QString text() const;

    struct TPrivate;
    TPrivate * p_;

    QFont font_;
    ItemRef fontSize_; // in points
    ColorRef color_;
  };

}


#endif // YAE_TEXT_INPUT_H_

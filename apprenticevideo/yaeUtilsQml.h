// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Aug 23 16:55:51 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_QML_H_
#define YAE_UTILS_QML_H_
#ifdef YAE_USE_PLAYER_QUICK_WIDGET

// Qt includes:
#include <QObject>
#include <QQmlContext>


namespace yae
{
  class UtilsQml : public QObject
  {
    Q_OBJECT;

  public:

    static UtilsQml * singleton();

    Q_INVOKABLE QObject *
    find_qobject(QObject * startHere, const QString & name) const;

    Q_INVOKABLE void
    dump_object_info(QObject * object) const;

    Q_INVOKABLE void
    dump_object_tree(QObject * object) const;

    Q_INVOKABLE void
    dump_object_tree_info(QObject * object) const;
  };
}


#endif // YAE_USE_PLAYER_QUICK_WIDGET
#endif // YAE_UTILS_QML_H_

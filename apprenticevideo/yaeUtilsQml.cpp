// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Aug 23 16:55:51 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local includes:
#include "yaeUtilsQml.h"


namespace yae
{
  //----------------------------------------------------------------
  // UtilsQml::singleton
  //
  UtilsQml *
  UtilsQml::singleton()
  {
    static UtilsQml utils;
    return &utils;
  }

  //----------------------------------------------------------------
  // UtilsQml::find_qobject
  //
  QObject *
  UtilsQml::find_qobject(QObject * startHere, const QString & name) const
  {
    if (!startHere)
    {
      return NULL;
    }

    if (startHere->objectName() == name)
    {
      return startHere;
    }

    return startHere->findChild<QObject *>(name);
  }

  //----------------------------------------------------------------
  // UtilsQml::dump_object_info
  //
  void
  UtilsQml::dump_object_info(QObject * object) const
  {
    if (object)
    {
      object->dumpObjectInfo();
    }
    else
    {
      qDebug("dump_object_info(NULL)");
    }
  }

  //----------------------------------------------------------------
  // UtilsQml::dump_object_tree
  //
  void
  UtilsQml::dump_object_tree(QObject * object) const
  {
    if (object)
    {
      object->dumpObjectTree();
    }
    else
    {
      qDebug("dump_object_tree(NULL)");
    }
  }

}

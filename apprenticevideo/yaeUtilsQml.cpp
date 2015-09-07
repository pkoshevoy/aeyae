// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Aug 23 16:55:51 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <iostream>
#include <sstream>
#include <string>

// Qt includes:
#include <QMetaProperty>

// yae includes:
#include "yae/api/yae_api.h"

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

  //----------------------------------------------------------------
  // UtilsQml::dump_object_tree_info
  //
  static void
  dump_object_tree_info(std::ostream & os, QObject * object, int indent = 0)
  {
    std::string ind;
    {
      static const char * s =
        "                                                                    ";
      static const std::size_t sz = strlen(s);

      int n = indent;
      while (n >= sz)
      {
        ind += s;
        n -= sz;
      }

      ind += (s + (sz - n));
    }

    os
      << ind
      << "--------------------------------------------------------------------"
      << std::endl;

    if (object)
    {
      const QMetaObject * meta = object->metaObject();
      QString name = object->objectName();
      QObject * parent = object->parent();

      os
        << ind << "class: " << meta->className()
        << ", object: " << object
        << ", parent: " << parent << '\n'
        << ind << "name: " << name.toUtf8().constData() << '\n'
        << ind << "properties:\n";

      int propCount = meta->propertyCount();
      for (int i = 0; i < propCount; i++)
      {
        QMetaProperty p = meta->property(i);

        // FIXME: show width/height only
        if (strcmp(p.name(), "width") != 0 &&
            strcmp(p.name(), "height") != 0 &&
            strcmp(p.name(), "implicitWidth") != 0 &&
            strcmp(p.name(), "implicitHeight") != 0)
        {
          continue;
        }

        os
          << ind << p.name() << " (" << p.typeName() << "): "
          << p.read(object).toString().toUtf8().constData()
          << std::endl;
      }

      QObjectList children = object->children();
      while (!children.empty())
      {
        QObject * child = children.front();
        children.pop_front();
        dump_object_tree_info(os, child, indent + 2);
      }
    }
    else
    {
      os << ind << "dump_object_tree_info(NULL)" << std::endl;
    }
  }

  //----------------------------------------------------------------
  // UtilsQml::dump_object_tree_info
  //
  void
  UtilsQml::dump_object_tree_info(QObject * object) const
  {
#if 0
    std::ostringstream oss;
    yae::dump_object_tree_info(oss, object);
    std::cerr << oss.str();
#else
    yae::dump_object_tree_info(std::cerr, object);
#endif
  }
}

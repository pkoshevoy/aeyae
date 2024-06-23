// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:34:13 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_UTILS_QT_H_
#define YAE_UTILS_QT_H_

// std includes:
#include <list>
#include <string.h>
#include <cstdio>
#include <sstream>

// yae includes:
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader_factory.h"
#include "yae/video/yae_reader.h"

// yaeui:
#include "yaeApplication.h"

// Qt includes:
#include <QAction>
#include <QApplication>
#if !defined(_WIN32) && !defined(__APPLE__)
#include <QtDBus/QtDBus>
#endif
#include <QEvent>
#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QShortcut>
#include <QSignalMapper>
#include <QString>


namespace yae
{

  //----------------------------------------------------------------
  // kExtEyetv
  //
  extern YAEUI_API const QString kExtEyetv;

  //----------------------------------------------------------------
  // toQString
  //
  YAEUI_API QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace = false);

  //----------------------------------------------------------------
  // splitIntoWords
  //
  YAEUI_API void
  splitIntoWords(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // toWords
  //
  YAEUI_API QString toWords(const QString & key);

  //----------------------------------------------------------------
  // prepareForSorting
  //
  YAEUI_API QString prepareForSorting(const QString & key);

  //----------------------------------------------------------------
  // overlapExists
  //
  YAEUI_API bool
  overlapExists(const QRect & a, const QRect & b);

  //----------------------------------------------------------------
  // overlapExists
  //
  YAEUI_API bool
  overlapExists(const QRect & a, const QPoint & b);

  //----------------------------------------------------------------
  // join
  //
  // returns:
  //  a if b is empty
  //  b if a is empty
  //  (a + separator + b) otherwise
  //
  YAEUI_API QString
  join(const QString & a, const QString & separator, const QString & b);

  //----------------------------------------------------------------
  // parseEyetvInfo
  //
  YAEUI_API bool
  parseEyetvInfo(const QString & eyetvPath,
                 QString & channelNumber,
                 QString & channelName,
                 QString & program,
                 QString & episode,
                 QString & timestamp);

  //----------------------------------------------------------------
  // xmlEncode
  //
  // this properly handles special characters &, <, >, ", etc...
  //
  YAEUI_API QString
  xmlEncode(const QString & text);

  //----------------------------------------------------------------
  // saveSetting
  //
  YAEUI_API bool
  saveSetting(const QString & key, const QString & value);

  //----------------------------------------------------------------
  // saveBooleanSetting
  //
  YAEUI_API bool
  saveBooleanSetting(const QString & key, bool value);

  //----------------------------------------------------------------
  // loadSetting
  //
  YAEUI_API bool
  loadSetting(const QString & key, QString & value);

  //----------------------------------------------------------------
  // loadSettingOrDefault
  //
  YAEUI_API QString
  loadSettingOrDefault(const QString & key, const QString & defaultValue);

  //----------------------------------------------------------------
  // loadBooleanSettingOrDefault
  //
  YAEUI_API bool
  loadBooleanSettingOrDefault(const QString & key, bool defaultValue);

  //----------------------------------------------------------------
  // removeSetting
  //
  YAEUI_API bool
  removeSetting(const QString & key);

  //----------------------------------------------------------------
  // findFiles
  //
  YAEUI_API bool
  findFiles(std::list<QString> & files,
            const QString & startHere,
            bool recursive = true);

  //----------------------------------------------------------------
  // addFolderToPlaylist
  //
  YAEUI_API bool
  addFolderToPlaylist(std::list<QString> & playlist, const QString & folder);

  //----------------------------------------------------------------
  // addToPlaylist
  //
  YAEUI_API bool
  addToPlaylist(std::list<QString> & playlist, const QString & path);

  //----------------------------------------------------------------
  // convert_path_to_utf8
  //
  YAEUI_API bool
  convert_path_to_utf8(const QString & path, std::string & path_utf8);

  //----------------------------------------------------------------
  // openFile
  //
  YAEUI_API IReaderPtr
  openFile(const yae::TReaderFactoryPtr & readerFactory,
           const QString & fn,
           bool hwdec = false);

  //----------------------------------------------------------------
  // testEachFile
  //
  YAEUI_API bool
  testEachFile(const yae::TReaderFactoryPtr & readerPrototype,
               const std::list<QString> & playlist);

  //----------------------------------------------------------------
  // to_str
  //
  YAEUI_API const char * to_str(QEvent::Type et);

  //----------------------------------------------------------------
  // SignalBlocker
  //
  struct SignalBlocker
  {
    SignalBlocker(QObject * qObj = NULL)
    {
      *this << qObj;
    }

    ~SignalBlocker()
    {
      while (!blocked_.empty())
      {
        QObject * qObj = blocked_.front();
        blocked_.pop_front();

        qObj->blockSignals(false);
      }
    }

    SignalBlocker & operator << (QObject * qObj)
    {
      if (qObj && !qObj->signalsBlocked())
      {
        qObj->blockSignals(true);
        blocked_.push_back(qObj);
      }

      return *this;
    }

    std::list<QObject *> blocked_;
  };

  //----------------------------------------------------------------
  // BlockSignal
  //
  struct BlockSignal
  {
    BlockSignal(const QObject * sender, const char * senderSignal,
                const QObject * receiver, const char * receiverSlot):
      sender_(sender),
      signal_(senderSignal),
      receiver_(receiver),
      slot_(receiverSlot),
      reconnect_(false)
    {
      reconnect_ = QObject::disconnect(sender_, signal_, receiver_, slot_);
    }

    ~BlockSignal()
    {
      if (reconnect_)
      {
        bool ok = true;
        ok = QObject::connect(sender_, signal_, receiver_, slot_);
        YAE_ASSERT(ok);
      }
    }

    const QObject * sender_;
    const char * signal_;

    const QObject * receiver_;
    const char * slot_;

    bool reconnect_;
  };

  //----------------------------------------------------------------
  // SignalMapper
  //
  class SignalMapper : public QSignalMapper
  {
    Q_OBJECT;

  public:
    SignalMapper(QObject * parent = nullptr):
      QSignalMapper(parent)
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
      YAE_ASSERT(connect(this, SIGNAL(mapped(int)),
                         this, SIGNAL(mapped_to(int))));
#else
      YAE_ASSERT(connect(this, SIGNAL(mappedInt(int)),
                         this, SIGNAL(mapped_to(int))));
#endif
    }

  signals:
    void mapped_to(int i);
  };

  //----------------------------------------------------------------
  // swapShortcuts
  //
  inline void
  swapShortcuts(QShortcut * a, QAction * b)
  {
    QKeySequence tmp = a->key();
    a->setKey(b->shortcut());
    b->setShortcut(tmp);
  }

  //----------------------------------------------------------------
  // add
  //
  template <typename TQObj>
  inline TQObj *
  add(QObject * parent, const char * objectName)
  {
    TQObj * obj = new TQObj(parent);
    obj->setObjectName(QString::fromUtf8(objectName));
    return obj;
  }

  //----------------------------------------------------------------
  // add_menu
  //
  inline QMenu *
  add_menu(const char * objectName)
  {
    QMenu * menu = new QMenu();
    menu->setObjectName(QString::fromUtf8(objectName));
    return menu;
  }

  //----------------------------------------------------------------
  // addMenuCopyTo
  //
  inline QAction *
  addMenuCopyTo(QMenu * dst, QMenu * src)
  {
    QAction * action = src->menuAction();
    dst->addAction(action);
    return action;
  }

  //----------------------------------------------------------------
  // find_last_separator
  //
  QAction *
  find_last_separator(const QMenu & menu, QAction *& next);

  //----------------------------------------------------------------
  // MenuBreak
  //
  struct MenuBreak
  {
    MenuBreak(QAction * separator = NULL,
              QAction * next = NULL):
      separator_(separator),
      next_(next)
    {}

    QAction * separator_;
    QAction * next_;
  };

  //----------------------------------------------------------------
  // find_menu_breaks
  //
  bool
  find_menu_breaks(const QMenu & menu, std::list<MenuBreak> & breaks);

  //----------------------------------------------------------------
  // show_in_file_manager
  //
  void
  show_in_file_manager(const char * path_utf8);

  //----------------------------------------------------------------
  // get_wheel_pos
  //
  QPointF get_wheel_pos(const QWheelEvent * e);

  //----------------------------------------------------------------
  // get_wheel_delta
  //
  // compatibility wrapper for Qt4, Qt5, Qt6;
  //
  int get_wheel_delta(const QWheelEvent * e);

  //----------------------------------------------------------------
  // get_wheel_delta_degrees
  //
  // Quoting from QWheelEvent docs:
  //
  //  " Most mouse types work in steps of 15 degrees,
  //    in which case the delta value is a multiple of 120;
  //    i.e., 120 units * 1/8 = 15 degrees. "
  //
  inline double
  get_wheel_delta_degrees(const QWheelEvent * e)
  { return yae::get_wheel_delta(e) * 0.125; }

}


#endif // YAE_UTILS_QT_H_

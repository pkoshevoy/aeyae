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
#include "yae/video/yae_reader.h"

// Qt includes:
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QShortcut>
#include <QString>

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0) && \
     QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define YAE_QT4 1
#define YAE_QT5 0
#else
#define YAE_QT4 0
#define YAE_QT5 1
#endif

#if YAE_QT4
#include <QDesktopServices>
#elif YAE_QT5
#include <QStandardPaths>
#endif


//----------------------------------------------------------------
// YAE_STANDARD_LOCATION
//
#if YAE_QT4
#define YAE_STANDARD_LOCATION(x) \
  QDesktopServices::storageLocation(QDesktopServices::x)
#endif

//----------------------------------------------------------------
// YAE_STANDARD_LOCATION
//
#if YAE_QT5
#define YAE_STANDARD_LOCATION(x) \
  QStandardPaths::writableLocation(QStandardPaths::x)
#endif


namespace yae
{

  //----------------------------------------------------------------
  // kExtEyetv
  //
  extern YAEUI_API const QString kExtEyetv;

  //----------------------------------------------------------------
  // kOrganization
  //
  // default: PavelKoshevoy, except on mac it's sourceforge.net
  //
  extern YAEUI_API QString kOrganization;

  //----------------------------------------------------------------
  // kApplication
  //
  // default: ApprenticeVideo
  //
  extern YAEUI_API QString kApplication;

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
  openFile(const yae::IReaderPtr & readerPrototype,
           const QString & fn);

  //----------------------------------------------------------------
  // testEachFile
  //
  YAEUI_API bool
  testEachFile(const yae::IReaderPtr & readerPrototype,
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
  // QueuedCallEvent
  //
  struct QueuedCallEvent : public QEvent
  {
    QueuedCallEvent(): QEvent(QEvent::User) {}
    virtual void execute() = 0;
  };

  //----------------------------------------------------------------
  // QueuedCall
  //
  template <typename TObj, typename TFunc>
  struct QueuedCall : public QueuedCallEvent
  {
    QueuedCall(TObj & obj,
               TFunc TObj::* const func):
      obj_(obj),
      func_(func)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)();
    }

    TObj & obj_;
    TFunc TObj::* const func_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc>
  void
  queue_call(TObj & obj, TFunc TObj::* const func)
  {
    qApp->postEvent(&obj, new QueuedCall<TObj, TFunc>(obj, func));
  }

  //----------------------------------------------------------------
  // QueuedCallArgs1
  //
  template <typename TObj, typename TFunc, typename TArg1>
  struct QueuedCallArgs1 : public QueuedCallEvent
  {
    QueuedCallArgs1(TObj & obj,
                    TFunc TObj::* const func,
                    TArg1 arg1):
      obj_(obj),
      func_(func),
      arg1_(arg1)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)(arg1_);
    }

    TObj & obj_;
    TFunc TObj::* const func_;
    TArg1 arg1_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc, typename TArg1>
  void
  queue_call(TObj & obj,
             TFunc TObj::* const func,
             TArg1 arg1)
  {
    qApp->postEvent
      (&obj, new QueuedCallArgs1<TObj, TFunc, TArg1>(obj, func, arg1));
  }

  //----------------------------------------------------------------
  // QueuedCallArgs1
  //
  template <typename TObj, typename TFunc, typename TArg1, typename TArg2>
  struct QueuedCallArgs2 : public QueuedCallEvent
  {
    QueuedCallArgs2(TObj & obj,
                    TFunc TObj::* const func,
                    TArg1 arg1,
                    TArg2 arg2):
      obj_(obj),
      func_(func),
      arg1_(arg1),
      arg2_(arg2)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)(arg1_, arg2_);
    }

    TObj & obj_;
    TFunc TObj::* const func_;
    TArg1 arg1_;
    TArg2 arg2_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc, typename TArg1, typename TArg2>
  void
  queue_call(TObj & obj,
             TFunc TObj::* const func,
             TArg1 arg1,
             TArg2 arg2)
  {
    qApp->postEvent
      (&obj,
       new QueuedCallArgs2<TObj, TFunc, TArg1, TArg2>
       (obj, func, arg1, arg2));
  }

  //----------------------------------------------------------------
  // handle_queued_call_event
  //
  bool
  handle_queued_call_event(QEvent * event);


  //----------------------------------------------------------------
  // Application
  //
  class Application : public QApplication
  {
    Q_OBJECT;

  public:
    Application(int & argc, char ** argv);

    // virtual: overridden to propagate custom events to the parent:
    bool notify(QObject * receiver, QEvent * event);

  signals:
    // emitted when QFileOpenEvent is processed:
    void file_open(const QString & filename);

  protected:
    // virtual: overridden to hadle QFileOpenEvent:
    bool event(QEvent * event);
  };

}


#endif // YAE_UTILS_QT_H_

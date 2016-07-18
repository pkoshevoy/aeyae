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
#include <QEvent>
#include <QObject>
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
  extern YAE_API const QString kExtEyetv;

  //----------------------------------------------------------------
  // toQString
  //
  YAE_API QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace = false);

  //----------------------------------------------------------------
  // splitIntoWords
  //
  YAE_API void
  splitIntoWords(const QString & key, std::list<QString> & tokens);

  //----------------------------------------------------------------
  // toWords
  //
  YAE_API QString toWords(const QString & key);

  //----------------------------------------------------------------
  // prepareForSorting
  //
  YAE_API QString prepareForSorting(const QString & key);

  //----------------------------------------------------------------
  // overlapExists
  //
  YAE_API bool
  overlapExists(const QRect & a, const QRect & b);

  //----------------------------------------------------------------
  // overlapExists
  //
  YAE_API bool
  overlapExists(const QRect & a, const QPoint & b);

  //----------------------------------------------------------------
  // join
  //
  // returns:
  //  a if b is empty
  //  b if a is empty
  //  (a + separator + b) otherwise
  //
  YAE_API QString
  join(const QString & a, const QString & separator, const QString & b);

  //----------------------------------------------------------------
  // parseEyetvInfo
  //
  YAE_API bool
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
  YAE_API QString
  xmlEncode(const QString & text);

  //----------------------------------------------------------------
  // saveSetting
  //
  YAE_API bool
  saveSetting(const QString & key, const QString & value);

  //----------------------------------------------------------------
  // saveBooleanSetting
  //
  YAE_API bool
  saveBooleanSetting(const QString & key, bool value);

  //----------------------------------------------------------------
  // loadSetting
  //
  YAE_API bool
  loadSetting(const QString & key, QString & value);

  //----------------------------------------------------------------
  // loadSettingOrDefault
  //
  YAE_API QString
  loadSettingOrDefault(const QString & key, const QString & defaultValue);

  //----------------------------------------------------------------
  // loadBooleanSettingOrDefault
  //
  YAE_API bool
  loadBooleanSettingOrDefault(const QString & key, bool defaultValue);

  //----------------------------------------------------------------
  // removeSetting
  //
  YAE_API bool
  removeSetting(const QString & key);

  //----------------------------------------------------------------
  // findFiles
  //
  YAE_API bool
  findFiles(std::list<QString> & files,
            const QString & startHere,
            bool recursive = true);

  //----------------------------------------------------------------
  // addFolderToPlaylist
  //
  YAE_API bool
  addFolderToPlaylist(std::list<QString> & playlist, const QString & folder);

  //----------------------------------------------------------------
  // addToPlaylist
  //
  YAE_API bool
  addToPlaylist(std::list<QString> & playlist, const QString & path);

  //----------------------------------------------------------------
  // openFile
  //
  YAE_API IReaderPtr
  openFile(const yae::IReaderPtr & readerPrototype,
           const QString & fn);

  //----------------------------------------------------------------
  // testEachFile
  //
  YAE_API bool
  testEachFile(const yae::IReaderPtr & readerPrototype,
               const std::list<QString> & playlist);

  //----------------------------------------------------------------
  // toString
  //
  YAE_API std::string
  toString(QEvent::Type et);

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

}


#endif // YAE_UTILS_QT_H_

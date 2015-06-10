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

// Qt includes:
#include <QString>
#include <QPainter>

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
  // shortenTextToFit
  //
  YAE_API bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight);

  //----------------------------------------------------------------
  // drawTextToFit
  //
  YAE_API void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText = NULL);

  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  //
  YAE_API void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & bgPen,
                          bool outlineShadow = true,
                          int shadowOffset = 1,
                          QRect * bboxText = NULL);

  //----------------------------------------------------------------
  // parseEyetvInfo
  //
  YAE_API bool
  parseEyetvInfo(const QString & eyetvPath,
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

}


#endif // YAE_UTILS_QT_H_

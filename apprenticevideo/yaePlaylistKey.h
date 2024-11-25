// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Feb 12 19:36:08 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_KEY_H_
#define YAE_PLAYLIST_KEY_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_video.h"
#include "yae/utils/yae_tree.h"

// standard:
#include <memory>
#include <set>
#include <vector>

// Qt:
#include <QObject>
#include <QString>

// local:
#include "yaeBookmarks.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistKey
  //
  struct PlaylistKey
  {
    PlaylistKey(const QString & key = QString(),
                const QString & ext = QString());

    bool operator == (const PlaylistKey & k) const;
    bool operator < (const PlaylistKey & k) const;
    bool operator > (const PlaylistKey & k) const;

    QString key_;
    QString ext_;
  };

  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const std::list<PlaylistKey> & keys);

  //----------------------------------------------------------------
  // getKeyPath
  //
  bool
  getKeyPath(std::list<PlaylistKey> & keyPath, QString & path);

  //----------------------------------------------------------------
  // getKeyPathHash
  //
  std::string
  getKeyPathHash(const std::list<PlaylistKey> & keyPath);

  //----------------------------------------------------------------
  // getKeyHash
  //
  std::string
  getKeyHash(const PlaylistKey & key);

  //----------------------------------------------------------------
  // getGroupHash
  //
  std::string
  getGroupHash(const QString & filepath);

  //----------------------------------------------------------------
  // loadBookmark
  //
  bool
  loadBookmark(const std::string & filepath, TBookmark & bookmark);

}

#endif // YAE_PLAYLIST_KEY_H_

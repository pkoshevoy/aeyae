// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Feb 12 19:37:05 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <list>
#include <string>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/filesystem.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// Qt:
#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

// local:
#include "yaePlaylistKey.h"
#include "yaeUtilsQt.h"

// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistKey::PlaylistKey
  //
  PlaylistKey::PlaylistKey(const QString & key, const QString & ext):
    key_(key),
    ext_(ext)
  {}

  //----------------------------------------------------------------
  // PlaylistKey::operator
  //
  bool
  PlaylistKey::operator == (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return false;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return !diff;
  }

  //----------------------------------------------------------------
  // PlaylistKey::operator <
  //
  bool
  PlaylistKey::operator < (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return diff < 0;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return diff < 0;
  }

  //----------------------------------------------------------------
  // PlaylistKey::operator >
  //
  bool PlaylistKey::operator > (const PlaylistKey & k) const
  {
    int diff = key_.compare(k.key_, Qt::CaseInsensitive);
    if (diff)
    {
      return diff > 0;
    }

    diff = ext_.compare(k.ext_, Qt::CaseInsensitive);
    return diff > 0;
  }


  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const std::list<PlaylistKey> & keys)
  {
    std::list<QString> words;
    for (std::list<PlaylistKey>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
    {
      if (!words.empty())
      {
        // right-pointing double angle bracket:
        words.push_back(QString::fromUtf8(" ""\xc2""\xbb"" "));
      }

      const PlaylistKey & key = *i;
      splitIntoWords(key.key_, words);

      if (!key.ext_.isEmpty())
      {
        words.push_back(key.ext_);
      }
    }

    return toQString(words, true);
  }


  //----------------------------------------------------------------
  // getKeyPath
  //
  bool
  getKeyPath(std::list<PlaylistKey> & keys, QString & path)
  {
    QString humanReadablePath = path;

    QFileInfo fi(path);
    if (fi.exists())
    {
      path = fi.absoluteFilePath();
      humanReadablePath = path;
    }
    else
    {
      QUrl url;

#if YAE_QT4
      url.setEncodedUrl(path.toUtf8(), QUrl::StrictMode);
#elif YAE_QT5
      url.setUrl(path, QUrl::StrictMode);
#endif

      if (url.isValid())
      {
        humanReadablePath = url.toString();
      }
    }

    fi = QFileInfo(humanReadablePath);
    QString name = toWords(fi.completeBaseName());

    if (name.isEmpty())
    {
#if 0
      yae_debug << "IGNORING: " << i->toUtf8().constData();
#endif
      return false;
    }

    // tokenize it, convert into a tree key path:
    while (true)
    {
      QString key = fi.fileName();
      if (key.isEmpty())
      {
        break;
      }

      QFileInfo parseKey(key);
      QString base;
      QString ext;

      if (keys.empty())
      {
        base = parseKey.completeBaseName();
        ext = parseKey.suffix();
      }
      else
      {
        base = parseKey.fileName();
      }

      if (keys.empty() && ext.compare(kExtEyetv, Qt::CaseInsensitive) == 0)
      {
        // handle Eye TV archive more gracefully:
        QString chNo;
        QString chName;
        QString program;
        QString episode;
        QString timestamp;
        if (!parseEyetvInfo(path, chNo, chName, program, episode, timestamp))
        {
          break;
        }

        if (episode.isEmpty())
        {
          key = timestamp + " " + program;
        }
        else
        {
          key = timestamp + " " + episode;
        }

        if (!(chNo.isEmpty() && chName.isEmpty()))
        {
          QString ch = join(chNo, QString::fromUtf8(" "), chName);
          key += " (" + ch + ")";
        }

        keys.push_front(PlaylistKey(key, kExtEyetv));

        key = program;
        keys.push_front(PlaylistKey(key, QString()));
      }
      else
      {
        key = prepareForSorting(base);
        keys.push_front(PlaylistKey(key, ext));
      }

      QString next = fi.absolutePath();
      fi = QFileInfo(next);
    }

    return true;
  }


  //----------------------------------------------------------------
  // getKeyPathHash
  //
  std::string
  getKeyPathHash(const std::list<PlaylistKey> & keyPath)
  {
    QCryptographicHash crypto(QCryptographicHash::Sha1);
    for (std::list<PlaylistKey>::const_iterator i = keyPath.begin();
         i != keyPath.end(); ++i)
    {
      const PlaylistKey & key = *i;
      crypto.addData(key.key_.toUtf8());
      crypto.addData(key.ext_.toUtf8());
    }

    std::string groupHash(crypto.result().toHex().constData());
    return groupHash;
  }

  //----------------------------------------------------------------
  // getKeyHash
  //
  std::string
  getKeyHash(const PlaylistKey & key)
  {
    QCryptographicHash crypto(QCryptographicHash::Sha1);
    crypto.addData(key.key_.toUtf8());
    crypto.addData(key.ext_.toUtf8());

    std::string itemHash(crypto.result().toHex().constData());
    return itemHash;
  }


}

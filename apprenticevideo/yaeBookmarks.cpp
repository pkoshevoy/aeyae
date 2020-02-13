// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jul 26 21:15:04 MDT 2013
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <limits>

// Qt includes:
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// local:
#include "yaeBookmarks.h"
#include "yaePlaylistKey.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // getBookmarkHash
  //
  static QString
  getBookmarkHash(const std::string & groupHash)
  {
    std::string bookmark("bookmark-");
    bookmark += groupHash;
    return QString::fromUtf8(bookmark.c_str());
  }

  //----------------------------------------------------------------
  // kBookmarkTag
  //
  static const char * kBookmarkTag = "bookmark";

  //----------------------------------------------------------------
  // kItemTag
  //
  static const char * kItemTag = "item";

  //----------------------------------------------------------------
  // kPlayheadTag
  //
  static const char * kPlayheadTag = "playhead";

  //----------------------------------------------------------------
  // kVideoTrackTag
  //
  static const char * kVideoTrackTag = "vtrack";

  //----------------------------------------------------------------
  // kAudioTrackTag
  //
  static const char * kAudioTrackTag = "atrack";

  //----------------------------------------------------------------
  // kSubtitleTrackTag
  //
  static const char * kSubtitleTrackTag = "strack";

  //----------------------------------------------------------------
  // kClosedCaptionsTag
  //
  static const char * kClosedCaptionsTag = "cc";


  //----------------------------------------------------------------
  // saveBookmark
  //
  bool
  saveBookmark(const std::string & groupHash,
               const std::string & itemHash,
               const IReader * reader,
               const double & positionInSeconds)
  {
    if (!reader || groupHash.empty() || itemHash.empty())
    {
      return false;
    }

    // serialize:
    QString value;
    QXmlStreamWriter xml(&value);

    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(1);
    xml.writeStartDocument();

    xml.writeStartElement(kBookmarkTag);
    xml.writeAttribute(kItemTag, QString::fromUtf8(itemHash.c_str()));

    std::string hhmmss = TTime(positionInSeconds).to_hhmmss(":").c_str();
    xml.writeTextElement(kPlayheadTag, QString::fromUtf8(hhmmss.c_str()));

    QString vtrack = QString::number(reader->getSelectedVideoTrackIndex());
    xml.writeTextElement(kVideoTrackTag, vtrack);

    QString atrack = QString::number(reader->getSelectedAudioTrackIndex());
    xml.writeTextElement(kAudioTrackTag, atrack);

    std::size_t nsubs = reader->subsCount();
    for (std::size_t i = 0; i < nsubs; i++)
    {
      if (reader->getSubsRender(i))
      {
        QString strack = QString::number(i);
        xml.writeTextElement(kSubtitleTrackTag, strack);
      }
    }

    unsigned int cc = reader->getRenderCaptions();
    if (cc)
    {
      QString captions = QString::number(cc);
      xml.writeTextElement(kClosedCaptionsTag, captions);
    }

    xml.writeEndElement();
    xml.writeEndDocument();

    bool ok = saveSetting(getBookmarkHash(groupHash), value);
    return ok;
  }

  //----------------------------------------------------------------
  // getXmlAttr
  //
  static bool
  getXmlAttr(QXmlStreamAttributes & attributes,
             const char * attr,
             std::string & value)
  {
    if (attributes.hasAttribute(attr))
    {
      value = attributes.value(attr).toString().toUtf8().constData();
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // getXmlElemText
  //
  bool
  getXmlElemText(QXmlStreamReader & xml, std::string & value)
  {
    QXmlStreamReader::TokenType token = xml.readNext();
    if (token == QXmlStreamReader::Characters)
    {
      value = xml.text().toString().toUtf8().constData();
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // loadBookmark
  //
  bool
  loadBookmark(const std::string & groupHash, TBookmark & bookmark)
  {
    QString value;
    if (!loadSetting(getBookmarkHash(groupHash), value))
    {
      return false;
    }

    bookmark = TBookmark();
    bookmark.groupHash_ = groupHash;

    QXmlStreamReader xml(value);
    while (!xml.atEnd())
    {
      QXmlStreamReader::TokenType token = xml.readNext();
      if (token != QXmlStreamReader::StartElement)
      {
        continue;
      }

      std::string elemName = xml.name().toString().toUtf8().constData();
      if (elemName == kBookmarkTag)
      {
        QXmlStreamAttributes attrs = xml.attributes();

        std::string val;
        if (getXmlAttr(attrs, kItemTag, val))
        {
          bookmark.itemHash_ = val;
        }
      }
      else if (elemName == kPlayheadTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          bookmark.positionInSeconds_ = parse_hhmmss_xxx(val.c_str(), ":");
        }
      }
      else if (elemName == kVideoTrackTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          bookmark.vtrack_ = to_scalar<std::size_t, std::string>(val);
        }
      }
      else if (elemName == kAudioTrackTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          bookmark.atrack_ = to_scalar<std::size_t, std::string>(val);
        }
      }
      else if (elemName == kSubtitleTrackTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          std::size_t i = to_scalar<std::size_t, std::string>(val);
          bookmark.subs_.push_back(i);
        }
      }
      else if (elemName == kClosedCaptionsTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          bookmark.cc_ = to_scalar<unsigned int, std::string>(val);
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // removeBookmark
  //
  bool
  removeBookmark(const std::string & groupHash)
  {
    bool ok = removeSetting(getBookmarkHash(groupHash));
    return ok;
  }


  //----------------------------------------------------------------
  // find_bookmark
  //
  bool
  find_bookmark(const std::string & filepath, TBookmark & bookmark)
  {
    QString path = QString::fromUtf8(filepath.c_str());

    std::list<PlaylistKey> keys;
    getKeyPath(keys, path);

    if (keys.empty())
    {
      return false;
    }

    PlaylistKey nameKey = keys.back();
    std::string nameHash = yae::getKeyHash(nameKey);
    keys.pop_back();

    std::string groupHash = yae::getKeyPathHash(keys);

    if (!loadBookmark(groupHash, bookmark))
    {
      return false;
    }

    bool found = (bookmark.itemHash_ == nameHash);
    return found;
  }

  //----------------------------------------------------------------
  // save_bookmark
  //
  bool
  save_bookmark(const std::string & filepath,
                const IReader * reader,
                const double & positionInSeconds)
  {
    QString path = QString::fromUtf8(filepath.c_str());

    std::list<PlaylistKey> keys;
    getKeyPath(keys, path);

    if (keys.empty())
    {
      return false;
    }

    PlaylistKey nameKey = keys.back();
    std::string nameHash = yae::getKeyHash(nameKey);
    keys.pop_back();

    std::string groupHash = yae::getKeyPathHash(keys);
    return saveBookmark(groupHash, nameHash, reader, positionInSeconds);
  }

  //----------------------------------------------------------------
  // remove_bookmark
  //
  bool
  remove_bookmark(const std::string & filepath)
  {
    QString path = QString::fromUtf8(filepath.c_str());

    std::list<PlaylistKey> keys;
    getKeyPath(keys, path);

    if (keys.empty())
    {
      return false;
    }

    keys.pop_back();
    std::string groupHash = yae::getKeyPathHash(keys);
    return removeBookmark(groupHash);
  }

}

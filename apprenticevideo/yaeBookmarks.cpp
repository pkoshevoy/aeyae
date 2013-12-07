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

// yae includes:
#include <yaeBookmarks.h>
#include <yaeUtils.h>


namespace yae
{

  //----------------------------------------------------------------
  // TBookmark::TBookmark
  //
  TBookmark::TBookmark():
    atrack_(std::numeric_limits<std::size_t>::max()),
    vtrack_(std::numeric_limits<std::size_t>::max()),
    positionInSeconds_(0)
  {}

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

    xml.writeEndElement();
    xml.writeEndDocument();

    bool ok = saveSetting(QString::fromUtf8(groupHash.c_str()), value);
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
    if (!loadSetting(QString::fromUtf8(groupHash.c_str()), value))
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
          bookmark.vtrack_ = toScalar<std::size_t, std::string>(val);
        }
      }
      else if (elemName == kAudioTrackTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          bookmark.atrack_ = toScalar<std::size_t, std::string>(val);
        }
      }
      else if (elemName == kSubtitleTrackTag)
      {
        std::string val;
        if (getXmlElemText(xml, val))
        {
          std::size_t i = toScalar<std::size_t, std::string>(val);
          bookmark.subs_.push_back(i);
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
    bool ok = removeSetting(QString::fromUtf8(groupHash.c_str()));
    return ok;
  }

}

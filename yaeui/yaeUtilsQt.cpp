// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:37:50 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <set>

// system:
#ifdef _WIN32
#include <Shlobj.h>
#endif

// aeyae:
#include "yae/video/yae_video.h"

// yaeui:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaeUtilsQt.h"

// Qt:
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFile>
#include <QFileOpenEvent>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPoint>
#include <QProcess>
#include <QRect>
#include <QStringList>
#include <QSettings>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


namespace yae
{
  const QString kExtApp = QString::fromUtf8("app");
  const QString kExtBundle = QString::fromUtf8("bundle");
  const QString kExtEyetv = QString::fromUtf8("eyetv");
  const QString kExtFramework = QString::fromUtf8("framework");
  const QString kExtPlugin = QString::fromUtf8("plugin");
  const QString kExtXcodeproj = QString::fromUtf8("xcodeproj");
  const QString kApplicationSupport = QString::fromUtf8("Application Support");
  const QString kApplicationSupport2 = QString::fromUtf8("ApplicationSupport");
  const QString kDerivedData = QString::fromUtf8("DerivedData");
  const QString kMan = QString::fromUtf8("man");
  const QString kLocalStorage = QString::fromUtf8("Local Storage");
  const QString kLocalStorage2 = QString::fromUtf8("LocalStorage");
  const QString kPlugins = QString::fromUtf8("plugins");
  const QString kWebKitCache = QString::fromUtf8("WebKitCache");

#ifdef __APPLE__
  QString kOrganization = QString::fromUtf8("sourceforge.net");
  QString kApplication = QString::fromUtf8("apprenticevideo");
#else
  QString kOrganization = QString::fromUtf8("PavelKoshevoy");
  QString kApplication = QString::fromUtf8("ApprenticeVideo");
#endif


  static const QChar kUnderscore = QChar('_');
  static const QChar kHyphen = QChar('-');
  static const QChar kSpace = QChar(' ');
  static const QChar kPeriod = QChar('.');
  static const QChar kComma = QChar(',');
  static const QChar kSemicolon = QChar(';');
  static const QChar kExclamation = QChar('!');
  static const QChar kQuestionmark = QChar('?');

  //----------------------------------------------------------------
  // isPunctuation
  //
  static inline bool
  isPunctuation(const QChar & ch)
  {
    return (ch == kPeriod ||
            ch == kComma ||
            ch == kSemicolon ||
            ch == kExclamation ||
            ch == kQuestionmark);
  }

  //----------------------------------------------------------------
  // toQString
  //
  QString
  toQString(const std::list<QString> & keys, bool trimWhiteSpace)
  {
    QString path;

    for (std::list<QString>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
    {
      const QString & key = *i;

      if (i != keys.begin() && !(key.size() == 1 && isPunctuation(key[0])))
      {
        path += kSpace;
      }

      path += trimWhiteSpace ? key.trimmed() : key;
    }

    return path;
  }

  //----------------------------------------------------------------
  // isNumeric
  //
  static inline bool
  isNumeric(const QChar & c)
  {
    return c >= QChar('0') && c <= QChar('9');
  }

  //----------------------------------------------------------------
  // isNumeric
  //
  static int
  isNumeric(const QString & key, int start = 0)
  {
    const int size = key.size();
    for (int i = start; i < size; i++)
    {
      QChar c = key[i];
      if (!isNumeric(c))
      {
        return 0;
      }
    }

    return (start < size) ? (size - start) : 0;
  }

  //----------------------------------------------------------------
  // isVaguelyNumeric
  //
  static int
  isVaguelyNumeric(const QString & key, int start = 0)
  {
    int numDigits = 0;
    const int size = key.size();

    for (int i = start; i < size; i++)
    {
      QChar ch = key[i];

      if (!ch.isLetterOrNumber() && !isPunctuation(ch))
      {
        return 0;
      }

      if (isNumeric(ch))
      {
        numDigits++;
      }
    }

    return (start < size && numDigits) ? (size - start) : 0;
  }

  //----------------------------------------------------------------
  // isVersionNumber
  //
  static bool
  isVersionNumber(const QString & word, int start = 0)
  {
    if (!word.isEmpty())
    {
      QChar l0 = word[start].toLower();
      if ((l0 == QChar('v') ||
           l0 == QChar('p') ||
           l0 == QChar('#')) &&
          isNumeric(word, start + 1))
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // capitalize
  //
  static QString
  capitalize(const QString & word)
  {
    if (word.isEmpty())
    {
      YAE_ASSERT(false);
      return word;
    }

    QString out = word;

    if (isVersionNumber(word))
    {
      QChar l0 = word[0].toLower();
      out[0] = l0;
    }
    else if (word.size() > 2 && !isVaguelyNumeric(word))
    {
      QChar u0 = word[0].toUpper();
      out[0] = u0;
    }

    return out;
  }

  //----------------------------------------------------------------
  // TPostprocessToken
  //
  typedef QString(*TPostprocessToken)(const QString &);

  //----------------------------------------------------------------
  // TSplitStringIntoTokens
  //
  typedef void(*TSplitStringIntoTokens)(const QString &,
                                        std::list<QString> &,
                                        TPostprocessToken);

  //----------------------------------------------------------------
  // splitVaguelyNumericTokens
  //
  static void
  splitVaguelyNumericTokens(const QString & key,
                            std::list<QString> & tokens,
                            TPostprocessToken postprocess = NULL)
  {
    const QChar * text = key.constData();
    const int size = key.size();
    int i = 0;
    int numDigits = 0;

    for (; i < size; i++, numDigits++)
    {
      const QChar & ch = text[i];
      if (!isNumeric(ch) && !isPunctuation(ch))
      {
        break;
      }
    }

    int numOthers = 0;
    for (; i < size; i++, numOthers++)
    {
      const QChar & ch = text[i];
      if (isNumeric(ch))
      {
        break;
      }
    }

    if (numDigits && (numDigits + numOthers == size))
    {
      QString digits(text, numDigits);
      tokens.push_back(digits);

      if (numDigits < size)
      {
        QString others(text + numDigits, size - numDigits);
        tokens.push_back(others);
      }
    }
    else if (postprocess)
    {
      QString token = postprocess(key);
      tokens.push_back(token);
    }
    else
    {
      tokens.push_back(key);
    }
  }

  //----------------------------------------------------------------
  // splitOnCamelCase
  //
  static void
  splitOnCamelCase(const QString & key,
                   std::list<QString> & tokens,
                   TPostprocessToken postprocess = NULL)
  {
    // attempt to split on camel case:
    QString token;

    QChar c0 = key[0];
    token += c0;

    const int size = key.size();
    for (int i = 1; i < size; i++)
    {
      QChar c1 = key[i];

      if (c0.isLetter() && c1.isLetter() && c0.isLower() && !c1.isLower())
      {
        splitVaguelyNumericTokens(token, tokens, postprocess);
        token = QString();
      }
      else if (isNumeric(token) && !isNumeric(c1) &&
               (c1.isNumber() || isVersionNumber(key, i)))
      {
        tokens.push_back(token);
        token = QString();
      }

      token += c1;
      c0 = c1;
    }

    if (!token.isEmpty())
    {
      splitVaguelyNumericTokens(token, tokens, postprocess);
    }
  }

  //----------------------------------------------------------------
  // kSeparatorTags
  //
  static const QChar kSeparatorTags[] = {
    QChar(' '),
    QChar('.'),
    QChar('-')
  };

  //----------------------------------------------------------------
  // kSeparatorTagsNum
  //
  static const std::size_t kSeparatorTagsNum =
    sizeof(kSeparatorTags) / sizeof(kSeparatorTags[0]);

  //----------------------------------------------------------------
  // indexOfSeparatorTag
  //
  std::size_t
  indexOfSeparatorTag(const QChar & ch)
  {
    for (std::size_t i = 0; i < kSeparatorTagsNum; ++i)
    {
      if (kSeparatorTags[i] == ch)
      {
        return i;
      }
    }

    return kSeparatorTagsNum;
  }

  //----------------------------------------------------------------
  // splitOnSeparators
  //
  static void
  splitOnSeparators(const QString & key,
                    std::list<QString> & tokens,
                    TPostprocessToken postprocess = NULL)
  {
    // first replace all underscores with spaces:
    QString cleanKey = key;
    const int size = cleanKey.size();

    for (int i = 0; i < size; i++)
    {
      QChar ch = cleanKey[i];
      if (ch == kUnderscore)
      {
        cleanKey[i] = kSpace;
      }
    }

    // split based on separator character:
    QString token;
    QString prev;
    for (int i = 0; i < size; i++)
    {
      QChar ch = cleanKey[i];
      QChar ch1 = ((i + 1) < size) ? cleanKey[i + 1] : QChar(0);
      QChar ch2 = ((i + 2) < size) ? cleanKey[i + 2] : QChar(0);

      std::size_t foundSeparatorTag = indexOfSeparatorTag(ch);
      if (foundSeparatorTag < kSeparatorTagsNum &&

          (ch != kPeriod ||
           (// avoid breaking up ellipsis and numbered list punctuation:
            ch1 != kPeriod &&
            ch1 != kSpace &&
            (ch1 != 0 || !token.endsWith(kPeriod)) &&
            // avoid breaking up YYYY.MM.DD dates:
            !(isNumeric(prev) && isNumeric(ch1)) &&
            // avoid breaking up A.B.C initials:
            !(prev.size() == 1 && prev[0].isLetter() &&
              ch1.isLetter() && (ch2 == kPeriod || ch2 == kSpace)))))
      {
        if (!token.isEmpty())
        {
          splitOnCamelCase(token, tokens, postprocess);
        }

        token = QString();
        prev = QString();

        if (ch == kHyphen &&
            (// preserve proper hyphenation:
             ch1 == kHyphen ||
             ch1 == kSpace) &&
            ch1 != 0)
        {
          if (!tokens.empty() && tokens.back().endsWith(kHyphen))
          {
            tokens.back() += QString(kHyphen);
          }
          else
          {
            tokens.push_back(QString(kHyphen));
          }
        }
      }
      else
      {
        token += ch;

        if (foundSeparatorTag < kSeparatorTagsNum)
        {
          prev = QString();
        }
        else
        {
          prev += QString(ch);
        }
      }
    }

    if (!token.isEmpty())
    {
      splitOnCamelCase(token, tokens, postprocess);
    }
  }

  //----------------------------------------------------------------
  // kGroupTags
  //
  static const QChar kGroupTags[][2] = {
    { QChar('<'), QChar('>') },
    { QChar('['), QChar(']') },
    { QChar('{'), QChar('}') },
    { QChar('('), QChar(')') },
    { QChar('"'), QChar('"') }
  };

  //----------------------------------------------------------------
  // kGroupTagsNum
  //
  static const std::size_t kGroupTagsNum =
    sizeof(kGroupTags) / sizeof(kGroupTags[0]);

  //----------------------------------------------------------------
  // indexOfGroupTag
  //
  std::size_t
  indexOfGroupTag(const QChar & ch, std::size_t columnIndex)
  {
    for (std::size_t i = 0; i < kGroupTagsNum; ++i)
    {
      if (kGroupTags[i][columnIndex] == ch)
      {
        return i;
      }
    }

    return kGroupTagsNum;
  }

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  static bool
  splitOnGroupTags(const QString & key,
                   int & keyPos,
                   const int keySize,
                   const QChar & closingTag,
                   std::list<QString> & tokens,
                   TSplitStringIntoTokens splitFurther = NULL,
                   TPostprocessToken postprocess = NULL)
  {
    // attempt to split on nested open/close tags:
    QString token;
    QChar ch;
    bool foundClosingTag = false;

    while (keyPos < keySize && !foundClosingTag)
    {
      ch = key[keyPos];
      ++keyPos;

      if (ch == closingTag)
      {
        foundClosingTag = true;
        break;
      }

      std::size_t foundNestedTag = indexOfGroupTag(ch, 0);
      if (foundNestedTag < kGroupTagsNum)
      {
        std::list<QString> nestedTokens;
        if (splitOnGroupTags(key,
                             keyPos,
                             keySize,
                             kGroupTags[foundNestedTag][1],
                             nestedTokens,
                             splitFurther,
                             postprocess))
        {
          if (!token.isEmpty())
          {
            if (splitFurther)
            {
              std::list<QString> furtherTokens;
              splitFurther(token, furtherTokens, postprocess);
              tokens.splice(tokens.end(), furtherTokens);
            }
            else
            {
              tokens.push_back(token);
            }
          }

          token = QString(ch);
          token += nestedTokens.front();
          nestedTokens.pop_front();

          tokens.push_back(token);
          tokens.splice(tokens.end(), nestedTokens);

          token = QString();
        }
        else
        {
          token += ch;

          if (!nestedTokens.empty())
          {
            token += nestedTokens.front();
            nestedTokens.pop_front();

            if (splitFurther)
            {
              std::list<QString> furtherTokens;
              splitFurther(token, furtherTokens, postprocess);
              tokens.splice(tokens.end(), furtherTokens);
            }
            else
            {
              tokens.push_back(token);
            }

            tokens.splice(tokens.end(), nestedTokens);

            token = QString();
          }
        }
      }
      else
      {
        token += ch;
      }
    }

    if (!token.isEmpty())
    {
      if (splitFurther)
      {
        std::list<QString> furtherTokens;
        splitFurther(token, furtherTokens, postprocess);
        tokens.splice(tokens.end(), furtherTokens);
      }
      else
      {
        tokens.push_back(token);
      }
    }

    if (foundClosingTag && ch != 0)
    {
      if (!tokens.empty())
      {
        tokens.back() += QString(ch);
      }
      else
      {
        tokens.push_back(QString(ch));
      }
    }

    return foundClosingTag;
  }

  //----------------------------------------------------------------
  // splitOnGroupTags
  //
  static void
  splitOnGroupTags(const QString & key,
                   std::list<QString> & tokens,
                   TSplitStringIntoTokens splitFurther = NULL,
                   TPostprocessToken postprocess = NULL)
  {
    int keyPos = 0;
    const int keySize = key.size();
    const QChar closingTag = QChar(0);
    splitOnGroupTags(key,
                     keyPos,
                     keySize,
                     closingTag,
                     tokens,
                     splitFurther,
                     postprocess);
  }

  //----------------------------------------------------------------
  // splitIntoWords
  //
  static void
  splitIntoWords(const QString & key,
                 std::list<QString> & words,
                 TPostprocessToken postprocess)
  {
    splitOnGroupTags(key, words, &splitOnSeparators, postprocess);
  }

  //----------------------------------------------------------------
  // splitIntoWords
  //
  void
  splitIntoWords(const QString & key, std::list<QString> & words)
  {
    splitOnGroupTags(key, words, &splitOnSeparators, &capitalize);
  }

  //----------------------------------------------------------------
  // toWords
  //
  QString
  toWords(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words, NULL);
    return toQString(words, false);
  }

  //----------------------------------------------------------------
  // prepareForSorting
  //
  QString
  prepareForSorting(const QString & key)
  {
    std::list<QString> words;
    splitIntoWords(key, words, &capitalize);

    int nwords = 0;
    QString out;
    QString prev;

    for (std::list<QString>::const_iterator i = words.begin();
         i != words.end(); ++i)
    {
      QString word = *i;

      if (word.size() == 1 && word[0] == kHyphen)
      {
        if (nwords == 1 && isNumeric(prev))
        {
          // hyde numeric list separator hyphen:
          continue;
        }

        std::list<QString>::const_iterator j = i;
        ++j;
        if (j != words.end())
        {
          const QString & next = *j;
          if (isVaguelyNumeric(next))
          {
            // hyde numeric list separator hyphen:
            continue;
          }
        }
      }

      if (!out.isEmpty() && !(word.size() == 1 && isPunctuation(word[0])))
      {
        out += kSpace;
      }

      // if the string is all numerical then pad it on the front so that
      // it would be properly sorted (2.avi would be before 10.avi)
      int numDigits = isNumeric(word);

      if (numDigits && numDigits < 8)
      {
        QString padding(8 - numDigits, kSpace);
        out += padding;
      }

      out += word;
      nwords++;

      prev = word;
    }

    return out;
  }

  //----------------------------------------------------------------
  // overlapExists
  //
  bool
  overlapExists(const QRect & a, const QRect & b)
  {
    if (a.height() && a.width() &&
        b.height() && b.width())
    {
      return a.intersects(b);
    }

    // ignore overlap with an empty region:
    return false;
  }

  //----------------------------------------------------------------
  // overlapExists
  //
  bool
  overlapExists(const QRect & a, const QPoint & b)
  {
    if (a.height() && a.width())
    {
      return a.contains(b);
    }

    // ignore overlap with an empty region:
    return false;
  }

  //----------------------------------------------------------------
  // readNextXmlLineAndStripNullBytes
  //
  inline static bool
  readNextXmlLineAndStripNullBytes(QTextStream & xml,
                                   QXmlStreamReader & parser)
  {
    if (xml.atEnd())
    {
      return false;
    }

    QString line = xml.readLine();

    // try to work around a real-life example of malformed XML
    // produced by EyeTV:
    //
    // Agatha Christie's Poirot - Yellow Iris.eyetv/000000001b11d82d.eyetvr
    //
    line.remove(QChar(0));
    parser.addData(line);

    return true;
  }

  //----------------------------------------------------------------
  // readNextValidToken
  //
  static QXmlStreamReader::TokenType
  readNextValidToken(QTextStream & xml, QXmlStreamReader & parser)
  {
    QXmlStreamReader::TokenType tt = parser.tokenType();

    for (int i = 0; i < 10; i++)
    {
      parser.readNext();
      tt = parser.tokenType();

      if (tt != QXmlStreamReader::NoToken &&
          tt != QXmlStreamReader::Invalid)
      {
        break;
      }

      if (parser.atEnd())
      {
        QXmlStreamReader::Error xerr = parser.error();

        if (xerr == QXmlStreamReader::NoError)
        {
          // EOF:
          break;
        }

        if (xerr == QXmlStreamReader::PrematureEndOfDocumentError &&
            !readNextXmlLineAndStripNullBytes(xml, parser))
        {
          // EOF, probably malformed:
          break;
        }
      }
    }

    return tt;
  }

  //----------------------------------------------------------------
  // parseXmlTag
  //
  static bool
  parseXmlTag(QTextStream & xml,
              QXmlStreamReader & parser,
              std::string & name,
              QString & value)
  {
    name.clear();
    value.clear();

    QXmlStreamReader::TokenType tt = parser.tokenType();
    while (!parser.atEnd())
    {
      if (tt == QXmlStreamReader::StartElement)
      {
        name = parser.name().toString().toLower().toUtf8().constData();

        tt = readNextValidToken(xml, parser);
        if (tt == QXmlStreamReader::StartElement)
        {
          return false;
        }

        if (tt == QXmlStreamReader::Characters)
        {
          value = parser.text().toString();
          tt = readNextValidToken(xml, parser);
        }

        if (tt == QXmlStreamReader::StartElement)
        {
          bool emptyValue = value.trimmed().isEmpty();
          YAE_ASSERT(emptyValue);
          value.clear();
          return false;
        }

        if (tt != QXmlStreamReader::EndElement)
        {
          YAE_ASSERT(false);
          value.clear();
          return false;
        }

        tt = readNextValidToken(xml, parser);
        if (tt == QXmlStreamReader::Characters)
        {
          std::string t =
            parser.text().toString().trimmed().toUtf8().constData();
          YAE_ASSERT(t.empty());
          readNextValidToken(xml, parser);
        }

        return true;
      }

      tt = readNextValidToken(xml, parser);
    }

    return false;
  }

  //----------------------------------------------------------------
  // same
  //
  static bool
  same(const std::list<std::string> & nodePath,
       const char * testPath)
  {
    const char * startHere = testPath;

    for (std::list<std::string>::const_iterator i = nodePath.begin();
         i != nodePath.end(); )
    {
      const std::string & node = *i;
      std::size_t size = node.size();
      if (strncmp(&node[0], startHere, size) != 0)
      {
        return false;
      }

      // skip to the next node:
      startHere += size;
      ++i;

      if (i == nodePath.end())
      {
        break;
      }
      else if (startHere[0] != '/')
      {
        return false;
      }

      // skip the '/' path separator:
      startHere++;
    }

    return startHere && startHere[0] == '\0';
  }

  //----------------------------------------------------------------
  // join
  //
  QString
  join(const QString & a, const QString & separator, const QString & b)
  {
    return (a.isEmpty() ? b :
            b.isEmpty() ? a :
            a + separator + b);
  }

  //----------------------------------------------------------------
  // parseEyetvInfo
  //
  bool
  parseEyetvInfo(const QString & eyetvPath,
                 QString & channelNumber,
                 QString & channelName,
                 QString & program,
                 QString & episode,
                 QString & timestamp)
  {
    static const QString kExtEyetvR = QString::fromUtf8("eyetvr");

    QStringList extFilters;
    QDirIterator iter(eyetvPath,
                      extFilters,
                      QDir::NoDotAndDotDot |
                      QDir::AllEntries |
                      QDir::Readable,
                      QDirIterator::FollowSymlinks);

    while (iter.hasNext())
    {
      iter.next();

      QFileInfo fi = iter.fileInfo();
      QString fn = fi.absoluteFilePath();
      QString ext = fi.suffix();

      if (ext == kExtEyetvR)
      {
        QFile xmlFile(fn);
        if (!xmlFile.open(QIODevice::ReadOnly))
        {
          return false;
        }

        QTextStream xml(&xmlFile);

        std::string filename = fn.toUtf8().constData();
        std::string name;
        QString value;
        std::list<std::string> nodePath;

        QXmlStreamReader parser;
        bool done = false;

        while (!done && readNextXmlLineAndStripNullBytes(xml, parser))
        {
          while (!done && !parser.atEnd())
          {
            bool nameValue = parseXmlTag(xml, parser, name, value);
            if (!nameValue)
            {
              if (name.empty())
              {
                break;
              }

              nodePath.push_back(name);
            }

            if (nameValue && name == "key")
            {
              std::string v = value.toLower().toUtf8().constData();

              if (v == "channel number" &&
                  same(nodePath, "plist/dict"))
              {
                if (!parseXmlTag(xml, parser, name, value))
                {
                  YAE_ASSERT(false);
                  return false;
                }

                value.replace('-', '.');
                channelNumber = value;
              }
              else if (v == "channel name" &&
                  same(nodePath, "plist/dict"))
              {
                if (!parseXmlTag(xml, parser, name, value))
                {
                  YAE_ASSERT(false);
                  return false;
                }

                channelName = value;
              }
              else if (v == "recording title" &&
                       same(nodePath, "plist/dict/dict"))
              {
                if (!parseXmlTag(xml, parser, name, value))
                {
                  YAE_ASSERT(false);
                  return false;
                }

                program = value;
              }
              else if (v == "episode title" &&
                       same(nodePath, "plist/dict/dict"))
              {
                if (!parseXmlTag(xml, parser, name, value))
                {
                  YAE_ASSERT(false);
                  return false;
                }

                episode = value;
              }
              else if (v == "start" && same(nodePath, "plist/dict/dict"))
              {
                if (!parseXmlTag(xml, parser, name, value))
                {
                  YAE_ASSERT(false);
                  return false;
                }

                QDateTime t = QDateTime::fromString(value, Qt::ISODate);
                timestamp = t.toLocalTime().toString("yyyyMMdd hhmm");
              }
            }

            QXmlStreamReader::TokenType tt = parser.tokenType();
            if (tt == QXmlStreamReader::EndElement)
            {
              const std::string & top = nodePath.back();
              name = parser.name().toString().toLower().toUtf8().constData();
              if (top != name)
              {
                YAE_ASSERT(false);
                return false;
              }

              nodePath.pop_back();
            }

            done = !(program.isEmpty() ||
                     episode.isEmpty() ||
                     timestamp.isEmpty());
          }
        }
      }
    }

    bool done = ((!program.isEmpty() ||
                  !episode.isEmpty()) &&
                 !timestamp.isEmpty());
    return done;
  }

  //----------------------------------------------------------------
  // xmlEncode
  //
  QString
  xmlEncode(const QString & text)
  {
    QString output;
    {
      QXmlStreamWriter stream(&output);
      stream.writeCharacters(text);
    }

    return output;
  }

  //----------------------------------------------------------------
  // saveSetting
  //
  bool
  saveSetting(const QString & key, const QString & value)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    settings.setValue(key, value);

    bool ok = (settings.status() == QSettings::NoError);
    return ok;
  }

  //----------------------------------------------------------------
  // kSettingTrue
  //
  static const QString kSettingTrue = QString::fromUtf8("true");

  //----------------------------------------------------------------
  // kSettingFalse
  //
  static const QString kSettingFalse = QString::fromUtf8("false");

  //----------------------------------------------------------------
  // saveBooleanSetting
  //
  bool
  saveBooleanSetting(const QString & key, bool value)
  {
    const QString & textValue = value ? kSettingTrue : kSettingFalse;
    return saveSetting(key, textValue);
  }

  //----------------------------------------------------------------
  // loadSetting
  //
  bool
  loadSetting(const QString & key, QString & value)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    if (!settings.contains(key))
    {
      return false;
    }

    value = settings.value(key).toString();
    return true;
  }

  //----------------------------------------------------------------
  // loadSettingOrDefault
  //
  QString
  loadSettingOrDefault(const QString & key, const QString & defaultValue)
  {
    QString value;
    if (loadSetting(key, value))
    {
      return value;
    }

    return defaultValue;
  }

  //----------------------------------------------------------------
  // loadBooleanSettingOrDefault
  //
  bool
  loadBooleanSettingOrDefault(const QString & key, bool defaultValue)
  {
    QString textValue;
    if (loadSetting(key, textValue))
    {
      bool value = (textValue == kSettingTrue);
      return value;
    }

    return defaultValue;
  }

  //----------------------------------------------------------------
  // removeSetting
  //
  bool
  removeSetting(const QString & key)
  {
    QSettings settings(QSettings::NativeFormat,
                       QSettings::UserScope,
                       kOrganization,
                       kApplication);

    settings.remove(key);

    bool ok = (settings.status() == QSettings::NoError);
    return ok;
  }

  //----------------------------------------------------------------
  // TExtIgnoreList
  //
  struct TExtIgnoreList
  {
    TExtIgnoreList()
    {
      const char * ext[] = {
        "eyetvsched",
        "eyetvp",
        "eyetvr",
        "eyetvi",
        "eyetvsl",
        "eyetvsg",
        "pages",
        "odp",
        "doc",
        "metadata",
        "pset",
        "sbstore",
        "xls",
        "xlsx",
        "ppt",
        "pptx",
        "pdf",
        "rtf",
        "htm",
        "css",
        "less",
        "rar",
        "jar",
        "hqx",
        "zip",
        "7z",
        "gz",
        "bz2",
        "sol",
        "war",
        "webhistory",
        "help",
        "helpindex",
        "machelp",
        "searchindexcache",
        "usercache",
        "lock",
        "lockfile",
        "storedata",
        "storedata-shm",
        "storedata-wal",
        "aclcddb",
        "aclcddb-shm",
        "aclcddb-wal",
        "abcdi",
        "abcdg",
        "abcdp",
        "abcddb",
        "abcddb-shm",
        "abcddb-wal",
        "collection",
        "keychain-db",
        "fsck",
        "sfl",
        "clr",
        "tmp",
        "tar",
        "tgz",
        "tbz2",
        "lzma",
        "url",
        "eml",
        "sgml",
        "html",
        "xsd",
        "xsl",
        "xslt",
        "xml",
        "yml",
        "wml",
        "mml",
        "aml",
        "mmp",
        "installhelper",
        "dtd",
        "tdt",
        "stg",
        "bak",
        "bat",
        "bats",
        "ini",
        "idl",
        "cfg",
        "cnf",
        "csv",
        "rdp",
        "el",
        "elc",
        "lisp",
        "ftp",
        "rb",
        "re",
        "rexpr",
        "cs",
        "java",
        "php",
        "map",
        "js",
        "md",
        "pl",
        "db",
        "db-shm",
        "db-wal",
        "db-lock",
        "tex",
        "texi",
        "txt",
        "text",
        "seen",
        "srt",
        "ass",
        "ssa",
        "idx",
        "sub",
        "sup",
        "ifo",
        "info",
        "info-1",
        "info-2",
        "nfo",
        "inf",
        "rsa",
        "md5",
        "sha1",
        "crc",
        "sfv",
        "m3u",
        "smil",
        "app",
        "tag",
        "tags",
        "index",
        "strings",
        "plist",
        "framework",
        "bundle",
        "rcproject",
        "ipmeta",
        "quickbook",
        "reno",
        "qtx",
        "qtr",
        "sc",
        "so",
        "lo",
        "la",
        "dylib",
        "dll",
        "dlls",
        "ax",
        "def",
        "lib",
        "a",
        "r",
        "t",
        "y",
        "o",
        "d",
        "s",
        "v",
        "v2",
        "w",
        "obj",
        "am",
        "in",
        "impl",
        "exe",
        "com",
        "cmd",
        "cab",
        "dat",
        "bat",
        "sys",
        "msi",
        "iss",
        "ism",
        "rul",
        "pickle",
        "pyste",
        "py",
        "pyc",
        "gypi",
        "order",
        "raw",
        "sh",
        "bash",
        "csh",
        "fsh",
        "vsh",
        "vert",
        "frag",
        "cg",
        "glsl",
        "comp",
        "fig",
        "dae",
        "command",
        "kms",
        "m4",
        "debug",
        "release",
        "pthreads-win32",
        "exp",
        "flex",
        "f90",
        "cpp",
        "cppx",
        "hpp",
        "tpp",
        "ipp",
        "SUNWCCh",
        "inc",
        "inv",
        "pch",
        "sed",
        "awk",
        "h",
        "hh",
        "m",
        "mm",
        "mms",
        "manx",
        "c",
        "cc",
        "cu",
        "yy",
        "ui",
        "mo",
        "po",
        "pm",
        "qm",
        "as",
        "asm",
        "rc",
        "qrc",
        "qs",
        "qss",
        "qch",
        "qhp",
        "qml",
        "qmlc",
        "qmlproject",
        "qmltypes",
        "qmodel",
        "sci",
        "scxml",
        "schematic",
        "layout",
        "tpl",
        "nim",
        "nims",
        "cbp",
        "dia",
        "mdla",
        "mdlap",
        "cmds",
        "cxx",
        "hxx",
        "txx",
        "log",
        "err",
        "out",
        "sqz",
        "xss",
        "xds",
        "xsp",
        "xcp",
        "xfs",
        "spfx",
        "iso",
        "pem",
        "p12",
        "pvk",
        "crt",
        "cer",
        "pfx",
        "spc",
        "pc",
        "td",
        "pkg",
        "dmg",
        "dmp",
        "svq",
        "svn",
        "svg",
        "itdb",
        "itl",
        "itc",
        "ipa",
        "vbox",
        "vdi",
        "vmdk",
        "sln",
        "suo",
        "manifest",
        "settings",
        "resx",
        "vdproj",
        "vcproj",
        "vcxproj",
        "vsprops",
        "filters",
        "csproj",
        "mode1v3",
        "pbxuser",
        "pbxproj",
        "pmproj",
        "proj",
        "rsrc",
        "nib",
        "xib",
        "icns",
        "ics",
        "icsalarm",
        "user",
        "lnk",
        "cd",
        "cw",
        "amz",
        "mcp",
        "pro",
        "pri",
        "prf",
        "prl",
        "flm",
        "applite",
        "ac",
        "mk",
        "mak",
        "make",
        "cmake",
        "dxy",
        "dox",
        "doxy",
        "doxygen",
        "doxyfile",
        "dsp",
        "dsw",
        "prj",
        "sic",
        "cvs",
        "inl",
        "guess",
        "amiga",
        "bcb3",
        "changes",
        "contributors",
        "cygming",
        "cygwin",
        "darwin",
        "dgux386",
        "environment",
        "ews4800",
        "freebsd",
        "gnu",
        "hp",
        "irix",
        "os2",
        "rs6000",
        "sgi",
        "uts",
        "mac",
        "macosx",
        "macros",
        "cords",
        "dj",
        "man",
        "spec",
        "kfreebsd",
        "linux",
        "mingw",
        "mingwdll",
        "solaris",
        "solaris2",
        "os4",
        "qnx",
        "unix",
        "vms",
        "h-vms",
        "sas",
        "ansi",
        "wat",
        "vcwin32",
        "win32",
        "win32-g++",
        "win32-g++sh",
        "win64",
        "cross",
        "graph",
        "graffle",
        "gr",
        "net",
        "tcl",
        "input",
        "output",
        "expect",
        "expected",
        "internal",
        "marks",
        "cpp_parameters",
        "check_cache",
        "depends",
        "includecache",
        "props",
        "pattern",
        "pattern2",
        "patch",
        "pat",
        "dot",
        "dif",
        "diff",
        "corpus",
        "readme",
        "config",
        "contrib",
        "autoconf",
        "examples",
        "footprint",
        "packaging",
        "install",
        "table",
        "scratchbox",
        "pod",
        "st",
        "sty",
        "any",
        "bcc",
        "vc",
        "mc",
        "mc6",
        "vc6",
        "vc9",
        "opt",
        "docbook",
        "doctree",
        "dtdxml",
        "toyxml",
        "gccxml",
        "gch",
        "gold",
        "mini",
        "cnj",
        "plg",
        "lst",
        "lsm",
        "asx",
        "eot",
        "ent",
        "enc",
        "extra",
        "woff",
        "hdf",
        "bdf",
        "otf",
        "ttf",
        "xcf",
        "xcscheme",
        "xcsettings",
        "xcuserstate",
        "xcworkspacedata",
        "hmap",
        "linkfilelist",
        "qph",
        "qpf",
        "pfa",
        "pfb",
        "fon",
        "key",
        "license",
        "ignore",
        "itc",
        "lz4",
        "rdf",
        "mozlz4",
        "json",
        "jsonlz4",
        "modulemap",
        "conf",
        "yae",
        "mb",
        "store",
        "fingerprint",
        "pb",
        "kb",
        "sig",
        "hbqueue",
        "wkt",
        "sublime-project",
        "sql",
        "sqlite",
        "sqlite-shm",
        "sqlite-wal",
        "sqlitedb",
        "sqlitedb-shm",
        "sqlitedb-wal",
        "sqlite3",
        "sqlite3-shm",
        "sqlite3-wal",
        "sqlite3-journal",
        "migrated-shm",
        "migrated-wal",
        "archive",
        "ldb",
        "albumlistmetadata",
        "foldermetadata",
        "ithmb",
        "host",
        "quick",
        "direct",
        "directory",
        "plugin",
        "dimacs",
        "dict",
        "old",
        "new",
        "bin",
        "rsp",
        "rst",
        "boostbook",
        "toc",
        "agr",
        "jam",
        "sunwcch",
        "gci",
        "qbk",
        "qbs",
        "qdoc",
        "qdocinc",
        "qdocconf",
        "verbatim",
        "ver",
        "version",
        "status",
        "fate",
        "ffpreset",
        "desktop",
        "cache",
        "lstm",
        "dist",
        "pas",
        "dpr",
        "xq",
        "xbel",
        "gperf",
        "re2js",
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
      };

      const std::size_t nExt = sizeof(ext) / sizeof(const char *);
      for (std::size_t i = 0; i < nExt; i++)
      {
        set_.insert(QString::fromUtf8(ext[i]));
      }
    }

    bool contains(const QString & ext) const
    {
      std::set<QString>::const_iterator found = set_.find(ext);
      return found != set_.end();
    }

  protected:
    std::set<QString> set_;
  };

  //----------------------------------------------------------------
  // kExtIgnore
  //
  static const TExtIgnoreList kExtIgnore;

  //----------------------------------------------------------------
  // shouldIgnore
  //
  static bool shouldIgnore(const QString & ext)
  {
    QString extLowered = ext.toLower();
    return
      extLowered.isEmpty() ||
      extLowered.endsWith("~") ||
      kExtIgnore.contains(extLowered);
  }

  //----------------------------------------------------------------
  // shouldIgnore
  //
  static bool
  shouldIgnore(const QString & fn, const QString & ext, QFileInfo & fi)
  {
    if (fi.isDir())
    {
      QString extLowered = ext.toLower();
      if (extLowered == QString::fromUtf8("eyetvsched"))
      {
        return true;
      }

      if (ext == kExtApp ||
          ext == kExtBundle ||
          ext == kExtFramework ||
          ext == kExtPlugin ||
          ext == kExtXcodeproj ||
          fn == kApplicationSupport ||
          fn == kApplicationSupport2 ||
          fn == kDerivedData ||
          fn == kLocalStorage ||
          fn == kLocalStorage2 ||
          fn == kMan ||
          fn == kPlugins ||
          fn == kWebKitCache)
      {
        return true;
      }

      return false;
    }

    if (fn.size() > 1 && fn[0] == '.' && fn[1] != '.')
    {
      // ignore dot files:
      return true;
    }

    return shouldIgnore(ext);
  }

  //----------------------------------------------------------------
  // findFiles
  //
  bool
  findFiles(std::list<QString> & files,
            const QString & startHere,
            bool recursive)
  {
    QStringList extFilters;
    if (QFileInfo(startHere).suffix() == kExtEyetv)
    {
      extFilters << QString::fromUtf8("*.mpg");
    }

    QDirIterator iter(startHere,
                      extFilters,
                      QDir::NoDotAndDotDot |
                      QDir::AllEntries |
                      QDir::Readable,
                      QDirIterator::FollowSymlinks);

    bool found = false;
    while (iter.hasNext())
    {
      iter.next();

      QFileInfo fi = iter.fileInfo();
      QString fullpath = fi.absoluteFilePath();
      QString filename = fi.fileName();
      QString ext = fi.suffix();

      if (!shouldIgnore(filename, ext, fi))
      {
        if (fi.isDir() && ext != kExtEyetv)
        {
          if (recursive)
          {
            if (findFiles(files, fullpath, recursive))
            {
              found = true;
            }
          }
        }
        else
        {
          files.push_back(fullpath);
          found = true;
        }
      }
    }

    return found;
  }

  //----------------------------------------------------------------
  // findFilesAndSort
  //
  static bool
  findFilesAndSort(std::list<QString> & files,
                   const QString & startHere,
                   bool recursive = true)
  {
    if (findFiles(files, startHere, recursive))
    {
      files.sort();
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // addFolderToPlaylist
  //
  bool
  addFolderToPlaylist(std::list<QString> & playlist, const QString & folder)
  {
    if (folder.isEmpty())
    {
      return false;
    }

    QFileInfo fi(folder);
    QString filename = fi.fileName();
    QString ext = fi.suffix();

    // find all files in the folder, sorted alphabetically
    if (!shouldIgnore(fi.fileName(), ext, fi))
    {
      if (fi.isDir() && ext != kExtEyetv)
      {
        return findFilesAndSort(playlist, folder, true);
      }

      playlist.push_back(folder);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // addToPlaylist
  //
  bool
  addToPlaylist(std::list<QString> & playlist, const QString & path)
  {
    QFileInfo fi(path);
    if (fi.exists() && !fi.isReadable())
    {
      return false;
    }

    QString filename = fi.fileName();
    QString ext = fi.suffix();
    if (shouldIgnore(filename, ext, fi))
    {
      return false;
    }

    if (fi.isDir() && ext != kExtEyetv)
    {
      return addFolderToPlaylist(playlist, path);
    }

    playlist.push_back(path);
    return true;
  }

  //----------------------------------------------------------------
  // kNormalizationForm
  //
  static const QString::NormalizationForm kNormalizationForm[] =
  {
    QString::NormalizationForm_D,
    QString::NormalizationForm_C,
    QString::NormalizationForm_KD,
    QString::NormalizationForm_KC
  };

  //----------------------------------------------------------------
  // kNumNormalizationForms
  //
  static const std::size_t kNumNormalizationForms =
    sizeof(kNormalizationForm) / sizeof(kNormalizationForm[0]);

  //----------------------------------------------------------------
  // convert_path_to_utf8
  //
  bool
  convert_path_to_utf8(const QString & path, std::string & path_utf8)
  {
    const bool starts_with_file = path.startsWith("file://");

    for (std::size_t i = 0; i < kNumNormalizationForms; i++)
    {
      // find UNICODE NORMALIZATION FORM that works
      // http://www.unicode.org/reports/tr15/
      QString tmp = path.normalized(kNormalizationForm[i]);
      path_utf8 = tmp.toUtf8().constData();

      try
      {
        std::string fn = starts_with_file ? path_utf8.substr(7) : path_utf8;

        if (TOpenFile(fn.c_str(), "rb").is_open())
        {
          return true;
        }
      }
      catch (...)
      {}
    }

    YAE_ASSERT(false);
    return false;
  }

  //----------------------------------------------------------------
  // openFile
  //
  IReaderPtr
  openFile(const yae::TReaderFactoryPtr & readerFactory,
           const QString & path,
           bool hwdec)
  {
    QString fn = path;
    QFileInfo fi(fn);

    if (fi.suffix() == kExtEyetv)
    {
      std::list<QString> found;
      findFiles(found, path, false);

      if (!found.empty())
      {
        fn = found.front();
      }
    }

    std::string filepath;
    if (convert_path_to_utf8(fn, filepath))
    {
      IReaderPtr reader = readerFactory->create(filepath);
      if (reader->open(filepath.c_str(), hwdec))
      {
        return reader;
      }
    }

    return IReaderPtr();
  }

  //----------------------------------------------------------------
  // testEachFile
  //
  bool
  testEachFile(const yae::TReaderFactoryPtr & readerFactory,
               const std::list<QString> & playlist)
  {
    std::size_t numOpened = 0;
    std::size_t numTotal = 0;

    for (std::list<QString>::const_iterator j = playlist.begin();
         j != playlist.end(); ++j)
    {
      const QString & fn = *j;
      numTotal++;

      IReaderPtr reader = yae::openFile(readerFactory, fn);
      if (reader)
      {
        numOpened++;
      }
    }

    bool ok = (numOpened == numTotal);
    return ok;
  }

  //----------------------------------------------------------------
  // to_str
  //
  const char *
  to_str(QEvent::Type et)
  {
    static int eventEnumIndex =
      QEvent::staticMetaObject.indexOfEnumerator("Type");

    const char * name =
      QEvent::staticMetaObject.
      enumerator(eventEnumIndex).
      valueToKey(et);

    return name;
  }

  //----------------------------------------------------------------
  // find_last_separator
  //
  QAction *
  find_last_separator(const QMenu & menu, QAction *& next)
  {
    next = NULL;

    QList<QAction *> actions = menu.actions();
    while (!actions.empty())
    {
      QAction * action = actions.back();
      if (action->isSeparator())
      {
        return action;
      }

      next = action;
      actions.pop_back();
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // find_menu_breaks
  //
  bool
  find_menu_breaks(const QMenu & menu, std::list<MenuBreak> & breaks)
  {
    QAction * next = NULL;
    QList<QAction *> actions = menu.actions();

    while (!actions.empty())
    {
      QAction * action = actions.back();
      if (action->isSeparator())
      {
        breaks.push_front(MenuBreak(action, next));
      }

      next = action;
      actions.pop_back();
    }

    return !breaks.empty();
  }

  //----------------------------------------------------------------
  // handle_queued_call_event
  //
  bool
  handle_queued_call_event(QEvent * event)
  {
    if (event->type() == QEvent::User)
    {
      yae::QueuedCallEvent * e = dynamic_cast<yae::QueuedCallEvent *>(event);
      if (e)
      {
        e->accept();
        e->execute();
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // show_in_file_manager
  //
  void
  show_in_file_manager(const char * path_utf8)
  {
#if defined(_WIN32)
    QString path = QDir::toNativeSeparators(QString::fromUtf8(path_utf8));
    PIDLIST_ABSOLUTE pidl = NULL;
    ULONG flags = 0;

    // https://docs.microsoft.com/en-us/windows/win32/api/
    //    shlobj_core/nf-shlobj_core-shparsedisplayname
    //
    // ... should call this function from a background thread.
    // Failure to do so could cause the UI to stop responding.
    //
    HRESULT hr = SHParseDisplayName((PCWSTR)path.utf16(),
                                    NULL,
                                    &pidl,
                                    0,
                                    &flags);
    if (hr == S_OK)
    {
      SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
    }

    if (pidl)
    {
      CoTaskMemFree(pidl);
    }
#elif !defined(__APPLE__)
    if (QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface file_manager("org.freedesktop.FileManager1",
                                  "/org/freedesktop/FileManager1");
      if (file_manager.isValid())
      {
        QStringList file_list;
        file_list.push_back(QString::fromUtf8(path_utf8));

        file_manager.call(QDBus::NoBlock,
                          "ShowItems",
                          QVariant(file_list),
                          QVariant(QString()));
      }
    }
#endif
  }

  //----------------------------------------------------------------
  // get_wheel_pos
  //
  QPointF
  get_wheel_pos(const QWheelEvent * e)
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QPointF pos = QPointF(e->pos());
#else
    QPointF pos = e->position();
#endif
    return pos;
 }

  //----------------------------------------------------------------
  // get_wheel_delta
  //
  int
  get_wheel_delta(const QWheelEvent * e)
  {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    int delta = e->delta();
#else
    int delta = e->angleDelta().y();
#endif
    return delta;
  }

}

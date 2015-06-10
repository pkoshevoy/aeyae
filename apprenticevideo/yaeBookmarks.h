// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jul 26 21:15:04 MDT 2013
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_BOOKMARKS_H_
#define YAE_BOOKMARKS_H_

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_reader.h"


namespace yae
{
  // forward declarations:
  struct IReader;

  //----------------------------------------------------------------
  // TBookmark
  //
  struct YAE_API TBookmark
  {
    TBookmark();

    std::string groupHash_;
    std::string itemHash_;
    std::size_t atrack_;
    std::size_t vtrack_;
    std::list<std::size_t> subs_;
    double positionInSeconds_;
  };

  //----------------------------------------------------------------
  // saveBookmark
  //
  YAE_API bool saveBookmark(const std::string & groupHash,
                            const std::string & itemHash,
                            const IReader * reader,
                            const double & positionInSeconds);

  //----------------------------------------------------------------
  // loadBookmark
  //
  YAE_API bool loadBookmark(const std::string & groupHash,
                            TBookmark & bookmark);

  //----------------------------------------------------------------
  // removeBookmark
  //
  YAE_API bool removeBookmark(const std::string & groupHash);

}


#endif // YAE_BOOKMARKS_H_

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
  struct YAEUI_API TBookmark : IBookmark
  {
    std::string groupHash_;
    std::string itemHash_;
  };

  //----------------------------------------------------------------
  // saveBookmark
  //
  YAEUI_API bool
  saveBookmark(const std::string & groupHash,
               const std::string & itemHash,
               const IReader * reader,
               const double & positionInSeconds);

  //----------------------------------------------------------------
  // loadBookmark
  //
  YAEUI_API bool
  loadBookmark(const std::string & groupHash,
               TBookmark & bookmark);

  //----------------------------------------------------------------
  // removeBookmark
  //
  YAEUI_API bool
  removeBookmark(const std::string & groupHash);

  //----------------------------------------------------------------
  // BookmarkHashInfo
  //
  struct YAEUI_API BookmarkHashInfo
  {
    std::string groupHash_;
    std::list<std::string> itemHash_;
  };


  //----------------------------------------------------------------
  // find_folder_bookmark
  //
  bool
  find_folder_bookmark(const std::string & filepath,
                       TBookmark & bookmark);

  //----------------------------------------------------------------
  // save_folder_bookmark
  //
  bool
  save_folder_bookmark(const std::string & filepath,
                       const IReader * reader,
                       const double & positionInSeconds);

  //----------------------------------------------------------------
  // remove_folder_bookmark
  //
  bool
  remove_folder_bookmark(const std::string & filepath);


  //----------------------------------------------------------------
  // find_bookmark
  //
  bool
  find_bookmark(const std::string & filepath,
                TBookmark & bookmark);

  //----------------------------------------------------------------
  // save_bookmark
  //
  bool
  save_bookmark(const std::string & filepath,
                const IReader * reader,
                const double & positionInSeconds);

  //----------------------------------------------------------------
  // remove_bookmark
  //
  bool
  remove_bookmark(const std::string & filepath);

}


#endif // YAE_BOOKMARKS_H_

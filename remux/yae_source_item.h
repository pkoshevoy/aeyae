// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Aug  5 14:12:36 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SOURCE_ITEM_H_
#define YAE_SOURCE_ITEM_H_

// standard:
#include <limits>

// Qt library
#include <QObject>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/utils/yae_time.h"

// local:
#include "yaeItem.h"
#include "yaeItemView.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // SourceItem
  //
  class YAE_API SourceItem : public QObject, public Item
  {
    Q_OBJECT;

  public:
    SourceItem(const char * id,
               ItemView & view,
               const std::string & name,
               const TDemuxerInterfacePtr & src);

    void layout();

  public:
    ItemView & view_;
    std::string name_;
    TDemuxerInterfacePtr src_;
  };

}


#endif // YAE_SOURCE_ITEM_H_

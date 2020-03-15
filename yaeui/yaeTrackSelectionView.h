// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:18:55 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TRACK_SELECTION_VIEW_H_
#define YAE_TRACK_SELECTION_VIEW_H_

// Qt library:
#include <QObject>
#include <QString>

// aeyae:
#include "yae/video/yae_video.h"

// yaeui:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaePlayerItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // TrackSelectionView
  //
  class YAEUI_API TrackSelectionView : public ItemView
  {
    Q_OBJECT;

  public:
    TrackSelectionView();

    void setStyle(ItemViewStyle * style);
    void setTracks(const std::vector<TTrackInfo> & tracks);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

  signals:
    void selected(const TTrackInfo & track);
    void done();

  protected:
    void sync_ui();

    ItemViewStyle * style_;
    std::vector<TTrackInfo> tracks_;
  };

}


#endif // YAE_TRACK_SELECTION_VIEW_H_

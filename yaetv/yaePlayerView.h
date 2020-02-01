// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:05:08 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_VIEW_H_
#define YAE_PLAYER_VIEW_H_

// Qt library:
#include <QObject>
#include <QString>

// yaeui:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeTimelineItem.h"
#include "yae_player_item.h"

// local:
#include "yaeAppView.h"
#include "yae_dvr.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerView
  //
  class YAEUI_API PlayerView : public ItemView
  {
    Q_OBJECT;

    const AppStyle * style_;
    QTimer sync_ui_;

  public:
    PlayerView();

    void setStyle(const AppStyle * style);

    // virtual:
    const AppStyle * style() const
    { return style_; }

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // virtual:
    void setEnabled(bool enable);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

  signals:
    void toggle_fullscreen();

  public:
    bool is_playback_paused() const;

  public slots:
    void toggle_playback();
    void sync_ui();

  protected:
    void layout(PlayerView & view, const AppStyle & style, Item & root);

  public:
    TRecordingPtr recording_;
    yae::shared_ptr<PlayerItem, Item> player_;
    yae::shared_ptr<TimelineItem, Item> timeline_;
  };

}


#endif // YAE_PLAYER_VIEW_H_

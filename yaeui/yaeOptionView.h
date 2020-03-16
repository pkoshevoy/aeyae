// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:18:55 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_OPTION_VIEW_H_
#define YAE_OPTION_VIEW_H_

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
  // OptionView
  //
  class YAEUI_API OptionView : public ItemView
  {
    Q_OBJECT;

  public:

    OptionView();

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    //----------------------------------------------------------------
    // Option
    //
    struct Option
    {
      uint32_t index_;
      std::string text_;
    };

    void setOptions(const std::vector<Option> & options);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

  signals:
    void selected(const Option & option);
    void done();

  protected:
    void sync_ui();

    ItemViewStyle * style_;
    std::vector<Option> options_;
    yae::shared_ptr<Item> hidden_;
    yae::shared_ptr<Item> panel_;
  };

}


#endif // YAE_OPTION_VIEW_H_

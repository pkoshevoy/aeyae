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
      std::string headline_;
      std::string fineprint_;
    };

    void setOptions(const std::vector<Option> & options,
                    int preselect_option);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

    inline int get_selected() const
    { return selected_; }

  signals:
    void option_selected(int index);
    void done();

  public slots:
    void set_selected(int index, bool is_done = false);

  protected:
    void sync_ui();

    ItemViewStyle * style_;
    std::vector<Option> options_;
    std::size_t selected_;

    yae::shared_ptr<Item> hidden_;
    yae::shared_ptr<Item> panel_;
  };

}


#endif // YAE_OPTION_VIEW_H_

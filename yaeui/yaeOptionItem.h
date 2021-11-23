// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Nov 22 16:29:51 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_OPTION_ITEM_H_
#define YAE_OPTION_ITEM_H_

// standard:
#include <string>
#include <vector>

// Qt library:
#include <QObject>

// aeyae:
#include "yae/video/yae_video.h"

// yaeui:
#include "yaeItem.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // OptionItem
  //
  class YAEUI_API OptionItem : public QObject, public Item
  {
    Q_OBJECT;

  public:
    OptionItem(const char * id, ItemView & view);

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
    void uncache();

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setVisible(bool visible);

    inline int get_selected() const
    { return selected_; }

  signals:
    void option_selected(int index);
    void done();

  public slots:
    void set_selected(int index, bool is_done = false);

  protected:
    void sync_ui();

    ItemView & view_;
    std::vector<Option> options_;
    std::size_t selected_;

    ItemRef unit_size_;
    yae::shared_ptr<Item> panel_;
  };

}


#endif // YAE_OPTION_ITEM_H_

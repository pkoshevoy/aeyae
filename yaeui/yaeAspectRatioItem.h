// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Nov 22 14:20:45 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASPECT_RATIO_ITEM_H_
#define YAE_ASPECT_RATIO_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// Qt:
#include <QObject>

// yaeui:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatio
  //
  struct YAEUI_API AspectRatio
  {
    enum Category {
      kNone,
      kStandard,
      kAuto,
      kOther
    };

    AspectRatio(double ar = 0.0,
                const char * label = NULL,
                Category category = kStandard,
                const char * select_subview = NULL);

    double ar_;
    std::string label_;

    // non-standard aspect ratio selection may need special handling:
    Category category_;

    // in case a separate view is required to handle the selection:
    std::string subview_;
  };


  //----------------------------------------------------------------
  // AspectRatioView
  //
  class YAEUI_API AspectRatioItem : public QObject, public Item
  {
    Q_OBJECT;

  public:
    AspectRatioItem(const char * id,
                    ItemView & view,
                    const AspectRatio * options,
                    std::size_t num_options);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setVisible(bool visible);

    // helpers:
    double getAspectRatio(std::size_t index) const;

    inline const std::vector<AspectRatio> & options() const
    { return options_; }

    inline const AspectRatio * get_options(std::size_t i) const
    { return i < options_.size() ? &options_.at(i) : NULL; }

    inline std::size_t currentSelection() const
    { return sel_; }

    inline double nativeAspectRatio() const
    { return native_; }

    inline double currentAspectRatio() const
    { return current_; }

    inline ItemView & view() const
    { return view_; }

  signals:
    void selected(const AspectRatio & ar);
    void aspectRatio(double ar);
    void done();

  public slots:
    void selectAspectRatioCategory(AspectRatio::Category category);
    void selectAspectRatio(std::size_t index);
    void setNativeAspectRatio(double ar);
    void setAspectRatio(double ar);

  protected:
    ItemView & view_;
    std::vector<AspectRatio> options_;

    // current selection:
    std::size_t sel_;
    double current_;
    double native_;
  };

}


#endif // YAE_ASPECT_RATIO_ITEM_H_

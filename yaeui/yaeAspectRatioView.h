// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Mar  9 19:16:48 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASPECT_RATIO_VIEW_H_
#define YAE_ASPECT_RATIO_VIEW_H_

// Qt library:
#include <QObject>
#include <QString>

// local:
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
  class YAEUI_API AspectRatioView : public ItemView
  {
    Q_OBJECT;

  public:
    AspectRatioView();

    void init(ItemViewStyle * style,
              const AspectRatio * options,
              std::size_t num_options);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

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
    ItemViewStyle * style_;
    std::vector<AspectRatio> options_;

    // current selection:
    std::size_t sel_;
    double current_;
    double native_;
  };

}


#endif // YAE_ASPECT_RATIO_VIEW_H_

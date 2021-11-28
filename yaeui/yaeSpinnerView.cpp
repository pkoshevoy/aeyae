// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Apr 24 21:41:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// local:
#include "yaeSpinnerView.h"


namespace yae
{

  //----------------------------------------------------------------
  // GetSpinnerText
  //
  struct GetSpinnerText : public TVarExpr
  {
    GetSpinnerText(const SpinnerView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = view_.text();
    }

    const SpinnerView & view_;
  };



  //----------------------------------------------------------------
  // SpinnerView::SpinnerView
  //
  SpinnerView::SpinnerView(const char * name):
    ItemView(name),
    style_(NULL),
    text_(tr("please wait"))
  {}

  //----------------------------------------------------------------
  // SpinnerView::~SpinnerView
  //
  SpinnerView::~SpinnerView()
  {
    TMakeCurrentContext currentContext(this->context().get());
    SpinnerView::clear();
    spinner_.reset();
  }

  //----------------------------------------------------------------
  // SpinnerView::setStyle
  //
  void
  SpinnerView::setStyle(ItemViewStyle * style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // SpinnerView::setEnabled
  //
  void
  SpinnerView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.clear();

    if (enable)
    {
      spinner_.reset(new SpinnerItem("SpinnerItem", *this));
      SpinnerItem & spinner = root.add<SpinnerItem>(spinner_);
      spinner.anchors_.fill(root);
      spinner.message_.set(new GetSpinnerText(*this));

      if (fg_.isValid())
      {
        spinner.fg_.set(fg_);
      }

      if (bg_.isValid())
      {
        spinner.bg_.set(bg_);
      }

      if (text_color_.isValid())
      {
        spinner.text_color_.set(text_color_);
      }

      spinner.layout();
      spinner.setVisible(true);
    }
    else if (spinner_)
    {
      spinner_->setVisible(false);
      spinner_.reset();
    }

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // SpinnerView::setText
  //
  void
  SpinnerView::setText(const QString & text)
  {
    text_ = text;
    requestUncache(root_.get());
    requestRepaint();
  }

}

// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 22:30:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local:
#include "yae_checkbox_item.h"
#include "yae_lshape.h"
#include "yaeItemViewStyle.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTransform.h"


namespace yae
{

  //----------------------------------------------------------------
  // CheckboxColor
  //
  struct CheckboxColor : public TColorExpr
  {
    CheckboxColor(const CheckboxItem & item):
      item_(item)
    {}

    void evaluate(Color & result) const
    {
      const ItemViewStyle * style = item_.view_.style();
      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      bool enabled = item_.enabled_.get();
      bool checked = item_.checked_.get();
      if (enabled)
      {
        if (checked)
        {
          result = style->cursor_.get();
        }
        else
        {
          result = style->fg_controls_.get();
        }
      }
      else
      {
        result = style->fg_controls_.get();
      }
    }

    const CheckboxItem & item_;
  };

  //----------------------------------------------------------------
  // CheckmarkColor
  //
  struct CheckmarkColor : public TColorExpr
  {
    CheckmarkColor(const CheckboxItem & item):
      item_(item)
    {}

    void evaluate(Color & result) const
    {
      const ItemViewStyle * style = item_.view_.style();
      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      bool enabled = item_.enabled_.get();
      if (enabled)
      {
        result = style->fg_.get();
      }
      else
      {
        result = style->bg_controls_.get();
      }
    }

    const CheckboxItem & item_;
  };


  //----------------------------------------------------------------
  // SpotlightAnimator
  //
  struct SpotlightAnimator : public ItemView::IAnimator
  {
    SpotlightAnimator(CheckboxItem & checkbox, RoundRect & spotlight):
      checkbox_(checkbox),
      spotlight_(spotlight),
      xn_(new Transition(0.5))
    {
      anchor_[0] = 0.0;
      anchor_[1] = 0.0;
    }

    // helper:
    double spotlight_opacity() const
    {
      yae::shared_ptr<Transition> xn = xn_;
      double t = xn->get_value();
      double v = (1.0 - t) * anchor_[0] + t * anchor_[1];
      return v;
    }

    // helper:
    void set_anchor(bool focused, bool pressed)
    {
      anchor_[0] = spotlight_opacity();

      if (pressed)
      {
        anchor_[1] = 0.4;
      }
      else if (focused)
      {
        anchor_[1] = 0.3;
      }
      else
      {
        const TVec2D & pt = checkbox_.view_.mousePt();
        anchor_[1] = spotlight_.overlaps(pt) ? 0.1 : 0.0;
      }

      double dt = fabs(anchor_[1] - anchor_[0]);
      xn_.reset(new Transition(dt * 1.25));
      xn_->start();
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animator)
    {
      yae::shared_ptr<Transition> xn = xn_;
      if (xn->is_done())
      {
        checkbox_.view_.delAnimator(animator);
      }

      spotlight_.opacity_.uncache();
    }

    CheckboxItem & checkbox_;
    RoundRect & spotlight_;

    // animation state:
    bool in_focus_;
    bool in_hover_;
    double anchor_[2];

    // tweening basis function:
    yae::shared_ptr<Transition> xn_;
  };


  //----------------------------------------------------------------
  // SpotlightOpacity
  //
  struct SpotlightOpacity : public TDoubleExpr
  {
    SpotlightOpacity(SpotlightAnimator & animator):
      animator_(animator)
    {}

    void evaluate(double & result) const
    {
      result = animator_.spotlight_opacity();
    }

    SpotlightAnimator & animator_;
  };


  //----------------------------------------------------------------
  // CheckboxItem::CheckboxItem
  //
  CheckboxItem::CheckboxItem(const char * id, ItemView & view):
    ClickableItem(id),
    view_(view),
    enabled_(true),
    checked_(true),
    color_(Color(0xee75ff))
  {
    RoundRect & spotlight = addNew<RoundRect>("spotlight");
    RoundRect & checkbox = addNew<RoundRect>("checkbox");
    checkbox.anchors_.fill(*this);
    checkbox.border_ = ItemRef::scale(checkbox, kPropertyWidth, 0.0875);
    checkbox.color_ = checkbox.addExpr(new CheckboxColor(*this));
    checkbox.colorBorder_ = ColorRef::reference(*this, kPropertyColor);
    checkbox.radius_ = ItemRef::scale(checkbox, kPropertyWidth, 0.15);

    spotlight.anchors_.hcenter_ =
      ItemRef::reference(checkbox, kPropertyHCenter);
    spotlight.anchors_.vcenter_ =
      ItemRef::reference(checkbox, kPropertyVCenter);
    spotlight.width_ =
      ItemRef::scale(checkbox, kPropertyWidth, 2.3);
    spotlight.radius_ =
      ItemRef::scale(spotlight, kPropertyWidth, 0.5);
    spotlight.color_ =
      ColorRef::reference(*this, kPropertyColor);
    spotlight.opacity_ =
      ItemRef::constant(0.2);
    spotlight.height_ = spotlight.width_;

    Transform & transform = addNew<Transform>("transform");
    transform.anchors_.hcenter_ =
      ItemRef::reference(checkbox, kPropertyHCenter);
    transform.anchors_.vcenter_ =
      ItemRef::reference(checkbox, kPropertyVCenter);
    transform.rotation_ =
      ItemRef::constant(-0.25 * M_PI);

    LShape & checkmark = transform.addNew<LShape>("checkmark");
    checkmark.anchors_.left_ =
      ItemRef::scale(checkbox, kPropertyWidth, -0.3);
    checkmark.anchors_.bottom_ =
      ItemRef::scale(checkbox, kPropertyWidth, 0.1);
    checkmark.width_ =
      ItemRef::scale(checkbox, kPropertyWidth, 0.58);
    checkmark.height_ =
      ItemRef::scale(checkbox, kPropertyWidth, 0.29);
    checkmark.weight_ =
      ItemRef::scale(checkbox, kPropertyWidth, 0.075);
    checkmark.color_ = checkmark.addExpr(new CheckmarkColor(*this));

    checkmark.visible_ = BoolRef::reference(*this, kPropertyChecked);

    this->spotlight_animator_.reset
      (new SpotlightAnimator(*this, spotlight));
  }

  //----------------------------------------------------------------
  // CheckboxItem::uncache
  //
  void
  CheckboxItem::uncache()
  {
    enabled_.uncache();
    checked_.uncache();
    color_.uncache();
    ClickableItem::uncache();
  }

  //----------------------------------------------------------------
  // CheckboxItem::get
  //
  void
  CheckboxItem::get(Property property, bool & value) const
  {
    if (property == kPropertyChecked)
    {
      value = checked_.get();
      return;
    }

    if (property == kPropertyEnabled)
    {
      value = enabled_.get();
      return;
    }

    ClickableItem::get(property, value);
  }

  //----------------------------------------------------------------
  // CheckboxItem::get
  //
  void
  CheckboxItem::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
      return;
    }

    ClickableItem::get(property, value);
  }

  //----------------------------------------------------------------
  // CheckboxItem::onMouseOver
  //
  bool
  CheckboxItem::onMouseOver(const TVec2D & itemCSysOrigin,
                            const TVec2D & rootCSysPoint)
  {
    
    return ClickableItem::onMouseOver(itemCSysOrigin, rootCSysPoint);
  }

}

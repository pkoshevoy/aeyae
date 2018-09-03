// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 22:30:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_log.h"

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
    CheckboxColor(const CheckboxItem & item, bool border = true):
      item_(item),
      border_(border)
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

          if (!border_)
          {
            result.set_a(0.0);
          }
        }
      }
      else
      {
        result = style->fg_controls_.get();
      }
    }

    const CheckboxItem & item_;
    bool border_;
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
      xn_(new Transition(0.25))
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
      // anchor_[0] = spotlight_opacity();

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
        anchor_[1] = 0.1; // spotlight_.overlaps(pt) ? 0.1 : 0.0;
      }
#if 0
      double dt = fabs(anchor_[1] - anchor_[0]);
      if (dt > 0.0)
      {
        yae::shared_ptr<Transition> xn(new Transition(dt * 1.25));
        xn->start();
        xn_ = xn;
      }
#else
      xn_->start();
#endif
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animator)
    {
      yae::shared_ptr<Transition> xn = xn_;
      yae_elog << "animate spotlight, done: " << xn->is_done() << '\n';
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
    checked_(true)
  {
    color_ = addExpr(new CheckboxColor(*this));
    RoundRect & spotlight = addNew<RoundRect>("spotlight");
    RoundRect & checkbox = addNew<RoundRect>("checkbox");
    checkbox.anchors_.fill(*this);
    // checkbox.border_ = ItemRef::scale(checkbox, kPropertyWidth, 0.0875);
    // checkbox.border_ = ItemRef::constant(2);
    checkbox.border_ = ItemRef::scale(checkbox, kPropertyWidth, 0.1);
    checkbox.color_ = addExpr(new CheckboxColor(*this, false));
    checkbox.color_.disableCaching();
    checkbox.colorBorder_ = ColorRef::reference(*this, kPropertyColor);
    checkbox.colorBorder_.disableCaching();
    // checkbox.radius_ = ItemRef::scale(checkbox, kPropertyWidth, 0.15);
    checkbox.radius_ = ItemRef::scale(checkbox, kPropertyWidth, 0.1);
    // checkbox.radius_ = ItemRef::constant(1.9);

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
    spotlight.color_.disableCaching();
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
    checkmark.color_ =
      checkmark.addExpr(new CheckmarkColor(*this));
    checkmark.visible_ =
      BoolRef::reference(*this, kPropertyChecked);
    checkmark.visible_.disableCaching();

    SpotlightAnimator * spotlight_animator = NULL;
    this->spotlight_animator_.reset
      (spotlight_animator = new SpotlightAnimator(*this, spotlight));

    spotlight.opacity_ = addExpr(new SpotlightOpacity(*spotlight_animator));
    spotlight.opacity_.disableCaching();
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
    yae_elog << "CheckboxItem::onMouseOver";
    yae::shared_ptr<SpotlightAnimator, Canvas::ILayer::IAnimator>
      spotlight_animator = spotlight_animator_;
    spotlight_animator->set_anchor(false, false);
    view_.addAnimator(spotlight_animator_);
    return true;
  }

  //----------------------------------------------------------------
  // CheckboxItem::onPress
  //
  bool
  CheckboxItem::onPress(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysPoint)
  {
    yae_elog << "CheckboxItem::onFocus";
    yae::shared_ptr<SpotlightAnimator, Canvas::ILayer::IAnimator>
      spotlight_animator = spotlight_animator_;
    spotlight_animator->set_anchor(true, true);
    view_.addAnimator(spotlight_animator_);
    checked_ = BoolRef::constant(!checked_.get());
    color_.uncache();
    return true;
  }

  //----------------------------------------------------------------
  // CheckboxItem::onFocus
  //
  void
  CheckboxItem::onFocus()
  {
    yae_elog << "CheckboxItem::onFocus";
    yae::shared_ptr<SpotlightAnimator, Canvas::ILayer::IAnimator>
      spotlight_animator = spotlight_animator_;
    spotlight_animator->set_anchor(true, false);
    view_.addAnimator(spotlight_animator_);
  }

  //----------------------------------------------------------------
  // CheckboxItem::onFocusOut
  //
  void
  CheckboxItem::onFocusOut()
  {
#if 0
    yae_elog << "CheckboxItem::onFocusOut";
    yae::shared_ptr<SpotlightAnimator, Canvas::ILayer::IAnimator>
      spotlight_animator = spotlight_animator_;
    spotlight_animator->set_anchor(false, false);
    view_.addAnimator(spotlight_animator_);
#endif
  }

  //----------------------------------------------------------------
  // CheckboxItem::processEvent
  //
  bool
  CheckboxItem::processEvent(Canvas::ILayer & canvasLayer,
                             Canvas * canvas,
                             QEvent * event)
  {
    QEvent::Type et = event->type();
    if (et == QEvent::KeyPress)
    {
      QKeyEvent & ke = *(static_cast<QKeyEvent *>(event));
      int key = ke.key();

      yae::shared_ptr<SpotlightAnimator, Canvas::ILayer::IAnimator>
        spotlight_animator = spotlight_animator_;

      bool checked = checked_.get();

      if (key == Qt::Key_Space ||
          key == Qt::Key_Return ||
          key == Qt::Key_Enter)
      {
        checked_ = BoolRef::constant(!checked);
      }
      else if ((key == Qt::Key_Backspace ||
                key == Qt::Key_Delete ||
                key == Qt::Key_Minus ||
                key == Qt::Key_0) &&
               checked)
      {
        checked_ = BoolRef::constant(false);
      }
      else if ((key == Qt::Key_Plus ||
                key == Qt::Key_1) &&
               !checked)
      {
        checked_ = BoolRef::constant(true);
      }
      else
      {
        return false;
      }

      spotlight_animator->set_anchor(true, true);
      view_.addAnimator(spotlight_animator_);
      color_.uncache();
      return true;
    }

    return false;
  }

}

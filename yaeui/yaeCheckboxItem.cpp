// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 22:30:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_log.h"

// local:
#include "yaeCheckboxItem.h"
#include "yaeItemViewStyle.h"
#include "yaeLShape.h"
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
      if (enabled)
      {
        result = style->fg_controls_.get();
      }
      else
      {
        result = style->fg_controls_.get().a_scaled(0.5);
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
        // result = style->fg_.get();
        result = style->bg_.get();
      }
      else
      {
        // result = style->bg_controls_.get();
      }
    }

    const CheckboxItem & item_;
  };


  //----------------------------------------------------------------
  // HoverAnimator
  //
  struct HoverAnimator : public ItemView::IAnimator
  {
    HoverAnimator(CheckboxItem & checkbox):
      checkbox_(checkbox)
    {}

    // helper:
    inline TransitionItem & transition_item() const
    { return checkbox_.Item::get<TransitionItem>("hover_opacity"); }

    // helper:
    inline bool in_progress() const
    { return transition_item().transition().in_progress(); }

    // helper:
    inline bool item_overlaps_mouse_pt() const
    { return checkbox_.view_.isMouseOverItem(checkbox_); }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animator)
    {
      TransitionItem & opacity = transition_item();
      const Transition & xn = opacity.transition();

      if (item_overlaps_mouse_pt() && xn.is_steady())
      {
        opacity.pause(ItemRef::constant(xn.get_value()));
        checkbox_.view_.delAnimator(animator);
      }
      else if (xn.is_done())
      {
        checkbox_.view_.delAnimator(animator);
      }

      opacity.uncache();
    }

    CheckboxItem & checkbox_;
  };

  //----------------------------------------------------------------
  // ClickAnimator
  //
  struct ClickAnimator : public ItemView::IAnimator
  {
    ClickAnimator(CheckboxItem & checkbox):
      checkbox_(checkbox)
    {}

    // helper:
    inline TransitionItem & transition_item() const
    { return checkbox_.Item::get<TransitionItem>("click_opacity"); }

    // helper:
    inline bool in_progress() const
    { return transition_item().transition().in_progress(); }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animator)
    {
      TransitionItem & opacity = transition_item();
      const Transition & xn = opacity.transition();

      if (xn.is_done())
      {
        checkbox_.view_.delAnimator(animator);
      }

      opacity.uncache();
    }

    CheckboxItem & checkbox_;
  };

  //----------------------------------------------------------------
  // SpotlightOpacity
  //
  struct SpotlightOpacity : public TDoubleExpr
  {
    SpotlightOpacity(HoverAnimator & hover_animator,
                     ClickAnimator & click_animator):
      hover_animator_(hover_animator),
      click_animator_(click_animator)
    {}

    void evaluate(double & result) const
    {
      double click_opacity = 0.0;
      if (click_animator_.in_progress())
      {
        click_animator_.transition_item().get(kPropertyTransition,
                                              click_opacity);
      }

      double hover_opacity = 0.0;
      hover_animator_.transition_item().get(kPropertyTransition,
                                            hover_opacity);

      result = std::max(click_opacity, hover_opacity);
    }

    HoverAnimator & hover_animator_;
    ClickAnimator & click_animator_;
  };


  //----------------------------------------------------------------
  // CheckboxItem::CheckboxItem
  //
  CheckboxItem::CheckboxItem(const char * id, ItemView & view):
    ClickableItem(id),
    view_(view),
    enabled_(true),
    checked_(true),
    animate_(false)
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

    typedef Transition::Polyline TPolyline;

    TransitionItem & hover_opacity = this->
      addHidden(new TransitionItem("hover_opacity",
                                   TPolyline(0.5, 0.0, 0.2, 10),
                                   TPolyline(0.05, 0.2, 0.2),
                                   TPolyline(0.2, 0.2, 0.0, 10)));

    TransitionItem & click_opacity = this->
      addHidden(new TransitionItem("click_opacity",
                                   TPolyline(0.2, 0.0, 0.75, 10),
                                   TPolyline(0.0, 0.75, 0.75),
                                   TPolyline(0.2, 0.75, 0.0, 10)));

    HoverAnimator * hover_animator = NULL;
    this->hover_.reset(hover_animator = new HoverAnimator(*this));

    ClickAnimator * click_animator = NULL;
    this->click_.reset(click_animator = new ClickAnimator(*this));

    spotlight.opacity_ = addExpr
      (new SpotlightOpacity(*hover_animator, *click_animator));
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
  // CheckboxItem::paintContent
  //
  void
  CheckboxItem::paintContent() const
  {
    animate_hover();
    ClickableItem::paintContent();
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
    animate_hover();
    return true;
  }

  //----------------------------------------------------------------
  // CheckboxItem::onPress
  //
  bool
  CheckboxItem::onPress(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysPoint)
  {
    animate_click();
    toggle();
    return true;
  }

  //----------------------------------------------------------------
  // CheckboxItem::onFocus
  //
  void
  CheckboxItem::onFocus()
  {
    animate_hover(true);
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

      yae::shared_ptr<HoverAnimator, Canvas::ILayer::IAnimator>
        hover_animator = hover_;

      bool checked = checked_.get();

      if (key == Qt::Key_Space ||
          key == Qt::Key_Return ||
          key == Qt::Key_Enter)
      {
        set_checked(!checked);
      }
      else if ((key == Qt::Key_Backspace ||
                key == Qt::Key_Delete ||
                key == Qt::Key_Minus ||
                key == Qt::Key_0) &&
               checked)
      {
        set_checked(false);
      }
      else if ((key == Qt::Key_Plus ||
                key == Qt::Key_1) &&
               !checked)
      {
        set_checked(true);
      }
      else
      {
        return false;
      }

      animate_click();
      return true;
    }

    return false;
  }


  //----------------------------------------------------------------
  // CheckboxItem::set_checked
  //
  void
  CheckboxItem::set_checked(bool checked)
  {
    if (checked_.get() == checked)
    {
      return;
    }

    if (on_toggle_)
    {
      const Action & on_toggle = *on_toggle_;
      on_toggle(*this);
    }
    else
    {
      checked_ = BoolRef::constant(checked);
    }

    view_.requestUncache(this);
    view_.requestRepaint();
  }

  //----------------------------------------------------------------
  // CheckboxItem::toggle
  //
  void
  CheckboxItem::toggle()
  {
    bool checked = checked_.get();
    set_checked(!checked);
  }

  //----------------------------------------------------------------
  // CheckboxItem::animate_hover
  //
  void
  CheckboxItem::animate_hover(bool force) const
  {
    if (!animate_.get())
    {
      return;
    }

    HoverAnimator & animator = dynamic_cast<HoverAnimator &>(*(hover_.get()));
    TransitionItem & opacity = Item::find<TransitionItem>("hover_opacity");

    if (force || animator.item_overlaps_mouse_pt())
    {
      opacity.start();
      view_.addAnimator(hover_);
    }
    else if (opacity.is_paused())
    {
      opacity.start();
      view_.addAnimator(hover_);
    }
  }

  //----------------------------------------------------------------
  // CheckboxItem::animate_click
  //
  void
  CheckboxItem::animate_click() const
  {
    if (!animate_.get())
    {
      return;
    }

    ClickAnimator & animator = dynamic_cast<ClickAnimator &>(*(click_.get()));
    TransitionItem & opacity = Item::find<TransitionItem>("click_opacity");

    opacity.start();
    view_.addAnimator(click_);
    color_.uncache();
  }

}
